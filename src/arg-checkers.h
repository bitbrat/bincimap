/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    arg-checkers.h
 *  
 *  Description:
 *    Checker classes for Argument Parser
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

#ifndef _ARG_CHECKERS_H
#define _ARG_CHECKERS_H

#include <string>
#include "convert.h"
#include <functional>
#include <set>

namespace ArgParser {

  //
  // Argument checkers
  //

  // check interface
  
  template<class T> class checker : public std::unary_function<T, bool> {
    public:
    virtual bool operator()(const T &value, std::string & error) const = 0;
  };
 
  // Default checker, always returns true

  template<class T> class ok_function : public checker<T> {
    public:
    bool operator()(const T &value, std::string & error) const {
      //      cerr << "ok_function" << endl;
      error = "";
      return true;
    }
  };

  // less than

  template<class T> class less_than : public checker<T> {
    T value;
    public:
    less_than(const T &v) : value(v) {}
    bool operator()(const T &value, std::string & error) const {
      bool result = std::less<T>()(value, this->value);
      Binc::BincStream s;
      if (!result)
	s << "value " << value << "not less than " << this->value;
      error = s.str();
      return result;
    }
  };

  // range check
  
  template<class V> class range_check : public checker<V> {
    V lower;
    V upper;
    public:
    range_check(const V & l, const V & u) : lower(l), upper(u) { }
    bool operator()(const V & value, std::string & error) const {
      bool result = (lower <= value && value <= upper);
      Binc::BincStream s;
      if (!result)
	s << "value " << value 
	  << " not in range [" 
	  << lower << ", " << upper << "]";
      error = s.str();
      return result;
    }
  };
  
  template<class V> class enum_check : public checker<V> {
    std::set<V> values;
    public:
    void add(const V&v) {
      values.insert(v);
    }
    bool operator()(const V & value, std::string & error) const {
      bool result = values.find(value) != values.end();
      Binc::BincStream s;
      //      cerr << "enum check" << endl;
      if (!result)
	s << "value " << value << " not in enumeration";
      error = s.str();
      return result;
    }
    enum_check<V> & operator,(const V & value) {
      add(value);
      return *this;
    }
    enum_check<V> & operator+=(const V & value) {
      add(value);
      return *this;
    }
  };

}

#endif
