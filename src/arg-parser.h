/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    arg-arg.h
 *  
 *  Description:
 *    Class for command line parsing
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

#ifndef _ARG_PARSER_H
#define _ARG_PARSER_H

#include "arg-arg.h"
#include <set>
#include <map>

namespace ArgParser {

  class Args {
    typedef std::map<std::string, Arg*> StringArgPtrMap;
    StringArgPtrMap args;
    std::map<int, std::string> argNum;
    int argc;
    char ** argv;
    std::set<int> requiredArgs;
    std::set<int> mode;
    std::map<std::string, std::string> helpStrings;
  protected:
    template<class T> int addArgs(const std::string &name, 
				  T &var, 
				  int type,
				  checker<T> & sc) {
      const int num = args.size();
      std::vector<std::string> v;
      tokenize(name, '|', v);
      std::vector<std::string>::const_iterator i = v.begin();
      while (i != v.end()) {
	if (type == Arg::REQUIRED)
	  requiredArgs.insert(num);
	args[*i] = new ArgT<T>(num, *i, var, type, sc);
	argNum[num] = *i;
	++i;
      }
      return num;
    }

    template<class T> class mapArg {
      T & target;
      T value;
    public:
      mapArg(T & t, const T &v) : target(t), value(v) { }
      void set(void) {
	target = value;
      }
    };

  public:
    Args(void) : argc(0), argv(NULL) { }
    Args(int c, char * v[]) : argc(c), argv(v) { }
    ~Args(void);
    
    void addHelpString(const std::string & key, const std::string & help);
    
    void displayHelp(std::ostream & o) const;

    void tokenize(const std::string &s, char sep, 
		  std::vector<std::string> &result) const;
    template<class T> void addMode(const std::string &name,
				   T &var, 
				   const T &value) {
      static ok_function<mapArg<T> > f;
      int i = addArgs(name, *(new mapArg<T>(var, value)), Arg::OPTIONAL, f);
      mode.insert(args[argNum[i]]->num);
    }
    
    template<class T> void addMode(const std::string &name,
				   T &var,
				   const T &value,
				   const checker<T> & sc) {
      int i = addArgs(name, mapArg<T>(var, value), Arg::OPTIONAL, sc);
      mode.insert(args[argNum[i]]->num);
    }

    template<class T> void addOptional(const std::string &name, T &var,
				       checker<T> & sc) {
      addArgs(name, var, Arg::OPTIONAL, sc);
    }
    
    template<class T> void addOptional(const std::string &name, T &var) {
      static ok_function<T> f;
      addArgs(name, var, Arg::OPTIONAL, f);
    }
    
    template<class T> void addRequired(const std::string &name, T &var,
				       const checker<T> & sc) {
      addArgs(name, var, Arg::REQUIRED, sc);
    }

    template<class T> void addRequired(const std::string &name, T &var) {
      static ok_function<T> f;
      addArgs(name, var, Arg::REQUIRED, f);
    }
    
    template<class T> bool get(const std::string &name, T &var) {
      StringArgPtrMap::iterator i = args.find(name);
      if (i == args.end()) return false;
      i->second->get(name, var);
      return true;
    }

    int parse(std::vector<std::string> & rest, ArgumentError & error) const;

    void assign(void);
    void assign(const std::string s);

  template<class T> void setValue(const std::string &name,
				  Args::mapArg<T> &value, const char * a)
    {
      value.set();
    }
  };
}

#endif
