/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-noop-pending.cc
 *  
 *  Description:
 *    Operator for the NOOP command, with pending extension
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

#include "mailbox.h"
#include "pendingupdates.h"
#include "io.h"

#include "recursivedescent.h"
#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
NoopPendingOperator::NoopPendingOperator(void) : NoopOperator()
{
}

//----------------------------------------------------------------------
NoopPendingOperator::~NoopPendingOperator(void)
{
}

//----------------------------------------------------------------------
Operator::ProcessResult NoopPendingOperator::process(Depot &depot,
						     Request &command)
{
  Mailbox *mailbox = depot.getSelected();
  if (!pendingUpdates(mailbox, 
		      PendingUpdates::EXPUNGE
		      | PendingUpdates::EXISTS 
		      | PendingUpdates::RECENT 
		      | PendingUpdates::FLAGS, true)) {
      Session &session = Session::getInstance();
      IO &com = IOFactory::getInstance().get(1);
      IO &logger = IOFactory::getInstance().get(2);
      com << "* BYE " << session.getLastError() << endl;
      logger << "when scanning mailbox: "
	     << session.getLastError() << endl;
  }

  return OK;
}
