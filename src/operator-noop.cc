/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-noop.cc
 *  
 *  Description:
 *    Operator for the NOOP command.
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

#include "recursivedescent.h"
#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
NoopOperator::NoopOperator(void)
{
}

//----------------------------------------------------------------------
NoopOperator::~NoopOperator(void)
{
}

//----------------------------------------------------------------------
const string NoopOperator::getName(void) const
{
  return "NOOP";
}

//----------------------------------------------------------------------
int NoopOperator::getState(void) const
{
  return Session::NONAUTHENTICATED
    | Session::AUTHENTICATED
    | Session::SELECTED;
}

//----------------------------------------------------------------------
Operator::ProcessResult NoopOperator::process(Depot &depot,
					      Request &command)
{
  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult NoopOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after NOOP");
    return res;
  }

  c_in.setName("NOOP");
  return ACCEPT;
}
