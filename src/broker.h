/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/broker.h
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
#ifndef broker_h_included
#define broker_h_included
#include "depot.h"
#include "operators.h"

#include <string>
#include <map>

namespace Binc {

  class Request;
  class Broker;

  //------------------------------------------------------------------
  class BrokerFactory {
  private:
    std::map<int, Broker *> brokers;

    //--
    BrokerFactory(void);

    mutable std::string lastError;

  public:
    Broker *getBroker(int state);
    void assign(const std::string &fname, Operator *o);
    void addCapability(const std::string &c);
    Operator *getOperator(int state, const std::string &name) const;

    inline const std::string &getLastError(void) const;
    inline void setLastError(const std::string &error) const;

    //--
    static BrokerFactory &getInstance(void);
    ~BrokerFactory(void);
  };

  //------------------------------------------------------------------
  inline const std::string &BrokerFactory::getLastError(void) const
  {
    return lastError;
  }

  //------------------------------------------------------------------
  inline void BrokerFactory::setLastError(const std::string &error) const
  {
    lastError = error;
  }

  //------------------------------------------------------------------
  class Broker {
  private:
    std::map<std::string, Operator *> operators;
    std::map<std::string, bool> deletables;

  public:
    Operator * get(const std::string &name) const;
    void assign(const std::string &fname, Operator *o, bool deletable = false);
    Operator::ParseResult parseStub(Request &cmd);

    //--
    inline Broker(Broker &);
    inline Broker(const Broker &);
    Broker(void);
    ~Broker(void);
  };

  inline Broker::Broker(Broker &)
  {
  }

  inline Broker::Broker(const Broker &)
  {
  }
}

#endif
