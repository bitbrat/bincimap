/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    mailbox.cc
 *  
 *  Description:
 *    Implementation of the Mailbox class.
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

#include "mailbox.h"
#include "message.h"
#include "io.h"

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
Mailbox::BaseIterator::BaseIterator(int sqn)
{
  sqnr = sqn;
}

//------------------------------------------------------------------------
Mailbox::BaseIterator::~BaseIterator(void)
{
}

//------------------------------------------------------------------------
Mailbox::Mailbox(void) : readOnly(false)
{
}

//------------------------------------------------------------------------
Mailbox::~Mailbox(void)
{
}

//------------------------------------------------------------------------
bool Mailbox::isReadOnly(void) const
{
  return readOnly;
}

//------------------------------------------------------------------------
void Mailbox::setReadOnly(void)
{
  readOnly = true;
}

//------------------------------------------------------------------------
Mailbox::iterator::iterator(BaseIterator &i)
  : realIterator(i)
{
}

//------------------------------------------------------------------------
Message &Mailbox::iterator::operator *(void)
{
  return *realIterator;
}

//------------------------------------------------------------------------
void Mailbox::iterator::operator ++(void)
{
  ++realIterator;
}

//------------------------------------------------------------------------
bool Mailbox::iterator::operator ==(const iterator &i) const
{
  return realIterator == i.realIterator;
}

//------------------------------------------------------------------------
bool Mailbox::iterator::operator !=(const iterator &i) const
{
  return realIterator != i.realIterator;
}

//------------------------------------------------------------------------
void Mailbox::iterator::erase(void)
{
  realIterator.erase();
}

//------------------------------------------------------------------------
unsigned int Mailbox::iterator::getSqnr(void) const
{
  return realIterator.sqnr;
}

//------------------------------------------------------------------------
void Mailbox::setName(const string &name)
{
  this->name = name;
}

//------------------------------------------------------------------------
const string Mailbox::getName(void) const
{
  return name;
}

//------------------------------------------------------------------------
const string &Mailbox::getLastError(void) const
{
  return lastError;
}

//------------------------------------------------------------------------
void Mailbox::setLastError(const string &error) const
{
  lastError = error;
}
