/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildir-scan.cc
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

#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "io.h"
#include "maildir.h"
#include "storage.h"

using namespace Binc;
using namespace ::std;

namespace {

  //----------------------------------------------------------------------
  class Lock {
    string lock;

  public:

    //--
    Lock(const string &path)
    {
      IO &logger = IOFactory::getInstance().get(2);

      lock = (path == "" ? "." : path) + "/bincimap-scan-lock";

      int lockfd = -1;
      while ((lockfd = ::open(lock.c_str(),
			      O_CREAT | O_WRONLY | O_EXCL, 0666)) == -1) {
	if (errno != EEXIST) {
	  logger << "unable to lock mailbox: " << lock
		 << ", " << string(strerror(errno)) << endl;
	  return;
	}
				 
	struct stat mystat;
	logger << "possible crash detected. waiting for mailbox lock " << lock << "." << endl;
	if (lstat(lock.c_str(), &mystat) == 0) {
	  if ((time(0) - mystat.st_ctime) > 300) {
	    if (unlink(lock.c_str()) == 0) continue;
	    else logger << "failed to force mailbox lock: " << lock
			<< ", " << string(strerror(errno)) << endl;
	  }
	} else {
	  if (errno != ENOENT) {
	    string err = "invalid lock " + lock + ": "
	      + strerror(errno);
	    logger << err << endl;
	    return;
	  }
	}
	
	// sleep one second.
	sleep(1);
      }

      close(lockfd);
    }

    //--
    ~Lock()
    {
      IO &logger = IOFactory::getInstance().get(2);

      // remove the lock
      if (unlink(lock.c_str()) != 0)
	logger << "failed to unlock mailbox: " << lock << ", "
	       << strerror(errno) << endl;
    }
  };
}

//------------------------------------------------------------------------
// scan the maildir. update flags, find messages in new/ and move them
// to cur, setting the recent flag in memory only. check for expunged
// messages. give newly arrived messages uids.
//------------------------------------------------------------------------
Maildir::ScanResult Maildir::scan(bool forceScan)
{
  IO &logger = IOFactory::getInstance().get(2);

  const string newpath = path + "/new/";
  const string curpath = path + "/cur/";
  const string uidvalfilename = path + "/bincimap-uidvalidity";
  const string cachefilename = path + "/bincimap-cache";

  // check wether or not we need to bother scanning the folder.
  if (firstscan || forceScan) {
    struct stat oldstat;
    if (stat(newpath.c_str(), &oldstat) != 0) {
      setLastError("Invalid Mailbox, " + newpath + ": "
		   + string(strerror(errno)));
      return PermanentError;
    }

    old_new_st_mtime = oldstat.st_mtime;
    old_new_st_ctime = oldstat.st_ctime;

    if (stat(curpath.c_str(), &oldstat) != 0) {
      setLastError("Invalid Mailbox, " + curpath + ": "
		   + string(strerror(errno)));
      return PermanentError;
    }

    old_cur_st_mtime = oldstat.st_mtime;
    old_cur_st_ctime = oldstat.st_ctime;

  } else {
    struct stat oldcurstat;
    struct stat oldnewstat;
    if (stat(newpath.c_str(), &oldnewstat) != 0) {
      setLastError("Invalid Mailbox, " + newpath + ": "
		   + string(strerror(errno)));
      return PermanentError;
    }

    if (stat(curpath.c_str(), &oldcurstat) != 0) {
      setLastError("Invalid Mailbox, " + curpath + ": "
		   + string(strerror(errno)));
      return PermanentError;
    }

    if (oldnewstat.st_mtime == old_new_st_mtime
	&& oldnewstat.st_ctime == old_new_st_ctime
	&& oldcurstat.st_mtime == old_cur_st_mtime
	&& oldcurstat.st_ctime == old_cur_st_ctime)
      return Success;

    old_cur_st_mtime = oldcurstat.st_mtime;
    old_cur_st_ctime = oldcurstat.st_ctime;
    old_new_st_mtime = oldnewstat.st_mtime;
    old_new_st_ctime = oldnewstat.st_ctime;
  }

  // lock the directory as we are scanning. this prevents race
  // conditions with uid delegation
  Lock lock(path);

  // Read the cache file if it's there. It holds important information
  // about the state of the depository, and serves to communicate
  // changes to the depot across Binc IMAP instances that can not be
  // communicated via the depot itself.
  switch (readCache()) {
  case NoCache:
  case Error:
    // An error with reading the cache files when it's not the first
    // time we scan the depot is treated as an error.
    if (!firstscan) {
      old_cur_st_mtime = (time_t) 0;
      old_cur_st_ctime = (time_t) 0;
      old_new_st_mtime = (time_t) 0;
      old_new_st_ctime = (time_t) 0;
      return TemporaryError;
    }

    uidnextchanged = true;
    mailboxchanged = true;
    break;
  default:
    break;
  }

  // open new/ directory
  DIR *pdir = opendir(newpath.c_str());
  if (pdir == 0) {
    string reason = "failed to open \"" + newpath + "\" (";
    reason += strerror(errno);
    reason += ")";
    setLastError(reason);

    return PermanentError;
  }

  // scan all entries
  struct dirent *pdirent;
  while ((pdirent = readdir(pdir)) != 0) {
    // "Unless you're writing messages to a maildir, the format of a
    // unique name is none of your business. A unique name can be
    // anything that doesn't contain a colon (or slash) and doesn't
    // start with a dot. Do not try to extract information from unique
    // names." - The Maildir spec from cr.yp.to
    string filename = pdirent->d_name;
    if (filename[0] == '.'
	|| filename.find(':') != string::npos
	|| filename.find('/') != string::npos)
      continue;

    string fullfilename = newpath + filename;

    // We need to find the timestamp of the message in order to
    // determine whether or not it's safe to move the message in from
    // new/. qmail's default message file naming algorithm forces us
    // to never move messages out of new/ that are less than one
    // second old.
    struct stat mystat;
    if (stat(fullfilename.c_str(), &mystat) != 0) {
      if (errno == ENOENT) {
	// a rare race between readdir and stat force us to restart
	// the scan.
	closedir(pdir);
	
	if ((pdir = opendir(newpath.c_str())) == 0) {
	  string reason = "Warning: opendir(\"" + newpath + "\") == 0 (";
	  reason += strerror(errno);
	  reason += ")";
	  setLastError(reason);
	  
	  return PermanentError;
	}
      } else
	logger << "junk in Maildir: \"" << fullfilename << "\": "
	       << strerror(errno);

      continue;
    }

    // this is important. do not move messages from new/ that are not
    // at least one second old or messages may disappear. this
    // introduces a special case: we can not cache the old st_ctime
    // and st_mtime. the next time the mailbox is scanned, it must not
    // simply be skipped. :-)

    vector<MaildirMessage>::const_iterator newIt = newMessages.begin();
    bool ours = false;
    for (; newIt != newMessages.end(); ++newIt) {
      if ((filename == (*newIt).getUnique())
	  && ((*newIt).getInternalFlags() & MaildirMessage::Committed)) {
	ours = true;
	break;
      }
    }
    
    if (!ours && ::time(0) <= mystat.st_mtime) {
      old_cur_st_mtime = (time_t) 0;
      old_cur_st_ctime = (time_t) 0;
      old_new_st_mtime = (time_t) 0;
      old_new_st_ctime = (time_t) 0;
      continue;
    }

    // move files from new/ to cur/
    if (rename((newpath + pdirent->d_name).c_str(), 
	       (curpath + pdirent->d_name).c_str()) != 0) {
      logger << "error moving messages from"
	" new to cur: skipping " << newpath 
	     << pdirent->d_name << ": " << strerror(errno) << endl;
      continue;
    }
  }

  closedir(pdir);

  // Now, assume all known messages were expunged and have them prove
  // otherwise.
  {
    Mailbox::iterator i = begin(SequenceSet::all(), INCLUDE_EXPUNGED | SQNR_MODE);
    for (; i != end(); ++i)
      (*i).setExpunged();
  }

  // Then, scan cur
  // open directory

  int oldmess = 0;
  int newmess = 0;

  if ((pdir = opendir(curpath.c_str())) == 0) {
    string reason = "Maildir::scan::opendir(\"" + curpath + "\") == 0 (";
    reason += strerror(errno);
    reason += ")";

    setLastError(reason);
    return PermanentError;
  }

  // erase all old maps between fixed filenames and actual file names.
  // we'll get a new list now, which will be more up to date.
  index.clearFileNames();

  // this is to sort recent messages by internaldate
  multimap<time_t, MaildirMessage> tempMessageMap;

  // scan all entries
  while ((pdirent = readdir(pdir)) != 0) {
    string filename = pdirent->d_name;
    if (filename[0] == '.')
      continue;

    string uniquename;
    string standard;
    string::size_type pos;
    if ((pos = filename.find(':')) != string::npos) {
      uniquename = filename.substr(0, pos);

      string tmp = filename.substr(pos);
      if ((pos = tmp.find("2,")) != string::npos)
	standard = tmp.substr(pos + 2);

    } else
      uniquename = filename;

    unsigned char mflags = Message::F_NONE;
    for (string::const_iterator i = standard.begin();
	 i != standard.end(); ++i) {
      switch (*i) {
      case 'R': mflags |= Message::F_ANSWERED; break;
      case 'S': mflags |= Message::F_SEEN; break;
      case 'T': mflags |= Message::F_DELETED; break;
      case 'D': mflags |= Message::F_DRAFT; break;
      case 'F': mflags |= Message::F_FLAGGED; break;
      default: break;
      }
    }

    index.insert(uniquename, 0, filename);

    struct stat mystat;
    MaildirMessage *message = get(uniquename);
    if (!message || message->getInternalDate() == 0) {
      string fullfilename = curpath + filename;
      if (stat(fullfilename.c_str(), &mystat) != 0) {
	if (errno == ENOENT) {
	  // a rare race between readdir and stat force us to restart
	  // the scan.
	  index.clearFileNames();
	  
	  closedir(pdir);
	  
	  if ((pdir = opendir(newpath.c_str())) == 0) {
	    string reason = "Warning: opendir(\"" + newpath + "\") == 0 (";
	    reason += strerror(errno);
	    reason += ")";
	    setLastError(reason);
	    
	    return PermanentError;
	  }
	}
	
	continue;
      }

      mailboxchanged = true;
    }
    
    // If we have this message in memory already..
    if (message) {
      oldmess++;

      if (message->getInternalDate() == 0) {
	mailboxchanged = true;
	message->setInternalDate(mystat.st_mtime);
      }

      // then confirm that this message was not expunged
      message->setUnExpunged();

      // update the flags with what new flags we found in the filename
      if (mflags != (message->getStdFlags() & ~Message::F_RECENT)) {
	message->resetStdFlags();
	message->setStdFlag(mflags);
      }

      continue;
    }

    newmess++;

    // Wait with delegating UIDs until all entries have been
    // read. Only then can we sort by internaldate and delegate new
    // UIDs.
    MaildirMessage m(*this);
    m.setUID(0);
    m.setSize(0);
    m.setInternalDate(mystat.st_mtime);
    m.setStdFlag(mflags | Message::F_RECENT);
    m.setUnique(uniquename);
    tempMessageMap.insert(make_pair(mystat.st_mtime, m));

    mailboxchanged = true;
  }

  closedir(pdir);

  // Recent messages are added, ordered by internaldate.
  {
    multimap<time_t, MaildirMessage>::iterator i = tempMessageMap.begin();
    while (i != tempMessageMap.end()) {
      i->second.setUID(uidnext++);
      multimap<time_t, MaildirMessage>::iterator itmp = i;
      ++itmp;
      add(i->second);
      tempMessageMap.erase(i);
      i = itmp;
      uidnextchanged = true;
    }
  }

  tempMessageMap.clear();

  // Messages that existed in the cache that we read, but did not
  // exist in the Maildir, are removed from the messages list.
  Mailbox::iterator jj = begin(SequenceSet::all(), INCLUDE_EXPUNGED | SQNR_MODE);
  while (jj != end()) {
    MaildirMessage &message = (MaildirMessage &)*jj;

    if (message.isExpunged()) {
      if (message.getInternalFlags() & MaildirMessage::JustArrived) {
	jj.erase();
	continue;
      }
    }

    ++jj;
  }

  // Special case: The first time we scan is in SELECT. All flags
  // changes for new messages will then appear to be recent, and
  // to avoid this to be sent to the client as a pending update,
  // we explicitly unset the "flagsChanged" flag in all messages.
  if (firstscan) {
    unsigned int lastuid = 0;

    Mailbox::iterator ii
      = begin(SequenceSet::all(), INCLUDE_EXPUNGED | SQNR_MODE);
    for (; ii != end(); ++ii) {
      MaildirMessage &message = (MaildirMessage &)*ii;
      message.clearInternalFlag(MaildirMessage::JustArrived);

      if (lastuid < message.getUID())
	lastuid = message.getUID();
      else {
	logger << "UID values are not strictly ascending in this"
	  " mailbox: " << path << ". This is usually caused by "
	       << "access from a broken accessor. Bumping UIDVALIDITY." 
	       << endl;

	setLastError("An error occurred while scanning the mailbox. "
		     "Please contact your system administrator.");

	if (!readOnly) {
	  bumpUidValidity(path);

	  old_cur_st_mtime = (time_t) 0;
	  old_cur_st_ctime = (time_t) 0;
	  old_new_st_mtime = (time_t) 0;
	  old_new_st_ctime = (time_t) 0;
	  return TemporaryError;
	} else {
      	  return PermanentError;
	}
      }
      
      message.setFlagsUnchanged();
    }
  }

  if (mailboxchanged && !readOnly) {
    if (!writeCache())
      return PermanentError;

    mailboxchanged = false;
  }

  if (uidnextchanged && !readOnly) {
    Storage uidvalfile(uidvalfilename, Storage::WriteOnly);
    uidvalfile.put("depot", "_uidvalidity", toString(uidvalidity));
    uidvalfile.put("depot", "_uidnext", toString(uidnext));
    uidvalfile.put("depot", "_version", UIDVALFILEVERSION);
    if (!uidvalfile.commit()) {
      setLastError("Unable to save cache file.");
      return PermanentError;
    }

    uidnextchanged = false;
  }

  firstscan = false;
  newMessages.clear();
  return Success;
}
