/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/parsers/imap/recursivedescent/recursivedescent.h
 *  
 *  Description:
 *    Declaration of a recursive descent IMAP command
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

#ifndef expectcommand_h_inluded
#define expectcommand_h_inluded
#include <stack>
#include <string>

#include "imapparser.h"
#include "operators.h"

namespace Binc {

  extern std::stack<int> inputBuffer;
  extern int charnr;
    
  int readChar(void);
  void unReadChar(int c_in);
  void unReadChar(const std::string &s_in);

  Operator::ParseResult expectTag(std::string &s_in);
  Operator::ParseResult expectTagChar(int &c_in);
  Operator::ParseResult expectSPACE(void);

  Operator::ParseResult expectFlag(std::vector<std::string> &v_in);

  Operator::ParseResult expectListMailbox(std::string &s_in);
  Operator::ParseResult expectListWildcards(int &c_in);

  Operator::ParseResult expectDateTime(std::string &s_in);
  Operator::ParseResult expectTime(std::string &s_in);
  Operator::ParseResult expectZone(std::string &s_in);

  Operator::ParseResult expectMailbox(std::string &s_in);
  Operator::ParseResult expectAstring(std::string &s_in);
  Operator::ParseResult expectAtom(std::string &s_in);
  Operator::ParseResult expectAtomChar(int &i_in);
  Operator::ParseResult expectString(std::string &s_in);
    
  Operator::ParseResult expectDate(std::string &s_in);

  Operator::ParseResult expectNumber(unsigned int &i_in);
  Operator::ParseResult expectDigit(unsigned int &i_in);
  Operator::ParseResult expectDigitNZ(unsigned int &i_in);

  Operator::ParseResult expectLiteral(std::string &s_in);
  Operator::ParseResult expectQuoted(std::string &s_in);
  Operator::ParseResult expectQuotedChar(int &c_in);

  Operator::ParseResult expectSet(SequenceSet &s_in);
  Operator::ParseResult expectSequenceNum(unsigned int &i_in);
  Operator::ParseResult expectNZNumber(unsigned int &i_in);

  Operator::ParseResult expectCRLF(void);
  Operator::ParseResult expectCR(void);
  Operator::ParseResult expectLF(void);

  Operator::ParseResult expectThisString(const std::string &s_in);
}


#endif
