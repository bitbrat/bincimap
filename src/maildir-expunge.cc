/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildir-expunge.cc
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

#include "io.h"
#include "maildir.h"
#include "maildirmessage.h"

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
void Maildir::expungeMailbox(void)
{
  if (readOnly) return;

  Mailbox::iterator i = begin(SequenceSet::all(), SQNR_MODE|INCLUDE_EXPUNGED);

  bool success = true;
  for (; success && i != end(); ++i) {
    MaildirMessage &message = reinterpret_cast<MaildirMessage &>(*i);

    if ((message.getStdFlags() & Message::F_DELETED) == 0)
      continue;

    message.setExpunged();

    const string &id = message.getUnique();

    // The message might be gone already
    MaildirIndexItem *item = index.find(id);
    if (!item)
      continue;

    string fpath = path + "/cur/" + item->fileName;

    while (unlink(fpath.c_str()) != 0) {
      if (errno != ENOENT) {
	IO &logger = IOFactory::getInstance().get(2);
	logger << "unable to remove " << fpath << ": "
	       << strerror(errno) << endl;
	break;
      }

      if (!scanFileNames()) {
	success = false;
	break;
      }

      if ((item = index.find(id)))
	break;
      else
	fpath = path + "/cur/" + item->fileName;
    }
  }
}
