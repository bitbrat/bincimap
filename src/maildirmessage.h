/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/maildirmessage.h
 *  
 *  Description:
 *    Declaration of the MaildirMessage class.
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

#ifndef maildirmessage_h_included
#define maildirmessage_h_included
#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iostream>
#include <time.h>

#include <stdio.h>
#include <string.h>

#include "message.h"
#include "address.h"
#include "mime.h"

namespace Binc {

  class Maildir;

  /*!
    \class MaildirMessage
    \brief The MaildirMessage class provides an interface for
    IMAP messages.

    Mailbox independent operations and properties are available
    through this interface.

    \sa Message
  */
  class MaildirMessage : public Message {
  public:
    /*!
      Sets the UID of a message.
      \param uid The UID that will be set.
    */
    void setUID(unsigned int uid);

    /*!
      Returns the UID of a message.
    */
    unsigned int getUID(void) const;

    /*!
      Sets the size of the message. This size must be consistent with
      the size reported when fetching the full message.

      \param size The size of the message in characters, after
      any conversion to CRLF.
    */
    void setSize(unsigned int size);

    /*!
      Returns the size of the message, optionally determining the size
      if it is not yet known.

      \param determine If render is true and the size is unknown, the
      size will be calculated and stored implicitly. Otherwise if the
      size is unknown, 0 is returned.
    */
    unsigned int getSize(bool determine = false) const;

    /*!
      Adds one or more flags to a message.

      \param flags This is a bitmask of flags from the Flags enum.
    */
    void setStdFlag(unsigned char flags);

    /*!
      Resets all flags on a message.
    */
    void resetStdFlags(void);

    /*!
      Returns the flags that are set on a message.
    */
    unsigned char getStdFlags(void) const;

    /*!
      Sets the internal flags.

      \param flags a bitmask of the Flags enum.
     */
    void setInternalFlag(unsigned char flags);

    /*!
      Removes the internal flags.

      \param flags a bitmask of the Flags enum.
    */
    void clearInternalFlag(unsigned char flags);

    /*!
      Returns the internal flags.
    */
    unsigned char getInternalFlags(void) const;

    /*!
      Sets a state in a message that indicates that no flags have been
      changed. Used together with hasFlagsChanged() to check if the
      flags in this message have been changed.
    */
    void setFlagsUnchanged(void);

    /*!
      Returns true if flags have been added or reset since the last
      call to setFlagsUnchanged(), otherwise returns false.
    */
    bool hasFlagsChanged(void) const;

    /*!
      Sets the internal date of a message. This is usually the date in
      which the message arrived in the mailbox.

      \param internaldate The internal date of the message in seconds
      since the epoch.
    */
    void setInternalDate(time_t internaldate);

    /*!
      Returns the internal date of the message in seconds since the
      epoch.
    */
    time_t getInternalDate(void) const;

    /*!
      Reads a chunk of up to 4096 bytes from a message.  Call close()
      before readChunk() to read the first chunk from a message.

      readChunk() is used for copying or appending a message to a
      mailbox.

      \param chunk The characters are stored in this string.
    */
    int readChunk(std::string &chunk);

    /*!
      Appends a chunk of bytes to a message. appendChunk() is used for
      copying or appending a message to a mailbox.

      \param chunk The content of this string is appended to the
      message.
    */
    bool appendChunk(const std::string &chunk);

    /*!
      Resets a message and frees all allocated resources.
    */
    void close(void);

    /*!
      Marks the message as expunged. Equivalent to calling
      setStdFlag() with F_EXPUNGED.
    */
    void setExpunged(void);

    /*!
      Removes the F_EXPUNGED flag from the message.
    */
    void setUnExpunged(void);

    /*!
      Returns true if the message is marked as expunged, otherwise
      returns false.
    */
    bool isExpunged(void) const;

    /*!
      Returns the first occurrance of a MIME header in a message,
      counting from the top of the message and downwards, or "" if no
      such header is found.
      \param header The name of the header to be fetched.
    */
    const std::string &getHeader(const std::string &header);

    bool headerContains(const std::string &header,
			const std::string &text);

    bool bodyContains(const std::string &text);
    bool textContains(const std::string &text);

    bool printBodyStructure(bool extended = true) const;

    bool printEnvelope(void) const;

    bool printHeader(const std::string &section,
		     std::vector<std::string> headers,
		     bool includeHeaders = false,
		     unsigned int startOffset = 0,
		     unsigned int length = UINTMAX,
		     bool mime = false) const;
    unsigned int getHeaderSize(const std::string &section,
			       std::vector<std::string> headers,
			       bool includeHeaders = false,
			       unsigned int startOffset = 0,
			       unsigned int length = UINTMAX,
			       bool mime = false) const;
    
    bool printBody(const std::string &section = "",
		   unsigned int startOffset = 0,
		   unsigned int length = UINTMAX) const;
    unsigned int getBodySize(const std::string &section,
			     unsigned int startOffset = 0,
			     unsigned int length = UINTMAX) const;
    
    bool printDoc(unsigned int startOffset = 0,
		  unsigned int length = UINTMAX,
		  bool onlyText = false) const;
    unsigned int getDocSize(unsigned int startOffset = 0,
			    unsigned int length = UINTMAX,
			    bool onlyText = false) const;

    void setUnique(const std::string &s_in);
    const std::string &getUnique(void) const;

    //--
    MaildirMessage(Maildir &home);
    ~MaildirMessage(void);

    friend class Maildir;

    bool operator < (const MaildirMessage &a) const;

    MaildirMessage(const MaildirMessage &copy);
    MaildirMessage &operator = (const MaildirMessage &copy);

    enum Flags {
      None = 0x00,
      Expunged = 0x01,
      FlagsChanged = 0x02,
      JustArrived = 0x04,
      WasWrittenTo = 0x08,
      Committed = 0x10      
    };

  protected:
    bool parseFull(void) const;
    bool parseHeaders(void) const;

    std::string getFixedFilename(void) const;
    std::string getFileName(void) const;

    void setFile(int fd);
    int getFile(void) const;

    void setSafeName(const std::string &name);
    const std::string &getSafeName(void) const;

  private:
    mutable int fd;
    mutable MimeDocument *doc;
    mutable unsigned char internalFlags;
    mutable unsigned char stdflags;
    mutable unsigned int uid;
    mutable unsigned int size;
    mutable std::string unique;
    mutable std::string safeName;
    time_t internaldate;
    
    Maildir &home;
    static std::string storage;
  };

  //------------------------------------------------------------------------
  class MaildirMessageCache
  {
  public:
    ~MaildirMessageCache();

    enum ParseStatus {
      NotParsed,
      HeaderParsed,
      AllParsed
    };

    static MaildirMessageCache &getInstance(void);

    void removeStatus(const MaildirMessage *);
    void addStatus(const MaildirMessage *, ParseStatus pstat);
    ParseStatus getStatus(const MaildirMessage *) const;
    void clear(void);

  private:
    MaildirMessageCache();

    mutable std::map<const MaildirMessage *, ParseStatus> statuses;
    mutable std::deque<const MaildirMessage *> parsed;
  };
}

#endif
