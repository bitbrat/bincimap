/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-login.cc
 *  
 *  Description:
 *    Implementation of the LOGIN command.
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

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include <string>
#include <iostream>

#include "recursivedescent.h"
#include "session.h"
#include "authenticate.h"

#include "io.h"

#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;


//----------------------------------------------------------------------
LoginOperator::LoginOperator(void)
{
}

//----------------------------------------------------------------------
LoginOperator::~LoginOperator(void)
{
}

//----------------------------------------------------------------------
const string LoginOperator::getName(void) const
{
  return "LOGIN";
}

//----------------------------------------------------------------------
int LoginOperator::getState(void) const
{
  return Session::NONAUTHENTICATED;
}

//------------------------------------------------------------------------
Operator::ProcessResult LoginOperator::process(Depot &depot,
					       Request &command)
{
  Session &session = Session::getInstance();
  IO &com = IOFactory::getInstance().get(1);

  const bool allowplain 
    = (session.globalconfig["Authentication"]["allow plain auth in non ssl"] == "yes");

  if (!session.command.ssl && !allowplain && !getenv("ALLOWPLAIN")) {
    session.setLastError("Plain text password authentication"
			 " is disallowed. Please try enabling SSL"
			 " or TLS in your mail client.");
    return NO;
  }

  putenv(strdup(("BINCIMAP_LOGIN=LOGIN+" + command.getTag()).c_str()));

  switch (authenticate(depot, command.getUserID(), command.getPassword())) {
  case 1: 
    session.setLastError("An internal error occurred when you attempted"
			 " to log in to the IMAP server. Please contact"
			 " your system administrator.");
    return NO;
  case 2:
    session.setLastError("Login failed. Either your user name"
			 " or your password was wrong. Please try again,"
			 " and if the problem persists, please contact"
			 " your system administrator.");
    return NO;
  case 3:
    com << "* BYE Timeout after " << session.idletimeout
	<< " seconds of inactivity." << endl;
    break;
  case -1:
    com << "* BYE The server died unexpectedly. Please contact "
      "your system administrator for more information." << endl;
    break;
  }

  // go to logout
  session.setState(Session::LOGOUT);

  return NOTHING;
}

//----------------------------------------------------------------------
Operator::ParseResult LoginOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected single SPACE after LOGIN");
    return res;
  }

  string userid;
  if ((res = expectAstring(userid)) != ACCEPT) {
    session.setLastError("Expected userid after LOGIN SPACE");
    return res;
  }

  c_in.setUserID(userid);
  
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after LOGIN SPACE userid");
    return res;
  }

  string password;
  if ((res = expectAstring(password)) != ACCEPT) {
    session.setLastError("Expected password after LOGIN "
			 "SPACE userid SPACE");
    return res;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after password");
    return res;
  }

  c_in.setPassword(password);
  c_in.setName("LOGIN");
  return ACCEPT;
}
