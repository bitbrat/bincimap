/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/operators.h
 *  
 *  Description:
 *    <--->
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

#ifndef operators_h_included
#define operators_h_included
#include <string>
#include <vector>

#include "imapparser.h"
#include "depot.h"
#include "message.h"

namespace Binc {
  
  //--------------------------------------------------------------------
  class Operator {
  public:
    enum ProcessResult {OK, BAD, NO, NOTHING, ABORT};
    enum ParseResult {ACCEPT, REJECT, ERROR, TIMEOUT};

    virtual ProcessResult process(Depot &, Request &) = 0;
    virtual ParseResult parse(Request &) const = 0;
    virtual int getState(void) const = 0;
    virtual const std::string getName(void) const = 0;

    //--
    virtual ~Operator(void) {};
  };

  //--------------------------------------------------------------------
  class AppendOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;
    const std::string getName(void) const;
    int getState(void) const;

    AppendOperator(void);
    ~AppendOperator(void);
  };

  //--------------------------------------------------------------------
  class AuthenticateOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    AuthenticateOperator(void);
    ~AuthenticateOperator(void);
  };

  //--------------------------------------------------------------------
  class CapabilityOperator : public Operator {
    std::vector<std::string> capabilities;
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;
    
    void addCapability(const std::string &cap);

    CapabilityOperator(void);
    ~CapabilityOperator(void);
  };

  //--------------------------------------------------------------------
  class CheckOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    CheckOperator(void);
    ~CheckOperator(void);
  };

  //--------------------------------------------------------------------
  class CreateOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    CreateOperator(void);
    ~CreateOperator(void);
  };

  //--------------------------------------------------------------------
  class CloseOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    CloseOperator(void);
    ~CloseOperator(void);
  };

  //--------------------------------------------------------------------
  class CopyOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    CopyOperator(void);
    ~CopyOperator(void);
  };

  //--------------------------------------------------------------------
  class DeleteOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    DeleteOperator(void);
    ~DeleteOperator(void);
  };

  //--------------------------------------------------------------------
  class ExpungeOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    ExpungeOperator(void);
    ~ExpungeOperator(void);
  };

  //--------------------------------------------------------------------
  class FetchOperator : public Operator {
  protected:
    ParseResult expectSectionText(BincImapParserFetchAtt &f_in) const;
    ParseResult expectSection(BincImapParserFetchAtt &f_in) const;
    ParseResult expectFetchAtt(BincImapParserFetchAtt &f_in) const;
    ParseResult expectOffset(BincImapParserFetchAtt &f_in) const;
    ParseResult expectHeaderList(BincImapParserFetchAtt &f_in) const;
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    FetchOperator(void);
    ~FetchOperator(void);
  };

  //--------------------------------------------------------------------
  class ListOperator : public Operator {
  protected:
    enum MailboxFlags {
      DIR_SELECT = 0x01,
      DIR_MARKED = 0x02,
      DIR_NOINFERIORS = 0x04,
      DIR_LEAF = 0x08
    };

    std::map<std::string, unsigned int> cache;
    time_t cacheTimeout;
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    ListOperator(void);
    ~ListOperator(void);
  };

  //--------------------------------------------------------------------
  class LoginOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    LoginOperator(void);
    ~LoginOperator(void);
  };

  //--------------------------------------------------------------------
  class LogoutOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    LogoutOperator(void);
    ~LogoutOperator(void);
  };

  //--------------------------------------------------------------------
  class LsubOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    LsubOperator(void);
    ~LsubOperator(void);
  };

  //--------------------------------------------------------------------
  class NoopOperator : public Operator {
  public:
    virtual ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    NoopOperator(void);
    ~NoopOperator(void);
  };

  //--------------------------------------------------------------------
  class NoopPendingOperator : public NoopOperator {
  public:
    ProcessResult process(Depot &, Request &);

    NoopPendingOperator(void);
    ~NoopPendingOperator(void);
  };

  //--------------------------------------------------------------------
  class RenameOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    RenameOperator(void);
    ~RenameOperator(void);
  };

  //--------------------------------------------------------------------
  class SearchOperator : public Operator {
  protected:
    ParseResult expectSearchKey(BincImapParserSearchKey &s_in) const;

    //------------------------------------------------------------------
    class SearchNode {

      std::string date;
      std::string astring;
      std::string bstring;
      unsigned int number;
      
      int type;
      mutable int weight;
      const SequenceSet *bset;
      
      std::vector<SearchNode> children;
      
    public:
      enum {
	S_ALL, S_ANSWERED, S_BCC, S_BEFORE, S_BODY, S_CC, S_DELETED,
	S_FLAGGED, S_FROM, S_KEYWORD, S_NEW, S_OLD, S_ON, S_RECENT,
	S_SEEN, S_SINCE, S_SUBJECT, S_TEXT, S_TO, S_UNANSWERED,
	S_UNDELETED, S_UNFLAGGED, S_UNKEYWORD, S_UNSEEN, S_DRAFT,
	S_HEADER, S_LARGER, S_NOT, S_OR, S_SENTBEFORE, S_SENTON,
	S_SENTSINCE, S_SMALLER, S_UID, S_UNDRAFT, S_SET, S_AND
      };
      
      static bool convertDate(const std::string &date, time_t &t, const std::string &delim = "-");
      static bool convertDateHeader(const std::string &d_in, time_t &t);

      void order(void);
      
      bool match(Mailbox *, Message *, 
		 unsigned seqnr, unsigned int lastmessage, 
		 unsigned int lastuid) const;
      
      int getType(void) const;
      int getWeight(void) const;
      void setWeight(int i);
      
      void init(const BincImapParserSearchKey &a);

      //-
      static bool compareNodes(const SearchNode &a, 
			       const SearchNode &b)
      {
	return a.getWeight() < b.getWeight();
      }
      
      SearchNode(void);
      SearchNode(const BincImapParserSearchKey &a);
    };
   
  public:

    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    SearchOperator(void);
    ~SearchOperator(void);
  };

  //--------------------------------------------------------------------
  class SelectOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    SelectOperator(void);
    ~SelectOperator(void);
  };

  //--------------------------------------------------------------------
  class ExamineOperator : public SelectOperator {
  public:
    const std::string getName(void) const;
    ExamineOperator(void);
    ~ExamineOperator(void);
  };

#ifdef WITH_SSL
  //--------------------------------------------------------------------
  class StarttlsOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    StarttlsOperator(void);
    ~StarttlsOperator(void);
  };
#endif

  //--------------------------------------------------------------------
  class StatusOperator : public Operator {

    std::map<int, Status> statuses;

  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    StatusOperator(void);
    ~StatusOperator(void);
  };

  //--------------------------------------------------------------------
  class StoreOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    StoreOperator(void);
    ~StoreOperator(void);
  };

  //--------------------------------------------------------------------
  class SubscribeOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    SubscribeOperator(void);
    ~SubscribeOperator(void);
  };

  //--------------------------------------------------------------------
  class UnsubscribeOperator : public Operator {
  public:
    ProcessResult process(Depot &, Request &);
    virtual ParseResult parse(Request &) const;

    const std::string getName(void) const;
    int getState(void) const;

    UnsubscribeOperator(void);
    ~UnsubscribeOperator(void);
  };
}

#endif
