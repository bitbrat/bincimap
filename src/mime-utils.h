/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    mime.cc
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
#ifndef mime_utils_h_included
#define mime_utils_h_included

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

#include "io.h"

using namespace ::std;

inline bool compareStringToQueue(const std::string &s_in, 
				 char *bqueue, int pos, int size)
{
  if (s_in[0] != bqueue[pos % size]) return false;

  for (int i = 0; i < size; ++i)
    if (s_in.at(i) != bqueue[(pos + i) % size])
      return false;

  return true;
}

extern int crlffile;
extern char crlfdata[];
extern unsigned int crlfoffset;
extern unsigned int crlftail;
extern unsigned int crlfhead;
extern char lastchar;

bool fillInputBuffer(void);

inline bool crlfGetChar(char &c)
{
  if (crlfhead == crlftail && !fillInputBuffer())
    return false;

  c = crlfdata[crlfhead++ & 0xfff];
  ++crlfoffset;
  return true;
}

inline void crlfUnGetChar(void)
{
  --crlfhead;
  --crlfoffset;
}

inline void crlfReset(void)
{
  if (crlfoffset != 0) {
    crlfoffset = 0;
    crlfhead = crlftail = 0;
    lastchar = '\0';
    lseek(crlffile, 0, SEEK_SET);
  }
}

inline void crlfSeek(unsigned int offset)
{
  if (crlfoffset > offset)
    crlfReset();
   
  char c;
  int n = 0;
  while (offset > crlfoffset) {
    if (!crlfGetChar(c))
      break;
    ++n;
  }
}
#endif
