/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-search.cc
 *  
 *  Description:
 *    Implementation of the SEARCH command.
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

#include <string>
#include <iostream>
#include <algorithm>

#include <ctype.h>
    
#include "imapparser.h"
#include "mailbox.h"
#include "mime.h"
#include "io.h"
#include "convert.h"

#include "recursivedescent.h"

#include "session.h"
#include "depot.h"
#include "operators.h"

using namespace ::std;
using namespace Binc;

//----------------------------------------------------------------------
bool SearchOperator::SearchNode::convertDate(const string &date, 
					     time_t &t,
					     const string &delim)
{
  vector<string> parts;
  split(date, delim, parts);
  if (parts.size() < 3) return false;
  
  struct tm mold;
  memset((char *) &mold, 0, sizeof(struct tm));
  mold.tm_mday = atoi(parts[0].c_str());
  mold.tm_year = atoi(parts[2].c_str()) - 1900;
  
  // accept mixed case months. this is more than the standard
  // accepts.
  string month = parts[1];
  lowercase(month);
  
  if (month == "jan") mold.tm_mon = 0;
  else if (month == "feb") mold.tm_mon = 1;
  else if (month == "mar") mold.tm_mon = 2;
  else if (month == "apr") mold.tm_mon = 3;
  else if (month == "may") mold.tm_mon = 4;
  else if (month == "jun") mold.tm_mon = 5;
  else if (month == "jul") mold.tm_mon = 6;
  else if (month == "aug") mold.tm_mon = 7;
  else if (month == "sep") mold.tm_mon = 8;
  else if (month == "oct") mold.tm_mon = 9;
  else if (month == "nov") mold.tm_mon = 10;
  else if (month == "dec") mold.tm_mon = 11;
  
  t = mktime(&mold);
  return true;
}

//----------------------------------------------------------------------
bool SearchOperator::SearchNode::convertDateHeader(const string &d_in,
						   time_t &t)
{
  string date = d_in;
  string::size_type n = date.find(',');
  if (n != string::npos)
    date = date.substr(n + 1);
  trim(date);
  
  bool result = convertDate(date, t, " ");
  return result;
}

//----------------------------------------------------------------------
SearchOperator::SearchNode::SearchNode(void)
{
}

//----------------------------------------------------------------------
SearchOperator::SearchNode::SearchNode(const BincImapParserSearchKey &a)
{
  init(a);
}

//----------------------------------------------------------------------
int SearchOperator::SearchNode::getType(void) const
{
  return type;
}

//----------------------------------------------------------------------
bool SearchOperator::SearchNode::match(Mailbox *mailbox,
				       Message *m,
				       unsigned int seqnr,
				       unsigned int lastmessage,
				       unsigned int lastuid) const
{
  HeaderItem hitem;
  string tmp;

  switch (type) {
    //--------------------------------------------------------------------
  case S_ALL: 
    return true;
    //--------------------------------------------------------------------
  case S_ANSWERED: 
    return (m->getStdFlags() & Message::F_ANSWERED);
    //--------------------------------------------------------------------
  case S_BCC:
    return m->headerContains("bcc", astring);
    //--------------------------------------------------------------------
  case S_BEFORE: {
    time_t mtime = m->getInternalDate();
    struct tm *mtime_ = localtime(&mtime);
    mtime_->tm_sec = 0;
    mtime_->tm_min = 0;
    mtime_->tm_hour = 0;
    mtime_->tm_wday = 0;
    mtime_->tm_yday = 0;
    mtime_->tm_isdst = 0;
    mtime = mktime(mtime_);

    time_t atime;
    if (!convertDate(date, atime)) {
      IO &logger = IOFactory::getInstance().get(2);

      logger << "warning, unable to convert " << date << 
	" to a time_t" << endl;
      return false;
    }

    return mtime < atime;
  } //--------------------------------------------------------------------
  case S_BODY:
    return m->bodyContains(astring);
    //--------------------------------------------------------------------
  case S_CC:
    return m->headerContains("cc", astring);
    //--------------------------------------------------------------------
  case S_DELETED:
    return (m->getStdFlags() & Message::F_DELETED);
    //--------------------------------------------------------------------
  case S_FLAGGED:
    return (m->getStdFlags() & Message::F_FLAGGED);
    //--------------------------------------------------------------------
  case S_FROM:
    return m->headerContains("from", astring);
    //--------------------------------------------------------------------
  case S_KEYWORD: 
    // the server does not support keywords
    return false;
    //--------------------------------------------------------------------
  case S_NEW:
    return (m->getStdFlags() & Message::F_RECENT) 
      && !(m->getStdFlags() & Message::F_SEEN);
    //--------------------------------------------------------------------
  case S_OLD:
    return !(m->getStdFlags() & Message::F_RECENT);
    //--------------------------------------------------------------------
  case S_ON: {
    time_t mtime = m->getInternalDate();
    struct tm *mtime_ = localtime(&mtime);
    mtime_->tm_sec = 0;
    mtime_->tm_min = 0;
    mtime_->tm_hour = 0;
    mtime_->tm_wday = 0;
    mtime_->tm_yday = 0;
    mtime_->tm_isdst = 0;
    mtime = mktime(mtime_);

    time_t atime;
    if (!convertDate(date, atime)) {
      IO &logger = IOFactory::getInstance().get(2);

      logger << "warning, unable to convert " << date << 
	" to a time_t" << endl;
      return false;
    }

    return mtime == atime;
  } //--------------------------------------------------------------------
  case S_RECENT:
    return (m->getStdFlags() & Message::F_RECENT);
    //--------------------------------------------------------------------
  case S_SEEN:
    return (m->getStdFlags() & Message::F_SEEN);
    //--------------------------------------------------------------------
  case S_SINCE: {
    time_t mtime = m->getInternalDate();
    struct tm *mtime_ = localtime(&mtime);
    mtime_->tm_sec = 0;
    mtime_->tm_min = 0;
    mtime_->tm_hour = 0;
    mtime_->tm_wday = 0;
    mtime_->tm_yday = 0;
    mtime_->tm_isdst = 0;
    mtime = mktime(mtime_);

    time_t atime;
    if (!convertDate(date, atime)) {
      IO &logger = IOFactory::getInstance().get(2);

      logger << "warning, unable to convert " << date << 
	" to a time_t" << endl;
      return false;
    }

    return mtime >= atime;
  } //--------------------------------------------------------------------
  case S_SUBJECT:
    return m->headerContains("subject", astring);
    //--------------------------------------------------------------------
  case S_TEXT:
    return m->textContains(astring);
    //--------------------------------------------------------------------
  case S_TO:
    return m->headerContains("to", astring);
    //--------------------------------------------------------------------
  case S_UNANSWERED:
    return !(m->getStdFlags() & Message::F_ANSWERED);
    //--------------------------------------------------------------------
  case S_UNDELETED:
    return !(m->getStdFlags() & Message::F_DELETED);
    //--------------------------------------------------------------------
  case S_UNFLAGGED:
    return !(m->getStdFlags() & Message::F_FLAGGED);
    //--------------------------------------------------------------------
  case S_UNKEYWORD:
    // the server does not support keywords
    return true;
    //--------------------------------------------------------------------
  case S_UNSEEN:
    return !(m->getStdFlags() & Message::F_SEEN);
    //--------------------------------------------------------------------
  case S_DRAFT:
    return (m->getStdFlags() & Message::F_DRAFT);
    //--------------------------------------------------------------------
  case S_HEADER:
    return m->headerContains(astring, bstring);
    //--------------------------------------------------------------------
  case S_LARGER: {
    return (m->getSize(true) > number);
  }
    //--------------------------------------------------------------------
  case S_NOT:
    for (vector<SearchNode>::const_iterator i = children.begin();
	 i != children.end(); ++i)
      if ((*i).match(mailbox, m, seqnr, lastmessage, lastuid))
	return false;
    return true;
    //--------------------------------------------------------------------
  case S_OR:
    for (vector<SearchNode>::const_iterator i = children.begin();
	 i != children.end(); ++i)
      if ((*i).match(mailbox, m, seqnr, lastmessage, lastuid))
	return true;
    return false;
    //--------------------------------------------------------------------
  case S_SENTBEFORE: {
    string tmp = m->getHeader("date");
    if (tmp == "")
      return false;

    lowercase(tmp);

    time_t mtime;
    if (!convertDateHeader(tmp, mtime))
      return false;

    if (mtime == (time_t) -1)
      return false;

    time_t atime;
    if (!convertDate(date, atime)) {
      IO &logger = IOFactory::getInstance().get(2);

      logger << "warning, unable to convert " << date << 
	" to a time_t" << endl;
      return false;
    }

    return mtime < atime;
  } //--------------------------------------------------------------------
  case S_SENTON: {
    string tmp = m->getHeader("date");
    if (tmp == "")
      return false;

    lowercase(tmp);

    time_t mtime;
    if (!convertDateHeader(tmp, mtime))
      return false;

    if (mtime == (time_t) -1)
      return false;

    time_t atime;
    if (!convertDate(date, atime)) {
      IO &logger = IOFactory::getInstance().get(2);

      logger << "warning, unable to convert " << date << 
	" to a time_t" << endl;
      return false;
    }

    return mtime == atime;
  } //--------------------------------------------------------------------
  case S_SENTSINCE: {
    string tmp = m->getHeader("date");
    if (tmp == "")
      return false;

    lowercase(tmp);

    time_t mtime;
    if (!convertDateHeader(tmp, mtime))
      return false;

    if (mtime == (time_t) -1)
      return false;

    time_t atime;
    if (!convertDate(date, atime)) {
      IO &logger = IOFactory::getInstance().get(2);

      logger << "warning, unable to convert " << date << 
	" to a time_t" << endl;
      return false;
    }

    return mtime >= atime;
  } //--------------------------------------------------------------------
  case S_SMALLER:
    return (m->getSize(true) < number);
    //--------------------------------------------------------------------
  case S_UID:
    if (!bset->isInSet(m->getUID()))
      if (!(m->getUID() == lastuid && !bset->isLimited()))
	return false;
    return true;
    //--------------------------------------------------------------------
  case S_UNDRAFT:
    return !(m->getStdFlags() & Message::F_DRAFT);
    //--------------------------------------------------------------------
  case S_SET:
    if (!bset->isInSet(seqnr))
      if (!(seqnr == lastmessage && !bset->isLimited()))
	return false;
    return true;
    //--------------------------------------------------------------------
  case S_AND:
    for (vector<SearchNode>::const_iterator i = children.begin();
	 i != children.end(); ++i)
      if (!(*i).match(mailbox, m, seqnr, lastmessage, lastuid))
	return false;
    return true;
  }

  return false;
}

//----------------------------------------------------------------------
void SearchOperator::SearchNode::init(const BincImapParserSearchKey &a)
{
  astring = a.astring;
  bstring = a.bstring;
  date = a.date;
  number = a.number;
  uppercase(astring);
  uppercase(bstring);
  uppercase(date);

  if (a.name      == "ALL")            { type = S_ALL;        weight = 1; }
  else if (a.name == "ANSWERED")       { type = S_ANSWERED;   weight = 1; }
  else if (a.name == "BCC")            { type = S_BCC;        weight = 2; }
  else if (a.name == "BEFORE")         { type = S_BEFORE;     weight = 2; }
  else if (a.name == "BODY")           { type = S_BODY;       weight = 1; }
  else if (a.name == "CC")             { type = S_CC;         weight = 2; }
  else if (a.name == "DELETED")        { type = S_DELETED;    weight = 1; }
  else if (a.name == "FLAGGED")        { type = S_FLAGGED;    weight = 1; }
  else if (a.name == "FROM")           { type = S_FROM;       weight = 2; }
  else if (a.name == "KEYWORD")        { type = S_KEYWORD;    weight = 3; }
  else if (a.name == "NEW")            { type = S_NEW;        weight = 1; }
  else if (a.name == "OLD")            { type = S_OLD;        weight = 1; }
  else if (a.name == "ON")             { type = S_ON;         weight = 1; }
  else if (a.name == "RECENT")         { type = S_RECENT;     weight = 1; }
  else if (a.name == "SEEN")           { type = S_SEEN;       weight = 1; }
  else if (a.name == "SINCE")          { type = S_SINCE;      weight = 1; }
  else if (a.name == "SUBJECT")        { type = S_SUBJECT;    weight = 2; }
  else if (a.name == "TEXT")           { type = S_TEXT;       weight = 4; }
  else if (a.name == "TO")             { type = S_TO;         weight = 2; }
  else if (a.name == "UNANSWERED")     { type = S_UNANSWERED; weight = 1; }
  else if (a.name == "UNDELETED")      { type = S_UNDELETED;  weight = 1; }
  else if (a.name == "UNFLAGGED")      { type = S_UNFLAGGED;  weight = 1; }
  else if (a.name == "UNKEYWORD")      { type = S_UNKEYWORD;  weight = 1; }
  else if (a.name == "UNSEEN")         { type = S_UNSEEN;     weight = 1; }
  else if (a.name == "DRAFT")          { type = S_DRAFT;      weight = 1; }
  else if (a.name == "HEADER")         { type = S_HEADER;     weight = 3; }
  else if (a.name == "LARGER")         { type = S_LARGER;     weight = 4; }
  else if (a.name == "NOT")            {
    // *******                         NOT
    type = S_NOT;
    weight = 1;

    vector<BincImapParserSearchKey>::const_iterator i = a.children.begin();
    while (i != a.children.end()) {
      SearchNode b(*i);
      weight += b.getWeight();
      children.push_back(b);
      ++i;
    }

  } else if (a.name == "OR") {
    // *******                         OR
    type = S_OR;
    weight = 0;

    vector<BincImapParserSearchKey>::const_iterator i = a.children.begin();
    while (i != a.children.end()) {
      SearchNode b(*i);
      weight += b.getWeight();

      children.push_back(b);
      ++i;
    }

  } else if (a.name == "SENTBEFORE")   { type = S_SENTBEFORE; weight = 1; }
  else if (a.name == "SENTON")         { type = S_SENTON;     weight = 1; }
  else if (a.name == "SENTSINCE")      { type = S_SENTSINCE;  weight = 1; }
  else if (a.name == "SMALLER")        { type = S_SMALLER;    weight = 4; } 
  else if (a.name == "UID") {
    bset = &a.getSet();
    type = S_UID;
    weight = 1;
  } else if (a.name == "UNDRAFT")        { type = S_UNDRAFT;    weight = 1; }
  else if (a.type == BincImapParserSearchKey::KEY_SET) {
    bset = &a.getSet();
    type = S_SET;
    weight = 1;
  } else if (a.type == BincImapParserSearchKey::KEY_AND) {
    // *******                         AND
    type = S_AND;
    weight = 0;

    vector<BincImapParserSearchKey>::const_iterator i = a.children.begin();
    while (i != a.children.end()) {
      SearchNode b(*i);
      weight += b.getWeight();
      children.push_back(b);
      ++i;
    }
  }
}

//----------------------------------------------------------------------
int SearchOperator::SearchNode::getWeight(void) const
{
  return weight;
}

//----------------------------------------------------------------------
void SearchOperator::SearchNode::setWeight(int i)
{
  weight = i;
}

//----------------------------------------------------------------------
void SearchOperator::SearchNode::order(void)
{
  for (vector<SearchNode>::iterator i = children.begin();
       i != children.end(); ++i)
    (*i).order();
  ::stable_sort(children.begin(), children.end(), compareNodes);
}

//----------------------------------------------------------------------
SearchOperator::SearchOperator(void)
{
}

//----------------------------------------------------------------------
SearchOperator::~SearchOperator(void)
{
}

//----------------------------------------------------------------------
const string SearchOperator::getName(void) const
{
  return "SEARCH";
}

//----------------------------------------------------------------------
int SearchOperator::getState(void) const
{
  return Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult SearchOperator::process(Depot &depot,
						Request &command)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  Mailbox *mailbox = depot.getSelected();

  if (command.getCharSet() != "" && command.getCharSet() != "US-ASCII") {
    session.setLastError("[BADCHARSET (\"US-ASCII\")]");
    return NO;
  }

  com << "* SEARCH";

  SearchNode s(command.searchkey);
  s.order();

  const unsigned int maxsqnr = mailbox->getMaxSqnr();
  const unsigned int maxuid = mailbox->getMaxUid();

  Mailbox::iterator i
    = mailbox->begin(SequenceSet::all(), Mailbox::SKIP_EXPUNGED);
  for (; i != mailbox->end(); ++i) {
    Message &message = *i;

    if (s.match(mailbox, &message, i.getSqnr(), maxsqnr, maxuid)) {
      com << " " << (command.getUidMode() ?  message.getUID() : i.getSqnr());
      com.flushContent();
    }

    message.close();
  }

  com << endl;
  return OK;
}

//------------------------------------------------------------------------
Operator::ParseResult SearchOperator::parse(Request & c_in) const
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE");
    return res;
  }

  if ((res = expectThisString("CHARSET")) == ACCEPT) {
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE after CHARSET");
      return res;
    }

    string charset;
    if ((res = expectAstring(charset)) != ACCEPT) {
      session.setLastError("Expected astring after CHARSET SPACE");
      return res;
    }

    c_in.setCharSet(charset);
    
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE after CHARSET SPACE astring");
      return res;
    }
  }

  BincImapParserSearchKey b;
  if ((res = expectSearchKey(b)) != ACCEPT) {
    session.setLastError("Expected search_key");
    return res;
  }

  c_in.searchkey.type = BincImapParserSearchKey::KEY_AND;
  c_in.searchkey.children.push_back(b);
  
  while (1) {
    if ((res = expectSPACE()) != ACCEPT)
      break;

    BincImapParserSearchKey c;
    if ((res = expectSearchKey(c)) != ACCEPT) {
      session.setLastError("Expected search_key after search_key SPACE");
      return res;
    }
      
    c_in.searchkey.children.push_back(c);
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF after search_key");
    return res;
  }
  
  c_in.setName("SEARCH");
  return ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult
SearchOperator::expectSearchKey(BincImapParserSearchKey &s_in) const
{
  Session &session = Session::getInstance();
  Operator::ParseResult res;

  s_in.type = BincImapParserSearchKey::KEY_OTHER;
  if ((res = expectThisString("ALL")) == ACCEPT) s_in.name = "ALL";
  else if ((res = expectThisString("ANSWERED")) == ACCEPT) s_in.name = "ANSWERED";
  else if ((res = expectThisString("BCC")) == ACCEPT) {
    s_in.name = "BCC";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAstring(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }
  } else if ((res = expectThisString("BEFORE")) == ACCEPT) {
    s_in.name = "BEFORE";
      
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectDate(s_in.date)) != ACCEPT) {
      session.setLastError("Expected date");
      return res;
    }
  } else if ((res = expectThisString("BODY")) == ACCEPT) {
    s_in.name = "BODY";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAstring(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }
  } else if ((res = expectThisString("CC")) == ACCEPT) {
    s_in.name = "CC";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAstring(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }
  } else if ((res = expectThisString("DELETED")) == ACCEPT) s_in.name = "DELETED";
  else if ((res = expectThisString("FLAGGED")) == ACCEPT) s_in.name = "FLAGGED";
  else if ((res = expectThisString("FROM")) == ACCEPT) {
    s_in.name = "FROM";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAstring(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }
  } else if ((res = expectThisString("KEYWORD")) == ACCEPT) {
    s_in.name = "KEYWORD";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAtom(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected flag_keyword");
      return res;
    }
  } else if ((res = expectThisString("NEW")) == ACCEPT) s_in.name = "NEW";
  else if ((res = expectThisString("OLD")) == ACCEPT) s_in.name = "OLD";
  else if ((res = expectThisString("ON")) == ACCEPT) {
    s_in.name = "ON";
      
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectDate(s_in.date)) != ACCEPT) {
      session.setLastError("Expected date");
      return res;
    }
  } else if ((res = expectThisString("RECENT")) == ACCEPT) s_in.name = "RECENT";
  else if ((res = expectThisString("SEEN")) == ACCEPT) s_in.name = "SEEN";
  else if ((res = expectThisString("SINCE")) == ACCEPT) {
    s_in.name = "SINCE";
      
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectDate(s_in.date)) != ACCEPT) {
      session.setLastError("Expected date");   
      return res;
    }
  } else if ((res = expectThisString("SUBJECT")) == ACCEPT) {
    s_in.name = "SUBJECT";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAstring(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }
  } else if ((res = expectThisString("TEXT")) == ACCEPT) {
    s_in.name = "TEXT";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAstring(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }
  } else if ((res = expectThisString("TO")) == ACCEPT) {
    s_in.name = "TO";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAstring(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }
  } else if ((res = expectThisString("UNANSWERED")) == ACCEPT)
    s_in.name = "UNANSWERED";
  else if ((res = expectThisString("UNDELETED")) == ACCEPT) s_in.name = "UNDELETED";
  else if ((res = expectThisString("UNFLAGGED")) == ACCEPT) s_in.name = "UNFLAGGED";
  else if ((res = expectThisString("UNKEYWORD")) == ACCEPT) {
    s_in.name = "UNKEYWORD";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    if ((res = expectAtom(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected flag_keyword");
      return res;
    }
  } else if ((res = expectThisString("UNSEEN")) == ACCEPT) s_in.name = "UNSEEN";
  else if ((res = expectThisString("DRAFT")) == ACCEPT) s_in.name = "DRAFT";
  else if ((res = expectThisString("HEADER")) == ACCEPT) { 
    s_in.name = "HEADER";
	
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }
	
    if ((res = expectAstring(s_in.astring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }

    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }
	
    if ((res = expectAstring(s_in.bstring)) != ACCEPT) {
      session.setLastError("Expected astring");
      return res;
    }
  } else if ((res = expectThisString("LARGER")) == ACCEPT) {
    s_in.name = "LARGER";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }
	
    if ((res = expectNumber(s_in.number)) != ACCEPT) {
      session.setLastError("Expected number");
      return res;
    }
  } else if ((res = expectThisString("NOT")) == ACCEPT) {
    s_in.name = "NOT";
    s_in.type = BincImapParserSearchKey::KEY_NOT;
	
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    BincImapParserSearchKey s;
    if ((res = expectSearchKey(s)) != ACCEPT) {
      session.setLastError("Expected search_key");
      return res;
    }
    s_in.children.push_back(s);
  } else if ((res = expectThisString("OR")) == ACCEPT) {
    s_in.name = "OR";
    s_in.type = BincImapParserSearchKey::KEY_OR;
	
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    BincImapParserSearchKey s;
    if ((res = expectSearchKey(s)) != ACCEPT) {
      session.setLastError("Expected search_key");
      return res;
    }
    s_in.children.push_back(s);

    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }

    BincImapParserSearchKey t;
    if ((res = expectSearchKey(t)) != ACCEPT) {
      session.setLastError("Expected search_key");
      return res;
    }
    s_in.children.push_back(t);
  } else if ((res = expectThisString("SENTBEFORE")) == ACCEPT) {
    s_in.name = "SENTBEFORE";
	
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }
	
    if ((res = expectDate(s_in.date)) != ACCEPT) {
      session.setLastError("Expected date");
      return res;
    }
  } else if ((res = expectThisString("SENTON")) == ACCEPT) {
    s_in.name = "SENTON";
	
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }
	
    if ((res = expectDate(s_in.date)) != ACCEPT) {
      session.setLastError("Expected date");      
      return res;
    }

  } else if ((res = expectThisString("SENTSINCE")) == ACCEPT) {
    s_in.name = "SENTSINCE";
	
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }
	
    if ((res = expectDate(s_in.date)) != ACCEPT) {
      session.setLastError("Expected date");
      return res;
    }
  } else if ((res = expectThisString("SMALLER")) == ACCEPT) {
    s_in.name = "SMALLER";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }
	
    if ((res = expectNumber(s_in.number)) != ACCEPT) {
      session.setLastError("Expected number");
      return res;
    }
  } else if ((res = expectThisString("UID")) == ACCEPT) {
    s_in.name = "UID";
    if ((res = expectSPACE()) != ACCEPT) {
      session.setLastError("Expected SPACE");
      return res;
    }
	
    if ((res = expectSet(s_in.bset)) != ACCEPT) {
      session.setLastError("Expected number");
      return res;
    }
  } else if ((res = expectThisString("UNDRAFT")) == ACCEPT) s_in.name = "UNDRAFT";
  else if ((res = expectSet(s_in.bset)) == ACCEPT) {
    s_in.name = "";
    s_in.type = BincImapParserSearchKey::KEY_SET;
  } else if ((res = expectThisString("(")) == ACCEPT) {
    s_in.type = BincImapParserSearchKey::KEY_AND;

    while (1) {
      BincImapParserSearchKey c;
      if ((res = expectSearchKey(c)) != ACCEPT) {
	session.setLastError("Expected search_key");
	return res;
      }
	 
      s_in.children.push_back(c);
	  
      if ((res = expectSPACE()) != ACCEPT)
	break;
    }

    if ((res = expectThisString(")")) != ACCEPT) {
      session.setLastError("Expected )");
      return res;
    }
  } else
    return REJECT;
    
  return ACCEPT;
}
