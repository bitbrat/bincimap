/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-close.cc
 *  
 *  Description:
 *    Implementation of the CLOSE command.
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
#include "session.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
CloseOperator::CloseOperator(void)
{
}

//----------------------------------------------------------------------
CloseOperator::~CloseOperator(void)
{
}

//------------------------------------------------------------------------
const string CloseOperator::getName(void) const
{
  return "CLOSE";
}

//------------------------------------------------------------------------
int CloseOperator::getState(void) const
{
  return Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult CloseOperator::process(Depot &depot,
					       Request &command)
{
  IO &logger = IOFactory::getInstance().get(2);

  Mailbox *mailbox = depot.getSelected();
  mailbox->expungeMailbox();
  mailbox->closeMailbox();

  Session &session = Session::getInstance();
  session.setState(Session::AUTHENTICATED);

  logger.setLogPrefix(session.getUserID() + "@" + session.getIP() + ":");

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult CloseOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after CLOSE");
    return res;
  }

  c_in.setName("CLOSE");
  return ACCEPT;
}
