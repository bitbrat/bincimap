/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    arg-arg.h
 *  
 *  Description:
 *    Class encapsulating one argument
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

#ifndef _ARG_ARG_H
#define _ARG_ARG_H

#include "arg-error.h"
#include "arg-checkers.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace ArgParser {

  // Base abstract class for one argument

  class Arg {
  protected:
    int num;
    std::string name;
    bool used;
  public:
    const static int OPTIONAL = 0;
    const static int REQUIRED = 1;
    Arg(int nu, const std::string &n) : num(nu), name(n), used(false) { }
    virtual ~Arg(void) { }
    virtual bool parse(const char * a, ArgumentError & error) = 0;
    virtual std::string getParam(void) const = 0;
    virtual void assign(void) = 0;
    bool isUsed(void) { return used; }
    template<class T> void get(T &v) = 0;
    friend class Args;
  };

  // template function to set argument value

 
  template<class T> void setValue(const std::string &name, T&value, 
				  const char * param,
				  ArgumentError & Error) {
    //    std::cerr << "Unimplemented setValue invoked" << endl;
    error = ArgumentError(ArgumentError::INVALID);
  }

  template<class T> class ArgT : public Arg {
    T value;
    T & valueRef;
    int type;
    checker<T> & sc;
  public:
    ArgT(int n, const std::string &name, T & v, int t, checker<T> & _sc) : 
      Arg(n, name),
      valueRef(v),
      type(t),
      sc(_sc) {
      value = v;
    }

    virtual void assign(void) {
      valueRef = value;
    }

    virtual void get(T &v) {
      v = value;
    }


    std::string getParam(void) const;
    
    virtual bool parse(const char * a, ArgumentError & error) {
      used = true;
      setValue(name, value, a, error);
      std::string err;
      if (!sc(value, err)) {
	error = ArgumentError(ArgumentError::CHECK, name, err);
	return a == NULL ? false : true;
      }
      return true;
    }
  };

  // Standard types: int, float, and std::string, vectors and maps

  void setValue(const std::string &name, int &value, const char * a, ArgumentError & error);
  void setValue(const std::string &name, float &value, const char * a, ArgumentError & error);
  void setValue(const std::string &name, std::string &value, const char * a, ArgumentError & error);

  template<class T> void setValue(const std::string &name,
				  std::vector<T> &value, 
				  const char * a,
				  ArgumentError & error)
    {
      //      cerr << "setValue vector<T>" << endl;
      if (a != NULL) {
	T arg;
	setValue(name, arg, a, error);
	//	cout << "vector<T> argument " << name << " = " << arg << endl;
	value.push_back(arg);
      }
      else
	error = ArgumentError(ArgumentError::MISSING, name);
    }
  
  template<class K, class V> void setValue(const std::string &name,
					   std::map<K, V> &value, 
					   const char * a,
					   ArgumentError & error)
    {
      //      cerr << "setValue map<K, V>" << endl;
      if (a != NULL) {
	K key;
	V val;
	std::string k(a);
	char * eq = strchr(a, '=');
	while (eq != NULL && eq != a && (*(eq-1) == '\\'))
	  eq = strchr(eq + 1, '=');
	if (eq == NULL) {
	  error = ArgumentError(ArgumentError::MISSING, name);
	  return;
	}
	k.resize(eq - a);
	std::string::size_type p = k.find("\\=");
	while (p != std::string::npos) {
	  k = k.substr(0, p) + k.substr(p + 1);
	  p = k.find("\\=");
	}
	std::string v(eq + 1);
	setValue(name, key, k.c_str(), error);
	setValue(name, val, v.c_str(), error);
	cout << "vector<T> argument " << name << " = ("
	     << k << " => " << val << ")" << endl;
	value[key] = val;
      }
      else
	error = ArgumentError(ArgumentError::MISSING, name);
    }

  class StringBoolArg {
    std::string & target;
    std::string yes;
    std::string no;
  public:
    StringBoolArg(std::string &t, 
		  const std::string &y = "yes", 
		  const std::string &n ="no")
      : target(t), yes(y), no(n)
      { 
	target = no;
      }
    void set(void) {
      target = yes;
    }
  };

  template<>
  inline bool ArgParser::ArgT<bool>::parse(const char * a, ArgumentError & error)
  {
    used = true;
    value = true;
    return false;
  }
  
  template<>
  inline bool ArgParser::ArgT<StringBoolArg>::parse(const char * a, ArgumentError & error)
  {
    used = true;
    value.set();
    return false;
  }

}

#endif
