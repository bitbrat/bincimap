/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    bincimapd-lsub.cc
 *  
 *  Description:
 *    Implementation of the LSUB command.
 *
 *  Authors:
 *    Andreas Aardal Hanssen <andreas-binc curly bincimap spot org>
 *
 *  Bugs:
 *
 *  ChangeLog:
 *
 *  --------------------------------------------------------------------
 *  Copyright 2002-2004 Andreas Aardal Hanssen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *  --------------------------------------------------------------------
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include "recursivedescent.h"

#include "io.h"
#include "mailbox.h"
#include "convert.h"
#include "regmatch.h"

#include "session.h"
#include "depot.h"
#include "operators.h"

namespace {
  const int DIR_SELECT      = 0x01;
  const int DIR_MARKED      = 0x02;
  const int DIR_NOINFERIORS = 0x04;
}

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
LsubOperator::LsubOperator(void)
{
}

//----------------------------------------------------------------------
LsubOperator::~LsubOperator(void)
{
}

//----------------------------------------------------------------------
const string LsubOperator::getName(void) const
{
  return "LSUB";
}

//----------------------------------------------------------------------
int LsubOperator::getState(void) const
{
  return Session::AUTHENTICATED | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult LsubOperator::process(Depot &depot,
					      Request &command)
{
  Session &session = Session::getInstance();
  IO &com = IOFactory::getInstance().get(1);
  const char delim = depot.getDelimiter();

  // remove leading or trailing delimiter in wildcard
  string wildcard = command.getListMailbox();
  trim(wildcard, string(&delim, 1));

  // convert wildcard to regular expression
  const string &regex = toRegex(wildcard, depot.getDelimiter());

  // remove leading or trailing delimiter in reference
  string ref = command.getMailbox();
  while (ref.length() > 1 && ref[0] == depot.getDelimiter())
    ref = ref.substr(1);

  // a multimap from mailbox name to flags
  multimap<string, int> mailboxes;

  // read through all entries in depository.
  for (Depot::iterator i = depot.begin("."); i != depot.end(); ++i) {
    const string path = *i;
    const string mpath = depot.filenameToMailbox(path);
    Mailbox *m = 0;

    // skip entries that are not identified as mailboxes
    if ((m = depot.get(mpath)) == 0)
      continue;

    // convert file name to mailbox name. skip it if there is no
    // corresponding mailbox name.
    string tmp = toCanonMailbox(depot.filenameToMailbox(path));
    trim(tmp, string(&delim, 1));
    if (tmp == "")
      continue;
    else {
      int flags = DIR_SELECT;
      multimap<string, int>::iterator mi = mailboxes.find(tmp);
      if (mi != mailboxes.end()) {
	flags |= mi->second;
	mailboxes.erase(mi);
      }

      mailboxes.insert(make_pair(tmp, flags));
    }

    // now add all superior mailboxes with no flags set if not
    // added already.
    string::size_type pos = tmp.rfind(delim);
    while (pos != string::npos) {
      tmp = tmp.substr(0, pos);
      trim(tmp, string(&delim, 1));

      multimap<string, int>::iterator mi = mailboxes.find(tmp);
      if (mi == mailboxes.end())
	mailboxes.insert(make_pair(tmp, 0));

      pos = tmp.rfind(delim);
    }
  }

  // find leaf nodes O(N^2)
  map<string, int>::iterator i;
  for (i = mailboxes.begin(); i != mailboxes.end(); ++i) {
    string mailbox = i->first;
    mailbox += delim;

    bool leaf = true;
    map<string, int>::const_iterator j;
    for (j = mailboxes.begin(); j != mailboxes.end(); ++j) {
      string::size_type pos = j->first.rfind(delim);
      if (pos == string::npos) continue;

      string base = j->first.substr(0, pos + 1);

      if (mailbox == base) {
	leaf = false;
	break;
      }
    }
  }

  session.loadSubscribes();

  vector<string> &subscribed = session.subscribed;
  sort(subscribed.begin(), subscribed.end());

  // finally, print all mailbox entries with flags.  
  for (vector<string>::const_iterator j = subscribed.begin();
       j != subscribed.end(); ++j) {
    if (ref != "" && (ref.length() > (*j).length()
		      || ((*j).substr(0, ref.length()) != ref)))
      continue;

    if (regexMatch((*j).substr(ref.length()), regex) != 0)
      continue;

    int flags = 0;
    
    for (i = mailboxes.begin(); i != mailboxes.end(); ++i) {
      if (i->first == *j) {
	flags = i->second;
	break;
      }
    }

    com << "* LSUB (";
    string sep = "";

    bool noselect = false;
    if (!(flags & DIR_SELECT)) {
      com << sep << "\\Noselect";
      sep = " ";
      noselect = true;
    }
	
    if (!noselect) {
      if (flags & DIR_MARKED)
	com << sep << "\\Marked";
      else
	com << sep << "\\Unmarked";
      sep = " ";
    }

    if (flags & DIR_NOINFERIORS)
      com << sep << "\\Noinferiors";
    
    com << ") \"" << depot.getDelimiter() << "\" "
	<< toImapString(*j) << endl;
  }

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult LsubOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after LSUB");
    return ERROR;
  }

  string mailbox;
  if ((res = expectMailbox(mailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox after LSUB SPACE");
    return ERROR;
  }

  c_in.setMailbox(mailbox);

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after LSUB SPACE mailbox");
    return ERROR;
  }

  string listmailbox;
  if ((res = expectListMailbox(listmailbox)) != ACCEPT) {
    session.setLastError("Expected list_mailbox after LSUB SPACE"
			 " mailbox SPACE");
    return ERROR;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after LSUB SPACE"
			 " mailbox SPACE list_mailbox");
    return ERROR;
  }

  c_in.setListMailbox(listmailbox);
  c_in.setName("LSUB");
  return ACCEPT;
}
