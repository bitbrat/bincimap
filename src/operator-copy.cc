/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-copy.cc
 *  
 *  Description:
 *    Implementation of the COPY command.
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
#include "maildir.h"
#include "operators.h"
#include "recursivedescent.h"
#include "session.h"
#include "convert.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
CopyOperator::CopyOperator(void)
{
}

//----------------------------------------------------------------------
CopyOperator::~CopyOperator(void)
{
}

//----------------------------------------------------------------------
const string CopyOperator::getName(void) const
{
  return "COPY";
}

//----------------------------------------------------------------------
int CopyOperator::getState(void) const
{
  return Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult CopyOperator::process(Depot &depot,
					      Request &command)
{
  Session &session = Session::getInstance();
  IO &logger = IOFactory::getInstance().get(2);

  // Get the current mailbox
  Mailbox *srcMailbox = depot.getSelected();

  // Get the destination mailbox
  string dmailbox = command.getMailbox();
  Mailbox *destMailbox = depot.get(toCanonMailbox(dmailbox));
  if (destMailbox == 0) {
    session.setResponseCode("TRYCREATE");
    session.setLastError("invalid mailbox " + toImapString(dmailbox));
    return NO;
  }

  unsigned int mode = Mailbox::SKIP_EXPUNGED;
  mode |= command.getUidMode() ? Mailbox::UID_MODE : Mailbox::SQNR_MODE;

  // Copy each message in the sequence set to the destination mailbox.
  bool success = true;
  Mailbox::iterator i = srcMailbox->begin(command.bset, mode);
  for (; success && i != srcMailbox->end(); ++i) {
    Message &source = *i;

    if (srcMailbox->fastCopy(source, *destMailbox,
			     depot.mailboxToFilename(toCanonMailbox(dmailbox))))
      continue;

    // Have the destination mailbox create a message for us.
    Message *dest 
      = destMailbox->createMessage(depot.mailboxToFilename(toCanonMailbox(dmailbox)),
				   source.getInternalDate());
    if (!dest) {
      session.setLastError(destMailbox->getLastError());
      success = false;
      break;
    }

    // Set the flags and internal date.
    dest->setStdFlag(source.getStdFlags());
    dest->setInternalDate(source.getInternalDate());

    // Copy chunks from the source message over to the destination
    // message.
    string chunk;
    do {
      int readSize = source.readChunk(chunk);

      if (readSize == 0)
	break;
      else if (readSize == -1) {
	logger << "when reading from message "
	       << i.getSqnr() << "/" << source.getUID()
	       << " in \"" << srcMailbox->getName() << "\": "
	       << source.getLastError() << endl;
	success = false;
      } else if (!dest->appendChunk(chunk)) {
	logger << "when writing to \""
	       << dmailbox << "\": "
	       << dest->getLastError() << endl;
	success = false;
      }
    } while (success);

    dest->close();
  }

  if (!success && !destMailbox->rollBackNewMessages()) {
    session.setLastError("Failed to rollback after unsuccessful copy: "
			 + destMailbox->getLastError());
    return NO;
  }

  if (success)
    if (!destMailbox->commitNewMessages(depot.mailboxToFilename(toCanonMailbox(dmailbox)))) {
      session.setLastError("Failed to commit after successful copy: "
			   + destMailbox->getLastError());
      return NO;
    }

  if (!success)
    session.setLastError("The transaction was unrolled. Please "
			 "contant your system administrator for "
			 "more information.");

  return success ? OK : NO;
}

//------------------------------------------------------------------------
Operator::ParseResult CopyOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after COPY");
    return res;
  }

  if ((res = expectSet(c_in.getSet())) != ACCEPT) {
    session.setLastError("Expected sequence set after COPY SPACE");
    return res;
  }

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after COPY SPACE set");
    return res;
  }

  string mailbox;
  if ((res = expectMailbox(mailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox after COPY SPACE set SPACE");
    return res;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after COPY SPACE set SPACE mailbox");
    return res;
  }

  c_in.setMailbox(mailbox);
  c_in.setName("COPY");

  return ACCEPT;
}
