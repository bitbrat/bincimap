/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    broker.cc
 *  
 *  Description:
 *    <--->
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
#include <map>
#include <string>

#include "broker.h"
#include "io.h"
#include "convert.h"
#include "operators.h"
#include "recursivedescent.h"
#include "session.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
BrokerFactory::BrokerFactory(void)
{
  brokers[Session::NONAUTHENTICATED] = new Broker();
  brokers[Session::AUTHENTICATED] = new Broker();
  brokers[Session::SELECTED] = new Broker();
}

//----------------------------------------------------------------------
BrokerFactory::~BrokerFactory(void)
{
  for (map<int, Broker *>::iterator i = brokers.begin(); 
       i != brokers.end(); ++i)
    delete i->second;
}

//----------------------------------------------------------------------
BrokerFactory &BrokerFactory::getInstance(void)
{
  static BrokerFactory brokerfactory;
  return brokerfactory;
}

//----------------------------------------------------------------------
void BrokerFactory::addCapability(const std::string &c)
{
  for (map<int, Broker *>::iterator i = brokers.begin(); 
       i != brokers.end(); ++i) {
    CapabilityOperator * o;
    o = dynamic_cast<CapabilityOperator*>(i->second->get("CAPABILITY"));
    if (o != 0) {
      o->addCapability(c);
      break;
    }
  }
}

//----------------------------------------------------------------------
void BrokerFactory::assign(const string &fname, Operator *o)
{
  int deletable = true;
  for (map<int, Broker *>::iterator i = brokers.begin(); 
       i != brokers.end(); ++i)
    if (i->first & o->getState()) {
      i->second->assign(fname, o, deletable);
      deletable = false;
    }
}

//----------------------------------------------------------------------
Operator *BrokerFactory::getOperator(int state, const string &name) const
{
  if (brokers.find(state) == brokers.end())
    return 0;
  else
    return brokers.find(state)->second->get(name);
}

//----------------------------------------------------------------------
Broker *BrokerFactory::getBroker(int state)
{
  if (brokers.find(state) == brokers.end()) {
    setLastError("No appropriate broker for state.");
    return 0;
  }

  return brokers[state];
}

//----------------------------------------------------------------------
Broker::Broker(void)
{
}

//----------------------------------------------------------------------
Broker::~Broker(void)
{
}

//----------------------------------------------------------------------
void Broker::assign(const string &fname, Operator *o, bool deletable)
{
  deletables[fname] = deletable;
  operators[fname] = o;
}

//----------------------------------------------------------------------
Operator *Broker::get(const string &name) const
{
  if (operators.find(name) == operators.end())
    return 0;

  return operators.find(name)->second;
}

//----------------------------------------------------------------------
Operator::ParseResult Broker::parseStub(Request &command)
{
  Session &session = Session::getInstance();

  string tag;
  string cmd;

  switch (expectTag(tag)) {
  case Operator::ACCEPT:
    break;
  case Operator::REJECT:
    session.setLastError("Syntax error; first token must be a tag");
  case Operator::ERROR:
    return Operator::ERROR;
  case Operator::TIMEOUT:
    return Operator::TIMEOUT;
  }

  switch (expectSPACE()) {
  case Operator::ACCEPT:
    break;
  case Operator::REJECT:
    session.setLastError("Syntax error; second token must be a SPACE");
  case Operator::ERROR:
    return Operator::ERROR;
  case Operator::TIMEOUT:
    return Operator::TIMEOUT;
  }

  switch (expectAstring(cmd)) {
  case Operator::ACCEPT:
    break;
  case Operator::REJECT:
    session.setLastError("Syntax error; third token must be a command");
  case Operator::ERROR:
    return Operator::ERROR;
  case Operator::TIMEOUT:
    return Operator::TIMEOUT;
  }

  uppercase(cmd);

  if (cmd == "UID") {
      command.setUidMode();

      switch (expectSPACE()) {
      case Operator::ACCEPT:
	break;
      case Operator::REJECT:
	session.setLastError("Syntax error; after UID there"
			     " must come a SPACE");
      case Operator::ERROR:
	return Operator::ERROR;
      case Operator::TIMEOUT:
	return Operator::TIMEOUT;
      }

      switch (expectAstring(cmd)) {
      case Operator::ACCEPT:
	break;
      case Operator::REJECT:
	session.setLastError("Syntax error; after UID "
			     "SPACE there must come a command");
      case Operator::ERROR:
	return Operator::ERROR;
      case Operator::TIMEOUT:
	return Operator::TIMEOUT;
      }

      uppercase(cmd);
    }

  command.setTag(tag);
  command.setName(cmd);

  return Operator::ACCEPT;
}
