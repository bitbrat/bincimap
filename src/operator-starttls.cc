/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-starttls.cc
 *  
 *  Description:
 *    Implementation of the STARTTLS command.
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

#ifdef WITH_SSL

#include <string>
#include <iostream>

#include "recursivedescent.h"
#include "io.h"
#include "io-ssl.h"
#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
StarttlsOperator::StarttlsOperator(void)
{
}

//----------------------------------------------------------------------
StarttlsOperator::~StarttlsOperator(void)
{
}

//----------------------------------------------------------------------
const string StarttlsOperator::getName(void) const
{
  return "STARTTLS";
}

//----------------------------------------------------------------------
int StarttlsOperator::getState(void) const
{
  return Session::NONAUTHENTICATED 
    | Session::AUTHENTICATED 
    | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult StarttlsOperator::process(Depot &depot,
						  Request &command)
{
  Session &session = Session::getInstance();
  IO &com = IOFactory::getInstance().get(1);

  if (session["sslmode"] != "") {
    session.setLastError("Already in TLS mode");
    return BAD;
  }

  com << command.getTag() 
      << " OK STARTTLS completed, begin TLS negotiation now" << endl;
  com.flushContent();
  com.flushOnEndl();
   
  SSLEnabledIO *sslcom;
  sslcom = dynamic_cast<SSLEnabledIO *>(&IOFactory::getInstance().get(1));
  if (!sslcom) {
    // Can this ever happen?
    session.setLastError("An internal error occurred when"
			 " entering SSL mode. ");
    return NO;
  } else {
    if (!sslcom->setModeSSL()) {
      session.setLastError(sslcom->getLastError());
      return NO;
    }
  }

  session.add("sslmode", "yes");

  return NOTHING;
}

//----------------------------------------------------------------------
Operator::ParseResult StarttlsOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF");
    return ERROR;
  } else
    return ACCEPT;
}

#endif
