/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    arg-error.h
 *  
 *  Description:
 *    Error class for Argument Parser
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

#ifndef _ARG_EXCEPTIONS_H
#define _ARG_EXCEPTIONS_H

#include <string>
#include <set>

namespace ArgParser {

  class ArgumentError {
  public:
    enum Type {
      NONE,
      CHECK,
      REQUIRED_COMMAND,
      REQUIRED_ARGUMENT,
      UNKNOWN,
      MISSING,
      INVALID 
    };
  protected:
    std::string argName;
    Type type;
    std::string error;
    std::set<std::string> missing;
  public:
    ArgumentError(Type t, const std::string &name = "", const std::string &e = "");

    const std::string & getName(void) const;
    const std::string & getErrorStr(void) const;
    const Type getErrorType(void) const;
    const std::set<std::string> & getMissing(void) const;

    void add(const std::string & s);

  };

}

#endif
