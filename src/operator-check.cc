/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-check.cc
 *  
 *  Description:
 *    Implementation of the CHECK command.
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
#include "pendingupdates.h"
#include "session.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
CheckOperator::CheckOperator(void)
{
}


//----------------------------------------------------------------------
CheckOperator::~CheckOperator(void)
{
}

//----------------------------------------------------------------------
const string CheckOperator::getName(void) const
{
  return "CHECK";
}

//----------------------------------------------------------------------
int CheckOperator::getState(void) const
{
  return Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult CheckOperator::process(Depot &depot,
					       Request &command)
{
  Mailbox *mailbox = depot.getSelected();
  if (mailbox != 0)
    pendingUpdates(mailbox, PendingUpdates::FLAGS 
		   | PendingUpdates::EXISTS
		   | PendingUpdates::RECENT, true);

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult CheckOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after CHECK");
    return res;
  }

  c_in.setName("CHECK");
  return ACCEPT;
}

