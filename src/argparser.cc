/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/argparser.cc
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
#include "argparser.h"
#include <string>
#include <map>

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
CommandLineArgs::CommandLineArgs()
{
  errString = "Unknown error";
  ac = 0;
}

//----------------------------------------------------------------------
bool CommandLineArgs::parse(int argc, char *argv[])
{
  ac = -1;
  head = argv[0];
  head += " <options> --\n";

  if (argc > 1) {
    string lastKey;
    bool lastIsBoolean = false;

    for (int i = 1; i < argc; ++i) {
      string s = argv[i];
      if (s.length() < 2) {
	errString = "syntax error: " + s;
	return false;
      }

      if (s[0] != '-') {
	// read value of last argument
	if (lastKey == "") {
	  errString = "syntax error: " + s;
	  return false;
	}

	if (lastIsBoolean && (s != "yes" && s != "no")) {
	  errString = "syntax error: " + s;
	  errString += " (expected yes or no)";
	  return false;
	}

	args[lastKey] = s;
	lastKey = "";
	lastIsBoolean = false;
      } else if (s[1] == '-') {
	if (lastKey != "") {
	  if (lastIsBoolean) {
	    args[lastKey] = "yes";
	    lastKey = "";
	    lastIsBoolean = false;
	  } else {
	    errString = "expected value of ";
	    errString += lastKey;
	    return false;
	  }
	}

	// break if '--' is detected
	if (s.length() == 2) {
	  ac = i + 1;
	  break;
	}

	// parse --argument
	string arg = s.substr(2);
	string val;
	string::size_type epos = arg.find('=');
	if (epos != string::npos) {
	  val = arg.substr(epos + 1);
	  arg = arg.substr(0, epos);
	}

	if (reg.find(arg) == reg.end()) {
	  errString = "unrecognized argument: --" + arg;
	  return false;
	}

	if (reg.find(arg)->second.b) {
	  if (val != "" && val != "yes" && val != "no") {
	    errString = "syntax error: " + val;
	    errString += " (expected yes or no)";
	    return false;
	  } else if (val == "") {
	    val = "yes";
	  }
	}

	if (val == "") {
	  errString = "syntax error: " + arg;
	  errString += " (expected --" + arg + "=<str>)";
	  return false;
	}

	args[arg] = val;
	lastKey = "";
	lastIsBoolean = false;
      } else {
	if (lastKey != "") {
	  if (lastIsBoolean) {
	    args[lastKey] = "yes";
	    lastKey = "";
	    lastIsBoolean = false;
	  } else {
	    errString = "expected value of ";
	    errString += lastKey;
	    return false;
	  }
	}

	// parse -argument
	string arg = s.substr(1);
	if (arg.length() == 1) {
	  map<string, ArgOpts>::const_iterator it = reg.begin();
	  bool match = false;
	  for (; it != reg.end(); ++it) {
	    if (it->second.c.find(arg[0]) != string::npos) {
	      lastKey = it->first;
	      if (it->second.b)
		lastIsBoolean = true;
	      match = true;
	      break;
	    }
	  }

	  if (!match) {
	    errString = "unrecognized argument: -";
	    errString += arg[0];
	    return false;
	  }

	} else {
	  string::const_iterator its = arg.begin();
	  for (; its != arg.end(); ++its) {
	    map<string, ArgOpts>::const_iterator it = reg.begin();
	    bool match = false;
	    for (; it != reg.end(); ++it) {
	      if (it->second.c.find(*its) != string::npos) {
		if (!it->second.b) {
		  errString = "argument is not a boolean: ";
		  errString += "--" + it->first;
		  errString += " / -";
		  errString += it->second.c;
		  return false;
		}

		match = true;
		args[it->first] = "yes";
		lastKey = "";
		lastIsBoolean = false;
		break;
	      }
	    }

	    if (!match) {
	      errString = "unrecognized argument: ";
	      errString += s;
	      return false;
	    }
	  }
	}
      }
    }

    if (lastKey != "") {
      if (lastIsBoolean) {
	args[lastKey] = "yes";
      } else {
	errString = "expected value of ";
	errString += lastKey;
	return false;
      }
    }
  }

  map<string, ArgOpts>::const_iterator it = reg.begin();
  for (; it != reg.end(); ++it) {
    if (args.find(it->first) == args.end()) {
      if (!it->second.o) {
	errString = "missing argument: ";
	errString += it->first;
	return false;
      }
      
      if (it->second.b)
	args[it->first] = "no";
    }
  }

  if (ac == -1)
    ac = argc;

  return true;
}

//----------------------------------------------------------------------
string CommandLineArgs::errorString(void) const
{
  return errString;
}

//----------------------------------------------------------------------
const string CommandLineArgs::operator [](const string &arg) const
{
  if (args.find(arg) == args.end())
    return "";

  return args.find(arg)->second;
}

//----------------------------------------------------------------------
void CommandLineArgs::addOptional(const string &arg, const string &desc,
				  bool boolean)
{
  registerArg(arg, desc, boolean, true);
}

//----------------------------------------------------------------------
void CommandLineArgs::addRequired(const string &arg,
				  const string &desc, bool boolean)
{
  registerArg(arg, desc, boolean, false);
}

//----------------------------------------------------------------------
void CommandLineArgs::registerArg(const string &arg, const string &desc,
				  bool boolean, bool optional)
{
  string name = arg;

  string shorts;
  while (name.size() > 1 && name[1] == '|') {
    shorts += name[0];
    name = name.substr(2);
  }

  reg.insert(make_pair(name, ArgOpts(shorts, boolean, optional, desc)));
}

//----------------------------------------------------------------------
string CommandLineArgs::usageString(void) const
{
  string tmp = head;
  tmp += '\n';

  map<string, ArgOpts>::const_iterator it = reg.begin();
  for (; it != reg.end(); ++it) {
    if (it->second.c != "") {
      string::const_iterator sit = it->second.c.begin();
      for (; sit != it->second.c.end(); ++sit) {
	if (sit != it->second.c.begin())
	  tmp += '\n';
	tmp += "  -";
	tmp += *sit;
      }
      tmp += ", ";
    } else {
      tmp += "      ";
    }

    tmp += "--";
    tmp += it->first;
    if (!it->second.b) {
      tmp += "=<str>";
    }

    if (!it->second.o)
      tmp += " (required)";

    string::size_type lineStart = tmp.rfind('\n');
    if (lineStart == string::npos)
      lineStart = 0;

    int pad = 21 - (tmp.length() - lineStart);
    if (pad < 0) {
      tmp += '\n';
      pad = 20;
    }

    tmp += string(pad, ' ');
    tmp += it->second.desc;
    tmp += '\n';
  }
  
  tmp += '\n';
  tmp += tail;
  tmp += '\n';

  return tmp;
}

//----------------------------------------------------------------------
int CommandLineArgs::argc(void) const
{
  return ac;
}

//----------------------------------------------------------------------
void CommandLineArgs::setTail(const string &str)
{
  tail = str;
}
