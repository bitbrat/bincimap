/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    depot.cc
 *  
 *  Description:
 *    <--->
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
#include <map>
#include <string>

#include "depot.h"
#include "mailbox.h"
#include "status.h"
#include "convert.h"
#include "io.h"

using namespace ::std;
using namespace Binc;

//--------------------------------------------------------------------
DepotFactory::DepotFactory(void)
{
}

//--------------------------------------------------------------------
DepotFactory::~DepotFactory(void)
{
  for (vector<Depot *>::iterator i = depots.begin(); i != depots.end();
       ++i)
    delete *i;
}

//--------------------------------------------------------------------
Depot *DepotFactory::get(const string &name) const
{
  for (vector<Depot *>::const_iterator i = depots.begin(); i != depots.end();
       ++i)
    if ((*i)->getName() == name)
      return *i;

  return 0;
}

//--------------------------------------------------------------------
void DepotFactory::assign(Depot *depot)
{
  depots.push_back(depot);
}

//--------------------------------------------------------------------
DepotFactory &DepotFactory::getInstance(void)
{
  static DepotFactory depotfactory;
  return depotfactory;
}

//--------------------------------------------------------------------
Depot::Depot(void) : enditerator(0, 0)
{
  defaultmailbox = 0;
  selectedmailbox = 0;

  delimiter = '/';
}

//--------------------------------------------------------------------
Depot::Depot(const string &name) : enditerator(0, 0)
{
  defaultmailbox = 0;
  selectedmailbox = 0;

  delimiter = '/';

  this->name = name;
}

//--------------------------------------------------------------------
Depot::~Depot(void)
{
}

//--------------------------------------------------------------------
const string &Depot::getLastError(void) const
{
  return lastError;
}

//--------------------------------------------------------------------
void Depot::setLastError(const string &error) const
{
  lastError = error;
}

//--------------------------------------------------------------------
void Depot::assign(Mailbox *m)
{
  for (vector<Mailbox *>::const_iterator i = backends.begin();
       i != backends.end(); ++i)
    if (*i == m) break;

  backends.push_back(m);
}

//--------------------------------------------------------------------
Mailbox *Depot::get(const string &s_in) const
{
  for (vector<Mailbox *>::const_iterator i = backends.begin();
       i != backends.end(); ++i)
    if ((*i)->isMailbox(mailboxToFilename(s_in)))
      return *i;

  setLastError("No such mailbox " + toImapString(s_in));
  return 0;
}

//--------------------------------------------------------------------
bool Depot::setSelected(Mailbox *m)
{
  for (vector<Mailbox *>::const_iterator i = backends.begin();
       i != backends.end(); ++i)
    if (*i == m) {
      selectedmailbox = m;
      return true;
    }

  setLastError("Attempted to select unregistered Mailbox type in Depot");
  return false;
}

//--------------------------------------------------------------------
const string &Depot::getName(void) const
{
  return name;
}

//--------------------------------------------------------------------
void Depot::setDelimiter(char c)
{
  delimiter = c;
}

//--------------------------------------------------------------------
const char Depot::getDelimiter(void) const
{
  return delimiter;
}

//--------------------------------------------------------------------
bool Depot::setDefaultType(const string &name)
{
  for (vector<Mailbox *>::const_iterator i = backends.begin();
       i != backends.end(); ++i)
    if ((*i)->getTypeName() == name) {
      defaultmailbox = *i;
      return true;
    }

  setLastError("attempt to default to unregistered Mailbox type " + name);
  return false;
}

//--------------------------------------------------------------------
Mailbox *Depot::getSelected(void) const
{
  return selectedmailbox;
}

//--------------------------------------------------------------------
Mailbox *Depot::getDefault(void) const
{
  return defaultmailbox;
}

//--------------------------------------------------------------------
bool Depot::createMailbox(const string &s_in) const
{
  const string &mailboxname = mailboxToFilename(toCanonMailbox(s_in));
  if (mailboxname == "") {
    setLastError("invalid mailbox name");
    return false;
  }

  Mailbox *mailbox = getDefault();
  if (mailbox == 0) {
    setLastError("no default mailbox defined");
    return false;
  }

  bool result = mailbox->createMailbox(mailboxname, 0777);
  if (result)
    return true;
  else {
    setLastError(mailbox->getLastError());
    return false;
  }
}

//--------------------------------------------------------------------
bool Depot::deleteMailbox(const string &s_in) const
{
  const string &mailboxname = mailboxToFilename(toCanonMailbox(s_in));

  Mailbox *mailbox = get(s_in);
  if (mailbox == 0) {
    setLastError(s_in + ": no such mailbox");
    return false;
  }

  bool result = mailbox->deleteMailbox(mailboxname);
  if (result)
    return true;
  else {
    setLastError(mailbox->getLastError());
    return false;
  }
}

//--------------------------------------------------------------------
bool Depot::renameMailbox(const string &s_in, const string &t_in) const
{
  IO &logger = IOFactory::getInstance().get(2);
    
  const string &source = mailboxToFilename(s_in).c_str();
  const string &dest = mailboxToFilename(t_in).c_str();

  int nrenamed = 0;
  const iterator e = end();
  for (iterator i = begin("."); i != e; ++i) {
    string entry = *i;

    if (entry.substr(0, source.length()) == source) {
      string sourcename, destname;

      if (entry.length() == source.length()) {
	sourcename = source;
	destname = dest;

      } else if (entry.length() > source.length() 
		 && entry[source.length()] == '.') {
	sourcename = entry;
	destname = dest + entry.substr(source.length());
      } else continue;

      if (rename(sourcename.c_str(), destname.c_str()) != 0) {
	logger << "error renaming " << sourcename << " to " 
	       << destname << ": " << strerror(errno) << endl;
      } else
	nrenamed++;
      
      Mailbox *mailbox;
      if ((mailbox = get(filenameToMailbox(sourcename))) != 0)
	mailbox->bumpUidValidity(filenameToMailbox(sourcename));
      if ((mailbox = get(filenameToMailbox(destname))) != 0)
	mailbox->bumpUidValidity(filenameToMailbox(destname));
    }
  }

  if (nrenamed == 0) {
    setLastError("An error occurred when renaming " 
		 + toImapString(s_in)
		 + " to " + toImapString(t_in)
		 + ". Try creating a new mailbox,"
		 " then copy over all messages."
		 " Finally, delete the original mailbox");
    return false;
  } else
    return true;
}

//--------------------------------------------------------------------
bool Depot::getStatus(const std::string &s_in, Status &dest) const
{
  const string mailbox = toCanonMailbox(s_in);
  Mailbox *m = get(mailbox);
  if (m == 0) {
    setLastError("Unrecognized mailbox: " + toImapString(s_in));
    return false;
  }
  
  int statusid = m->getStatusID(mailboxToFilename(mailbox));
  if (mailboxstatuses.find(mailbox) != mailboxstatuses.end()) {
    dest = mailboxstatuses[mailbox];
    if (dest.getStatusID() == statusid)
      return true;
  }

  if (!m->getStatus(mailboxToFilename(mailbox), dest)) {
    setLastError(m->getLastError());
    return false;
  }

  dest.setStatusID(statusid);
  mailboxstatuses[mailbox] = dest;
  return true;
}

//--------------------------------------------------------------------
Depot::iterator::iterator(void)
{
  dirp = 0;
  ref = new int;
  *ref = 1;
}

//--------------------------------------------------------------------
Depot::iterator::iterator(DIR *dp, struct dirent *sp)
{
  dirp = dp;
  direntp = sp;

  ref = new int;
  *ref = 1;
}

//--------------------------------------------------------------------
Depot::iterator::iterator(const iterator &copy)
{
  if (*copy.ref != 0)
    ++(*copy.ref);

  ref = copy.ref;
  dirp = copy.dirp;
  direntp = copy.direntp;
}

//--------------------------------------------------------------------
Depot::iterator::~iterator(void)
{
  deref();
}

//--------------------------------------------------------------------
Depot::iterator &Depot::iterator::operator =(const iterator &copy)
{
  if (*copy.ref != 0)
    ++(*copy.ref);

  deref();

  ref = copy.ref;
  dirp = copy.dirp;
  direntp = copy.direntp;

  return *this;
}

//--------------------------------------------------------------------
void Depot::iterator::deref(void)
{
  // decrease existing copy ref if there is one
  if (*ref != 0 && --(*ref) == 0) {
    if (dirp) {
      closedir(dirp);
      dirp = 0;
    }

    delete ref;
    ref = 0;
  }
}

//--------------------------------------------------------------------
string Depot::iterator::operator * (void) const
{
  if (direntp == 0)
    return "";

  return direntp->d_name;
}

//--------------------------------------------------------------------
void Depot::iterator::operator ++ (void)
{
  direntp = readdir(dirp);
}

//--------------------------------------------------------------------
bool Depot::iterator::operator == (Depot::iterator i) const
{
  return direntp == i.direntp;
}

//--------------------------------------------------------------------
bool Depot::iterator::operator != (Depot::iterator i) const
{
  return direntp != i.direntp;
}

//--------------------------------------------------------------------
Depot::iterator Depot::begin(const string &path) const
{
  Depot::iterator i;

  if ((i.dirp = opendir(path.c_str())) == 0) {
    IO &logger = IOFactory::getInstance().get(2);

    logger << "opendir on " + path + " failed" << endl;
    setLastError("opendir on " + path + " failed");
    return end();
  }

  ++i;
  return i;
}

//--------------------------------------------------------------------
const Depot::iterator &Depot::end(void) const
{
  return enditerator;
}

//--------------------------------------------------------------------
MaildirPPDepot::MaildirPPDepot(void) : Depot("Maildir++")
{
}

//--------------------------------------------------------------------
MaildirPPDepot::~MaildirPPDepot(void)
{
}

//--------------------------------------------------------------------
string MaildirPPDepot::mailboxToFilename(const string &m) const
{
  string prefix = "INBOX"; prefix += delimiter;

  string mm = m;
  trim(mm, string(&delimiter, 1));
  string tmp = mm;
  uppercase(tmp);
  if (tmp != "INBOX" && tmp.substr(0, 6) != "INBOX/") {
    setLastError("With a Maildir++ depot, you must create all"
		 " mailboxes under INBOX. Try creating"
		 " INBOX/" + mm + ".");
    return "";
  }

  string twodelim;
  twodelim += delimiter;
  twodelim += delimiter;

  if (mm == "INBOX") return ".";
  else if (mm.length() <= 6) {
    setLastError("With a Maildir++ depot, you must create all"
		 " mailboxes under INBOX.");
    return "";
  } else if (mm.substr(0, 6) != prefix) {
    setLastError("With a Maildir++ depot, you must create all"
		 " mailboxes under INBOX.");
    return "";
  } else if (mm.find(twodelim) != string::npos) {
    setLastError("Invalid character combination " 
		 + twodelim + " in mailbox name");
    return "";
  } else if (mm != "" && mm.substr(1).find('.') != string::npos) {
    setLastError("Invalid character '.' in mailbox name");
    return "";
  } else {
    string tmp = mm.substr(6);
    for (string::iterator i = tmp.begin(); i != tmp.end(); ++i)
      if (*i == '/') *i = '.';

    return "." + tmp;
  }
}

//--------------------------------------------------------------------
string MaildirPPDepot::filenameToMailbox(const string &m) const
{
  if (m == ".") return "INBOX";
  else if (m.find(delimiter) != string::npos) return "";
  else if (m != "" && m[0] == '.') {
    string tmp = m;
    for (string::iterator i = tmp.begin(); i != tmp.end(); ++i)
      if (*i == '.') *i = delimiter;

    return "INBOX" + tmp;
  } else return "";
}

//--------------------------------------------------------------------
IMAPdirDepot::IMAPdirDepot(void) : Depot("IMAPdir")
{
}

//--------------------------------------------------------------------
IMAPdirDepot::~IMAPdirDepot(void)
{
}

//--------------------------------------------------------------------
string IMAPdirDepot::mailboxToFilename(const string &m) const
{
  string tmp;
  string mm = m;
  trim(mm, string(&delimiter, 1));

  string twodelim;
  twodelim += delimiter;
  twodelim += delimiter;

  if (mm.find(twodelim) != string::npos) {
    setLastError("Invalid character combination " 
		 + twodelim + " in mailbox name");
    return "";
  }

  string::const_iterator i = mm.begin();
  while (i != mm.end()) {
    if (*i == delimiter) {
      tmp += '.';
    } else if (*i == '\\') {
      tmp += "\\\\";
    } else if (*i == '.') {
      if (i == mm.begin())
	tmp += ".";
      else
	tmp += "\\.";
    } else {
      tmp += *i;
    }

    ++i;
  }

  return tmp;
}

//--------------------------------------------------------------------
string IMAPdirDepot::filenameToMailbox(const string &m) const
{
  string tmp;
  bool escape = false;

  // hide the magic "." mailbox.
  if (m == "." || m == "..")
    return "";

  string::const_iterator i = m.begin();
  while (i != m.end()) {
    if (*i == '.') {
      if (i != m.begin() && !escape) tmp += delimiter;
      else if (i == m.begin() || escape) tmp += '.';
      escape = false;
    } else if (*i == '\\') {
      if (!escape) escape = true; else {
	tmp += '\\';
	escape = false;
      }
    } else if (*i == delimiter) {
      return "";
    } else {
      if (escape) return "";
      else {
	tmp += *i;
	escape = false;
      }
    }

    ++i;
  }

  return tmp;
}
