/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    arg-parser.cc
 *  
 *  Description:
 *    Implementation of argument parser
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

#include "arg-parser.h"
#include <algorithm>

using namespace ArgParser;
using namespace std;

Args::~Args(void) {
  StringArgPtrMap::const_iterator i = args.begin();
  while (i != args.end()) {
    delete i->second;
    ++i;
  }
}
    
void Args::tokenize(const string &s, char sep, 
		    vector<string> &result) const
{
  string::size_type p;
  string::size_type o = 0;
  while ((p = s.find(sep, o)) != string::npos) {
    result.push_back(s.substr(0, p));
    o = p + 1;
  }
  result.push_back(s.substr(o));
}

int Args::parse(vector<string> & rest, ArgumentError & error) const {
  set<int> parsed;
  int i;
  for (i = 1; i < argc; ++i) {
    const string & arg = argv[i];
    // If we encounter '--' then stop parsing
    if (arg == "--") {
      ++i;
      break;
    }
    
    // Find next argument, and put in next if it doesn't start with
    // '-'
    string::size_type c = arg.find("=");
    char const *  next;
    string a;
    if (c != string::npos) {
      a = arg.substr(0, c);
      //cerr << a << endl;
      next = arg.c_str() + c + 1;
    }
    else {
      a = arg;
      next = i < (argc - 1) ? argv[i + 1] : NULL;
    }
    
    vector<string> cmd;
    
    // Strip "--" and "-" from argument
    if (a.substr(0, 2) == "--") {
      a = a.substr(2);
      cmd.push_back(a);
    }
    else if (a.substr(0, 1) == "-") {
      a = a.substr(1);
      for (string::const_iterator j = a.begin(); j != a.end(); ++j)
	cmd.push_back(string("") + *j);
    }
    else {
      // Not starting with "--" or "-"; push them on the rest vector
      rest.push_back(argv[i]);
      continue;
    }
    
    if (next != NULL && *next == '-') next = NULL;
    
    vector<string>::const_iterator k = cmd.begin();
    int used = 0;
    while (k != cmd.end()) {
      // Find handler
      map<string, Arg*>::const_iterator j = args.find(*k);
      if (j != args.end()) {
	if (j->second->parse(next, error)) used = (next == NULL);
	parsed.insert(j->second->num);
      }
      else {
	// Handler not found; if the argument starts with a "-", then
	// flag an error, or else push it on the rest vector.
	if (arg.size() > 0 && arg[0] == '-') {
	  error = ArgumentError(ArgumentError::UNKNOWN, *k);
	  return -1;
	}
	rest.push_back(argv[i]);
      }
      ++k;
    }
    i += used;
  }
  if (!includes(parsed.begin(), parsed.end(),
		requiredArgs.begin(), requiredArgs.end())) {
    ArgumentError e(ArgumentError::REQUIRED_ARGUMENT);
    set<int> missing;
    insert_iterator<set<int> > missing_ins(missing, missing.begin());
    set_difference(requiredArgs.begin(), requiredArgs.end(),
		   parsed.begin(), parsed.end(), missing_ins);
    set<int>::const_iterator i = missing.begin();
    while (i != missing.end()) {
      e.add(argNum.find(*i)->second);
      ++i;
    }
    error = e;
    return -1;
  }
  if (mode.size() > 0) {
    set<int> other;
    insert_iterator<set<int> > other_ins(other, other.begin());
    set_intersection(mode.begin(), mode.end(),
		     parsed.begin(), parsed.end(), other_ins);
    if (other.size() != 1) {
      error = ArgumentError(ArgumentError::REQUIRED_COMMAND);
      return -1;
    }
  }
  return i;
}

void Args::assign(void)
{
  StringArgPtrMap::iterator i = args.begin();
  while (i != args.end()) {
    if (i->second->isUsed())
      i->second->assign();
    ++i;
  }
}

void Args::assign(const std::string s)
{
  StringArgPtrMap::iterator i = args.find(s);
  if (i != args.end() && i->second->isUsed())
    i->second->assign();
}

void Args::addHelpString(const string & key, const string & help)
{
  helpStrings[key] = help;
}
    
void Args::displayHelp(ostream & o) const
{
  for (map<string, string>::const_iterator i = helpStrings.begin(); i != helpStrings.end(); ++i) {
    vector<string> v;
    tokenize(i->first, '|', v);
    vector<string>::const_iterator j = v.begin(); 
    while (j != v.end()) {
      StringArgPtrMap::const_iterator pi = args.find(*j);
      if (pi != args.end()) {
	if ((*j).size() == 1)
	o << "-" << *j << pi->second->getParam();
	else
	  o << "--" << *j << pi->second->getParam();
	++j;
	if (j != v.end())
	  o << ", ";
      }
    }
    o << endl;

    v.clear();
    tokenize(i->second, '\n', v);
    j = v.begin(); 
    while (j != v.end()) {
      o << "\t" << *j << endl;
      ++j;
    }
    o << endl;
  }
}
