/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    bincimapd-capability.cc
 *  
 *  Description:
 *    Implementation of the CAPABILITY command.
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
#include "operators.h"
#include "recursivedescent.h"
#include "session.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
CapabilityOperator::CapabilityOperator(void)
{
}

//----------------------------------------------------------------------
CapabilityOperator::~CapabilityOperator(void)
{
}

//----------------------------------------------------------------------
const string CapabilityOperator::getName(void) const
{
  return "CAPABILITY";
}

//----------------------------------------------------------------------
int CapabilityOperator::getState(void) const
{
  return Session::NONAUTHENTICATED 
    | Session::AUTHENTICATED
    | Session::SELECTED;
}

//----------------------------------------------------------------------
void CapabilityOperator::addCapability(const string &cap)
{
  capabilities.push_back(cap);
}

//----------------------------------------------------------------------
Operator::ProcessResult CapabilityOperator::process(Depot &depot,
						    Request &command)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  com << "* CAPABILITY IMAP4rev1";

  if (session.getState() == Session::NONAUTHENTICATED) {
    if (!session.command.ssl
	&& (session.globalconfig["Authentication"]["disable starttls"] != "yes"))
      com << " STARTTLS";

    const bool allowplain 
      = (session.globalconfig["Authentication"]["allow plain auth in non ssl"] == "yes");
    
    if (session.command.ssl || allowplain || getenv("ALLOWPLAIN"))
      com << " AUTH=LOGIN AUTH=PLAIN";
    else
      com << " LOGINDISABLED";
  }

  vector<string>::const_iterator i = capabilities.begin();
  while (i != capabilities.end()) {
    com << " " << *i;
    ++i;
  }
  com << endl;

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult CapabilityOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after CAPABILITY");
    return res;
  }

  c_in.setName("CAPABILITY");
  return ACCEPT;
}
