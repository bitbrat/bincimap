/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/mailbox/maildir.h
 *  
 *  Description:
 *    Declaration of the Maildir class.
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

#ifndef maildir_h_included
#define maildir_h_included
#include <string>
#include <vector>
#include <map>

#include "mailbox.h"
#include "maildirmessage.h"

namespace Binc {
  static const std::string CACHEFILEVERSION = "1.0.5";
  static const std::string UIDVALFILEVERSION = "1.0.5";
  
  //------------------------------------------------------------------------
  class MaildirIndexItem {
  public:
    unsigned int uid;
    std::string fileName;
  };

  //------------------------------------------------------------------------
  class MaildirIndex
  {
  private:
    std::map<std::string, MaildirIndexItem> idx;

  public:
    void insert(const std::string &unique, unsigned int uid,
		const std::string &fileName = "");
    void remove(const std::string &unique);
    void clear(void);
    void clearFileNames(void);
    void clearUids(void);
    unsigned int getSize(void) const;
    MaildirIndexItem *find(const std::string &unique);
  };

  //------------------------------------------------------------------------
  class Maildir : public Mailbox {
  public:
    typedef std::map<unsigned int, MaildirMessage> MessageMap;

    class iterator : public BaseIterator {
    public:
      iterator(void);
      iterator(Maildir *home, MessageMap::iterator i,
	       const SequenceSet &bset,
	       unsigned int mod = INCLUDE_EXPUNGED | SQNR_MODE);
      iterator(const iterator &copy);
      ~iterator(void);

      Message &operator *(void);
      void operator ++(void);
      bool operator ==(const BaseIterator &) const;
      bool operator !=(const BaseIterator &) const;

      iterator &operator =(const iterator &copy);

      void erase(void);

      friend class Maildir;

    protected:
      void reposition(void);
      MaildirMessage &curMessage(void);

    private:
      Maildir *mailbox;
      SequenceSet bset;
      int mod;

      MessageMap::iterator i;
      iterator(iterator &external);
    };
 
    const std::string getTypeName(void) const;

    Mailbox::iterator begin(const SequenceSet &bset, unsigned int mod = INCLUDE_EXPUNGED | SQNR_MODE) const;
    Mailbox::iterator end(void) const;

    unsigned int getMaxUid(void) const;
    unsigned int getMaxSqnr(void) const;
    unsigned int getUidValidity(void) const;
    unsigned int getUidNext(void) const;

    bool getUpdates(bool doscan, unsigned int type,
		    PendingUpdates &updates);

    const std::string &getPath(void) const;
    void setPath(const std::string &path_in);

    void bumpUidValidity(const std::string &) const;

    unsigned int getStatusID(const std::string &) const;
    bool getStatus(const std::string &, Status &) const;
    void updateFlags(void);

    bool isMailbox(const std::string &) const;
    bool isMarked(const std::string &) const;
    bool selectMailbox(const std::string &name, const std::string &s_in);
    void closeMailbox(void);
    void expungeMailbox(void);
    bool createMailbox(const std::string &s, mode_t mode, uid_t owner = 0, gid_t group = 0, bool root = false);
    bool deleteMailbox(const std::string &s);

    Message *createMessage(const std::string &mbox, time_t idate = 0);
    bool commitNewMessages(const std::string &mbox);
    bool rollBackNewMessages(void);

    bool fastCopy(Message &source, Mailbox &desttype, const std::string &destname);

    //--
    Maildir(void);
    ~Maildir(void);

    friend class Maildir::iterator;
    friend class MaildirMessage;

  protected:
    enum ReadCacheResult {
      Ok,
      NoCache,
      Error
    };

    ReadCacheResult readCache(void);
    bool writeCache(void);
    bool scanFileNames(void) const;

    enum ScanResult {
      Success = 0,
      TemporaryError = 1,
      PermanentError = 2
    };

    ScanResult scan(void);

    MaildirMessage *get(const std::string &id);
    void add(MaildirMessage &m);

  private:
    std::vector<MaildirMessage> newMessages;

    unsigned int uidvalidity;
    unsigned int uidnext;
    bool selected;
    std::string path;

    mutable iterator beginIterator;
    mutable iterator endIterator;

    mutable bool firstscan;
    mutable bool cacheRead;
    mutable MaildirIndex index;
    mutable MessageMap messages;

    mutable unsigned int oldrecent;
    mutable unsigned int oldexists;

    mutable time_t old_cur_st_mtime;
    mutable time_t old_cur_st_ctime;
    mutable time_t old_new_st_mtime;
    mutable time_t old_new_st_ctime;

    mutable bool uidnextchanged;
    mutable bool mailboxchanged;
  };
}

#endif
