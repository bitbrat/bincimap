/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildir-updateflags.cc
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

#include <dirent.h>
#include <unistd.h>

#include "io.h"

using namespace ::std;

//------------------------------------------------------------------------
void Binc::Maildir::updateFlags(void)
{
  IO &logger = IOFactory::getInstance().get(2);

  if (readOnly) return;

  string curpath = path + "/cur/";
  DIR *pdir = opendir(curpath.c_str());
  if (pdir == 0) {
    string reason = "failed to open " + curpath + ": ";
    reason += strerror(errno);

    logger << reason << endl;
    return;
  }
  
  struct dirent *pdirent;
  while ((pdirent = readdir(pdir)) != 0) {
    string filename = pdirent->d_name;
    if (filename[0] == '.')
      continue;

    string uniquename;
    string::size_type pos;
    if ((pos = filename.find(":2,")) != string::npos)
      uniquename = filename.substr(0, pos);
    else
      uniquename = filename;    

    MaildirMessage *message = get(uniquename);
    if (message) {
      string flags;
      int mflags = message->getStdFlags();
      if (mflags & Message::F_DRAFT) flags += "D";
      if (mflags & Message::F_FLAGGED) flags += "F";
      if (mflags & Message::F_ANSWERED) flags += "R";
      if (mflags & Message::F_SEEN) flags += "S";
      if (mflags & Message::F_DELETED) flags += "T";

      string srcname = curpath + filename;
      string destname = curpath + uniquename + ":2," + flags;
     
      if (srcname != destname) {
	if (rename(srcname.c_str(), destname.c_str()) != 0) {
	  if (errno == ENOENT) {
	    // FIXME: restart scan
	  }

	  logger << "warning: rename(" << srcname
		 << "," << destname << ") == "
		 << errno << ": " << strerror(errno) << endl;
	} else {
	  index.insert(uniquename, 0,  uniquename + ":2," + flags);
	}
      }
   
      continue;
    }
  }

  closedir(pdir);
}
