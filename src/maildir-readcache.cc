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

#include "maildir.h"

#include "io.h"
#include "convert.h"
#include "storage.h"

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
Maildir::ReadCacheResult Maildir::readCache(void)
{
  const string uidvalfilename = path + "/bincimap-uidvalidity";
  const string cachefilename = path + "/bincimap-cache";

  bool uidvalfiledropped = false;

  Storage uidvalfile(uidvalfilename, Storage::ReadOnly);
  string section, key, value;
  string uidvalfileversion;

  uidvalidity = 0;
  uidnext = 0;

  while (uidvalfile.get(&section, &key, &value))
    if (section == "depot" && key == "_uidvalidity")
      uidvalidity = (unsigned int) atoi(value);
    else if (section == "depot" && key == "_uidnext")
      uidnext  = (unsigned int) atoi(value);
    else if (section == "depot" && key == "_version")
      uidvalfileversion = value;

  if (!uidvalfile.eof()) {
    uidnext = 1;
    uidvalidity = time(0);
    uidvalfiledropped = true;
    return NoCache;
  }

  if (uidvalfileversion != UIDVALFILEVERSION 
      || uidvalidity == 0 || uidnext == 0) {
    uidnext = 1;
    uidvalidity = time(0);
    uidvalfiledropped = true;
    return NoCache;
  }

  index.clearUids();

  Storage cache(cachefilename, Storage::ReadOnly);
  string lastsection;
  unsigned int _uid = 0;
  unsigned int _size = 0;
  unsigned int _internaldate = 0;
  string _id;

  while (cache.get(&section, &key, &value)) {
    if (section == "depot" && key == "_version" && value != CACHEFILEVERSION) {
      uidnext = 1;
      uidvalidity = time(0);
      uidvalfiledropped = true;
      return NoCache;
    } else if (isdigit(section[0])) {
      if (lastsection != section) {
	lastsection = section;
	if (_id != "") {
	  MaildirMessage m(*this);
	  m.setUnique(_id);
	  m.setInternalDate(_internaldate);

	  if (index.find(_id) == 0) {
	    m.setUID(_uid);
	    m.setInternalFlag(MaildirMessage::JustArrived);
	    m.setSize(_size);
	    add(m);
	  } else {
	    // Remember to insert the uid of the message again - we reset this
	    // at the top of this function.
	    index.insert(_id, _uid);
	  }
	} 
	 
	_uid = 0;
	_size = 0;
	_internaldate = 0;
	_id = "";
      }

      unsigned int n = (unsigned int)atoi(value);
      if (key == "_ID") _id = value;
      else if (key == "_Size") _size = n;
      else if (key == "_InternalDate") _internaldate = n;
      else if (key == "_UID") _uid = n;
    }
  }

  // Catch the last message too.
  if (index.find(_id) == 0) {
    MaildirMessage m(*this);
    m.setUnique(_id);
    m.setInternalDate(_internaldate);
    m.setUID(_uid);
    m.setInternalFlag(MaildirMessage::JustArrived);
    add(m);
  }

  // Remember to insert the uid of the message again - we reset this
  // at the top of this function.
  index.insert(_id, _uid);

  if (!cache.eof()) {
    // Assume there is no cache file.
    uidnext = 1;
    uidvalidity = time(0);
    uidvalfiledropped = true;
    return NoCache;
  }

  return Ok;
}

