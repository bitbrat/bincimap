/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-list.cc
 *  
 *  Description:
 *    Implementation of the LIST command.
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

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <string>
#include <iostream>

#include "io.h"

#include "mailbox.h"

#include "regmatch.h"
#include "convert.h"

#include "recursivedescent.h"

#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

namespace {
  const time_t LIST_CACHE_TIMEOUT = 10;
}

//----------------------------------------------------------------------
ListOperator::ListOperator(void)
{
  cacheTimeout = 0;
}

//----------------------------------------------------------------------
ListOperator::~ListOperator(void)
{
}

//----------------------------------------------------------------------
const string ListOperator::getName(void) const
{
  return "LIST";
}

//----------------------------------------------------------------------
int ListOperator::getState(void) const
{
  return Session::AUTHENTICATED | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult ListOperator::process(Depot &depot,
					      Request &command)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();
  const char delim = depot.getDelimiter();

  // special case: if the mailbox argument is empty, then give a
  // hard coded reply.
  string wildcard;
  if ((wildcard = command.getListMailbox()) == "") {
    com << "* LIST (\\Noselect) \"" << delim << "\" \"\"" << endl;
    return OK;
  }

  // remove leading or trailing delimiter in wildcard
  trim(wildcard, string(&delim, 1));

  // convert wildcard to regular expression
  const string &regex = toRegex(wildcard, depot.getDelimiter());

  // remove leading or trailing delimiter in reference
  string ref = command.getMailbox();
  trim(ref, string(&delim, 1));

  // a map from mailbox name to flags
  map<string, unsigned int> mailboxes;

  if (cacheTimeout == 0 || cacheTimeout < time(0) - LIST_CACHE_TIMEOUT
      || session.mailboxchanges) {
    session.mailboxchanges = false;

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
      if (tmp == "") continue;
      else {
	// inherit flags that were already set for this mailbox.
	int flags = DIR_SELECT;
	if (m->isMarked(path)) flags |= DIR_MARKED;
	if (mailboxes.find(tmp) != mailboxes.end()) flags |= mailboxes[tmp];
	mailboxes[tmp] = flags;
      }

      // now add all superior mailboxes with no flags set if not
      // added already.
      string::size_type pos = tmp.rfind(delim);
      while (pos != string::npos) {
	tmp = tmp.substr(0, pos);
	trim(tmp, string(&delim, 1));
	
	if (mailboxes.find(tmp) == mailboxes.end())
	  mailboxes[tmp] = 0;
	
	pos = tmp.rfind(delim);
      }
    }

    // find leaf nodes O(N^2)
    map<string, unsigned int>::iterator i;
    for (i = mailboxes.begin(); i != mailboxes.end(); ++i) {
      string mailbox = i->first;
      mailbox += delim;
      
      bool leaf = true;
      map<string, unsigned int>::const_iterator j = mailboxes.begin();
      for (; j != mailboxes.end(); ++j) {
	string::size_type pos = j->first.rfind(delim);
	if (pos == string::npos) continue;
	
	string base = j->first.substr(0, pos + 1);
	
	if (mailbox == base) {
	  leaf = false;
	  break;
	}
      }

      if (leaf) {
	unsigned int flags = i->second;
	flags |= DIR_LEAF;
	i->second = flags;
      }
    }

    cache = mailboxes;
    cacheTimeout = time(0);
  } else {
    mailboxes = cache;
    cacheTimeout = time(0);
  }

  // finally, print all mailbox entries with flags.  
  map<string, unsigned int>::iterator i = mailboxes.begin();
  for (; i != mailboxes.end(); ++i) {
    if (ref == "" || ((ref.length() <= i->first.length()) 
		      && (i->first.substr(0, ref.length()) == ref)))
      if (regexMatch(i->first, regex) == 0) {
	com << "* LIST (";
	string sep = "";
	
	int flags = i->second;
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
	    << toImapString(i->first) << endl;
      }
  }

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult ListOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after LIST");
    return res;
  }

  string mailbox;
  if ((res = expectMailbox(mailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox after LIST SPACE");
    return res;
  }

  c_in.setMailbox(mailbox);

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after LIST SPACE mailbox");
    return res;
  }

  string listmailbox;
  if ((res = expectListMailbox(listmailbox)) != ACCEPT) {
    session.setLastError("Expected list_mailbox after LIST SPACE"
			 " mailbox SPACE");
    return res;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after LIST SPACE mailbox"
			 " SPACE list_mailbox");
    return res;
  }

  c_in.setListMailbox(listmailbox);
  c_in.setName("LIST");
  return ACCEPT;
}
