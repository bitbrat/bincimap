/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    arg-arg.cc
 *  
 *  Description:
 *    Implementation of standard command line types
 *
 *  Authors:
 *    Eivind Kvedalen <argparser@eivind.kvedalen.name>
 *
 *  Bugs:
 *
 *  ChangeLog:
 *
 *  --------------------------------------------------------------------
 *  Copyright 2003 Eivind Kvedalen
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

#include "arg-arg.h"
#include <stdlib.h>

using namespace ArgParser;
using namespace std;

void ArgParser::setValue(const string &name, int &value, const char * a, ArgumentError & error) 
{
  if (a != NULL) {
    char * end;
    value = strtol(a, &end, 10);
    if (*end != '\0') error = ArgumentError(ArgumentError::INVALID, name);
    // cout << "int argument " << "name" << " = " << value << endl;
  }
  else
    error = ArgumentError(ArgumentError::MISSING, name);
}

void ArgParser::setValue(const string &name, float &value, const char * a, ArgumentError & error)
{
  if (a != NULL) {
    char * end;
    value = strtod(a, &end);
    if (*end != '\0') error = ArgumentError(ArgumentError::INVALID, name);
    // cout << "float argument " << name << " = " << value << endl;
  }
  else
    error = ArgumentError(ArgumentError::MISSING, name);
}

void ArgParser::setValue(const string &name, string &value, const char * a, ArgumentError & error)
{
  if (a != NULL) {
    value = a;
    // cout << "string argument " << name << " = " << value << endl;
  }
  else
    error = ArgumentError(ArgumentError::MISSING, name);
}

string ArgT<bool>::getParam(void) const
{
  return "";
}

string ArgT<int>::getParam(void) const
{
  return "=integer[" + Binc::toString(valueRef) + "]";
}

string ArgT<float>::getParam(void) const
{
  return "=number";
}

string ArgT<string>::getParam(void) const
{
  return "=string[" + valueRef + "]";
}
