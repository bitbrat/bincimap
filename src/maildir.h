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

    /*! 
      \class Maildir::iterator
     
      \brief The iterator class provides a special Mailbox iterator
      for the Maildir mailbox type.

      It is a proxy iterator; the real iterator is hidden inside. This
      allows you to use the iterator with polymorphic access, so this
      works:

      Mailbox::iterator i = maildir->begin();
      while (i != maildir->end()) {
        ++i;
      }

      The messages are traversed in ascending order of UID.
    */
    class iterator : public BaseIterator {
    public:
      /*!
	Constructs an invalid iterator.
      */
      iterator(void);

      /*!
	Constructs an iterator.

	\param home The Maildir to iterate over.
	\param i The MessageMap iterator to encapsulate.
	\param mod Flags. See also Mailbox.
      */
      iterator(Maildir *home, MessageMap::iterator i,
	       const SequenceSet &bset,
	       unsigned int mod = INCLUDE_EXPUNGED | SQNR_MODE);

      /*!
	Constructs a copy of an iterator.

	\param copy The new iterator becomes the copy of this
	iterator.
      */
      iterator(const iterator &copy);

      /*!
	Destructs an iterator.
      */
      ~iterator(void);

      /*!
	Returns the reference to the current MaildirMessage as a
	Message reference. If the iterator is invalid, the message
	reference will also be invalid so make sure to check for
	begin() and end().
      */
      Message &operator *(void);

      /*!
	Advances the iterator one step.
      */
      void operator ++(void);

      /*!
	Returns true if this iterator is equal to the one passed as
	argument; meaning that they both point to the same message or
	both to the end iterator, otherwise returns false.
      */
      bool operator ==(const BaseIterator &i) const;

      /*!
	Returns true if this iterator is not equal to the one passed
	as argument; meaning that they both point to the same message
	or both to the end iterator, otherwise returns false.
      */
      bool operator !=(const BaseIterator &i) const;

      /*!
	Makes this iterator a copy of the iterator passed as argument.
	This is an assignment iterator:

	iterator i, j;
	iterator i = j; // assignment
      */
      iterator &operator =(const iterator &copy);

      /*!
	Removes the message that this iterator points to from the
	MessageMap.  Advances the iterator one step.
      */
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

    /*!
      Returns the type name of this Mailbox. Always returns "Maildir".
    */ 
    const std::string getTypeName(void) const;

    /*!
      Returns an iterator that points to the first message in this
      Maildir.  If there are no messages, returns the invalid position
      equal to end().

      \param mod The flags.
    */
    Mailbox::iterator begin(const SequenceSet &bset, 
			    unsigned int mod = INCLUDE_EXPUNGED | SQNR_MODE) const;

    /*!
      Returns an iterator that points to the invalid position after
      the last (if any) message in the Maildir.
    */
    Mailbox::iterator end(void) const;

    /*!
      Returns the maximum UID value of all messages in this Maildir.
    */
    unsigned int getMaxUid(void) const;

    /*!
      Returns the maximum sequence number of all messages in this
      Maildir.  This is equal to the number of messages.
    */
    unsigned int getMaxSqnr(void) const;

    /*!
      Returns the Maildir's UIDVALIDITY.
    */
    unsigned int getUidValidity(void) const;

    /*!
      Returns the next UID that will be assigned by this Maildir
      (UIDNEXT).
    */
    unsigned int getUidNext(void) const;

    /*!
      Returns certain changes to the Maildir since the last time a
      similar check was done. Returns true if the check succeeed;
      otherwise returns false.
      
      \param doscan If true, the depository will be scanned for
      changes. Otherwise the cached information is checked.
      \param type A bitwise mask of the types of changes to check
      for. See also PendingUpdates.
      \param updates A reference to a structure where the changes are
      stored.
    */
    bool getUpdates(bool doscan, unsigned int type,
		    PendingUpdates &updates);

    /*!
      Returns the file system path to the Maildir.
    */
    const std::string &getPath(void) const;

    /*!
      Sets the file system path to the Maildir.
    */
    void setPath(const std::string &path_in);

    /*!
      Advances UIDVALIDITY. Resets the internal cache and drops the
      UID values of the Maildir, forcing the server to re-delegate
      UIDs the next time the mailbox is selected.

      \param path The path of the Maildir whose UIDVALIDITY is to be
      bumped.
    */
    void bumpUidValidity(const std::string &path) const;

    /*!
      Returns a unique ID that represents the state of the mailbox.
      If a change is made to the mailbox, this status ID must also
      change unconditionally. This value should be very quick to
      calculate, and scanning the mailbox should be avoided.

      \param path The path to the Maildir whose status ID is returned.
    */
    unsigned int getStatusID(const std::string &path) const;

    /*!
      Fetches the status of a Maildir at a certain path, such as the
      number of messages, the UIDVALIDITY value, the UIDNEXT value,
      the number of recent and unseen messages.

      \param path The path to the Maildir whose status is fetched
      \param status A Status structure that is fed with the status
      of the mailbox.
    */
    bool getStatus(const std::string &path, Status &status) const;

    /*!
      Commits the in-memory message flags to the mailbox.
    */
    void updateFlags(void);

    /*!
      Returns true if a directory entry qualifies as a Maildir;
      otherwise returns false. The criteria are that the entry is a
      directory, and that it contains the subdirectories cur, new and
      tmp.

      \param path The path to the directory that is evaluated.
    */
    bool isMailbox(const std::string &path) const;

    /*!
      Returns true if a Maildir directory entry contains recent
      messages; otherwise returns false.

      \param path The path to the directory that is evaluated.
    */
    bool isMarked(const std::string &) const;

    /*!
      Selects a mailbox. Returns true if the select was successful;
      otherwise returns false.

      \param name The IMAP name of the mailbox.
      \param path The file system path to the mailbox.
    */
    bool selectMailbox(const std::string &name, const std::string &path);

    /*!
      Closes any selected mailbox.
    */
    void closeMailbox(void);

    /*!
      Expunges all messages from the mailbox that have the \Deleted
      flag set.
    */
    void expungeMailbox(void);

    /*!
      Creates a Maildir. Returns true if the create succeeded;
      otherwise returns false.

      \param path The file system path to the new Maildir.
      \param mode The file system mode to use when creating the mailbox.
      \param owner The unix user that will own the mailbox.
      \param group The unix group that will own the mailbox.
      \param root If true, the Maildir at "." is created.
    */
    bool createMailbox(const std::string &path, mode_t mode, uid_t owner = 0, gid_t group = 0, bool root = false);

    /*!
      Deletes a Maildir. Removes all messages and returns true on
      success. If false is returned, the state of the Maildir is
      undefined.

      \param path The path to the Maildir to be deleted.
    */
    bool deleteMailbox(const std::string &path);

    /*!
      Creates a message in a mailbox, with a given internal date.
      Returns a pointer to a MaildirMessage which is still open.  Data
      is then written to the message with Message::appendChunk, and
      Message::close is then called. When done with creating messages,
      commitNewMessages() will move the message into new/ and then
      cur/.

      \param path The path to the Maildir in which the message is
      created.
      \param idate The internal date of the message, in UNIX local
      time.
    */
    Message *createMessage(const std::string &path, time_t idate = 0);

    /*!
      Commits all newly created messages in a Maildir. These messages
      recide in tmp/, and after this function has been called, they
      will have shortly visited new/ before being moved to
      cur/. Returns true if the commit succeeded; otherwise returns
      false.

      \param path The path to the Maildir in which new messages should
      be committed.
    */
    bool commitNewMessages(const std::string &path);

    /*!
      Removes all newly created messages that have not been
      committed. If the rollback failed for any reason, false is
      returned. Otherwise returns true.
    */
    bool rollBackNewMessages(void);
    
    /*!
      Attempts to copy a message to a mailbox using a method that is
      faster that copying bytes. Returns true on success, otherwise
      returns false.
    */
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
