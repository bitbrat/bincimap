/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildir-delete.cc
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

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <unistd.h>

using namespace ::std;
using namespace Binc;

namespace {

  bool recursiveDelete(const string &path)
  {
    DIR *mydir = opendir(path.c_str());
    if (mydir == 0)
      return false;

    struct dirent *mydirent;
    while ((mydirent = readdir(mydir)) != 0) {
      string d = mydirent->d_name;
      if (d == "." || d == "..")
	continue;

      string f = path + "/" + d;

      struct stat mystat;
      if (lstat(f.c_str(), &mystat) != 0) {
	if (errno == ENOENT)
	  continue;
	return false;
      }

      if (S_ISDIR(mystat.st_mode)) {
	if (!recursiveDelete(f)) {
	  closedir(mydir);
	  return false;
	}
	if (rmdir(f.c_str()) != 0 && errno != ENOENT) {
	  closedir(mydir);
	  return false;
	}
      } else {
	if (unlink(f.c_str()) != 0 && errno != ENOENT) {
	  closedir(mydir);
	  return false;
	}
      }
    }

    closedir(mydir);
    return true;
  }
}

//------------------------------------------------------------------------
bool Binc::Maildir::deleteMailbox(const string &s_in)
{
  if (s_in == ".") {
    setLastError("disallowed by rule");
    return false;
  }

  if (!recursiveDelete(s_in)) {
    setLastError("error deleting Maildir - status is undefined");
    return false;
  }

  if (rmdir(s_in.c_str()) != 0) {
    setLastError("error deleting Maildir: " 
		 + string(strerror(errno))
		 + " - status is undefined");
    return false;
  }

  return true;
}
