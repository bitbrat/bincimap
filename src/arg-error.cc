/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    arg-error.cc
 *  
 *  Description:
 *    Implementation of error class for Argument Parser
 *
 *  Authors:
 *    Eivind Kvedalen <argparser@eivind.kvedalen.name>
 *
 *  Bugs:
 *
 *  ChangeLog:
 *
 *  --------------------------------------------------------------------
 *  Copyright 2002-2004 Eivind Kvedalen
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

#include "arg-error.h"

using namespace std;
using namespace ArgParser;

ArgumentError::ArgumentError(Type t, const string &name, const string &e)
  : argName(name),
    type(t),
    error(e)
{
}

const string & ArgumentError::getName(void) const
{
  return argName;
}

const string & ArgumentError::getErrorStr(void) const
{
  return error;
}

const ArgumentError::Type ArgumentError::getErrorType(void) const
{
  return type;
}

const set<string> & ArgumentError::getMissing(void) const
{
  return missing;
}

void ArgumentError::add(const string & s)
{
  missing.insert(s);
}
