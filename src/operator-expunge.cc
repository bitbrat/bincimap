/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-expunge.cc
 *  
 *  Description:
 *    Implementation of the EXPUNGE command
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
#include "imapparser.h"
#include "recursivedescent.h"
#include "pendingupdates.h"
#include "session.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
ExpungeOperator::ExpungeOperator(void)
{
}

//----------------------------------------------------------------------
ExpungeOperator::~ExpungeOperator(void)
{
}

//----------------------------------------------------------------------
const string ExpungeOperator::getName(void) const
{
  return "EXPUNGE";
}

//----------------------------------------------------------------------
int ExpungeOperator::getState(void) const
{
  return Session::SELECTED;
}

//----------------------------------------------------------------------
Operator::ProcessResult ExpungeOperator::process(Depot &depot,
						 Request &command)
{
  Mailbox *mailbox = depot.getSelected();
  mailbox->expungeMailbox();

  pendingUpdates(mailbox, PendingUpdates::EXPUNGE
		 | PendingUpdates::EXISTS 
		 | PendingUpdates::RECENT
		 | PendingUpdates::FLAGS, true);

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult ExpungeOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res; 
 if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF");
    return res;
  }

  c_in.setName("EXPUNGE");
  return ACCEPT;
}
