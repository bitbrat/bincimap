/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-logout.cc
 *  
 *  Description:
 *    Implementation of the LOGOUT command
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

#include "io.h"

#include "mailbox.h"
#include "recursivedescent.h"
#include "session.h"
#include "convert.h"

#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
LogoutOperator::LogoutOperator(void)
{
}

//----------------------------------------------------------------------
LogoutOperator::~LogoutOperator(void)
{
}

//----------------------------------------------------------------------
const string LogoutOperator::getName(void) const
{
  return "LOGOUT";
}

//----------------------------------------------------------------------
int LogoutOperator::getState(void) const
{
  return Session::NONAUTHENTICATED
    | Session::AUTHENTICATED
    | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult LogoutOperator::process(Depot &depot,
						Request &command)
{
  IO &com = IOFactory::getInstance().get(1);

  com << "* BYE Binc IMAP shutting down" << endl;
  com << command.getTag() << " OK LOGOUT completed" << endl;
  com.flushContent();
  
#ifdef BINCIMAPD
  Mailbox *mailbox = 0;
  if ((mailbox = depot.getSelected()) != 0) {
    mailbox->closeMailbox();
    delete mailbox;
  }
#endif
  
  Session &session = Session::getInstance();
  session.setState(Session::LOGOUT);

  return NOTHING;
}

//----------------------------------------------------------------------
Operator::ParseResult LogoutOperator::parse(Request & c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF");
    return res;
  }

  c_in.setName("LOGOUT");
  return ACCEPT;
}
