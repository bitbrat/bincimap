/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    convert.cc
 *  
 *  Description:
 *    Implementation of miscellaneous convertion functions.
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

#include "convert.h"
#include "io.h"
#include <string>

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
BincStream::BincStream(void)
{
}

//------------------------------------------------------------------------
BincStream::~BincStream(void)
{
  clear();
}

//------------------------------------------------------------------------
const string &BincStream::str(void) const
{
  return nstr;
}

//------------------------------------------------------------------------
void BincStream::clear(void)
{
  nstr = "";
}

//------------------------------------------------------------------------
int BincStream::getSize(void) const
{
  return nstr.length();
}

//------------------------------------------------------------------------
BincStream &BincStream::operator << (std::ostream&(*)(std::ostream&))
{
  nstr += "\r\n";
  return *this;
}

//------------------------------------------------------------------------
BincStream &BincStream::operator << (const string &t)
{
  nstr += t;
  return *this;
}

//------------------------------------------------------------------------
BincStream &BincStream::operator << (int t)
{
  nstr += toString(t);
  return *this;
}

//------------------------------------------------------------------------
BincStream &BincStream::operator << (unsigned int t)
{
  nstr += toString(t);
  return *this;
}

//------------------------------------------------------------------------
BincStream &BincStream::operator << (char t)
{
  nstr += t;
  return *this;
}
