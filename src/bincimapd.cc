/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    bincimapd.cc
 *  
 *  Description:
 *    Implementation of the main bincimapd service
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

#include "broker.h"
#include "depot.h"
#include "recursivedescent.h"
#include "io.h"
#include "operators.h"
#include "session.h"

#include "maildirmessage.h"
#include "maildir.h"

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
int main(int argc, char *argv[]) 
{
  Session &session = Session::getInstance();
  if (!session.initialize(argc, argv))
    return 111;

  IO &com = IOFactory::getInstance().get(1);
  IO &logger = IOFactory::getInstance().get(2);

  logger << "<" << session.getUserID()
	 << "> authenticated" << endl;

  logger.flushContent();
  bool recovery = false;
  bool timeout = false;
  bool disconnected = false;
  bool abrt = false;

  // Read requests and run functions
  do {
    com.enableInputLimit();

    // recover from syntax error. There will be trash in the input
    // buffer. We need to flush everything until we see an LF.
    if (recovery) {
      for (;;) {
	int c = com.readChar();
	if (c == '\n') break;
	if (c == -1) {
	  disconnected = true;
	  abrt = true;
	  break;
	}

	if (c == -2) {
	  timeout = true;
	  abrt = true;
	  break;
	}
      }
      
      if (abrt)
	break;
    }

    Request request;
    recovery = false;

    BrokerFactory &brokerFactory = BrokerFactory::getInstance();
    Broker *broker = brokerFactory.getBroker(session.getState());
    if (!broker) {
      // will never happen
    }    

    com.noFlushOnEndl();
    com.flushContent();
    com.enableInputLimit();

    switch (broker->parseStub(request)) {
    case Operator::TIMEOUT:
      com << "* BYE Timeout after " << session.timeout()
	  << " seconds of inactivity." << endl;
      timeout = true;
      abrt = true;
      break;
    case Operator::REJECT:
      com << "* NO " << session.getLastError() << endl;
      recovery = true;
      continue;
    case Operator::ERROR:
      com << "* BAD " << session.getLastError() << endl;
      recovery = true;
      continue;
    default:
      break;
    }

    if (abrt)
      break;
    
    Operator *o = broker->get(request.getName());
    if (!o) {
      com << "* NO The command \"" 
	  << (request.getUidMode() ? "UID " : "") << request.getName()
	  << "\" is not supported." << endl;
      recovery = true;
      continue;
    }

    switch (o->parse(request)) {
    case Operator::TIMEOUT:
      com << "* BYE Timeout after " << session.timeout()
	  << " seconds of inactivity." << endl;
      timeout = true;
      abrt = true;
      break;
    case Operator::REJECT:
      com << "* NO " << session.getLastError() << endl;
      recovery = true;
      continue;
    case Operator::ERROR:
      com << "* BAD " << session.getLastError() << endl;
      recovery = true;
      continue;
    default:
      break;
    }

    session.addStatement();
    Depot *dep = session.getDepot();

    switch (o->process(*dep, request)) {
    case Operator::OK:
      com << request.getTag() << " OK " << request.getName()
	  << " completed" << endl;
      break;
    case Operator::NO: 
      com << request.getTag() << " NO " << session.getResponseCode()
          << request.getName() << " failed: " << session.getLastError() << endl;
      session.clearResponseCode();
      break;
    case Operator::BAD:
      com << request.getTag() << " BAD " << request.getName() 
	  << " failed: " << session.getLastError() << endl;
      break;
    case Operator::NOTHING:
      break;
    case Operator::ABORT:
      session.setState(Session::LOGOUT);
      abrt = true;
      break;
    }

    if (abrt)
      break;

  } while (session.getState() != Session::LOGOUT);

  if (abrt) {
    logger << "shutting down (";
    if (timeout)
      logger << "timeout after " << session.idletimeout << "s";
    else
      logger << "client disconnected";
    logger << ") - bodies:" 
	   << session.getBodies() << " statements:"
	   << session.getStatements() << endl;
  } else {
    logger << "<" << session.getUserID() << "> logged off - bodies:" 
	   << session.getBodies() << " statements:"
	   << session.getStatements() << endl;
  }

  com.flushContent();

  return timeout ? 113 : 0;
}
