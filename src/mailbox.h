/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/mailbox/mailbox.h
 *  
 *  Description:
 *    Declaration of the Mailbox class.
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

#ifndef mailbox_h_included
#define mailbox_h_included

#include <map>
#include <string>
#include <queue>
#include <vector>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "imapparser.h"

namespace Binc {

  class Message;
  class Status;
  class PendingUpdates;
  class File;

  //------------------------------------------------------------------------
  class Mailbox {
  public:

    //----------------------------------------------------------------------
    class BaseIterator {
    public:
      BaseIterator(int sqn = 0);
      virtual ~BaseIterator(void);
      
      virtual Message &operator *(void) = 0;
      virtual void operator ++(void) = 0;
      virtual bool operator !=(const BaseIterator &d) const = 0;
      virtual bool operator ==(const BaseIterator &d) const = 0;

      virtual void erase(void) = 0;

      unsigned int sqnr;
    };

    //----------------------------------------------------------------------
    class iterator {
    public:
      iterator(BaseIterator &i);

      Message &operator *(void);
      void operator ++(void);
      bool operator ==(const iterator &) const;
      bool operator !=(const iterator &) const;
      
      unsigned int getSqnr() const;

      void erase(void);

    protected:
      BaseIterator &realIterator;
    };

    enum Iterator {
      INCLUDE_EXPUNGED = 1,
      SKIP_EXPUNGED    = 2
    };

    enum Mode {
      UID_MODE = 4,
      SQNR_MODE = 8
    };

    virtual iterator begin(const SequenceSet &bset, unsigned int mod = INCLUDE_EXPUNGED | SQNR_MODE) const = 0;
    virtual iterator end(void) const = 0;

    //-- Generic for one mailbox type
    virtual bool getStatus(const std::string &, Status &) const = 0;
    virtual bool isMailbox(const std::string &) const = 0;
    virtual bool isMarked(const std::string &) const = 0;
    virtual unsigned int getStatusID(const std::string &) const = 0;
    virtual void bumpUidValidity(const std::string &) const = 0;

    //-- Specific for one mailbox
    void setReadOnly(void);
    bool isReadOnly(void) const;

    virtual const std::string getTypeName(void) const = 0;
    const std::string getName(void) const;
    void setName(const std::string &name);

    virtual unsigned int getMaxUid(void) const = 0;
    virtual unsigned int getMaxSqnr(void) const = 0;
    virtual unsigned int getUidNext(void) const = 0;
    virtual unsigned int getUidValidity(void) const = 0;

    virtual bool getUpdates(bool scan, unsigned int type,
			    PendingUpdates &updates) = 0;

    virtual void updateFlags(void) = 0;
    virtual void expungeMailbox(void) = 0;
    virtual bool selectMailbox(const std::string &name, const std::string &s_in) = 0;
    virtual bool createMailbox(const std::string &s, mode_t mode, uid_t owner = 0, gid_t group = 0, bool root = false) = 0;
    virtual bool deleteMailbox(const std::string &s) = 0;
    virtual void closeMailbox(void) = 0;

    virtual Message *createMessage(const std::string &mbox, time_t idate = 0) = 0;
    virtual bool commitNewMessages(const std::string &mbox) = 0;
    virtual bool rollBackNewMessages(void) = 0;
    virtual bool fastCopy(Message &source, Mailbox &desttype, const std::string &destname) = 0;

    const std::string &getLastError(void) const;
    void setLastError(const std::string &error) const;

    //--
    Mailbox(void);
    virtual ~Mailbox(void);

    friend class Mailbox::iterator;

  protected:
    bool readOnly;

  private:
    Mailbox(const Mailbox &copy);

    mutable std::string lastError;

    std::string name;
  };
}

#endif
