/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    imapparser.cc
 *  
 *  Description:
 *    Implementation of the common items for parsing IMAP input
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

#include "imapparser.h"
#include "io.h"
#include "convert.h"

#include <stdio.h>
#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <exception>

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
Request::Request(void) 
  : extra(0), flags(), statuses(), bset(), searchkey(), fatt()
{
  uidmode = false;
}

Request::~Request(void)
{
  if (extra != 0)
    delete extra;
}

//------------------------------------------------------------------------
void Request::setUidMode(void)
{
  uidmode = true;
}

//------------------------------------------------------------------------
bool Request::getUidMode(void) const
{
  return uidmode;
}

//------------------------------------------------------------------------
void Request::setTag(string &t_in)
{
  tag = t_in;
}

//------------------------------------------------------------------------
const string &Request::getTag(void) const
{
  return tag;
}

//------------------------------------------------------------------------
void Request::setMode(const string &m_in)
{
  mode = m_in;
}

//------------------------------------------------------------------------
const string &Request::getMode(void) const
{ 
  return mode;
}

//------------------------------------------------------------------------
void Request::setName(const string &s_in)
{
  name = s_in;
}

//------------------------------------------------------------------------
const string &Request::getName(void) const
{
  return name;
}

//------------------------------------------------------------------------
void Request::setAuthType(const string &s_in)
{
  authtype = s_in;
}

//------------------------------------------------------------------------
const string &Request::getAuthType(void) const
{
  return authtype;
}

//------------------------------------------------------------------------
void Request::setLiteral(const string &s_in)
{
  literal = s_in;
}

//------------------------------------------------------------------------
const string &Request::getLiteral(void) const
{
  return literal;
}

//------------------------------------------------------------------------
void Request::setDate(const string &s_in)
{
  date = s_in;
}

//------------------------------------------------------------------------
const string &Request::getDate(void) const
{
  return date;
}

//------------------------------------------------------------------------
void Request::setCharSet(const string &s_in)
{
  charset = s_in;
}

//------------------------------------------------------------------------
const string &Request::getCharSet(void) const
{
  return charset;
}

//------------------------------------------------------------------------
void Request::setUserID(const string &s_in)
{
  userid = s_in;
}

//------------------------------------------------------------------------
const string &Request::getUserID(void) const
{
  return userid;
}

//------------------------------------------------------------------------
void Request::setPassword(const string &s_in)
{
  password = s_in;
}

//------------------------------------------------------------------------
const string &Request::getPassword(void) const
{
  return password;
}

//------------------------------------------------------------------------
void Request::setMailbox(const string &s_in)
{
  mailbox = s_in;
}

//------------------------------------------------------------------------
const string &Request::getMailbox(void) const
{
  return mailbox;
}

//------------------------------------------------------------------------
void Request::setListMailbox(const string &s_in)
{
  listmailbox = s_in;
}

//------------------------------------------------------------------------
const string &Request::getListMailbox(void) const
{
  return listmailbox;
}

//------------------------------------------------------------------------
void Request::setNewMailbox(const string &s_in)
{
  newmailbox = s_in;
}

//------------------------------------------------------------------------
const string &Request::getNewMailbox(void) const
{
  return newmailbox;
}

//------------------------------------------------------------------------
SequenceSet &Request::getSet(void)
{  
  return bset;
}

//------------------------------------------------------------------------
vector<string> &Request::getStatuses(void)
{
  return statuses;
}

//------------------------------------------------------------------------
vector<string> &Request::getFlags(void)
{
  return flags;
}

//------------------------------------------------------------------------
SequenceSet::SequenceSet(void) : limited(true), nullSet(false)
{
}

//------------------------------------------------------------------------
SequenceSet::SequenceSet(const SequenceSet &copy) 
  : limited(copy.limited), nullSet(copy.nullSet), internal(copy.internal)
{
}

//------------------------------------------------------------------------
SequenceSet &SequenceSet::operator = (const SequenceSet &copy) 
{
  limited = copy.limited;
  nullSet = copy.nullSet;
  internal = copy.internal;

  return *this;
}

//------------------------------------------------------------------------
SequenceSet::~SequenceSet(void)
{
}

//------------------------------------------------------------------------
SequenceSet &SequenceSet::null(void)
{
  static SequenceSet nil;
  nil.nullSet = true;
  return nil;
}

//------------------------------------------------------------------------
bool SequenceSet::isNull(void) const
{
  return nullSet;
}

//------------------------------------------------------------------------
SequenceSet &SequenceSet::all(void)
{
  static bool initialized = false;
  static SequenceSet all;

  if (!initialized) {
    all.addRange(1, (unsigned int)-1);
    initialized = true;
  }

  return all;
}

//------------------------------------------------------------------------
SequenceSet::Range::Range(unsigned int a, unsigned int b)
{
  if (a > b) {
    from = b;
    to = a;
  } else {
    from = a;
    to = b;
  }
}

//------------------------------------------------------------------------
void SequenceSet::addRange(unsigned int a, unsigned int b)
{
  if (a == (unsigned int)-1 || b == (unsigned int)-1) limited = false;
  internal.push_back(Range(a, b));
}

//------------------------------------------------------------------------
void SequenceSet::addNumber(unsigned int a)
{
  if (a == (unsigned int)-1) limited = false;
  internal.push_back(Range(a, a));
}

//------------------------------------------------------------------------
bool SequenceSet::isInSet(unsigned int n) const
{
  unsigned int maxvalue = 0;
  for (vector<Range>::const_iterator i = internal.begin();
       i != internal.end(); ++i) {
    const Range &r = *i;
    if (r.from > maxvalue) maxvalue = r.from;
    else if (r.to > maxvalue) maxvalue = r.to;

    if (n >= (*i).from && n <= (*i).to) {
      return true;
    }
  }

  return (n > maxvalue && !limited);
}

//------------------------------------------------------------------------
BincImapParserFetchAtt::BincImapParserFetchAtt(const std::string &typeName)
  : type(typeName)
{
  offsetstart = 0;
  offsetlength = (unsigned int) -1;
  hassection = false;
}

//------------------------------------------------------------------------
string BincImapParserFetchAtt::toString(void)
{
  string tmp;
  if (type == "BODY.PEEK")
    tmp = "BODY";
  else
    tmp = type;

  if (type == "BODY" || type == "BODY.PEEK") {
    if (hassection) {
      tmp += "[";
      tmp += section;
      if (sectiontext != "") {
	if (section != "")
	  tmp += ".";
	tmp += sectiontext;
	
	if (headerlist.size() != 0) {
	  tmp += " (";
	  for (vector<string>::iterator i = headerlist.begin();
	       i != headerlist.end(); ++i) {
	    if (i != headerlist.begin())
	      tmp += " ";
	    tmp += *i;
	  }
	  tmp += ")";
	}
      }
      tmp += "]";
    
      if (offsetstart == 0 && offsetlength == (unsigned int) -1)
	tmp += " ";
      else
	tmp += "<" + Binc::toString(offsetstart) + "> ";
    }
  }

  return tmp;
}

//------------------------------------------------------------------------
BincImapParserSearchKey::BincImapParserSearchKey(void)
{
  type = 0;
  number = 0;
}

//------------------------------------------------------------------------
const SequenceSet& BincImapParserSearchKey::getSet(void) const
{
  return bset;
}
