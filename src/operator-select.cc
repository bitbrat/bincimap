/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-select.cc
 *  
 *  Description:
 *    Implementation of the SELECT command.
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
#include <string>

#include "depot.h"
#include "io.h"
#include "mailbox.h"
#include "operators.h"
#include "recursivedescent.h"
#include "storage.h"
#include "pendingupdates.h"
#include "session.h"
#include "convert.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
SelectOperator::SelectOperator(void)
{
}

//----------------------------------------------------------------------
SelectOperator::~SelectOperator(void)
{
}

//----------------------------------------------------------------------
const string SelectOperator::getName(void) const
{
  return "SELECT";
}

//----------------------------------------------------------------------
int SelectOperator::getState(void) const
{
  return Session::NONAUTHENTICATED
    | Session::AUTHENTICATED
    | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult SelectOperator::process(Depot &depot,
						Request &command)
{
  Session &session = Session::getInstance();
  IO &com = IOFactory::getInstance().get(1);
  IO &logger = IOFactory::getInstance().get(2);

  const bool examine = (command.getName() == "EXAMINE");

  const string &srcmailbox = command.getMailbox();
  const string &canonmailbox = toCanonMailbox(srcmailbox);

  Mailbox *mailbox = depot.getSelected();
  if (mailbox != 0) {
    mailbox->closeMailbox();
    mailbox = 0;
  }

  mailbox = depot.get(canonmailbox);
  if (mailbox == 0) {
    session.setLastError(depot.getLastError());
    return NO;
  }

  if (!mailbox->selectMailbox(canonmailbox,
			      depot.mailboxToFilename(canonmailbox))) {
    logger << "selecting mailbox failed" << endl;
    session.setLastError(mailbox->getLastError());
    return NO;
  }

  // find first unseen
  int unseen = -1;
  Mailbox::iterator i
    = mailbox->begin(SequenceSet::all(), Mailbox::SKIP_EXPUNGED | Mailbox::SQNR_MODE);
  for (; i != mailbox->end(); ++i) {
    Message &message = *i;

    if (unseen == -1 && ((message.getStdFlags() & Message::F_SEEN) == 0)) {
      unseen = i.getSqnr();
      break;
    }
  }

  // show pending updates with only exists and recent response. do not
  // re-scan.
  pendingUpdates(mailbox, PendingUpdates::EXISTS 
		 | PendingUpdates::RECENT,
		 false);

  // unseen
  if (unseen != -1)
    com << "*" << " OK [UNSEEN " << unseen << "] Message "
	<< unseen << " is first unseen" << endl;

  // uidvalidity
  com << "*" << " OK [UIDVALIDITY " << mailbox->getUidValidity() << "]"
      << endl;

  // uidnext
  com << "*" << " OK [UIDNEXT " << toString(mailbox->getUidNext()) << "] "
      << toString(mailbox->getUidNext()) << " is the next UID" << endl;

  // flags
  com << "*" 
      << " FLAGS (\\Answered \\Flagged \\Deleted \\Recent \\Seen \\Draft)"
      << endl;

  // permanentflags
  com << "*" 
      << " OK [PERMANENTFLAGS (\\Answered \\Flagged \\Deleted "
      << "\\Seen \\Draft)] Limited" 
      << endl;

  session.setState(Session::SELECTED);
  depot.setSelected(mailbox);

  if (examine)
    mailbox->setReadOnly();

  logger.setLogPrefix(session.getUserID() + "@" + session.getIP()
		      + ":" + srcmailbox);

  session.setLastError(examine ? "[READ-ONLY]" : "[READ-WRITE]");
  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult SelectOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();
  
  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after" + c_in.getName());
    return res;
  }

  string mailbox;  
  if ((res = expectMailbox(mailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox after " + c_in.getName()
			 + " SPACE");
    return res;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after " + c_in.getName()
			 + " SPACE mailbox");
    return res;
  }
  
  c_in.setMailbox(mailbox);
  return ACCEPT;
}
