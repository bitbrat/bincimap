/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    recursivedescent.cc
 *  
 *  Description:
 *    Implementation of a recursive descent IMAP command
 *    parser.
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

// #define DEBUG

#include "imapparser.h"
#include "recursivedescent.h"
#include "io.h"
#include "convert.h"
#include "session.h"

#include <stdio.h>
#include <ctype.h>
#include <stack>
#include <iostream>
#include <iomanip>

using namespace ::std;
using namespace Binc;

stack<int> Binc::inputBuffer;
int Binc::charnr = 0;


//----------------------------------------------------------------------
Operator::ParseResult Binc::expectThisString(const string &s_in)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

#ifdef DEBUG
  cout << "expectThisString(\"" << s_in << "\")" << endl << flush;
#endif
  string tmp;

  bool match = true;
  for (string::const_iterator i = s_in.begin(); i != s_in.end(); ++i) {

    int c = com.readChar(session.timeout());
    if (c == -1) {
      session.setLastError(com.getLastError());
      return Operator::ERROR;
    } else if (c == -2)
      return Operator::TIMEOUT;


    tmp += c;
    
    if (toupper(*i) != toupper(c)) {
      match = false;
      break;
    }
  }
  
  if (!match) {
    com.unReadChar(tmp);
    return Operator::REJECT;
  } else
    return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectDateTime(string &s_in)
{
  Session &session = Session::getInstance();

  if (expectThisString("\"") != Operator::ACCEPT)
    return Operator::REJECT;

  unsigned int digit1, digit2;
  if (expectSPACE() == Operator::ACCEPT) {
    digit1 = 0;
    Operator::ParseResult res;
    if ((res = expectDigit(digit2)) != Operator::ACCEPT) {
      session.setLastError("expected digit (day) after \" and a SPACE.");
      return res;
    }
  } else {
    Operator::ParseResult res;
    if ((res = expectDigit(digit1)) != Operator::ACCEPT) {
      session.setLastError("expected first digit of day");
      return res;
    }
    if ((res = expectDigit(digit2)) != Operator::ACCEPT) {
      session.setLastError("expected second digit of day");
      return res;
    }
  }

  int day = digit1 * 10 + digit2;

  BincStream daystr;

  if (day < 10)
    daystr << '0';
  daystr << day;

  s_in += daystr.str();

  Operator::ParseResult res;
  if ((res = expectThisString("-")) != Operator::ACCEPT) {
    session.setLastError("expected -");
    return res;
  }

  s_in += "-";

  /* month */
  if ((res = expectThisString("Jan")) == Operator::ACCEPT) s_in += "Jan";
  else if ((res = expectThisString("Feb")) == Operator::ACCEPT) s_in += "Feb";
  else if ((res = expectThisString("Mar")) == Operator::ACCEPT) s_in += "Mar";
  else if ((res = expectThisString("Apr")) == Operator::ACCEPT) s_in += "Apr";
  else if ((res = expectThisString("May")) == Operator::ACCEPT) s_in += "May";
  else if ((res = expectThisString("Jun")) == Operator::ACCEPT) s_in += "Jun";
  else if ((res = expectThisString("Jul")) == Operator::ACCEPT) s_in += "Jul";
  else if ((res = expectThisString("Aug")) == Operator::ACCEPT) s_in += "Aug";
  else if ((res = expectThisString("Sep")) == Operator::ACCEPT) s_in += "Sep";
  else if ((res = expectThisString("Oct")) == Operator::ACCEPT) s_in += "Oct";
  else if ((res = expectThisString("Nov")) == Operator::ACCEPT) s_in += "Nov";
  else if ((res = expectThisString("Dec")) == Operator::ACCEPT) s_in += "Dec";
  else {
    session.setLastError("expected month");
    return res;
  }

  if ((res = expectThisString("-")) != Operator::ACCEPT) {
    session.setLastError("expected -");
    return res;
  }

  s_in += "-";    

  /* year */
  unsigned int year, c;
  if ((res = expectDigit(year)) != Operator::ACCEPT) {
    session.setLastError("expected digit (first digit of year)");
    return res;
  }

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit (second digit of year)");
    return res;
  }

  year = (year * 10) + c;

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit (third digit of year)");
    return res;
  }

  year = (year * 10) + c;

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit (last digit of year)");
    return res;
  }

  year = (year * 10) + c;

  BincStream yearstr;
    
  yearstr << year;

  s_in += yearstr.str();

  if ((res = expectSPACE()) != Operator::ACCEPT) {
    session.setLastError("expected SPACE");
    return res;
  }

  s_in += " ";

  if ((res = expectTime(s_in)) != Operator::ACCEPT) {
    session.setLastError("expected time");
    return res;
  }

  if ((res = expectSPACE()) != Operator::ACCEPT) {
    session.setLastError("expected SPACE");
    return res;
  }

  s_in += " ";

  if ((res = expectZone(s_in)) != Operator::ACCEPT) {
    session.setLastError("expected zone");
    return res;
  }

  if ((res = expectThisString("\"")) != Operator::ACCEPT) {
    session.setLastError("expected \"");
    return res;
  }

  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectTime(string &s_in)
{
  Session &session = Session::getInstance();

  unsigned int c, t;
  Operator::ParseResult res;
  if ((res = expectDigit(t)) != Operator::ACCEPT)
    return res;

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  t = (t * 10) + c;

  BincStream tstr;

  tstr << t;

  s_in += tstr.str();

  if ((res = expectThisString(":")) != Operator::ACCEPT) {
    session.setLastError("expected colon");
    return res;
  }

  s_in += ":";

  if ((res = expectDigit(t)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  t = (t * 10) + c;

  tstr.clear();

  tstr << t;

  s_in += tstr.str();

  if ((res = expectThisString(":")) != Operator::ACCEPT) {
    session.setLastError("expected colon");
    return res;
  }

  s_in += ":";

  if ((res = expectDigit(t)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  t = (t * 10) + c;

  tstr.clear();

  tstr << t;

  s_in += tstr.str();

  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectZone(string &s_in)
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  if ((res = expectThisString("-")) == Operator::ACCEPT)
    s_in += "-";
  else if ((res = expectThisString("+")) == Operator::ACCEPT)
    s_in += "+";
  else
    return res;

  unsigned int c, t;
  if ((res = expectDigit(t)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  t = (t * 10) + c;

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  t = (t * 10) + c;

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  t = (t * 10) + c;

  BincStream tstr;

  tstr << t;

  s_in += tstr.str();
    
  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectListWildcards(int &c_in)
{
  Operator::ParseResult res;
  if ((res = expectThisString("%")) == Operator::ACCEPT) {
    c_in = '%';
    return Operator::ACCEPT;
  } else if ((res = expectThisString("*")) == Operator::ACCEPT) {
    c_in = '*';
    return Operator::ACCEPT;
  } else
    return res;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectListMailbox(string &s_in)
{
  IO &com = IOFactory::getInstance().get(1);

  Operator::ParseResult res;
  if ((res = expectString(s_in)) == Operator::ACCEPT)
    return Operator::ACCEPT;
    
  int c;
  if ((res = expectAtomChar(c)) == Operator::ACCEPT
      || (res = expectListWildcards(c)) == Operator::ACCEPT
      || (res = expectThisString("]")) == Operator::ACCEPT) {
    do {
      s_in += (char) c;
      if ((res = expectAtomChar(c)) != Operator::ACCEPT
	  && (res = expectListWildcards(c)) != Operator::ACCEPT
	  && (res = expectThisString("]")) != Operator::ACCEPT)
	return Operator::ACCEPT;
    } while (1);
  }

  com.unReadChar(s_in);
  
  return res;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectFlag(vector<string> &v_in)
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  string flag;
  if ((res = expectThisString("\\Answered")) == Operator::ACCEPT)
    v_in.push_back("\\Answered");
  else if ((res = expectThisString("\\Flagged")) == Operator::ACCEPT)
    v_in.push_back("\\Flagged");
  else if ((res = expectThisString("\\Deleted")) == Operator::ACCEPT) 
    v_in.push_back("\\Deleted");
  else if ((res = expectThisString("\\Seen")) == Operator::ACCEPT) 
    v_in.push_back("\\Seen");
  else if ((res = expectThisString("\\Draft")) == Operator::ACCEPT)
    v_in.push_back("\\Draft");
  else if ((res = expectThisString("\\Answered")) == Operator::ACCEPT)
    v_in.push_back("\\Answered");
  else {
    if ((res = expectThisString("\\")) == Operator::ACCEPT) {
      if ((res = expectAtom(flag)) == Operator::ACCEPT)
	v_in.push_back("\\" + flag);
      else {
	session.setLastError("expected atom");
	return res;
      }

    } else if (expectAtom(flag) == Operator::ACCEPT) {
      v_in.push_back(flag);
    } else
      return res;
  }

  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectDate(string &s_in)
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  bool quoted = false;
  if ((res = expectThisString("\"")) == Operator::ACCEPT)
    quoted = true;

  /* day */
  unsigned int day, c;
  if ((res = expectDigit(c)) == Operator::ACCEPT) {
    day = c;
    if ((res = expectDigit(c)) == Operator::ACCEPT)
      day = (day * 10) + c;

    BincStream daystr;

    daystr << day;

    s_in += daystr.str();
  } else {
    session.setLastError("expected digit");
    return res;
  }

  /* - */
  if ((res = expectThisString("-")) != Operator::ACCEPT) {
    session.setLastError("expected -");
    return res;
  }

  s_in += '-';

  /* month */
  if ((res = expectThisString("Jan")) == Operator::ACCEPT) s_in += "Jan";
  else if ((res = expectThisString("Feb")) == Operator::ACCEPT) s_in += "Feb";
  else if ((res = expectThisString("Mar")) == Operator::ACCEPT) s_in += "Mar";
  else if ((res = expectThisString("Apr")) == Operator::ACCEPT) s_in += "Apr";
  else if ((res = expectThisString("May")) == Operator::ACCEPT) s_in += "May";
  else if ((res = expectThisString("Jun")) == Operator::ACCEPT) s_in += "Jun";
  else if ((res = expectThisString("Jul")) == Operator::ACCEPT) s_in += "Jul";
  else if ((res = expectThisString("Aug")) == Operator::ACCEPT) s_in += "Aug";
  else if ((res = expectThisString("Sep")) == Operator::ACCEPT) s_in += "Sep";
  else if ((res = expectThisString("Oct")) == Operator::ACCEPT) s_in += "Oct";
  else if ((res = expectThisString("Nov")) == Operator::ACCEPT) s_in += "Nov";
  else if ((res = expectThisString("Dec")) == Operator::ACCEPT) s_in += "Dec";
  else {
    session.setLastError("expected month");
    return res;
  }
      
  /* - */
  if ((res = expectThisString("-")) != Operator::ACCEPT) {
    session.setLastError("expected -");
    return res;
  }

  s_in += '-';

  /* year */
  unsigned int year;
  if ((res = expectDigit(year)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  year = (year * 10) + c;

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  year = (year * 10) + c;

  if ((res = expectDigit(c)) != Operator::ACCEPT) {
    session.setLastError("expected digit");
    return res;
  }

  year = (year * 10) + c;

  BincStream yearstr;

  yearstr << year;

  s_in += yearstr.str();

  if (quoted)
    if ((res = expectThisString("\"")) != Operator::ACCEPT) {
      session.setLastError("expected \"");
      return res;
    }
    
  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectCRLF(void)
{
  Operator::ParseResult res;
  if ((res = expectCR()) == Operator::ACCEPT
      && (res = expectLF()) == Operator::ACCEPT)
    return Operator::ACCEPT;
  else
    return res;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectCR(void)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  int c = com.readChar(session.timeout());
  if (c == -1) {
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  } else if (c == -2)
    return Operator::TIMEOUT;

  if (c == 0x0d)
    return Operator::ACCEPT;
  else {
    com.unReadChar(c);
    return Operator::REJECT;
  }
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectLF(void)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  int c = com.readChar(session.timeout());
  if (c == -1) {
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  } else if (c == -2)
      return Operator::TIMEOUT;

  if (c == 0x0a)
    return Operator::ACCEPT;
  else {
    com.unReadChar(c);
    return Operator::REJECT;
  }
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectTagChar(int &c_in)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();
 
  int c = com.readChar(session.timeout());
  if (c == -1) {
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  } else if (c == -2)
    return Operator::TIMEOUT;

  switch (c) {
  case 041:    case 043:    case 044:    case 046:    case 047:    case 054:
  case 055:    case 056:    case 057:    case 060:    case 061:    case 062:
  case 063:    case 064:    case 065:    case 066:    case 067:    case 070:
  case 071:    case 072:    case 073:    case 074:    case 075:    case 076:
  case 077:    case 0100:   case 0101:   case 0102:   case 0103:   case 0104:
  case 0105:   case 0106:   case 0107:   case 0110:   case 0111:   case 0112:
  case 0113:   case 0114:   case 0115:   case 0116:   case 0117:   case 0120:
  case 0121:   case 0122:   case 0123:   case 0124:   case 0125:   case 0126:
  case 0127:   case 0130:   case 0131:   case 0132:   case 0133:   case 0135:
  case 0136:   case 0137:   case 0140:   case 0141:   case 0142:   case 0143:
  case 0144:   case 0145:   case 0146:   case 0147:   case 0150:   case 0151:
  case 0152:   case 0153:   case 0154:   case 0155:   case 0156:   case 0157:
  case 0160:   case 0161:   case 0162:   case 0163:   case 0164:   case 0165:
  case 0166:   case 0167:   case 0170:   case 0171:   case 0172:   case 0174:
  case 0175:   case 0176:
    c_in = c;
    return Operator::ACCEPT;
  default:
    break;
  }
  
  com.unReadChar(c);

  return Operator::REJECT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectTag(string &s_in)
{
  string tag;
  int tagchar;

  int eres = expectTagChar(tagchar);
  if (eres == Operator::REJECT)
    return Operator::REJECT;
  else if (eres == Operator::ERROR)
    return Operator::ERROR;
  else if (eres == Operator::TIMEOUT)
    return Operator::TIMEOUT;
  else {
    tag += tagchar;

    bool done = false;

    while (!done) {
      switch (expectTagChar(tagchar)) {
      case Operator::ACCEPT:
	tag += tagchar;
	break;
      case Operator::REJECT:
	done = true;
	break;
      case Operator::ERROR:
	return Operator::ERROR;
      case Operator::TIMEOUT:
	return Operator::TIMEOUT;
      }
    }
  }

  s_in = tag;

  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectSPACE(void)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  int c = com.readChar(session.timeout());
  if (c == -1) {
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  } else if (c == -2)
    return Operator::TIMEOUT;

  if (c == ' ')
    return Operator::ACCEPT;
  else {
    com.unReadChar(c);
    return Operator::REJECT;
  }
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectMailbox(string &s_in)
{
  return expectAstring(s_in);
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectAstring(string &s_in)
{
  Operator::ParseResult res;
  if ((res = expectAtom(s_in)) == Operator::ACCEPT)
    return Operator::ACCEPT;
  
  if ((res = expectString(s_in)) == Operator::ACCEPT)
    return Operator::ACCEPT;
  
  return res;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectAtomChar(int &c_in)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  int c = com.readChar(session.timeout());
  if (c == -1) {
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  } else if (c == -2)
    return Operator::TIMEOUT;

  switch (c) {
  case 041:    case 043:    case 044:    case 046:    case 047:    case 053:
  case 054:    case 055:    case 056:    case 057:    case 060:    case 061:
  case 062:    case 063:    case 064:    case 065:    case 066:    case 067:
  case 070:    case 071:    case 072:    case 073:    case 074:    case 075:
  case 076:    case 077:    case 0100:   case 0101:   case 0102:   case 0103:
  case 0104:   case 0105:   case 0106:   case 0107:   case 0110:   case 0111:
  case 0112:   case 0113:   case 0114:   case 0115:   case 0116:   case 0117:
  case 0120:   case 0121:   case 0122:   case 0123:   case 0124:   case 0125:
  case 0126:   case 0127:   case 0130:   case 0131:   case 0132:   case 0133:
  case 0135:   case 0136:   case 0137:   case 0140:   case 0141:   case 0142:
  case 0143:   case 0144:   case 0145:   case 0146:   case 0147:   case 0150:
  case 0151:   case 0152:   case 0153:   case 0154:   case 0155:   case 0156:
  case 0157:   case 0160:   case 0161:   case 0162:   case 0163:   case 0164:
  case 0165:   case 0166:   case 0167:   case 0170:   case 0171:   case 0172:
  case 0174:   case 0175:   case 0176:
    c_in = c;
    return Operator::ACCEPT;
  default:
    break;
  }
  
  com.unReadChar(c);
  return Operator::REJECT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectAtom(string &s_in)
{
  IO &com = IOFactory::getInstance().get(1);

  string atom;
  int atomchar;

  Operator::ParseResult res;
  while ((res = expectAtomChar(atomchar)) == Operator::ACCEPT)
    atom += atomchar;

  if (atom == "") {
    com.unReadChar(atom);
    return res;
  } else
    s_in = atom;

  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectString(string &s_in)
{
  Operator::ParseResult res;
  if ((res = expectQuoted(s_in)) == Operator::ACCEPT)
    return Operator::ACCEPT;

  if ((res = expectLiteral(s_in)) == Operator::ACCEPT)
    return Operator::ACCEPT;

  return res;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectQuoted(string &s_in)
{
  IO &com = IOFactory::getInstance().get(1);

  string quoted;
  int quotedchar;
  Operator::ParseResult res;

  if ((res = expectThisString("\"")) != Operator::ACCEPT)
    return res;
    
  while ((res = expectQuotedChar(quotedchar)) == Operator::ACCEPT)
    quoted += quotedchar;
    
  if ((res = expectThisString("\"")) != Operator::ACCEPT) {
    com.unReadChar("\"" + quoted);
    return res;
  }

  s_in = quoted;
  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectQuotedChar(int &c_in)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  int c = com.readChar(session.timeout());
  if (c == -1) {
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  } else if (c == -2)
    return Operator::TIMEOUT;

  switch (c) {
  case 01:    case 02:    case 03:    case 04:    case 05:    case 06:    case 07:
  case 010:   case 011:   case 013:   case 014:   case 016:   case 017:
  case 020:   case 021:   case 022:   case 023:   case 024:   case 025:   case 026:   case 027:
  case 030:   case 031:   case 032:   case 033:   case 034:   case 035:   case 036:   case 037:
  case 040:   case 041:   case 043:   case 044:   case 045:   case 046:   case 047:
  case 050:   case 051:   case 052:   case 053:   case 054:   case 055:   case 056:   case 057:
  case 060:   case 061:   case 062:   case 063:   case 064:   case 065:   case 066:   case 067:
  case 070:   case 071:   case 072:   case 073:   case 074:   case 075:   case 076:   case 077:
  case 0100:  case 0101:  case 0102:  case 0103:  case 0104:  case 0105:  case 0106:  case 0107:
  case 0110:  case 0111:  case 0112:  case 0113:  case 0114:  case 0115:  case 0116:  case 0117:
  case 0120:  case 0121:  case 0122:  case 0123:  case 0124:  case 0125:  case 0126:  case 0127:
  case 0130:  case 0131:  case 0132:  case 0133:  case 0135:  case 0136:  case 0137:
  case 0140:  case 0141:  case 0142:  case 0143:  case 0144:  case 0145:  case 0146:  case 0147:
  case 0150:  case 0151:  case 0152:  case 0153:  case 0154:  case 0155:  case 0156:  case 0157:
  case 0160:  case 0161:  case 0162:  case 0163:  case 0164:  case 0165:  case 0166:  case 0167:
  case 0170:  case 0171:  case 0172:  case 0173:  case 0174:  case 0175:  case 0176:  case 0177:
    c_in = c;
    return Operator::ACCEPT;
  case '\\': {
    int d = com.readChar(session.timeout());
    if (d == -1) {
      session.setLastError(com.getLastError());
      return Operator::ERROR;
    } else if (d == -2)
      return Operator::TIMEOUT;

    if (d == '\"' || d == '\\') {
      c_in = d;
      return Operator::ACCEPT;
    } else {
      com.unReadChar(d);
      com.unReadChar(c);
      return Operator::REJECT;
    }
  }
  default:
    break;
  }
  
  com.unReadChar(c);
  return Operator::REJECT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectLiteral(string &s_in)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  string literal;
  Operator::ParseResult res;

  if ((res = expectThisString("{")) != Operator::ACCEPT)
    return res;

  unsigned int nchar;

  if ((res = expectNumber(nchar)) != Operator::ACCEPT) {
    session.setLastError("expected number");
    return res;
  }

  if ((res = expectThisString("}")) != Operator::ACCEPT) {
    session.setLastError("expected }");
    return res;
  }

  if ((res = expectCRLF()) != Operator::ACCEPT) {
    session.setLastError("expected CRLF");
    return Operator::ERROR;
  }

  com << "+ ok, send " << nchar << " bytes of data." << endl;
  com.flushContent();

  for (unsigned int i = 0; i < nchar; ++i) {
    int c = com.readChar(session.timeout());
    if (c == -1) {
      session.setLastError(com.getLastError());
      return Operator::ERROR;
    } else if (c == -2)
      return Operator::TIMEOUT;
    else
      literal += c;
  }

  s_in = literal;

  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectNumber(unsigned int &i_in)
{
  i_in = 0;
  unsigned int n;
  Operator::ParseResult res;

  while ((res = expectDigit(n)) == Operator::ACCEPT) {
    if (i_in == 0)
      i_in = n;
    else
      i_in = (i_in * 10) + n;
  }

  if (res == Operator::TIMEOUT)
    return res;
  
  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectDigit(unsigned int &i_in)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  int c = com.readChar(session.timeout());
  if (c == -1) {
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  } else if (c == -2)
    return Operator::TIMEOUT;

  if (c == '0') {
    i_in = 0;
    return Operator::ACCEPT;
  } else
    com.unReadChar(c);

  if (expectDigitNZ(i_in) != Operator::ACCEPT)
    return Operator::REJECT;

  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectDigitNZ(unsigned int &i_in)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  int c;
  switch ((c = com.readChar(session.timeout()))) {
  case '1': i_in = 1; break;
  case '2': i_in = 2; break;
  case '3': i_in = 3; break;
  case '4': i_in = 4; break;
  case '5': i_in = 5; break;
  case '6': i_in = 6; break;
  case '7': i_in = 7; break;
  case '8': i_in = 8; break;
  case '9': i_in = 9; break;
  case -1: 
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  case -2:
    return Operator::TIMEOUT;
  default:
    com.unReadChar(c);
    return Operator::REJECT;
  }
  
  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectSet(SequenceSet &s_in)
{
  Session &session = Session::getInstance();
  unsigned int seqnum = (unsigned int) -1;
  
  Operator::ParseResult res;

  /* if a set does not start with a sequencenum, then it's not a
   * set. :-) seqnum == -1 means '*'. */
  if ((res = expectSequenceNum(seqnum)) != Operator::ACCEPT)
    return res;

  /* the first number is always a part of the set */
  s_in.addNumber(seqnum);

  /* if _after_ a set there is a ':', then there will always be a
   * sequencenum after the colon. if not, it's a syntax error. a
   * colon delimits two numbers in a range. */
  if ((res = expectThisString(":")) == Operator::ACCEPT) {
    unsigned int seqnum2 = (unsigned int) -1;
    if ((res = expectSequenceNum(seqnum2)) != Operator::ACCEPT) {
      session.setLastError("expected sequencenum");
      return res;
    }

    s_in.addRange(seqnum, seqnum2);
  }

  /* if _after_ a set there is a ',', then there will always be
   * a set after the comma. if not, it's a syntax error. */
  if ((res = expectThisString(",")) == Operator::ACCEPT)
    if ((res = expectSet(s_in)) != Operator::ACCEPT) {
      session.setLastError("expected set");
      return res;
    }

  return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectSequenceNum(unsigned int &i_in)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  int c = com.readChar(session.timeout());
  if (c == -1) {
    session.setLastError(com.getLastError());
    return Operator::ERROR;
  } else if (c == -2)
    return Operator::TIMEOUT;

  if (c == '*') {
    i_in = (unsigned int) -1;
    return Operator::ACCEPT;
  } else
    com.unReadChar(c);

  if (expectNZNumber(i_in) != Operator::ACCEPT)
    return Operator::REJECT;
  else
    return Operator::ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult Binc::expectNZNumber(unsigned int &i_in)
{
  unsigned int c;
  Operator::ParseResult res;

  if ((res = expectDigitNZ(c)) != Operator::ACCEPT)
    return res;
    
  i_in = c;
  while ((res = expectDigit(c)) == Operator::ACCEPT)
    i_in = (i_in * 10) + c;

  if (res == Operator::TIMEOUT)
    return res;

  return Operator::ACCEPT;
}
