/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    base64.cc
 *  
 *  Description:
 *    Implementation of base64 Utilities
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

#include "base64.h"
#include <string>
#include <iostream>

using namespace ::std;

typedef unsigned char byte;	      /* Byte type */

#define TRUE  1
#define FALSE 0

#define LINELEN 72		      /* Encoded line length (max 76) */

static byte dtable[256];

string Binc::base64encode(const string &s_in)
{
  int i;
  string result;

  /*	Fill dtable with character encodings.  */

  for (i = 0; i < 26; i++) {
    dtable[i] = 'A' + i;
    dtable[26 + i] = 'a' + i;
  }
  for (i = 0; i < 10; i++) {
    dtable[52 + i] = '0' + i;
  }
  dtable[62] = '+';
  dtable[63] = '/';

  string::const_iterator s_i = s_in.begin();
  while (s_i != s_in.end()) {

    byte igroup[3], ogroup[4];
    int c, n;

    igroup[0] = igroup[1] = igroup[2] = 0;
    for (n = 0; n < 3 && s_i != s_in.end(); n++) {
      c = *s_i++;
      igroup[n] = (byte) c;
    }
    if (n > 0) {
      ogroup[0] = dtable[igroup[0] >> 2];
      ogroup[1] = dtable[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
      ogroup[2] = dtable[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)];
      ogroup[3] = dtable[igroup[2] & 0x3F];

      /* Replace characters in output stream with "=" pad
	 characters if fewer than three characters were
	 read from the end of the input stream. */

      if (n < 3) {
	ogroup[3] = '=';
	if (n < 2) {
	  ogroup[2] = '=';
	}
      }

      for (i = 0; i < 4; i++)
	result += ogroup[i];
    }
  }

  return result;
}

string Binc::base64decode(const string &s_in)
{
  string result;
  int i;

  for (i = 0; i < 255; i++) {
    dtable[i] = 0x80;
  }
  for (i = 'A'; i <= 'Z'; i++) {
    dtable[i] = 0 + (i - 'A');
  }
  for (i = 'a'; i <= 'z'; i++) {
    dtable[i] = 26 + (i - 'a');
  }
  for (i = '0'; i <= '9'; i++) {
    dtable[i] = 52 + (i - '0');
  }
  dtable[(int) '+'] = 62;
  dtable[(int) '/'] = 63;
  dtable[(int) '='] = 0;

  /*CONSTANTCONDITION*/
  string::const_iterator s_i = s_in.begin();
  while (s_i != s_in.end()) {
    byte a[4], b[4], o[3];

    for (i = 0; i < 4 && s_i != s_in.end(); i++) {
      int c = *s_i++;

      if (dtable[c] & 0x80)
	return result;

      a[i] = (byte) c;
      b[i] = (byte) dtable[c];
    }

    o[0] = (b[0] << 2) | (b[1] >> 4);
    o[1] = (b[1] << 4) | (b[2] >> 2);
    o[2] = (b[2] << 6) | b[3];

    i = a[2] == '=' ? 1 : (a[3] == '=' ? 2 : 3);

    for (int j = 0; j < i; ++j)
      result += o[j];

    if (i < 3)
      break;
  }

  return result;
}
