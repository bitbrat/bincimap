/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/mailbox/message.h
 *  
 *  Description:
 *    Declaration of the Message class.
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

#ifndef message_h_included
#define message_h_included
#include <vector>
#include <string>
#include <time.h>

#ifndef UINTMAX
#define UINTMAX ((unsigned int)-1)
#endif

namespace Binc {

  /*!
    \class Message
    \brief The Message class provides an interface for
    IMAP messages.

    Mailbox independent operations and properties are available
    through this interface.

    This class is an abstract, and has no implementation.

    \sa MaildirMessage
  */
  class Message {
  public:

    /*!
      Standard IMAP message flags.

    */
    enum Flags {
      F_NONE = 0x00,        /*!< No flag is set  */
      F_SEEN = 0x01,        /*!< The message has been seen */
      F_ANSWERED = 0x02,    /*!< The message has been answered */
      F_DELETED = 0x04,     /*!< The message is marked as deleted */
      F_DRAFT = 0x08,       /*!< The message is a draft */
      F_RECENT = 0x10,      /*!< The message arrived recently */
      F_FLAGGED = 0x20,     /*!< The message is flagged / important */
      F_EXPUNGED = 0x40,    /*!< The message has been expunged */
    };

    virtual void setUID(unsigned int) = 0;
    virtual unsigned int getUID(void) const = 0;

    virtual void setSize(unsigned int) = 0;
    virtual unsigned int getSize(bool render = false) const = 0;

    virtual void setStdFlag(unsigned char) = 0;
    virtual void resetStdFlags(void) = 0;
    virtual unsigned char getStdFlags(void) const = 0;

    virtual void setFlagsUnchanged(void) = 0;
    virtual bool hasFlagsChanged(void) const = 0;

    virtual void setInternalDate(time_t) = 0;
    virtual time_t getInternalDate(void) const = 0;

    //    virtual void rewind(void) = 0;
    virtual int readChunk(std::string &) = 0;
    virtual bool appendChunk(const std::string &) = 0;
    virtual void close(void) = 0;

    virtual void setExpunged(void) = 0;
    virtual void setUnExpunged(void) = 0;
    virtual bool isExpunged(void) const = 0;

    virtual const std::string &getHeader(const std::string &header) = 0;
    
    virtual bool headerContains(const std::string &header,
				const std::string &text) = 0;

    virtual bool bodyContains(const std::string &text) = 0;
    virtual bool textContains(const std::string &text) = 0;

    virtual bool printBodyStructure(bool extended = true) const = 0;

    virtual bool printEnvelope(void) const = 0;

    virtual bool printHeader(const std::string &section,
			     std::vector<std::string> headers,
			     bool includeHeaders = false,
			     unsigned int startOffset = 0,
			     unsigned int length = UINTMAX,
			     bool mime = false) const = 0;
    virtual unsigned int getHeaderSize(const std::string &section,
				       std::vector<std::string> headers,
				       bool includeHeaders = false,
				       unsigned int startOffset = 0,
				       unsigned int length = UINTMAX,
				       bool mime = false) const = 0;

    virtual bool printBody(const std::string &section,
			   unsigned int startOffset = 0,
			   unsigned int length = UINTMAX) const = 0;
    virtual unsigned int getBodySize(const std::string &section,
				     unsigned int startOffset = 0,
				     unsigned int length = UINTMAX) const = 0;

    virtual bool printDoc(unsigned int startOffset = 0,
			  unsigned int length = UINTMAX,
			  bool onlyText = false) const = 0;
    virtual unsigned int getDocSize(unsigned int startOffset = 0,
				    unsigned int length = UINTMAX,
				    bool onlyText = false) const = 0;
    
    Message(void);
    virtual ~Message(void);

    void setLastError(const std::string &) const;
    const std::string &getLastError(void) const;

  private:
    static std::string lastError;
  };

  inline Message::Message(void)
  {
  }

  inline Message::~Message(void)
  {
  }

  inline void Message::setLastError(const std::string &error) const
  {
    lastError = error;
  }

  inline const std::string &Message::getLastError(void) const
  {
    return lastError;
  }
}

#endif
