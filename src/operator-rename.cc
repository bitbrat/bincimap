/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-rename.cc
 *  
 *  Description:
 *    Implementation of the RENAME command.
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

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mailbox.h"
#include "io.h"
#include "storage.h"
#include "convert.h"

#include "recursivedescent.h"

#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
RenameOperator::RenameOperator(void)
{
}

//----------------------------------------------------------------------
RenameOperator::~RenameOperator(void)
{
}

//----------------------------------------------------------------------
const string RenameOperator::getName(void) const
{
  return "RENAME";
}

//----------------------------------------------------------------------
int RenameOperator::getState(void) const
{
  return Session::AUTHENTICATED | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult RenameOperator::process(Depot &depot,
						Request &command)
{
  Session &session = Session::getInstance();

  const string &srcmailbox = command.getMailbox();
  const string &canonmailbox = toCanonMailbox(srcmailbox);
  const string &canondestmailbox = toCanonMailbox(command.getNewMailbox());

  // renaming INBOX should actually create the destination mailbox,
  // move over all the messages and then leave INBOX empty.
  if (canonmailbox == "INBOX") {
    session.setLastError("Sorry, renaming INBOX is not yet supported"
			 " by this IMAP server. Try copying the messages"
			 " instead");
    return NO;
  }

  if (canondestmailbox == "INBOX") {
    session.setLastError("It is not allowed to rename a mailbox to INBOX");
    return NO;
  }

  if (depot.renameMailbox(canonmailbox, canondestmailbox))
    return OK;
  else
    return NO;
}

//----------------------------------------------------------------------
Operator::ParseResult RenameOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();
 
  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after RENAME");
    return res;
  }

  string mailbox;
  if ((res = expectMailbox(mailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox after RENAME SPACE");
    return res;
  }

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after RENAME SPACE mailbox");
    return res;
  }

  string newmailbox;
  if ((res = expectMailbox(newmailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox after RENAME SPACE"
			 " mailbox SPACE");
    return res;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after RENAME SPACE"
			 " mailbox SPACE mailbox");
    return res;
  }

  session.mailboxchanges = true;

  c_in.setName("RENAME");
  c_in.setMailbox(mailbox);
  c_in.setNewMailbox(newmailbox);
  return ACCEPT;
}
