/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildir-writecache.cc
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
#include "storage.h"

using namespace ::std;

//------------------------------------------------------------------------
bool Binc::Maildir::writeCache(void)
{
  if (readOnly)
    return true;

  const string cachefilename = path + "/bincimap-cache";
  const string uidvalfilename = path + "/bincimap-uidvalidity";

  Storage cache(cachefilename, Storage::WriteOnly);
  int n = 0;

  Mailbox::iterator i = begin(SequenceSet::all(), INCLUDE_EXPUNGED);
  for (; i != end(); ++i) {
    MaildirMessage &message = (MaildirMessage &)*i;
    string nstr = toString(n++);

    cache.put(nstr, "_UID", toString(message.getUID()));
    if (message.getSize() != 0)
      cache.put(nstr, "_Size", toString(message.getSize()));
    
    cache.put(nstr, "_ID", message.getUnique());
    cache.put(nstr, "_InternalDate",
	      toString((int) message.getInternalDate()));
  }

  cache.put("depot", "_version", CACHEFILEVERSION);
  if (!cache.commit()) {
    setLastError(cache.getLastError());
    return false;
  }

  return true;
}
