/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-store.cc
 *  
 *  Description:
 *    Implementation of the STORE command.
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

#include <string>
#include <iostream>
    
#include "imapparser.h"
#include "mailbox.h"
#include "pendingupdates.h"
#include "io.h"

#include "recursivedescent.h"

#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
StoreOperator::StoreOperator(void)
{
}

//----------------------------------------------------------------------
StoreOperator::~StoreOperator(void)
{
}

//----------------------------------------------------------------------
const string StoreOperator::getName(void) const
{
  return "STORE";
}

//----------------------------------------------------------------------
int StoreOperator::getState(void) const
{
  return Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult StoreOperator::process(Depot &depot,
					       Request &command)
{
  Mailbox *mailbox = depot.getSelected();

  // mask all passed flags together
  unsigned int newflags = (unsigned int) Message::F_NONE;
  vector<string>::const_iterator f_i = command.flags.begin();
  while (f_i != command.flags.end()) {
    if (*f_i == "\\Deleted") newflags |= Message::F_DELETED;
    if (*f_i == "\\Answered") newflags |= Message::F_ANSWERED;
    if (*f_i == "\\Seen") newflags |= Message::F_SEEN;
    if (*f_i == "\\Draft") newflags |= Message::F_DRAFT;
    if (*f_i == "\\Flagged") newflags |= Message::F_FLAGGED;
    ++f_i;
  }

  // pass through all messages
  unsigned int mode 
    = command.getUidMode() ? Mailbox::UID_MODE : Mailbox::SQNR_MODE;

  Mailbox::iterator i
    = mailbox->begin(command.bset, Mailbox::SKIP_EXPUNGED | mode);

  for (; i != mailbox->end(); ++i) {
    Message &message = *i;

    // get and reset the old flags
    unsigned int oldflags = (unsigned int) message.getStdFlags();
    unsigned int flags = oldflags;
    
    bool recent = (flags & Message::F_RECENT) != 0;
    flags &= (~Message::F_RECENT);

    // add, remove or set flags
    if (command.getMode()[0] == '+') flags |= newflags;
    else if (command.getMode()[0] == '-') flags &= ~newflags;
    else flags = newflags;

    // set new flags, even if they weren't changed.
    if (recent) flags |= Message::F_RECENT;
    message.resetStdFlags();
    message.setStdFlag(flags);
  }

  // commit flag changes to mailbox (might change mailbox)
  mailbox->updateFlags();

  // check mailbox for updates, and report them
  if (command.getMode().find(".SILENT") != string::npos)
    pendingUpdates(mailbox,
		   PendingUpdates::EXISTS
		   | PendingUpdates::RECENT, false);
  else
    pendingUpdates(mailbox,
		   PendingUpdates::EXISTS 
		   | PendingUpdates::RECENT 
		   | PendingUpdates::FLAGS, false);

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult StoreOperator::parse(Request & c_in) const
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE");
    return res;
  }

  if ((res = expectSet(c_in.getSet())) != ACCEPT) {
    session.setLastError("Expected Set");
    return res;
  }

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE");
    return res;
  }

  string mode;
  if ((res = expectThisString("+")) == ACCEPT)
    mode = "+";
  else if ((res = expectThisString("-")) == ACCEPT)
    mode = "-";

  if ((res = expectThisString("FLAGS")) != ACCEPT) {
    session.setLastError("Expected FLAGS");
    return res;
  } else
    mode += "FLAGS";

  if ((res = expectThisString(".SILENT")) == ACCEPT)
    mode += ".SILENT";

  c_in.setMode(mode);

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE");
    return res;
  }

  bool paren = false;
  if ((res = expectThisString("(")) == ACCEPT)
    paren = true;
     
  if ((res = expectFlag(c_in.getFlags())) == ACCEPT)
    while (1) {
      if ((res = expectSPACE()) != ACCEPT)
	break;

      if ((res = expectFlag(c_in.getFlags())) != ACCEPT) {
	session.setLastError("Expected flag after SPACE");
	return res;
      }
    }

  if (paren)
    if ((res = expectThisString(")")) != ACCEPT) {
      session.setLastError("Expected )");
      return res;
    }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF");
    return res;
  }

  return ACCEPT;
}
