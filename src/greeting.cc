/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    greeting.cc
 *  
 *  Description:
 *    Implementation of the inital greeting.
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

#include <time.h>

#include "io.h"
#include "session.h"

using namespace ::std;
using namespace Binc;

static const unsigned int ISO8601SIZE = 32;

namespace Binc {
  void showGreeting(void);
};


//------------------------------------------------------------------------
void Binc::showGreeting(void)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  time_t t = time(0);
  struct tm *mytm = localtime(&t);

  char mytime[ISO8601SIZE];
  unsigned int size = strftime(mytime, sizeof(mytime),
			       "%Y-%m-%d %H:%M:%S %Z", mytm);
  if (size >= sizeof(mytime))
    mytime[0] = 0;

  string version;
  string tmp = session.globalconfig["Security"]["version in greeting"];
  lowercase(tmp);
  if (tmp == "yes")
    version = "v"VERSION" ";

  com << "* OK Welcome to Binc IMAP " << version 
      << "Copyright (C) 2002-2004 Andreas Aardal Hanssen at "
      << mytime << endl;
}
