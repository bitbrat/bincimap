/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/argparser.h
 *  
 *  Description:
 *    <--->
 *
 *  Authors:
 *    Andreas Aardal Hanssen <bincimap@andreas.hanssen.name>
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
#ifndef ARGPARSER_H_INCLUDED
#define ARGPARSER_H_INCLUDED
#include <string>
#include <map>

namespace Binc {
  class ArgOpts {
  public:
    std::string c;
    bool b;
    bool o;
    std::string desc;

    inline ArgOpts(const std::string &chr, bool boolean, bool optional,
		   const std::string &descr)
    {
      c = chr;
      b = boolean;
      o = optional;
      desc = descr;
    }
  };

  class CommandLineArgs {
  public:
    CommandLineArgs(void);

    bool parse(int argc, char *argv[]);
    std::string errorString(void) const;

    int argc(void) const;
  
    const std::string operator [](const std::string &arg) const;
  
    void addOptional(const std::string &arg, const std::string &desc,
		     bool boolean);
    void addRequired(const std::string &arg, const std::string &desc,
		     bool boolean);
    bool hasArg(const std::string &arg) const;

    std::string usageString(void) const;

    void setTail(const std::string &str);

  private:
    void registerArg(const std::string &arg, const std::string &desc,
		     bool boolean, bool optional);

    std::string errString;
    std::map<std::string, ArgOpts> reg;
    std::map<std::string, std::string> args;
    std::map<std::string, bool> passedArgs;
    std::string tail;
    std::string head;
    int ac;
  };
}

#endif
