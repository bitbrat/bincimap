/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildir.cc
 *  
 *  Description:
 *    Implementation of the Maildir class.
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

#include <iostream>
#include <iomanip>
#include <algorithm>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "io.h"
#include "session.h"
#include "status.h"
#include "storage.h"
#include "convert.h"
#include "maildir.h"
#include "maildirmessage.h"
#include "pendingupdates.h"

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
Maildir::iterator::iterator(void)
{
}

//------------------------------------------------------------------------
Maildir::iterator::iterator(Maildir *home,
			    MessageMap::iterator it,
			    const SequenceSet &_bset,
			    unsigned int _mod) 
  : BaseIterator(1), mailbox(home), bset(_bset), mod(_mod), i(it)
{
}

//------------------------------------------------------------------------
Maildir::iterator::iterator(const iterator &copy)
  : BaseIterator(copy.sqnr), mailbox(copy.mailbox),
    bset(copy.bset), mod(copy.mod), i(copy.i)
{
}

//------------------------------------------------------------------------
Maildir::iterator &Maildir::iterator::operator =(const iterator &copy)
{
  sqnr = copy.sqnr;
  mailbox = copy.mailbox;
  bset = copy.bset;
  mod = copy.mod;
  i = copy.i;
  return *this;
}

//------------------------------------------------------------------------
Maildir::iterator::~iterator(void)
{
}

//------------------------------------------------------------------------
MaildirMessage &Maildir::iterator::curMessage(void)
{
  return i->second;
}

//------------------------------------------------------------------------
Message &Maildir::iterator::operator *(void)
{
  return curMessage();
}

//------------------------------------------------------------------------
void Maildir::iterator::operator ++(void)
{
  ++i;
  ++sqnr;
  reposition();
}

//------------------------------------------------------------------------
bool Maildir::iterator::operator ==(const BaseIterator &a) const
{
  const iterator *b = dynamic_cast<const iterator *>(&a);
  return b ? (i == b->i) : false;
}

//------------------------------------------------------------------------
bool Maildir::iterator::operator !=(const BaseIterator &a) const
{
  return !((*this) == a);
}

//------------------------------------------------------------------------
void Maildir::iterator::reposition(void)
{
  for (;;) {
    if (i == mailbox->messages.end())
      break;

    Message &message = curMessage();
    if ((mod & SKIP_EXPUNGED) && message.isExpunged()) {
      ++i;
      continue;
    }

    if (!bset.isInSet(mod & SQNR_MODE ? sqnr : message.getUID())) {
      ++i;
      if (!message.isExpunged())
	++sqnr;
      continue;
    }

    break;
  }
}

//------------------------------------------------------------------------
Mailbox::iterator Maildir::begin(const SequenceSet &bset,
				 unsigned int mod) const
{
  beginIterator = iterator((Maildir *)this, messages.begin(), bset, mod);
  beginIterator.reposition();

  return Mailbox::iterator(beginIterator);
}

//------------------------------------------------------------------------
Mailbox::iterator Maildir::end(void) const
{
  endIterator = iterator((Maildir *)this, messages.end(),
			 endIterator.bset, endIterator.mod);
  return Mailbox::iterator(endIterator);
}

//------------------------------------------------------------------------
void Maildir::iterator::erase(void)
{
  MessageMap::iterator iter = i;
  ++iter;
   
  MaildirMessageCache::getInstance().removeStatus(&curMessage());
  mailbox->index.remove(i->second.getUnique());
  mailbox->messages.erase(i);

  i = iter;
  reposition();
}

//------------------------------------------------------------------------
Maildir::Maildir(void) : Mailbox()
{
  firstscan = true;
  cacheRead = false;
  uidvalidity = 0;
  uidnext = 1;
  selected = false;
  oldrecent = 0;
  oldexists = 0;
}

//------------------------------------------------------------------------
Maildir::~Maildir(void)
{
}

//------------------------------------------------------------------------
void Maildir::setPath(const string &path_in)
{
  path = path_in;
}

//------------------------------------------------------------------------
bool Maildir::getUpdates(bool doscan, unsigned int type,
			 PendingUpdates &updates, bool forceScan)
{
  if (doscan && scan(forceScan) != Success)
    return false;

  unsigned int exists = 0;
  unsigned int recent = 0;

  // count messages, find recent
  if (!readOnly && (type & PendingUpdates::EXPUNGE)) {  
    Mailbox::iterator i = begin(SequenceSet::all(),
				INCLUDE_EXPUNGED | SQNR_MODE);

    while (i != end()) {
      Message &message = *i;

      if (message.isExpunged()) {
	updates.addExpunged(i.getSqnr());
	i.erase();
      } else
	++i;
    }
  }

  Mailbox::iterator i = begin(SequenceSet::all(),
			      INCLUDE_EXPUNGED | SQNR_MODE);
  for (; i != end(); ++i) {
    Message & message = *i;
    // at this point, there is a message that is not expunged
    ++exists;
    if (message.getStdFlags() & Message::F_RECENT) ++recent;
  }

  if (exists != oldexists)
    updates.setExists(oldexists = exists);

  if (recent != oldrecent)
    updates.setRecent(oldrecent = recent);
  
  if (type & PendingUpdates::FLAGS) {   
    Mailbox::iterator i = begin(SequenceSet::all(), SQNR_MODE);
    for (; i != end(); ++i) {
      Message &message = *i;
      if (message.hasFlagsChanged()) {
	int flags = message.getStdFlags();
	
	updates.addFlagUpdates(i.getSqnr(), flags);

	message.setFlagsUnchanged();
      }
    }
  }  

  return true;
}

//------------------------------------------------------------------------
bool Maildir::isMailbox(const std::string &s_in) const
{
  struct stat mystat;

  return ((stat((s_in + "/cur").c_str(), &mystat) == 0
	   && S_ISDIR(mystat.st_mode))
      && (stat((s_in + "/new").c_str(), &mystat) == 0 
	  && S_ISDIR(mystat.st_mode))
      && (stat((s_in + "/tmp").c_str(), &mystat) == 0 
	  && S_ISDIR(mystat.st_mode)));
}

//------------------------------------------------------------------------
const std::string Maildir::getTypeName(void) const
{
  return "Maildir";
}

//------------------------------------------------------------------------
void Maildir::bumpUidValidity(const string &s_in) const
{
  unlink((s_in + "/bincimap-uidvalidity").c_str());
  unlink((s_in + "/bincimap-cache").c_str());
}

//------------------------------------------------------------------------
bool Maildir::isMarked(const std::string &s_in) const
{
  DIR *dirp = opendir((s_in + "/new").c_str());
  if (dirp == 0) return false;

  struct dirent *direntp;  
  while ((direntp = readdir(dirp)) != 0) {
    string s = direntp->d_name;
    if (s[0] != '.' 
	&& s.find('/') == string::npos
	&& s.find(':') == string::npos) {
      closedir(dirp);
      return true;
    }
  }

  closedir(dirp);
  return false;
}

//------------------------------------------------------------------------
unsigned int Maildir::getStatusID(const string &path) const 
{
  unsigned int statusid = 0;
  struct stat mystat;
  if (stat((path + "/new").c_str(), &mystat) == 0)
    statusid = mystat.st_ctime;

  if (stat((path + "/cur").c_str(), &mystat) == 0)
    statusid += mystat.st_ctime;

  if (stat((path + "/bincimap-cache").c_str(), &mystat) == 0)
    statusid += mystat.st_ctime;

  return statusid;
}

//------------------------------------------------------------------------
bool Maildir::getStatus(const string &path, Status &s) const 
{
  unsigned int messages = 0;
  unsigned int unseen = 0;
  unsigned int recent = 0;

  const string cachefilename = path + "/bincimap-cache";
  const string uidvalfilename = path + "/bincimap-uidvalidity";

  Storage cache(cachefilename, Storage::ReadOnly);
  Storage uidvalfile(uidvalfilename, Storage::ReadOnly);

  string section, key, value;
  map<string, bool> mincache;
  while (cache.get(&section, &key, &value))
    if (isdigit(section[0]) && key == "_ID")
      mincache[value] = true;

  unsigned int uidvalidity = 0;
  unsigned int uidnext = 0;
  while (uidvalfile.get(&section, &key, &value))
    if (section == "depot" && key == "_uidvalidity")
      uidvalidity = (unsigned int) atoi(value);
    else if (section == "depot" && key == "_uidnext")
      uidnext  = (unsigned int) atoi(value);

  s.setUidValidity(uidvalidity < 1 ? time(0) : uidvalidity);

  // Scan new
  DIR *dirp = opendir((path + "/new").c_str());
  if (dirp == 0) return false;

  struct dirent *direntp;  
  while ((direntp = readdir(dirp)) != 0) {
    const string filename = direntp->d_name;
    if (filename[0] == '.'
	|| filename.find(':') != string::npos
	|| filename.find('/') != string::npos)
      continue;

    ++recent;
    ++uidnext;
    ++unseen;
    ++messages;
  }

  closedir(dirp);

  // Scan cur
  if ((dirp = opendir((path + "/cur").c_str())) == 0)
    return false;

  while ((direntp = readdir(dirp)) != 0) {
    const string dname = direntp->d_name;
    if (dname[0] == '.')
      continue;

    ++messages;

    // Add to unseen if it doesn't have the seen flag or if it has no
    // flags.
    const string::size_type pos = dname.find(':');
    if (pos != string::npos) {
      if (mincache.find(dname.substr(0, pos)) == mincache.end()) {
	++recent;
	++uidnext;
      }

      if (dname.substr(pos).find('S') == string::npos)
	++unseen;
    } else {
      if (mincache.find(dname) == mincache.end()) {
	++recent;
	++uidnext;
      }

      ++unseen;
    }
  }

  closedir(dirp);
  
  s.setRecent(recent);
  s.setMessages(messages);
  s.setUnseen(unseen);
  s.setUidNext(uidnext);

  return true;
}

//------------------------------------------------------------------------
unsigned int Maildir::getMaxUid(void) const
{
  unsigned int max = 0;
  Mailbox::iterator i = begin(SequenceSet::all(), SKIP_EXPUNGED | SQNR_MODE);
  for (; i != end(); ++i)
    if ((*i).getUID() > max)
      max = (*i).getUID();

  return max;
}

//------------------------------------------------------------------------
unsigned int Maildir::getMaxSqnr(void) const
{
  unsigned int max = 0;
  Mailbox::iterator i = begin(SequenceSet::all(), SKIP_EXPUNGED | SQNR_MODE);
  for (; i != end(); ++i)
    if (i.getSqnr() > max)
      max = i.getSqnr();

  return max;
}

//------------------------------------------------------------------------
unsigned int Maildir::getUidValidity(void) const
{
  // Why scan?
  //  if (uidvalidity < 1) {
  // FIXME: Catch any error here
  //    scan();
  //}

  return uidvalidity;
}

//------------------------------------------------------------------------
unsigned int Maildir::getUidNext(void) const
{
  // Why scan?
  //if (uidnext < 1) {
    // FIXME: Catch any error here
    //scan();
  //}
  
  return uidnext;
}

//------------------------------------------------------------------------
Message *Maildir::createMessage(const string &mbox, time_t idate)
{
  string sname = mbox + "/tmp/bincimap-XXXXXX";
  char *safename = strdup(sname.c_str());

  int fd = mkstemp(safename);
  if (fd == -1) {
    setLastError("Unable to create safe name.");
    return 0;
  }

  string safenameStr = safename;
  delete safename;

  MaildirMessage message(*this);

  message.setFile(fd);
  message.setSafeName(safenameStr);
  message.setInternalDate(idate);

  newMessages.push_back(message);
  vector<MaildirMessage>::iterator i = newMessages.end();
  --i;
  return &(*i);
}

//------------------------------------------------------------------------
bool Maildir::commitNewMessages(const string &mbox)
{
  Session &session = Session::getInstance();
  IO &logger = IOFactory::getInstance().get(2);

  vector<MaildirMessage>::iterator i = newMessages.begin();
  map<MaildirMessage *, string> committedMessages;

  char hostname[512];
  int hostnamelen = gethostname(hostname, sizeof(hostname));
  if (hostnamelen == -1 || hostnamelen == sizeof(hostname))
    strcpy(hostname, "localhost");

  struct timeval youngestFile = {0, 0};
  
  bool abort = false;
  while (!abort && i != newMessages.end()) {
    MaildirMessage &m = *i;
    string safeName = m.getSafeName();

    for (int attempt = 0; !abort && attempt < 1000; ++attempt) {
      struct timeval tv;
      gettimeofday(&tv, 0);
      youngestFile = tv;

      BincStream ssid;
      ssid << (int) tv.tv_sec
	   << "." << (int) session.getPid()
	   << (attempt == 0 ? "" : ("_" + toString(attempt)))
	   << "_" << (int) tv.tv_usec
	   << (rand() % 0xffff) << "_BincIMAP." << session.getHostname();
      
      BincStream ss;
      ss << mbox << "/new/" << ssid.str();
      
      string fileName = ss.str();

      if (link(safeName.c_str(), fileName.c_str()) == 0) {
	unlink(safeName.c_str());
	m.setInternalDate(tv.tv_sec);
	m.setUnique(ssid.str());
	m.setUID(0);
	committedMessages[&m] = fileName;
	break;
      }

      if (errno == EEXIST)
	continue;

      logger << "Warning: link(" << toImapString(safeName) << ", " 
	     << toImapString(fileName) << ") failed: "
	     << strerror(errno) << endl;

      session.setResponseCode("TRYCREATE");
      session.setLastError("failed, internal error.");
      abort = true;
      break;
    }

    ++i;
  }

  // abort means to make an attempt to restore the mailbox to
  // its original state.
  if (abort) {
    // Fixme: Messages that are in committedMessages should be skipped
    // here.
    for (i = newMessages.begin(); i != newMessages.end(); ++i)
      unlink((*i).getSafeName().c_str());

    map<MaildirMessage *, string>::const_iterator j
      = committedMessages.begin();
    while (j != committedMessages.end()) {
      if (unlink(j->second.c_str()) != 0) {
	if (errno == ENOENT) {
	  // FIXME: The message was probably moves away from new/ by
	  // another IMAP session.
	  logger << "error rollbacking after failed commit to "
		 << toImapString(mbox) << ", failed to unlink "
		 << toImapString(j->second) 
		 << ": " << strerror(errno) << endl;
	} else {
	  logger << "error rollbacking after failed commit to "
		 << toImapString(mbox) << ", failed to unlink "
		 << toImapString(j->second) 
		 << ": " << strerror(errno) << endl;
	  newMessages.clear();
	  return false;
	}
      }

      ++j;
    }

    newMessages.clear();
    return false;
  }

  // cover the extremely unlikely event that another concurrent
  // Maildir accessor has just made a file with the same timestamp and
  // random number by sleeping until the timestamp has changed before
  // moving the message into cur.
  struct timeval tv;
  gettimeofday(&tv, 0);
  while (tv.tv_sec == youngestFile.tv_sec 
	 && tv.tv_usec == youngestFile.tv_usec) {
    gettimeofday(&tv, 0);
  }

  map<MaildirMessage *, string>::const_iterator j
    = committedMessages.begin();
  for (;j != committedMessages.end(); ++j) {
    string basename = j->second.substr(j->second.rfind('/') + 1);

    int flags = j->first->getStdFlags();
    string flagStr;
    if (flags & Message::F_DRAFT) flagStr += "D";
    if (flags & Message::F_FLAGGED) flagStr += "F";
    if (flags & Message::F_ANSWERED) flagStr += "R";
    if (flags & Message::F_SEEN) flagStr += "S";
    if (flags & Message::F_DELETED) flagStr += "T";
    
    string dest = mbox + "/cur/" + basename + ":2," + flagStr;
    if (rename(j->second.c_str(), dest.c_str()) == 0)
      continue;

    if (errno != ENOENT)
      logger << "when setting flags on: " << j->second 
	     << ": " << strerror(errno) << endl;
  }

  committedMessages.clear();
  newMessages.clear();
  return true;
}

//------------------------------------------------------------------------
bool Maildir::rollBackNewMessages(void)
{
  vector<MaildirMessage>::const_iterator i = newMessages.begin();
  // Fixme: Messages that are in committedMessages should be skipped
  // here.
  for (; i != newMessages.end(); ++i)
    unlink((*i).getSafeName().c_str());

  newMessages.clear();

  return true;
}

//------------------------------------------------------------------------
bool Maildir::fastCopy(Message &m, Mailbox &desttype,
		       const std::string &destname)
{
  // At this point, fastCopy is broken because the creation time is
  // equal for the two clones. The new clone must have a new creation
  // time. Symlinks are a possibility, but they break if other Maildir
  // accessors rename mailboxes.
  //  return false;

  Session &session = Session::getInstance();
  IO &logger = IOFactory::getInstance().get(2);

  MaildirMessage *message = dynamic_cast<MaildirMessage *>(&m);
  if (!message)
    return false;

  string mfilename = message->getFileName();
  if (mfilename == "")
    return false;

  Maildir *mailbox = dynamic_cast<Maildir *>(&desttype);
  if (!mailbox)
    return false;

  for (int attempt = 0; attempt < 1000; ++attempt) {

    struct timeval tv;
    gettimeofday(&tv, 0);

    BincStream ssid;
    ssid << (int) tv.tv_sec
	 << "." << (int) session.getPid()
	 << (attempt == 0 ? "" : ("_" + toString(attempt)))
	 << "_" << (int) tv.tv_usec 
	 << (rand() % 0xffff) << "_BincIMAP." << session.getHostname();

    BincStream ss;
    ss << destname << "/tmp/" << ssid.str();
    
    string fileName = ss.str();
    
    if (link((path + "/cur/" + mfilename).c_str(), fileName.c_str()) == 0) {
      MaildirMessage newmess = *message;
      newmess.setSafeName(fileName);
      newmess.setUnique(ssid.str());
      newmess.setInternalDate((time_t) tv.tv_sec);
      newmess.setUID(0);
      newMessages.push_back(newmess);
      break;
    }
    
    if (errno == EEXIST)
      continue;
    
    logger << "Warning: link(" << toImapString(path + "/cur/" + mfilename) 
	   << ", " << toImapString(fileName) << ") failed: "
	   << strerror(errno) << endl;
    
    session.setResponseCode("TRYCREATE");
    session.setLastError("failed, internal error.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------
MaildirMessage *Maildir::get(const std::string &id)
{
  MaildirIndexItem *item = index.find(id);
  if (!item)
    return 0;

  unsigned int uid = item->uid;
  if (messages.find(uid) == messages.end())
    return 0;

  return &messages.find(uid)->second;
}

//------------------------------------------------------------------------
void Maildir::add(MaildirMessage &m)
{
  messages.insert(make_pair(m.getUID(), m));
  index.insert(m.getUnique(), m.getUID());
}

//------------------------------------------------------------------------
unsigned int MaildirIndex::getSize(void) const
{
  return idx.size();
}

//------------------------------------------------------------------------
void MaildirIndex::insert(const string &unique, unsigned int uid,
			  const string &fileName)
{
  if (idx.find(unique) == idx.end()) {
    MaildirIndexItem item;
    item.uid = uid;
    item.fileName = fileName;
    idx[unique] = item;
  } else {
    MaildirIndexItem &item = idx[unique];
    if (uid != 0) item.uid = uid;
    if (fileName != "") item.fileName = fileName;
  }
}

//------------------------------------------------------------------------
void MaildirIndex::remove(const string &unique)
{
  map<string, MaildirIndexItem>::iterator it = idx.find(unique);
  if (it != idx.end())
    idx.erase(it);
}

//------------------------------------------------------------------------
MaildirIndexItem *MaildirIndex::find(const string &unique)
{
  map<string, MaildirIndexItem>::iterator it = idx.find(unique);
  if (it != idx.end())
    return &it->second;

  return 0;
}

//------------------------------------------------------------------------
void MaildirIndex::clear(void)
{
  idx.clear();
}

//------------------------------------------------------------------------
void MaildirIndex::clearUids(void)
{
  map<string, MaildirIndexItem>::iterator it = idx.begin();
  for (; it != idx.end(); ++it)
    it->second.uid = 0;
}

//------------------------------------------------------------------------
void MaildirIndex::clearFileNames(void)
{
  map<string, MaildirIndexItem>::iterator it = idx.begin();
  for (; it != idx.end(); ++it)
    it->second.fileName = "";
}
