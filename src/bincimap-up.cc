/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    bincimap-up.cc
 *  
 *  Description:
 *    Implementation of the preauthenticated bincimap stub
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

#include "broker.h"
#include "recursivedescent.h"
#include "io.h"
#ifdef WITH_SSL
#include "io-ssl.h"
#endif
#include "operators.h"
#include "session.h"

using namespace ::std;
using namespace Binc;

namespace Binc {
  bool showGreeting(void);
}

//------------------------------------------------------------------------
int main(int argc, char *argv[]) 
{
  Session &session = Session::getInstance();
  if (!session.initialize(argc, argv)) {
    if (session.getLastError() != "") {
      IO &logger = IOFactory::getInstance().get(2);
      logger << "error initializing Binc IMAP: " << session.getLastError()
	     << endl;
      logger.flushContent();
    }
    return 111;
  }

  IO &com = IOFactory::getInstance().get(1);
  IO &logger = IOFactory::getInstance().get(2);

  logger << "connection from " << session.getIP() << endl;

  // Show standard greeting
  showGreeting();
  bool recovery = false;
  bool timedout = false;
  bool disconnect = false;
  bool abrt = false;

  // Read commands and run functions
  do {
    com.enableInputLimit();

    // recover from syntax error. There will be trash in the input
    // buffer. We need to flush everything until we see an LF.
    if (recovery) {
      for (;;) {
	int c = com.readChar(session.timeout());
	if (c == '\n') break;
	if (c == -1) {
	  disconnect = true;
	  abrt = true;
	  break;
	} else if (c == -2) {
	  com << "* BYE Timeout after " << session.timeout()
	      << " seconds of inactivity." << endl;
	  timedout = true;
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

    com.flushContent();
    com.noFlushOnEndl();

    switch (broker->parseStub(request)) {
    case Operator::TIMEOUT:
      com << "* BYE Timeout after " << session.timeout()
	  << " seconds of inactivity." << endl;
      timedout = true;
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
	  << (request.getUidMode() ? "UID " : "")
	  << request.getName()
	  << "\" is unsupported in this state. "
	  << "Please authenticate." << endl;
      recovery = true;
      continue;
    }

    switch (o->parse(request)) {
    case Operator::TIMEOUT:
      com << "* BYE Timeout after " << session.timeout()
	  << " seconds of inactivity." << endl;
      timedout = true;
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

    switch (o->process(*session.getDepot(), request)) {
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
      recovery = true;
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

  logger << "shutting down ";
  if (timedout)
    logger << "(timeout " + toString(session.timeout()) + "s) ";
  else if (disconnect)
    logger << "(" << com.getLastError() << ") ";
  
  logger << "- read:"
	 << session.getReadBytes() 
	 << " bytes, wrote:" << session.getWriteBytes() 
	 << " bytes." << endl;
  
  com.flushContent();
}
