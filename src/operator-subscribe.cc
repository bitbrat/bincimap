/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-subscribe.cc
 *  
 *  Description:
 *    Implementation of the SUBSCRIBE command.
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

#include "storage.h"
#include "io.h"
#include "convert.h"

#include <sys/stat.h>

#include "recursivedescent.h"

#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
SubscribeOperator::SubscribeOperator(void)
{
}

//----------------------------------------------------------------------
SubscribeOperator::~SubscribeOperator(void)
{
}

//----------------------------------------------------------------------
const string SubscribeOperator::getName(void) const
{
  return "SUBSCRIBE";
}

//----------------------------------------------------------------------
int SubscribeOperator::getState(void) const
{
  return Session::AUTHENTICATED | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult SubscribeOperator::process(Depot &depot,
						   Request &command)
{
  Session &session = Session::getInstance();

  const string &srcmailbox = command.getMailbox();
  const string &canonmailbox = toCanonMailbox(srcmailbox);

  session.loadSubscribes();
  session.subscribeTo(canonmailbox);
  session.saveSubscribes();

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult SubscribeOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  if (c_in.getUidMode())
    return REJECT;

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE");
    return res;
  }

  string mailbox;
  if ((res = expectMailbox(mailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox");
    return res;
  }
  c_in.setMailbox(mailbox);

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF");
    return res;
  }

  return ACCEPT;
}
