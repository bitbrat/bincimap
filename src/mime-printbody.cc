/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    mime-printbody.cc
 *  
 *  Description:
 *    Implementation of main mime parser components
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

#include "mime.h"
#include "mime-utils.h"
#include "convert.h"
#include "io.h"
#include <string>
#include <vector>
#include <map>
#include <exception>
#include <iostream>

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

using namespace ::std;

//------------------------------------------------------------------------
void Binc::MimePart::printBody(int fd, IO &output, unsigned int startoffset,
			      unsigned int length) const
{
  if (crlffile != fd) {
    crlffile = fd;
    crlfoffset = 1;
    crlfSeek(0);
  }

  crlfSeek(bodystartoffsetcrlf + startoffset);

  if (startoffset + length > bodylength)
    length = bodylength - startoffset;

  char c = '\0';
  for (unsigned int i = 0; i < length; ++i) {
    if (!crlfGetChar(c))
      break;

    output << (char)c;
  }
}
