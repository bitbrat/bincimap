/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    pendingupdates.cc
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
#include <iostream>
#include <string>
#include <vector>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "session.h"
#include "pendingupdates.h"
#include "message.h"
#include "mailbox.h"
#include "io.h"

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
PendingUpdates::PendingUpdates(void) : expunges(), flagupdates()
{
  recent = 0;
  exists = 0;

  newrecent = false;
  newexists = false;
}

//------------------------------------------------------------------------
PendingUpdates::~PendingUpdates(void)
{
}

//------------------------------------------------------------------------
void PendingUpdates::addExpunged(unsigned int uid)
{
  expunges.push_back(uid);
}

//------------------------------------------------------------------------
void PendingUpdates::addFlagUpdates(unsigned int uid, unsigned int flags)
{
  flagupdates[uid] = flags;
}

//------------------------------------------------------------------------
void PendingUpdates::setExists(unsigned int n)
{
  exists = n;
  newexists = true;
}

//------------------------------------------------------------------------
void PendingUpdates::setRecent(unsigned int n)
{
  recent = n;
  newrecent = true;
}

//------------------------------------------------------------------------
unsigned int PendingUpdates::getExists(void) const
{
  return exists;
}

//------------------------------------------------------------------------
unsigned int PendingUpdates::getRecent(void) const
{
  return recent;
}

//------------------------------------------------------------------------
bool PendingUpdates::newExists(void) const
{
  return newexists;
}

//------------------------------------------------------------------------
bool PendingUpdates::newRecent(void) const
{
  return newrecent;
}

//------------------------------------------------------------------------
PendingUpdates::expunged_const_iterator::expunged_const_iterator(void)
{
}

//------------------------------------------------------------------------
PendingUpdates::expunged_const_iterator::expunged_const_iterator(vector<unsigned int>::iterator i) : internal(i)
{
}

//------------------------------------------------------------------------
unsigned int PendingUpdates::expunged_const_iterator::operator * (void) const
{
  return *internal;
}

//------------------------------------------------------------------------
void PendingUpdates::expunged_const_iterator::operator ++ (void)
{
  ++internal;
}

//------------------------------------------------------------------------
bool PendingUpdates::expunged_const_iterator::operator == (expunged_const_iterator i) const
{
  return internal == i.internal;
}

//------------------------------------------------------------------------
bool PendingUpdates::expunged_const_iterator::operator != (expunged_const_iterator i) const
{
  return internal != i.internal;
}

//------------------------------------------------------------------------
PendingUpdates::expunged_const_iterator PendingUpdates::beginExpunged(void)
{
  return expunged_const_iterator(expunges.begin());
}

//------------------------------------------------------------------------
PendingUpdates::expunged_const_iterator PendingUpdates::endExpunged(void)
{
  return expunged_const_iterator(expunges.end());
}

//------------------------------------------------------------------------
PendingUpdates::flagupdates_const_iterator::flagupdates_const_iterator(void)
{
}

//------------------------------------------------------------------------
PendingUpdates::flagupdates_const_iterator::flagupdates_const_iterator(map<unsigned int, unsigned int>::iterator i) : internal(i)
{
}

//------------------------------------------------------------------------
void PendingUpdates::flagupdates_const_iterator::operator ++ (void)
{
  ++internal;
}

//------------------------------------------------------------------------
bool PendingUpdates::flagupdates_const_iterator::operator != (flagupdates_const_iterator i) const
{
  return internal != i.internal;
}

//------------------------------------------------------------------------
PendingUpdates::flagupdates_const_iterator PendingUpdates::beginFlagUpdates(void)
{
  return flagupdates_const_iterator(flagupdates.begin());
}

//------------------------------------------------------------------------
PendingUpdates::flagupdates_const_iterator PendingUpdates::endFlagUpdates(void)
{
  return flagupdates_const_iterator(flagupdates.end());
}

//------------------------------------------------------------------------
unsigned int PendingUpdates::flagupdates_const_iterator::first(void) const
{
  return internal->first;
}

//------------------------------------------------------------------------
unsigned int PendingUpdates::flagupdates_const_iterator::second(void) const
{
  return internal->second;
}

//--------------------------------------------------------------------
bool Binc::pendingUpdates(Mailbox *mailbox, int type, bool rescan)
{
  Session &session = Session::getInstance();
  IO &com = IOFactory::getInstance().get(1);

  if (mailbox == 0)
    return true;

  PendingUpdates p;
  if (!mailbox->getUpdates(rescan, type, p)) {
    session.setLastError(mailbox->getLastError());
    return false;
  }

  if (type & PendingUpdates::EXPUNGE) {
    PendingUpdates::expunged_const_iterator i = p.beginExpunged();
    PendingUpdates::expunged_const_iterator e = p.endExpunged();

    while (i != e) {
      com << "* " << *i << " EXPUNGE" << endl;
      ++i;
    }
  }

  if ((type & PendingUpdates::EXISTS) && p.newExists())
    com << "* " << p.getExists() << " EXISTS" << endl;

  if ((type & PendingUpdates::RECENT) && p.newRecent())
    com << "* " << p.getRecent() << " RECENT" << endl;

  if (type & PendingUpdates::FLAGS) {
    PendingUpdates::flagupdates_const_iterator i = p.beginFlagUpdates();
    PendingUpdates::flagupdates_const_iterator e = p.endFlagUpdates();

    while (i != e) {
      int flags = i.second();

      vector<string> flagv;
      if (flags & Message::F_SEEN) flagv.push_back("\\Seen");
      if (flags & Message::F_ANSWERED) flagv.push_back("\\Answered");
      if (flags & Message::F_DELETED) flagv.push_back("\\Deleted");
      if (flags & Message::F_DRAFT) flagv.push_back("\\Draft");
      if (flags & Message::F_RECENT) flagv.push_back("\\Recent");
      if (flags & Message::F_FLAGGED) flagv.push_back("\\Flagged");

      com << "* " << i.first() << " FETCH (FLAGS (";  
      for (vector<string>::const_iterator k
	     = flagv.begin(); k != flagv.end(); ++k) {
	if (k != flagv.begin()) com << " ";
	com << *k;
      }
	
      com << "))" << endl;

      ++i;
    }
  }

  return true;
}
