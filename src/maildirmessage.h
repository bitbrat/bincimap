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

    /*!
      Returns true if the message has a certain header that contains a
      certain text; otherwise returns false. Used when searching
      messages.

      \param header The name of the header to search in.
      \param text The text to search for.
    */
    bool headerContains(const std::string &header,
			const std::string &text);

    /*!
      Returns true if the body of the message contains a certain text;
      otherwise returns false;

      \param text The text to search for.
    */
    bool bodyContains(const std::string &text);

    /*!
      Returns true if the header or body of the message contains a
      certain text; otherwise returns false.

      \param text The text to search for.
    */
    bool textContains(const std::string &text);

    /*!
      Prints the IMAP body structure of the message to the client.

      \param extended If true, the extended body structure is also
      printed.
    */
    bool printBodyStructure(bool extended = true) const;

    /*!
      Prints the IMAP envelope of the message to the client.
    */
    bool printEnvelope(void) const;

    /*!
      Prints headers of the message.

      \param section If empty, the root level mime part's headers are
      printed; otherwise this string is interpreted as a MIME part,
      and the headers of that part are printed.
      \param headers A list of headers to include or exclude,
      depending on the value of includeHeaders.
      \param includeHeaders If true, only the headers in the "headers"
      argument are printed; otherwise any headers but those in the
      "headers" argument are printed.
      \param startOffset Describes at what character offset the
      printing of headers should start.
      \param length Limits the number of characters to print.
      \param mime If true, the headers of the encapsulating part of an
      enclosed rfc822 message is printed; otherwise the enclosed
      message's headers are printed.
    */
    bool printHeader(const std::string &section,
		     std::vector<std::string> headers,
		     bool includeHeaders = false,
		     unsigned int startOffset = 0,
		     unsigned int length = UINTMAX,
		     bool mime = false) const;

    /*!
      Determines the number of bytes that will be printed by
      printHeader().  All arguments are the same as with
      printHeader().
    */
    unsigned int getHeaderSize(const std::string &section,
			       std::vector<std::string> headers,
			       bool includeHeaders = false,
			       unsigned int startOffset = 0,
			       unsigned int length = UINTMAX,
			       bool mime = false) const;
    
    /*!
      Prints body parts of a message.

      \param section If empty, the root level mime part's headers are
      printed; otherwise this string is interpreted as a MIME part,
      and the headers of that part are printed.
      \param startOffset Describes at what character offset the
      printing should start.
      \param length Limits the number of characters to print.
    */
    bool printBody(const std::string &section = "",
		   unsigned int startOffset = 0,
		   unsigned int length = UINTMAX) const;

    /*!
      Determines the number of bytes that will be printed by
      printBody().  All arguments are the same as with
      printBody().
    */
    unsigned int getBodySize(const std::string &section,
			     unsigned int startOffset = 0,
			     unsigned int length = UINTMAX) const;
    
    /*!
      Prints the whole message raw.

      \param startOffset Describes at what character offset the
      printing should start.
      \param length Limits the number of characters to print.
    */
    bool printDoc(unsigned int startOffset = 0,
		  unsigned int length = UINTMAX,
		  bool onlyText = false) const;

    /*!
      Determines the number of bytes that will be printed by
      printDoc().  All arguments are the same as with
      printDoc().
    */
    unsigned int getDocSize(unsigned int startOffset = 0,
			    unsigned int length = UINTMAX,
			    bool onlyText = false) const;

    /*!
      Sets the unique part of this Maildir message's file name.

      \param s_in The unique part of the message's file name.
    */
    void setUnique(const std::string &s_in);

    /*!
      Returns the unique part of this Maildir message's file name.
    */
    const std::string &getUnique(void) const;

    /*!
      Sets the temporary file name of this message. Used when creating
      a bundle of new message, when it's practical to remember the
      safe names so that it's easier to clean up afterwards.
    */
    void setSafeName(const std::string &name);

    /*!
      Returns the safe name of this message.
    */
    const std::string &getSafeName(void) const;

    /*!
      Constructs a MaildirMessage.

      \param home The Maildir that owns this message.
    */
    MaildirMessage(Maildir &home);

    /*!
      Constructs a copy of a MaildirMessage.

      \param copy The new MaildirMessage will become a copy of this
      MaildirMessage.
    */
    MaildirMessage(const MaildirMessage &copy);

    /*!
      Make this MaildirMessage a copy of another MaildirMessage.

      \param copy The original MaildirMessage will become a copy of
      this MaildirMessage.
    */
    MaildirMessage &operator = (const MaildirMessage &copy);

    /*!
      Destructs this MaildirMessage.
    */
    ~MaildirMessage(void);

    friend class Maildir;

    /*!
      This operator is used to sort messages in a Maildir by their UID
      value.
    */
    bool operator < (const MaildirMessage &a) const;

    enum Flags {
      None = 0x00,
      Expunged = 0x01,
      FlagsChanged = 0x02,
      JustArrived = 0x04,
      WasWrittenTo = 0x08
    };

  protected:
    /*!
      Triggers a parse of the whole MIME message unless it has already
      been parsed.
     */
    bool parseFull(void) const;

    /*!
      Triggers a parse of only the first headers of the MIME message,
      unless this message has already been parsed.
    */
    bool parseHeaders(void) const;

    /*!
      Returns the last known filename for this message.
    */
    std::string getFileName(void) const;

    /*!
      Sets the file descriptor of this Message.

      \param fd The file descriptor.
    */
    void setFile(int fd);

    /*!
      Returns the file descriptor of this message; implicitly opening
      it if it's not already open.
    */
    int getFile(void) const;

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

  /*!
    \class MaildirMessageCache
    \brief The MaildirMessageCache class provides a place to
    store the status of a MaildirMessage.

    Currently, the status that can be stored is whether the file is
    unparsed, has only its headers parsed or whether the whole message
    has been parsed.

    When the cache is full, the oldest MaildirMessage is removed. If
    this message is open, it is closed.

    \sa MaildirMessage
  */

  class MaildirMessageCache
  {
  public:
    ~MaildirMessageCache();

    enum ParseStatus {
      NotParsed,
      HeaderParsed,
      AllParsed
    };

    /*!
      Returns a reference to the MaildirMessageCache singleton.
    */
    static MaildirMessageCache &getInstance(void);

    /*!
      Adds a MaildirMessage reference to the cache, with a certain status.
      If the reference already exists in the cache, its status is updated.

      \param message A point to the MaildirMessage whose status is stored.
      \param status The status of the message.
    */
    void addStatus(const MaildirMessage *message, ParseStatus status);

    /*!
      Gets the status of a MaildirMessage.

      \param message A point to the MaildirMessage whose status is fetched.
    */
    ParseStatus getStatus(const MaildirMessage *message) const;
    
    /*!
      Removes the status of a MaildirMessage from the cache, and
      closes the message if it's open.

      \param message A point to the MaildirMessage whose status is removed.    
    */
    void removeStatus(const MaildirMessage *message);

    /*!
      Remove the status of all messages in the cache, closing them if
      they're open.
    */
    void clear(void);

  private:
    /*!
      Constructs a MaildirMessageCache. There must always only be one
      cache, therefore this object is a singleton and must be accessed
      via getInstance().
    */
    MaildirMessageCache();

    mutable std::map<const MaildirMessage *, ParseStatus> statuses;
    mutable std::deque<const MaildirMessage *> parsed;
  };
}

#endif
