/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildir-scanfilesnames.cc
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

#include <iostream>
#include <iomanip>

#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#include "io.h"

using namespace ::std;

//------------------------------------------------------------------------
bool Binc::Maildir::scanFileNames(void) const
{
  string curpath = path + "/cur/";
  DIR *pdir = opendir(curpath.c_str());
  if (pdir == 0) {
    setLastError("when scanning mailbox \""
		 + path + "\": " + string(strerror(errno)));
    IO &logger = IOFactory::getInstance().get(2);
    logger << getLastError() << endl;
    return false;
  }

  index.clearFileNames();

  struct dirent *pdirent;
  while ((pdirent = readdir(pdir)) != 0) {
    if (!isdigit(pdirent->d_name[0])) continue;

    string filename = pdirent->d_name;
    string uniquename;

    string::size_type pos;
    if ((pos = filename.find(':')) == string::npos)
      uniquename = filename;
    else
      uniquename = filename.substr(0, pos);

    index.insert(uniquename, 0, pdirent->d_name);
  }

  closedir(pdir);
  return true;
}
