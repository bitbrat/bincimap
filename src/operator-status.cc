/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-status.cc
 *  
 *  Description:
 *    Implementation of the STATUS command.
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

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "io.h"
#include "mailbox.h"
#include "status.h"
#include "storage.h"
#include "convert.h"

#include "recursivedescent.h"

#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
StatusOperator::StatusOperator(void)
{
}

//----------------------------------------------------------------------
StatusOperator::~StatusOperator(void)
{
}

//----------------------------------------------------------------------
const string StatusOperator::getName(void) const
{
  return "STATUS";
}

//----------------------------------------------------------------------
int StatusOperator::getState(void) const
{
  return Session::AUTHENTICATED | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult StatusOperator::process(Depot &depot,
						Request &command)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  Status status;
  if (!depot.getStatus(command.getMailbox(), status)) {
    session.setLastError(depot.getLastError());
    return NO;
  }

  com << "* STATUS " << toImapString(command.getMailbox()) << " (";
    
  string prefix;
  for (vector<string>::const_iterator i = command.statuses.begin();
       i != command.statuses.end(); ++i) {
    string tmp = *i;
    uppercase(tmp);
    if (tmp == "UIDNEXT") {
      com << prefix << "UIDNEXT " << status.getUidNext(); prefix = " ";
    } else if (tmp == "MESSAGES") {
      com << prefix << "MESSAGES " << status.getMessages(); prefix = " ";
    } else if (tmp == "RECENT") {
      com << prefix << "RECENT " << status.getRecent(); prefix = " ";
    } else if (tmp == "UIDVALIDITY") {
      com << prefix << "UIDVALIDITY " << status.getUidValidity(); prefix = " ";
    } else if (tmp == "UNSEEN") {
      com << prefix << "UNSEEN " << status.getUnseen(); prefix = " ";
    }
  }
  com << ")" << endl;

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult StatusOperator::parse(Request &c_in) const
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

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE");
    return res;
  }

  if ((res = expectThisString("(")) != ACCEPT) {
    session.setLastError("Expected (");
    return res;
  }

  while (1) {
    if ((res = expectThisString("MESSAGES")) == ACCEPT)
      c_in.getStatuses().push_back("MESSAGES");
    else if ((res = expectThisString("RECENT")) == ACCEPT)
      c_in.getStatuses().push_back("RECENT");
    else if ((res = expectThisString("UIDNEXT")) == ACCEPT)
      c_in.getStatuses().push_back("UIDNEXT");
    else if ((res = expectThisString("UIDVALIDITY")) == ACCEPT)
      c_in.getStatuses().push_back("UIDVALIDITY");
    else if ((res = expectThisString("UNSEEN")) == ACCEPT)
      c_in.getStatuses().push_back("UNSEEN");
    else {
      session.setLastError("Expected status_att");
      return res;
    }

    if (expectSPACE() != ACCEPT)
      break;
  }

  if ((res = expectThisString(")")) != ACCEPT) {
    session.setLastError("Expected )");
    return res;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF");
    return ERROR;
  }

  return ACCEPT;
}
