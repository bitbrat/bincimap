/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    tools.cc
 *  
 *  Description:
 *    Implementation of miscellaneous tools.
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
#include <errno.h>

#include "tools.h"

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
Tools::Tools(void)
{
}

//------------------------------------------------------------------------
Tools &Tools::getInstance(void)
{
  static Tools tools;
  return tools;
}

//------------------------------------------------------------------------
void Tools::setenv(const string &key, const string &value) const
{
  char *c = strdup((key + "=" + value).c_str());
  putenv(c);
}

//------------------------------------------------------------------------
string Tools::getenv(const string &key) const
{
  static const string NIL = "";

  const char *c = ::getenv((char *)key.c_str());
  if (c == 0)
    return NIL;
  else
    return string(c);
}
