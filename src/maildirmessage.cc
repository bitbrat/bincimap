/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildirmessage.cc
 *  
 *  Description:
 *    Implementation of the MaildirMessage class.
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

#include <stack>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <utime.h>

#include "maildir.h"
#include "maildirmessage.h"
#include "convert.h"
#include "mime.h"
#include "io.h"
#include "mime-utils.h"

using namespace ::std;
using namespace Binc;

string Message::lastError;
string MaildirMessage::storage;

namespace {
  //----------------------------------------------------------------------
  void printOneHeader(IO &io, const MimePart *message, const string &s_in,
		      bool removecomments = true)
  {
    string tmp = "";
    HeaderItem hitem;

    if (message->h.getFirstHeader(s_in, hitem)) {
      tmp = hitem.getValue();
      io << toImapString(unfold(tmp, removecomments));
    } else
      io << "NIL";
  }

  //----------------------------------------------------------------------
  void printOneAddressList(IO &io, const MimePart *message,
			   const string &s_in, bool removecomments = true)
  {
    string tmp = "";
    HeaderItem hitem;

    if (message->h.getFirstHeader(s_in, hitem)) {
      tmp = hitem.getValue();
      vector<string> addr;
      splitAddr(unfold(tmp, removecomments), addr);
      if (addr.size() != 0) {
	io << "(";
	for (vector<string>::const_iterator i = addr.begin();
	     i != addr.end(); ++i)
	  io << Address(*i).toParenList();
	io << ")";
      } else io << "NIL";
    } else
      io << "NIL";
  }

  //----------------------------------------------------------------------
  void envelope(IO &io, const MimePart *message)
  {
    HeaderItem hitem;
    io << "(";
    printOneHeader(io, message, "date");
    io << " ";
    printOneHeader(io, message, "subject", false);
    io << " ";
    printOneAddressList(io, message, "from", false);
    io << " ";
    printOneAddressList(io, message, 
			message->h.getFirstHeader("sender", hitem) 
			? "sender" : "from", false);
    io << " ";
    printOneAddressList(io, message, 
			message->h.getFirstHeader("reply-to", hitem) 
			? "reply-to" : "from", false);
    io << " ";
    printOneAddressList(io, message, "to", false);
    io << " ";
    printOneAddressList(io, message, "cc", false);
    io << " ";
    printOneAddressList(io, message, "bcc", false);
    io << " ";
    printOneHeader(io, message, "in-reply-to");
    io << " ";
    printOneHeader(io, message, "message-id");
    io << ")";
  }

  //----------------------------------------------------------------------
  void bodyStructure(IO &io, const MimePart *message, bool extended = true)
  {
    HeaderItem hitem;
    if (message->isMultipart() && message->members.size() > 0) {
      io << "(";
      
      for (vector<MimePart>::const_iterator i = message->members.begin();
	   i != message->members.end(); ++i)
	bodyStructure(io, &(*i));

      io << " ";
      io << toImapString(message->getSubType());
      io << " ";

      vector<string> parameters;
      vector<string> headers;
      string tmp;

      string type, subtype;

      tmp = "";
      if (message->h.getFirstHeader("content-type", hitem)) {
	tmp = unfold(hitem.getValue());
	trim(tmp);

	vector<string> v;
	split(tmp, ";", v);
	
	for (vector<string>::const_iterator i = v.begin(); i != v.end(); ++i) {
	  string element = *i;
	  trim(element);
	  if (element.find('=') != string::npos) {
	    string::size_type pos = element.find('=');
	    string s = element.substr(0, pos);
	    string t = element.substr(pos + 1);
	    trim(s, " \"");
	    trim(t, " \"");
	    parameters.push_back(s);
	    parameters.push_back(t);
	  }
	}

	if (parameters.size() != 0) {
	  io << "(";
	  for (vector<string>::const_iterator i = parameters.begin();
	       i != parameters.end(); ++i) {
	    if (i != parameters.begin())
	      io << " ";
	    io << toImapString(*i);
	  }
	  io << ")";
	} else
	  io << "NIL";
      } else
	io << "NIL";

      // CONTENT-DISPOSITION
      io << " ";
      tmp = "";
      if (message->h.getFirstHeader("content-disposition", hitem)) {
	tmp = hitem.getValue();
	trim(tmp);
	
	vector<string> v;
	split(tmp, ";", v);
	if (v.size() > 0) {
	  string disp = v[0];
	  trim(disp);
	  io << "(" << toImapString(disp);
	  io << " ";
	  if (v.size() > 1) {
	    io << "(";
	    vector<string>::const_iterator i = v.begin();
	    ++i;
	    bool wrote = false;
	    while (i != v.end()) {
	      string s = *i;
	      trim(s);
	      
	      string::size_type pos = s.find('=');
	      string key = s.substr(0, pos);
	      string value = s.substr(pos + 1);
	      trim(key);
	      trim(value);
	      
	      trim(key, " \"");
	      trim(value, " \"");
	      
	      if (!wrote) wrote = true;
	      else io << " ";
	      io << toImapString(key);
	      
	      io << " ";
	      io << toImapString(value);
	      
	      ++i;
	    }
	    io << ")";
	  } else 
	    io << "NIL";
	  io << ")";
	} else
	  io << "NIL";
      } else
	io << "NIL";
      
      // CONTENT-LANGUAGE
      io << " ";
      printOneHeader(io, message, "content-language");

      io << ")";
    } else {
      io << "(";

      vector<string> parameters;
      vector<string> headers;
      string tmp;
      tmp = "";
      string type, subtype;

      tmp = "";
      if (message->h.getFirstHeader("content-type", hitem)) {
	tmp = unfold(hitem.getValue());
	
	vector<string> v;
	split(tmp, ";", v);

	if (v.size() > 0) {
	  vector<string> b;
	  split(v[0], "/", b);
	    
	  if (b.size() > 0)
	    type = b[0];
	  else
	    type = "text";

	  if (b.size() > 1)
	    subtype = b[1];
	  else
	    subtype = "plain";
	}
	
	for (vector<string>::const_iterator i = v.begin(); i != v.end(); ++i) {
	  if (i == v.begin()) continue;

	  string element = *i;
	  trim(element);

	  if (element.find('=') != string::npos) {
	    string::size_type pos = element.find('=');
	    string s = element.substr(0, pos);
	    string t = element.substr(pos + 1);
	    trim(s, " \"");
	    trim(t, " \"");
	    parameters.push_back(s);
	    parameters.push_back(t);
	  }
	}
      } else {
	type = "text";
	subtype = "plain";
      }

      io << toImapString(type);
      io << " ";
      io << toImapString(subtype);

      io << " ";
      if (parameters.size() != 0) {
	io << "(";
	for (vector<string>::const_iterator i = parameters.begin();
	     i != parameters.end(); ++i) {
	  if (i != parameters.begin())
	    io << " ";
	  io << toImapString(*i);
	}
	io << ")";
      } else
	io << "NIL";
      
      // CONTENT-ID
      io << " ";
      printOneHeader(io, message, "content-id");

      // CONTENT-DESCRIPTION
      io << " ";
      printOneHeader(io, message, "content-description");

      // CONTENT-TRANSFER-ENCODING
      io << " ";
      tmp = "";
      if (message->h.getFirstHeader("content-transfer-encoding", hitem)) {
	tmp = hitem.getValue();
	trim(tmp);
	io << toImapString(tmp);
      } else
	io << "\"7bit\"";
      io << " ";

      // Size of body in octets
      io << message->getBodyLength();

      lowercase(type);
      if (type == "text") {
	io << " ";
	io << message->getNofBodyLines();
      } else if (message->isMessageRFC822()) {
	io << " ";
	envelope(io, &message->members[0]);
	io << " ";
	bodyStructure(io, &message->members[0]);
	io << " ";
	io << message->getNofBodyLines();
      }
      
      // Extension data follows

      if (extended) {

	// CONTENT-MD5
	io << " ";
	printOneHeader(io, message, "content-md5");

	// CONTENT-DISPOSITION
	io << " ";
	tmp = "";
	if (message->h.getFirstHeader("content-disposition", hitem)) {
	  tmp = hitem.getValue();
	  trim(tmp);

	  vector<string> v;
	  split(tmp, ";", v);
	  if (v.size() > 0) {
	    string disp = v[0];
	    trim(disp);
	    io << "(" << toImapString(disp);
	    io << " ";
	    if (v.size() > 1) {
	      io << "(";
	      vector<string>::const_iterator i = v.begin();
	      ++i;
	      bool wrote = false;
	      while (i != v.end()) {
		string s = *i;
		trim(s);

		string::size_type pos = s.find('=');
		string key = s.substr(0, pos);
		string value = s.substr(pos + 1);
		trim(key);
		trim(value);

		trim(key, " \"");
		trim(value, " \"");

		if (!wrote) wrote = true;
		else io << " ";
		io << toImapString(key);

		io << " ";
		io << toImapString(value);

		++i;
	      }
	      io << ")";
	    } else 
	      io << "NIL";
	    io << ")";
	  } else
	    io << "NIL";
	} else
	  io << "NIL";
      
	// CONTENT-LANGUAGE
	io << " ";
	printOneHeader(io, message, "content-language");

	// CONTENT-LOCATION
	io << " ";
	printOneHeader(io, message, "content-location");
      }

      io << ")";
    }
  }
}

//------------------------------------------------------------------------
MaildirMessage::MaildirMessage(Maildir &hom) 
  : fd(-1), doc(0), internalFlags(None), stdflags(F_NONE),
    uid(0), size(0), unique(""), safeName(""), internaldate(0),
    home(hom)
{
}

//------------------------------------------------------------------------
MaildirMessage::MaildirMessage(const MaildirMessage &copy) 
  : fd(copy.fd), doc(copy.doc), internalFlags(copy.internalFlags),
    stdflags(copy.stdflags), uid(copy.uid), size(copy.size),
    unique(copy.unique), safeName(copy.safeName),
    internaldate(copy.internaldate), home(copy.home)
{
}

//------------------------------------------------------------------------
MaildirMessage::~MaildirMessage(void)
{
}

//------------------------------------------------------------------------
MaildirMessage &MaildirMessage::operator =(const MaildirMessage &copy) 
{
  fd = copy.fd;
  doc = copy.doc; 
  internalFlags = copy.internalFlags;
  stdflags = copy.stdflags;
  uid = copy.uid;
  size = copy.size;
  unique = copy.unique; 
  safeName = copy.safeName;
  internaldate = copy.internaldate;
  home = copy.home;

  return *this;
}

//------------------------------------------------------------------------
bool MaildirMessage::operator <(const MaildirMessage &a) const
{
  return uid < a.uid;
}
//------------------------------------------------------------------------
void MaildirMessage::close(void)
{
  if (fd != -1) {
    if ((internalFlags & WasWrittenTo) && fsync(fd) != 0
	&& errno != EINVAL && errno != EROFS) {
      // FIXME: report this error
    }

    if (::close(fd) != 0) {
      // FIXME: report this error
    }

    fd = -1;
  }

  // The file will not be moved from tmp/ before this function has
  // finished. So it's safe to assume that safeName is still valid.
  if (internalFlags & WasWrittenTo) {
    if (internaldate != 0) {
      struct utimbuf tim = {internaldate, internaldate};
      utime(safeName.c_str(), &tim);
    } else {
      time_t t = time(0);
      struct utimbuf tim = {t, t};
      utime(safeName.c_str(), &tim);
    }

    internalFlags &= ~WasWrittenTo;
  }


  if (doc) {
    doc->clear();
    delete doc;
    doc = 0;
  }
}

//------------------------------------------------------------------------
void MaildirMessage::setExpunged(void)
{
  internalFlags |= Expunged;
}

//------------------------------------------------------------------------
void MaildirMessage::setUnExpunged(void)
{
  internalFlags &= ~Expunged;
}

//------------------------------------------------------------------------
void MaildirMessage::setFlagsUnchanged(void)
{
  internalFlags &= ~FlagsChanged;
}

//------------------------------------------------------------------------
bool MaildirMessage::hasFlagsChanged(void) const
{
  return (internalFlags & FlagsChanged) != 0;
}

//------------------------------------------------------------------------
unsigned char MaildirMessage::getStdFlags(void) const
{
  return stdflags;
}

//------------------------------------------------------------------------
bool MaildirMessage::isExpunged(void) const
{
  return (internalFlags & Expunged) != 0;
}

//------------------------------------------------------------------------
unsigned int MaildirMessage::getUID(void) const
{
  return uid;
}

//------------------------------------------------------------------------
unsigned int MaildirMessage::getSize(bool render) const
{
  if (size == 0 && render) {
    size = getDocSize();
    home.mailboxchanged = true;
  }

  return size;
}

//------------------------------------------------------------------------
const string &MaildirMessage::getUnique(void) const
{
  return unique;
}

//------------------------------------------------------------------------
time_t MaildirMessage::getInternalDate(void) const
{
  return internaldate;
}

//------------------------------------------------------------------------
void MaildirMessage::setInternalDate(time_t t)
{
  internaldate = t;
}

//------------------------------------------------------------------------
void MaildirMessage::setStdFlag(unsigned char f_in)
{
  internalFlags |= FlagsChanged;
  stdflags |= f_in;
}

//------------------------------------------------------------------------
void MaildirMessage::resetStdFlags(void)
{
  internalFlags |= FlagsChanged;
  stdflags = F_NONE;
}

//------------------------------------------------------------------------
void MaildirMessage::setUID(unsigned int i_in)
{
  uid = i_in;
}

//------------------------------------------------------------------------
void MaildirMessage::setSize(unsigned int i_in)
{
  size = i_in;
}

//------------------------------------------------------------------------
void MaildirMessage::setUnique(const string &s_in)
{
  unique = s_in;
}

//------------------------------------------------------------------------
int MaildirMessage::getFile(void) const
{
  if (fd != -1)
    return fd;

  const string &id = getUnique();
  MaildirIndexItem *item = home.index.find(id);
  if (item) {
    string fpath = home.path + "/cur/" + item->fileName;
    
    unsigned int oflags = O_RDONLY;
#ifdef HAVE_OLARGEFILE
    oflags |= O_LARGEFILE;
#endif
    while ((fd = open(fpath.c_str(), oflags)) == -1) {
      if (errno != ENOENT) {
	IO &logger = IOFactory::getInstance().get(2);
	logger << "unable to open " << fpath << ": "
	       << strerror(errno) << endl;
	return -1;
      }
      
      home.scanFileNames();
      if ((item = home.index.find(id)) == 0)
	break;
      else
	fpath = home.path + "/cur/" + item->fileName;
    }

    MaildirMessageCache &cache = MaildirMessageCache::getInstance();
    cache.addStatus(this, MaildirMessageCache::NotParsed);

    return fd;
  }

  return -1;
}

//------------------------------------------------------------------------
void MaildirMessage::setFile(int fd)
{
  this->fd = fd;
}

//------------------------------------------------------------------------
void MaildirMessage::setSafeName(const string &name)
{
  safeName = name;
}

//------------------------------------------------------------------------
const string &MaildirMessage::getSafeName(void) const
{
  return safeName;
}

//------------------------------------------------------------------------
string MaildirMessage::getFileName(void) const
{
  MaildirIndexItem *item = home.index.find(getUnique());
  if (!item) {
    home.scanFileNames();

    if ((item = home.index.find(getUnique())) == 0)
      return "";
  }

  return item->fileName;
}

//------------------------------------------------------------------------
int MaildirMessage::readChunk(string &chunk)
{
  if (fd == -1) {
    if ((fd == getFile()) == -1)
      return -1;
  }


  char buffer[1024];
  ssize_t readBytes = read(fd, buffer, (size_t) sizeof(buffer));
  if (readBytes == -1) {
    setLastError("Error reading from " + getFileName()
		 + ": " + string(strerror(errno)));
    return -1;
  }

  chunk = string(buffer, readBytes);
  return readBytes;
}

//------------------------------------------------------------------------
bool MaildirMessage::appendChunk(const string &chunk)
{
  if (fd == -1) {
    setLastError("Error writing to " + getSafeName() 
		 + ": File is not open");
    return false;
  }

  internalFlags |= WasWrittenTo;

  string out;
  for (string::const_iterator i = chunk.begin(); i != chunk.end(); ++i) {
    const char c = *i;
    if (c != '\r')
      out += c;
  }

  ssize_t wroteBytes = 0;
  for (;;) {
    wroteBytes = write(fd, out.c_str(), (size_t) out.length());
    if (wroteBytes == -1) {
      if (errno == EINTR)
	continue;
      wroteBytes = 0;
    }

    break;
  }

  if (wroteBytes == (ssize_t) out.length())
    return true;

  setLastError("Error writing to " + getSafeName() 
	       + ": " + string(strerror(errno)));
  return false;
}

//------------------------------------------------------------------------
bool MaildirMessage::parseFull(void) const
{
  MaildirMessageCache &cache = MaildirMessageCache::getInstance();
  MaildirMessageCache::ParseStatus ps = cache.getStatus(this);
  if (ps == MaildirMessageCache::AllParsed && doc)
    return true;

  int fd = getFile();
  if (fd == -1)
    return false;

  // FIXME: parse errors
  if (!doc)
    doc = new MimeDocument;
  doc->parseFull(fd);

  cache.addStatus(this, MaildirMessageCache::AllParsed);

  return true;
}

//------------------------------------------------------------------------
bool MaildirMessage::parseHeaders(void) const
{
  MaildirMessageCache &cache = MaildirMessageCache::getInstance();
  MaildirMessageCache::ParseStatus ps = cache.getStatus(this);
  if ((ps == MaildirMessageCache::AllParsed
      || ps == MaildirMessageCache::HeaderParsed) && doc)
    return true;

  int fd = getFile();
  if (fd == -1)
    return false;

  // FIXME: parse errors
  if (!doc)
    doc = new MimeDocument;
  doc->parseOnlyHeader(fd);

  cache.addStatus(this, MaildirMessageCache::HeaderParsed);

  return true;
}

//------------------------------------------------------------------------
bool MaildirMessage::printBodyStructure(bool extended) const
{
  if (!parseFull())
    return false;

  IO &com = IOFactory::getInstance().get(1);
  bodyStructure(com, doc, extended);
  return true;
}

//------------------------------------------------------------------------
bool MaildirMessage::printEnvelope(void) const
{
  if (!parseFull())
    return false;

  IO &com = IOFactory::getInstance().get(1);
  envelope(com, doc);
  return true;
}

//------------------------------------------------------------------------
bool MaildirMessage::printHeader(const std::string &section,
				 std::vector<std::string> headers,
				 bool includeHeaders,
				 unsigned int startOffset,
				 unsigned int length,
				 bool mime) const
{
  IO &com = IOFactory::getInstance().get(1);
  com << storage;
  storage = "";
  return true;
}

//------------------------------------------------------------------------
unsigned int MaildirMessage::getHeaderSize(const std::string &section,
					   std::vector<std::string> headers,
					   bool includeHeaders,
					   unsigned int startOffset,
					   unsigned int length,
					   bool mime) const
{
  IO &com = IOFactory::getInstance().get(1);

  if (section == "") {
    if (!parseHeaders())
      return 0;
  } else if (!parseFull())
    return 0;

  const MimePart *part = doc->getPart(section, "", mime ? MimePart::FetchMime : MimePart::FetchHeader);
  if (!part) {
    storage = "";
    return 0;
  }
  
  int fd = getFile();
  if (fd == -1)
    return 0;

  storage = "";
  part->printHeader(fd, com, headers, 
		    includeHeaders, startOffset,
		    length, storage);

  return storage.size();
}

//------------------------------------------------------------------------
bool MaildirMessage::printBody(const std::string &section,
			       unsigned int startOffset,
			       unsigned int length) const
{
  IO &com = IOFactory::getInstance().get(1);
  if (!parseFull())
    return false;

  const MimePart *part = doc->getPart(section, "");
  if (!part) {
    storage = "";
    return 0;
  }
  
  int fd = getFile();
  if (fd == -1)
    return false;

  storage = "";
  part->printBody(fd, com, startOffset, length);
  return true;
}

//------------------------------------------------------------------------
unsigned int MaildirMessage::getBodySize(const std::string &section,
					 unsigned int startOffset,
					 unsigned int length) const
{
  if (!parseFull())
    return false;

  const MimePart *part = doc->getPart(section, "");
  if (!part) {
    storage = "";
    return 0;
  }

  if (startOffset > part->bodylength)
    return 0;
  
  unsigned int s = part->bodylength - startOffset;
  return s < length ? s : length;
}

//------------------------------------------------------------------------
bool MaildirMessage::printDoc(unsigned int startOffset, 
			      unsigned int length, bool onlyText) const
{
  IO &com = IOFactory::getInstance().get(1);
  if (!parseFull())
    return false;

  int fd = getFile();
  if (fd == -1)
    return false;

  if (onlyText)
    startOffset += doc->bodystartoffsetcrlf;

  storage = "";
  doc->printDoc(fd, com, startOffset, length);
  return true;
}

//------------------------------------------------------------------------
unsigned int MaildirMessage::getDocSize(unsigned int startOffset,
					unsigned int length, 
					bool onlyText) const
{
  if (!parseFull())
    return false;

  unsigned int s = doc->size;
  if (onlyText) s -= doc->bodystartoffsetcrlf;

  if (startOffset > s)
    return 0;

  s -= startOffset;
  return s < length ? s : length;
}

//------------------------------------------------------------------------
bool MaildirMessage::headerContains(const std::string &header,
				    const std::string &text)
{
  if (!parseHeaders())
    return false;

  HeaderItem hitem;
  if (!doc->h.getFirstHeader(header, hitem))
    return false;

  string tmp = hitem.getValue();
  uppercase(tmp);
  string tmp2 = text;
  uppercase(tmp2);
  return (tmp.find(tmp2) != string::npos);
}

//------------------------------------------------------------------------
bool MaildirMessage::bodyContains(const std::string &text)
{
  if (!parseFull())
    return false;

  // search the body part of the message..
  int fd = getFile();
  if (fd == -1)
    return false;

  crlffile = fd;
  crlfReset();

  char c;
  for (unsigned int i = 0; i < doc->getBodyStartOffset(); ++i)
    if (!crlfGetChar(c))
      break;
  
  char *ring = new char[text.length()];
  int pos = 0;
  int length = doc->getBodyLength();
  while (crlfGetChar(c) && length--) {
    ring[pos % text.length()] = toupper(c);
    
    if (compareStringToQueue(text, ring, pos + 1, text.length())) {
      delete ring;
      return true;
    }
    
    ++pos;
  }
  
  delete ring;
  return false;
}

//------------------------------------------------------------------------
bool MaildirMessage::textContains(const std::string &text)
{
  // search the body part of the message..
  int fd = getFile();
  if (fd == -1)
    return false;

  crlffile = fd;
  crlfReset();

  char c;
  char *ring = new char[text.length()];
  int pos = 0;
  while (crlfGetChar(c)) {
    ring[pos % text.length()] = toupper(c);
    
    if (compareStringToQueue(text, ring, pos + 1, text.length())) {
      delete ring;
      return true;
    }
    
    ++pos;
  }
  
  delete ring;
  return false;
}

//------------------------------------------------------------------------
const std::string &MaildirMessage::getHeader(const std::string &header)
{
  static string NIL = "";

  if (!parseHeaders())
    return NIL;

  HeaderItem hitem;
  if (!doc->h.getFirstHeader(header, hitem))
    return NIL;
  
  return hitem.getValue(); 
}

//------------------------------------------------------------------------
MaildirMessageCache::MaildirMessageCache(void)
{
}

//------------------------------------------------------------------------
MaildirMessageCache::~MaildirMessageCache(void)
{
  clear();
}

//------------------------------------------------------------------------
MaildirMessageCache &MaildirMessageCache::getInstance(void)
{
  static MaildirMessageCache cache;
  return cache;
}

//------------------------------------------------------------------------
void MaildirMessageCache::addStatus(const MaildirMessage *m,
				    ParseStatus s)
{
  if (statuses.find(m) == statuses.end()) {
    // Insert status. Perhaps remove oldest status.
    if (statuses.size() > 2) {
      MaildirMessage *message = const_cast<MaildirMessage *>(parsed.front());

      removeStatus(message);
    }

    parsed.push_back(m);
  }

  statuses[m] = s;
}

//------------------------------------------------------------------------
MaildirMessageCache::ParseStatus
MaildirMessageCache::getStatus(const MaildirMessage *m) const
{
  if (statuses.find(m) == statuses.end())
    return NotParsed;

  return statuses[m];
}

//------------------------------------------------------------------------
void MaildirMessageCache::clear(void)
{
  for (deque<const MaildirMessage *>::iterator i = parsed.begin();
       i != parsed.end(); ++i)
    const_cast<MaildirMessage *>(*i)->close();

  parsed.clear();
  statuses.clear();
}

//------------------------------------------------------------------------
void MaildirMessageCache::removeStatus(const MaildirMessage *m)
{
  if (statuses.find(m) == statuses.end())
    return;

  statuses.erase(statuses.find(m));

  for (deque<const MaildirMessage *>::iterator i = parsed.begin();
       i != parsed.end(); ++i) {
    if (*i == m) {
      const_cast<MaildirMessage *>(*i)->close();
      parsed.erase(i);
      break;
    }
  }
}

//------------------------------------------------------------------------
void MaildirMessage::setInternalFlag(unsigned char f)
{
  internalFlags |= f;
}

//------------------------------------------------------------------------
unsigned char MaildirMessage::getInternalFlags(void) const
{
  return internalFlags;
}

//------------------------------------------------------------------------
void MaildirMessage::clearInternalFlag(unsigned char f)
{
  internalFlags &= ~f;
}
