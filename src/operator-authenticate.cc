/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-authenticate.cc
 *  
 *  Description:
 *    Implementation of the AUTHENTICATE command.
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

#include "authenticate.h"
#include "depot.h"
#include "io.h"
#include "session.h"
#include "operators.h"
#include "recursivedescent.h"
#include "base64.h"
#include "convert.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
AuthenticateOperator::AuthenticateOperator(void)
{
}

//----------------------------------------------------------------------
AuthenticateOperator::~AuthenticateOperator(void)
{
}

//----------------------------------------------------------------------
const string AuthenticateOperator::getName(void) const
{
  return "AUTHENTICATE";
}

//----------------------------------------------------------------------
int AuthenticateOperator::getState(void) const
{
  return Session::NONAUTHENTICATED;
}

//------------------------------------------------------------------------
Operator::ProcessResult AuthenticateOperator::process(Depot &depot, 
						      Request &command)
{
  Session &session = Session::getInstance();
  IO &com = IOFactory::getInstance().get(1);

  string authtype = command.getAuthType();
  uppercase(authtype);

  string username;
  string password;

  bool allowplain = (session.globalconfig["Authentication"]["allow plain auth in non ssl"] == "yes");
    

  // for now, we only support LOGIN.
  if (authtype == "LOGIN") {
    // we only allow this type of authentication over a plain
    // connection if it is passed as argument or given in the conf
    // file.
    if (!session.command.ssl && !allowplain && !getenv("ALLOWPLAIN")) {
      session.setLastError("Plain text password authentication"
			   " is disallowed. Please try enabling SSL"
			   " or TLS in your mail client.");
      return NO;
    }
    
    com << "+ " << base64encode("User Name") << endl;
    com.flushContent();

    // Read user name
    for (;;) {
      int c = com.readChar();
      if (c == -1)
	return BAD;

      if ((char) c == '\n')
	break;

      username += c;
    }
    
    if (username != "" && username[0] == '*') {
      session.setLastError("Authentication cancelled by user");
      return NO;
    }
    
    com << "+ " << base64encode("Password") << endl;
    com.flushContent();

    // Read password    
    for (;;) {
      int c = com.readChar();
      if (c == -1)
	return BAD;

      if ((char)c == '\n')
	break;
       
      password += c;
    }
    
    if (password != "" && password[0] == '*') {
      session.setLastError("Authentication cancelled by user");
      return NO;
    }

    username = base64decode(username);
    password = base64decode(password);

  } else if (authtype == "PLAIN") {
    // we only allow this type of authentication over an SSL encrypted
    // connection.
    if (!session.command.ssl && !allowplain && !getenv("ALLOWPLAIN")) {
      session.setLastError("Plain text password authentication"
			   " is disallowed. Please try enabling SSL"
			   " or TLS in your mail client.");
      return NO;
    }

    com << "+ " << endl;
    com.flushContent();
    
    string b64;
    for (;;) {
      int c = com.readChar();
      if (c == -1) {
	session.setLastError("unexpected EOF");
	return BAD;
      }

      if ((char) c == '\r') {
	com.readChar();
	break;
      }
      
      b64 += (char) c;
    }

    if (b64.size() >= 1 && b64[0] == '*') {
      session.setLastError("Authentication cancelled by user");
      return NO;
    }

    string plain = base64decode(b64);
    string::size_type pos;
    if ((pos = plain.find('\0')) == string::npos) {
      session.setLastError("Authentication failed. In PLAIN mode,"
			   " there must be at least two null characters"
			   " in the input string, but none were found");
      return NO;
    }

    plain = plain.substr(pos + 1);
    if ((pos = plain.find('\0')) == string::npos) {
      session.setLastError("Authentication failed. In PLAIN mode,"
			   " there must be at least two null characters"
			   " in the input string, but only one was found");
      return NO;
    }

    username = plain.substr(0, pos);
    password = plain.substr(pos + 1);
  } else {
    session.setLastError("The authentication method " 
			 + toImapString(authtype) + " is not supported."
			 " Please try again with a different method."
			 " There is built in support for \"PLAIN\""
			 " and \"LOGIN\".");
    return NO;
  }

  putenv(strdup(("BINCIMAP_LOGIN=AUTHENTICATE+" + command.getTag()).c_str()));

  // the authenticate function calls a stub which does the actual
  // authentication. the function returns 0 (success), 1 (internal
  // error) or 2 (failed)
  switch (authenticate(depot,
		       (const string &)username,
		       (const string &)password)) {
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

  // auth was ok. go to logout state
  session.setState(Session::LOGOUT);
  return NOTHING;
}


//----------------------------------------------------------------------
Operator::ParseResult AuthenticateOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected single SPACE after AUTHENTICATE");
    return res;
  }

  string authtype;
  if ((res = expectAtom(authtype)) != ACCEPT) {
    session.setLastError("Expected auth_type after AUTHENTICATE SPACE");
    return ERROR;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after AUTHENTICATE SPACE auth_type");
    return res;
  }

  c_in.setAuthType(authtype);

  c_in.setName("AUTHENTICATE");
  return ACCEPT;
}
