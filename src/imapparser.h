/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/parsers/imap/imapparser.h
 *  
 *  Description:
 *    Declaration of the common items for parsing IMAP input
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

#ifndef imapparser_h_included
#define imapparser_h_included

/* stl includes */
#include <string>
#include <map>
#include <vector>

namespace Binc {
  //------------------------------------------------------------------------
  class SequenceSet {
  public:
    void addRange(unsigned int a_in, unsigned int b_in);
    bool isInSet(unsigned int n) const;
    void addNumber(unsigned int a_in);
    inline bool isLimited(void) const { return limited; }

    static SequenceSet &all(void);

    static SequenceSet &null(void);

    SequenceSet &operator = (const SequenceSet &copy);

    SequenceSet(void);
    SequenceSet(const SequenceSet &copy);
    ~SequenceSet(void);

  protected:
    bool isNull(void) const;

  private:
    bool limited;
    bool nullSet;

    class Range {
    public:
      unsigned int from;
      unsigned int to;
      Range(unsigned int from, unsigned int to);
    };

    std::vector<Range> internal;
  };

  //------------------------------------------------------------------------
  class BincImapParserFetchAtt {
  public:
    std::string type;
    std::string section;
    std::string sectiontext;
    std::vector<std::string> headerlist;
    unsigned int offsetstart;
    unsigned int offsetlength;
    bool hassection;

    BincImapParserFetchAtt(const std::string &typeName = "");

    std::string toString(void);
  };

  //------------------------------------------------------------------------
  class BincImapParserSearchKey {
  public:
    std::string name;
    std::string date;
    std::string astring;
    std::string bstring;
    int type;
    unsigned int number;
    SequenceSet bset;
    enum {KEY_AND, KEY_OR, KEY_NOT, KEY_OTHER, KEY_SET};
      
    std::vector<BincImapParserSearchKey> children;

    const SequenceSet& getSet(void) const;

    BincImapParserSearchKey(void);
  };

  //------------------------------------------------------------------------
  class BincImapParserData {
  public:
    virtual ~BincImapParserData(void) {}
  };

  //------------------------------------------------------------------------
  class Request {
  private:
    std::string tag;
    std::string name;
    std::string mode;
    std::string date;
    std::string userid;
    std::string password;
    std::string mailbox;
    std::string newmailbox;
    std::string authtype;
    std::string listmailbox;
    std::string charset;
    std::string literal;
    bool uidmode;
      
  public:
    BincImapParserData * extra;
    std::vector<std::string> flags;
    std::vector<std::string> statuses;

    SequenceSet bset;
    BincImapParserSearchKey searchkey;
    std::vector<BincImapParserFetchAtt> fatt;

    void setUidMode(void);
    bool getUidMode(void) const;

    void setTag(std::string &t_in);
    const std::string &getTag(void) const;

    void setMode(const std::string &m_in);
    const std::string &getMode(void) const;

    void setName(const std::string &s_in);
    const std::string &getName(void) const;

    void setLiteral(const std::string &s_in);
    const std::string &getLiteral(void) const;

    void setDate(const std::string &s_in);
    const std::string &getDate(void) const;

    void setCharSet(const std::string &s_in);
    const std::string &getCharSet(void) const;

    void setUserID(const std::string &s_in);
    const std::string &getUserID(void) const;

    void setPassword(const std::string &s_in);
    const std::string &getPassword(void) const;

    void setMailbox(const std::string &s_in);
    const std::string &getMailbox(void) const;

    void setAuthType(const std::string &s_in);
    const std::string &getAuthType(void) const;

    void setNewMailbox(const std::string &s_in);
    const std::string &getNewMailbox(void) const;

    void setListMailbox(const std::string &s_in);
    const std::string &getListMailbox(void) const;

    SequenceSet &getSet(void);

    std::vector<std::string> &getFlags(void);
    std::vector<std::string> &getStatuses(void);

    Request(void);
    ~Request(void);
  };
}


#endif
