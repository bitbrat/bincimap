/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    maildir-create.cc
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

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
bool Binc::Maildir::createMailbox(const string &s_in, mode_t mode,
				  uid_t owner, gid_t group, bool root)
{
  if (s_in != "." && mkdir(s_in.c_str(), mode) == -1) {
    setLastError("unable to create " + s_in + ": " 
		 + string(strerror(errno)));
    return false;
  }

  // Allow uidvalidity, which is generated from time(0), to
  // increase with one second to avoid race conditions.
  sleep(1);

  if (mkdir((s_in + "/cur").c_str(), mode) == -1) {
    setLastError("unable to create " + s_in + "/cur: " 
		 + string(strerror(errno)));
    return false;
  }

  if (mkdir((s_in + "/new").c_str(), mode) == -1) {
    setLastError("unable to create " + s_in + "/new: " 
		 + string(strerror(errno)));
    return false;
  }

  if (mkdir((s_in + "/tmp").c_str(), mode) == -1) {
    setLastError("unable to create " + s_in + "/tmp: " 
		 + string(strerror(errno)));
    return false;
  }

  if (owner == 0 && group == 0)
    return true;

  if (chown(s_in.c_str(), owner, group) == -1) {
    setLastError("unable to chown " + s_in + ": " 
		 + string(strerror(errno)));
    return false;
  }

  if (chown((s_in + "/cur").c_str(), owner, group) == -1) {
    setLastError("unable to chown " + s_in + "/cur: "
		 + string(strerror(errno)));
    return false;
  }

  if (chown((s_in + "/new").c_str(), owner, group) == -1) {
    setLastError("unable to chown " + s_in + "/new: "
		 + string(strerror(errno)));
    return false;
  }

  if (chown((s_in + "/tmp").c_str(), owner, group) == -1) {
    setLastError("unable to chown " + s_in + "/tmp: "
		 + string(strerror(errno)));
    return false;
  }

  return true;
}
