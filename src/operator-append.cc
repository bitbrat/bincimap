/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-append.cc
 *  
 *  Description:
 *    Implementation of the APPEND command.
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

#include <algorithm>
#include <string>

#include <fcntl.h>

#include "depot.h"
#include "io.h"
#include "mailbox.h"
#include "operators.h"
#include "recursivedescent.h"
#include "pendingupdates.h"
#include "session.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
AppendOperator::AppendOperator(void)
{
}

//----------------------------------------------------------------------
AppendOperator::~AppendOperator(void)
{
}

//----------------------------------------------------------------------
const string AppendOperator::getName(void) const
{
  return "APPEND";
}

//----------------------------------------------------------------------
int AppendOperator::getState(void) const
{
  return Session::AUTHENTICATED | Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult AppendOperator::process(Depot &depot,
						Request &command)
{
  IO &com = IOFactory::getInstance().get(1);

  Session &session = Session::getInstance();

  const string &srcmailbox = command.getMailbox();
  const string &canonmailbox = toCanonMailbox(srcmailbox);
  Mailbox *mailbox = 0;

  if ((mailbox = depot.get(canonmailbox)) == 0) {
    session.setResponseCode("TRYCREATE");
    session.setLastError("invalid destination mailbox "
			 + toImapString(srcmailbox));
    return NO;
  }

  // mask all passed flags together
  unsigned int newflags = (unsigned int) Message::F_NONE;
  vector<string>::const_iterator f_i = command.flags.begin();
  while (f_i != command.flags.end()) {
    if (*f_i == "\\Deleted") newflags |= Message::F_DELETED;
    if (*f_i == "\\Answered") newflags |= Message::F_ANSWERED;
    if (*f_i == "\\Seen") newflags |= Message::F_SEEN;
    if (*f_i == "\\Draft") newflags |= Message::F_DRAFT;
    if (*f_i == "\\Flagged") newflags |= Message::F_FLAGGED;
    ++f_i;
  }
  
  int mday, year, hour, minute, second;
  char month[4];

  struct tm mytm;
  if (command.getDate() != "") {  
    sscanf(command.getDate().c_str(), "%2i-%3s-%4i %2i:%2i:%2i",
	   &mday, month, &year, &hour, &minute, &second);

    month[3] = '\0';
    string monthstr = month;
    lowercase(monthstr);
    mytm.tm_sec = second;
    mytm.tm_min = minute;
    mytm.tm_hour = hour;
    mytm.tm_year = year - 1900;
    mytm.tm_mday = mday;
    if (monthstr == "jan") mytm.tm_mon = 0;
    else if (monthstr == "feb") mytm.tm_mon = 1;
    else if (monthstr == "mar") mytm.tm_mon = 2;
    else if (monthstr == "apr") mytm.tm_mon = 3;
    else if (monthstr == "may") mytm.tm_mon = 4;
    else if (monthstr == "jun") mytm.tm_mon = 5;
    else if (monthstr == "jul") mytm.tm_mon = 6;
    else if (monthstr == "aug") mytm.tm_mon = 7;
    else if (monthstr == "sep") mytm.tm_mon = 8;
    else if (monthstr == "oct") mytm.tm_mon = 9;
    else if (monthstr == "nov") mytm.tm_mon = 10;
    else if (monthstr == "dec") mytm.tm_mon = 11;
    mytm.tm_isdst = -1;
  }

  // Read number of characters in literal. Literal is required here.
  if (com.readChar() != '{') {
    session.setLastError("expected literal");
    return BAD;
  }
  
  string nr;
  while (1) {
    int c = com.readChar();
    if (c == -1) {
      session.setLastError("unexcepted EOF");
      return BAD;
    }

    if (c == '}')
      break;
    nr += (char) c;
  }

  int nchars = atoi(nr.c_str());
  if (nchars < 0) {
    session.setLastError("expected positive size of appended message");
    return BAD;
  }

  if (com.readChar() != '\r') {
    session.setLastError("expected CR");
    return BAD;
  }

  if (com.readChar() != '\n') {
    session.setLastError("expected LF");
    return BAD;
  }

  time_t newtime = (command.getDate() != "") ? mktime(&mytm) : time(0);
  if (newtime == -1) newtime = time(0);
  Message *dest = mailbox->createMessage(depot.mailboxToFilename(canonmailbox),
					 newtime);
  if (!dest) {
    session.setLastError(mailbox->getLastError());
    return NO;
  }

  com << "+ go ahead with " << nchars << " characters" << endl;
  com.flushContent();
  com.disableInputLimit();

  while (nchars > 0) {
    // Read in chunks of 8192, followed by an optional chunk at the
    // end which is < 8192 bytes.
    string s;
    int bytesToRead = nchars > 8192 ? 8192 : nchars;
    int readBytes = com.readStr(s, bytesToRead);
    if (readBytes <= 0) {
      mailbox->rollBackNewMessages();
      session.setLastError(com.getLastError());
      return NO;
    }

    // Expect the exact number of bytes from readStr.
    if (readBytes != bytesToRead) {
      mailbox->rollBackNewMessages();
      session.setLastError("expected " + toString(nchars)
			   + " bytes, but got " + toString(readBytes));
      return NO;
    }

    // Write the chunk to the message.
    if (!dest->appendChunk(s)) {
      mailbox->rollBackNewMessages();
      session.setLastError(dest->getLastError());
      return NO;
    }

    // Update the message count.
    nchars -= readBytes;
  }

  // Read the trailing CRLF after the message data.
  if (com.readChar() != '\r') {
    mailbox->rollBackNewMessages();
    session.setLastError("expected CR");
    return BAD;
  }

  if (com.readChar() != '\n') {
    mailbox->rollBackNewMessages();
    session.setLastError("expected LF");
    return BAD;
  }

  // Commit the message.
  dest->close();
  dest->setStdFlag(newflags);
  dest->setInternalDate(mktime(&mytm));

  if (!mailbox->commitNewMessages(depot.mailboxToFilename(canonmailbox))) {
    session.setLastError("failed to commit after successful APPEND: "
			 + mailbox->getLastError());
    return NO;
  }

  if (mailbox == depot.getSelected()) {
    pendingUpdates(mailbox, PendingUpdates::EXISTS
		   | PendingUpdates::RECENT 
		   | PendingUpdates::FLAGS, true, true);
  }

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult AppendOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();
  Operator::ParseResult res;

  if (c_in.getUidMode())
    return REJECT;

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after APPEND");
    return res;
  }

  string mailbox;
  if ((res = expectMailbox(mailbox)) != ACCEPT) {
    session.setLastError("Expected mailbox after APPEND SPACE");
    return res;
  }

  c_in.setMailbox(mailbox);

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after APPEND SPACE mailbox");
    return res;
  }

  if ((res = expectThisString("(")) == ACCEPT) {
    if ((res = expectFlag(c_in.getFlags())) == ACCEPT)
      while (1) {
	if ((res = expectSPACE()) != ACCEPT)
	  break;
	if ((res = expectFlag(c_in.getFlags())) != ACCEPT) {
	  session.setLastError("expected a flag after the '('");
	  return res;
	}
      }

    if ((res = expectThisString(")")) != ACCEPT) {
      session.setLastError("expected a ')'");
      return res;
    }

    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("expected a SPACE after the flag list");
      return res;
    }
  }

  string date;
  if ((res = expectDateTime(date)) == ACCEPT)
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("expected a SPACE after date_time");
      return res;
    }

  c_in.setDate(date);
  c_in.setName("APPEND");
  return ACCEPT;
}
