/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    bincimapd-create.cc
 *  
 *  Description:
 *    Implementation of the CREATE command.
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

#include "depot.h"
#include "io.h"
#include "mailbox.h"
#include "operators.h"
#include "imapparser.h"
#include "recursivedescent.h"
#include "session.h"
#include "convert.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
CreateOperator::CreateOperator(void)
{
}

//----------------------------------------------------------------------
CreateOperator::~CreateOperator(void)
{
}

//----------------------------------------------------------------------
const string CreateOperator::getName(void) const
{
  return "CREATE";
}

//----------------------------------------------------------------------
int CreateOperator::getState(void) const
{
  return Session::AUTHENTICATED | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult CreateOperator::process(Depot &depot,
						Request &command)
{
  if (depot.createMailbox(command.getMailbox()))
    return OK;
  else {
    Session &session = Session::getInstance();
    session.setLastError(depot.getLastError());
    return NO;
  }
}

//----------------------------------------------------------------------
Operator::ParseResult CreateOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after CREATE");
    return res;
  }

  string mailbox;
  if ((res = expectMailbox(mailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox after CREATE SPACE");
    return res;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after CREATE SPACE CRLF");
    return res;
  }

  session.mailboxchanges = true;

  c_in.setName("CREATE");
  c_in.setMailbox(mailbox);
  return ACCEPT;
}
