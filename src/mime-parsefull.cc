/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    mime-parsefull.cc
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

int crlffile = 0;
char crlfdata[4096];
unsigned int crlftail = 0;
unsigned int crlfhead = 0;
unsigned int crlfoffset = 0;
char lastchar = '\0';

//------------------------------------------------------------------------
bool fillInputBuffer(void)
{
  char raw[1024];
  
  ssize_t nbytes;
  for (;;) {
    nbytes = read(crlffile, raw, sizeof(raw) - 1);
    if (nbytes <= 0) {
      // FIXME: If ferror(crlffile) we should log this.
      return false;
    }
    else break;
  }
  
  for (ssize_t i = 0; i < nbytes; ++i) {
    const char c = raw[i];
    switch (c) {
    case '\r':
      if (lastchar == '\r') {
	crlfdata[crlftail++ & 0xfff] = '\r';
	crlfdata[crlftail++ & 0xfff] = '\n';
      }
      break;
    case '\n':
      crlfdata[crlftail++ & 0xfff] = '\r';
      crlfdata[crlftail++ & 0xfff] = '\n';
      break;
    default:
      if (lastchar == '\r') {
	crlfdata[crlftail++ & 0xfff] = '\r';
	crlfdata[crlftail++ & 0xfff] = '\n';
      }

      crlfdata[crlftail++ & 0xfff] = c;
      break;
    }
      
    lastchar = c;
  }

  return true;
}


//------------------------------------------------------------------------
void Binc::MimeDocument::parseFull(int fd) const
{
  if (allIsParsed)
    return;

  allIsParsed = true;

  crlffile = fd;
  crlfReset();

  headerstartoffsetcrlf = 0;
  headerlength = 0;
  bodystartoffsetcrlf = 0;
  bodylength = 0;
  size = 0;
  messagerfc822 = false;
  multipart = false;

  int bsize = 0;
  MimePart::parseFull("", bsize);

  // eat any trailing junk to get the correct size
  char c;
  while (crlfGetChar(c));

  size = crlfoffset;
}

//------------------------------------------------------------------------
int Binc::MimePart::parseFull(const string &toboundary, int &boundarysize) const
{
  string name;
  string content;
  char cqueue[4];
  memset(cqueue, 0, sizeof(cqueue));

  bool quit = false;
  char c;
  bool eof = false;

  headerstartoffsetcrlf = crlfoffset;

  while (!quit && !eof) {
    // read name
    while (1) {
      if (!crlfGetChar(c)) {
	eof = true;
	break;
      }

      if (c == '\n') ++nlines;
      if (c == ':') break;
      if (c == '\n') {
	// If we encounter a \n before we got to the first ':', then
	// if the line is not empty, rewind back to the start of the
	// line and assume we're at the start of the body. If not,
	// just skip the line and assume we're at the start of the
	// body.
	string ntmp = name;
	trim(ntmp);
	if (ntmp != "")
	  for (int i = name.length() - 1; i >= 0; --i)
	    crlfUnGetChar();

	quit = true;
	name = "";
	break;
      }

      name += c;

      if (name.length() == 2 && name.substr(0, 2) == "\r\n") {
	name = "";
	quit = true;
	break;
      }
    }

    if (name.length() == 1 && name[0] == '\r') {
      name = "";
      break;
    }

    if (quit || eof) break;

    while (!quit) {
      if (!crlfGetChar(c)) {
	quit = true;
	break;
      }

      if (c == '\n') ++nlines;

      for (int i = 0; i < 3; ++i)
	cqueue[i] = cqueue[i + 1];
      cqueue[3] = c;

      if (strncmp(cqueue, "\r\n\r\n", 4) == 0) {
	quit = true;
	break;
      }

      if (cqueue[2] == '\n') {

	// guess the mime rfc says what can not appear on the beginning
	// of a line.
	if (!isspace(cqueue[3])) {
	  if (content.length() > 2)
	    content.resize(content.length() - 2);

	  trim(content);
	  h.add(name, content);

	  name = c;
	  content = "";
	  break;
	}
      }

      content += c;
    }
  }

  if (name != "") {
    if (content.length() > 2)
      content.resize(content.length() - 2);
    h.add(name, content);
  }

  // Headerlength includes the seperating CRLF. Body starts after the
  // CRLF.
  headerlength = crlfoffset - headerstartoffsetcrlf;
  bodystartoffsetcrlf = crlfoffset;

  // If we encounter the end of file, we return 1 as if we found our
  // parent's terminal boundary. This will cause a safe exit, and
  // whatever we parsed until now will be available.
  if (eof)
    return 1;

  // Do simple parsing of headers to determine the
  // type of message (multipart,messagerfc822 etc)
  HeaderItem ctype;
  if (h.getFirstHeader("content-type", ctype)) {
    vector<string> types;
    split(ctype.getValue(), ";", types);

    if (types.size() > 0) {
      // first element should describe content type
      string tmp = types[0];
      trim(tmp);
      vector<string> v;
      split(tmp, "/", v);
      string key, value;

      key = (v.size() > 0) ? v[0] : "text";
      value = (v.size() > 1) ? v[1] : "plain";
      lowercase(key);
      
      if (key == "multipart") {
	multipart = true;
	lowercase(value);
	subtype = value;
      } else if (key == "message") {
	lowercase(value);
	if (value == "rfc822")
	  messagerfc822 = true;
      }
    }

    for (vector<string>::const_iterator i = types.begin();
	 i != types.end(); ++i) {
      string element = *i;
      trim(element);

      if (element.find("=") != string::npos) {
	string::size_type pos = element.find('=');
	string key = element.substr(0, pos);
	string value = element.substr(pos + 1);
	
	lowercase(key);
	trim(key);

	if (key == "boundary") {
	  trim(value, " \"");
	  boundary = value;
	}
      }
    }
  }

  bool foundendofpart = false;
  if (messagerfc822) {
    // message rfc822 means a completely enclosed mime document. we
    // call the parser recursively, and pass on the boundary string
    // that we got. when parse() finds this boundary, it returns 0. if
    // it finds the end boundary (boundary + "--"), it returns != 0.
    MimePart m;

    // parsefull returns the number of bytes that need to be removed
    // from the body because of the terminating boundary string.
    int bsize;
    if (m.parseFull(toboundary, bsize))
      foundendofpart = true;

    // make sure bodylength doesn't overflow    
    bodylength = crlfoffset;
    if (bodylength > bodystartoffsetcrlf) {
      bodylength -= bodystartoffsetcrlf;
      if (bodylength > (unsigned int) bsize) {
	bodylength -= (unsigned int) bsize;
      } else {
	bodylength = 0;
      }
    } else {
      bodylength = 0;
    }

    nbodylines += m.getNofLines();

    members.push_back(m);

  } else if (multipart) {
    // multipart parsing starts with skipping to the first
    // boundary. then we call parse() for all parts. the last parse()
    // command will return a code indicating that it found the last
    // boundary of this multipart. Note that the first boundary does
    // not have to start with CRLF.
    string delimiter = "--" + boundary;

    char *delimiterqueue = 0;
    int endpos = delimiter.length();
    delimiterqueue = new char[endpos];
    int delimiterpos = 0;
    bool eof = false;

    // first, skip to the first delimiter string. Anything between the
    // header and the first delimiter string is simply ignored (it's
    // usually a text message intended for non-mime clients)
    do {    
      if (!crlfGetChar(c)) {
	eof = true;
	break;
      }

      if (c == '\n')
	++nlines;

      delimiterqueue[delimiterpos++ % endpos] = c;

      // Fixme: Must also check for all parents' delimiters.
    } while (!compareStringToQueue(delimiter, delimiterqueue, delimiterpos, endpos));

    delete delimiterqueue;

    if (!eof)
      boundarysize = delimiter.size();

    // Read two more characters. This may be CRLF, it may be "--" and
    // it may be any other two characters.
    char a;
    if (!crlfGetChar(a))
      eof = true;

    if (a == '\n')
      ++nlines; 

    char b;
    if (!crlfGetChar(b))
      eof = true;
    
    if (b == '\n')
      ++nlines;
    
    // If we find two dashes after the boundary, then this is the end
    // of boundary marker.
    if (!eof) {
      if (a == '-' && b == '-') {
	foundendofpart = true;
	boundarysize += 2;
	
	if (!crlfGetChar(a))
	  eof = true;
	
	if (a == '\n')
	  ++nlines; 
	
	if (!crlfGetChar(b))
	  eof = true;
	
	if (b == '\n')
	  ++nlines;
      }

      if (a == '\r' && b == '\n') {
	// This exception is to handle a special case where the
	// delimiter of one part is not followed by CRLF, but
	// immediately followed by a CRLF prefixed delimiter.
	if (!crlfGetChar(a) || !crlfGetChar(b))
	  eof = true; 
	else if (a == '-' && b == '-') {
	  crlfUnGetChar();
	  crlfUnGetChar();
	  crlfUnGetChar();
	  crlfUnGetChar();
	} else {
	  crlfUnGetChar();
	  crlfUnGetChar();
	}

	boundarysize += 2;
      } else {
	crlfUnGetChar();
	crlfUnGetChar();
      }
    }

    // make sure bodylength doesn't overflow    
    bodylength = crlfoffset;
    if (bodylength > bodystartoffsetcrlf) {
      bodylength -= bodystartoffsetcrlf;
      if (bodylength > (unsigned int) boundarysize) {
	bodylength -= (unsigned int) boundarysize;
      } else {
	bodylength = 0;
      }
    } else {
      bodylength = 0;
    }

    // read all mime parts.
    if (!foundendofpart && !eof) {
      bool quit = false;
      do {
	MimePart m;

	// If parseFull returns != 0, then it encountered the multipart's
	// final boundary.
	int bsize = 0;
	if (m.parseFull(boundary, bsize)) {
	  quit = true;
	  boundarysize = bsize;
	}

	members.push_back(m);

      } while (!quit);
    }

    if (!foundendofpart && !eof) {
      // multipart parsing starts with skipping to the first
      // boundary. then we call parse() for all parts. the last parse()
      // command will return a code indicating that it found the last
      // boundary of this multipart. Note that the first boundary does
      // not have to start with CRLF.
      string delimiter = "\r\n--" + toboundary;

      char *delimiterqueue = 0;
      int endpos = delimiter.length();
      delimiterqueue = new char[endpos];
      int delimiterpos = 0;
      bool eof = false;

      // first, skip to the first delimiter string. Anything between the
      // header and the first delimiter string is simply ignored (it's
      // usually a text message intended for non-mime clients)
      do {    
	if (!crlfGetChar(c)) {
	  eof = true;
	  break;
	}

	if (c == '\n')
	  ++nlines;

	delimiterqueue[delimiterpos++ % endpos] = c;

	// Fixme: Must also check for all parents' delimiters.
      } while (!compareStringToQueue(delimiter, delimiterqueue, delimiterpos, endpos));

      delete delimiterqueue;

      if (!eof)
	boundarysize = delimiter.size();

      // Read two more characters. This may be CRLF, it may be "--" and
      // it may be any other two characters.
      char a;
      if (!crlfGetChar(a))
	eof = true;

      if (a == '\n')
	++nlines; 

      char b;
      if (!crlfGetChar(b))
	eof = true;
    
      if (b == '\n')
	++nlines;
    
      // If we find two dashes after the boundary, then this is the end
      // of boundary marker.
      if (!eof) {
	if (a == '-' && b == '-') {
	  foundendofpart = true;
	  boundarysize += 2;
	
	  if (!crlfGetChar(a))
	    eof = true;
	
	  if (a == '\n')
	    ++nlines; 
	
	  if (!crlfGetChar(b))
	    eof = true;
	
	  if (b == '\n')
	    ++nlines;
	}

	if (a == '\r' && b == '\n') {
	  // This exception is to handle a special case where the
	  // delimiter of one part is not followed by CRLF, but
	  // immediately followed by a CRLF prefixed delimiter.
	  if (!crlfGetChar(a) || !crlfGetChar(b))
	    eof = true; 
	  else if (a == '-' && b == '-') {
	    crlfUnGetChar();
	    crlfUnGetChar();
	    crlfUnGetChar();
	    crlfUnGetChar();
	  } else {
	    crlfUnGetChar();
	    crlfUnGetChar();
	  }

	  boundarysize += 2;
	} else {
	  crlfUnGetChar();
	  crlfUnGetChar();
	}
      }
    }

  } else {
    // If toboundary is empty, then we read until the end of the
    // file. Otherwise we will read until we encounter toboundary.
    string _toboundary; 
    if (toboundary != "") {
      _toboundary = "\r\n--";
      _toboundary += toboundary;
    }

    char *boundaryqueue = 0;
    int endpos = _toboundary.length();
    if (toboundary != "")
      boundaryqueue = new char[endpos];
    int boundarypos = 0;

    boundarysize = 0;

    string line;
    int nchars = 0;
    while (crlfGetChar(c)) {
      if (c == '\n') { ++nbodylines; ++nlines; }
      nchars++;

      if (toboundary == "")
	continue;

      // find boundary
      boundaryqueue[boundarypos++ % endpos] = c;
      
      if (compareStringToQueue(_toboundary, boundaryqueue, boundarypos, endpos)) {
	boundarysize = _toboundary.length();
	break;
      }
    }

    delete boundaryqueue;
 
    if (toboundary != "") {
      char a;
      if (!crlfGetChar(a))
	eof = true;

      if (a == '\n')
	++nlines;
      char b;
      if (!crlfGetChar(b))
	eof = true;

      if (b == '\n') 
	++nlines;

      if (a == '-' && b == '-') {
	boundarysize += 2;
	foundendofpart = true;
	if (!crlfGetChar(a))
	  eof = true;

	if (a == '\n')
	  ++nlines;

	if (!crlfGetChar(b))
	  eof = true;
	  
	if (b == '\n')
	  ++nlines;
      }

      if (a == '\r' && b == '\n') {
	// This exception is to handle a special case where the
	// delimiter of one part is not followed by CRLF, but
	// immediately followed by a CRLF prefixed delimiter.
	if (!crlfGetChar(a) || !crlfGetChar(b))
	  eof = true; 
	else if (a == '-' && b == '-') {
	  crlfUnGetChar();
	  crlfUnGetChar();
	  crlfUnGetChar();
	  crlfUnGetChar();
	} else {
	  crlfUnGetChar();
	  crlfUnGetChar();
	}

	boundarysize += 2;
      } else {
	crlfUnGetChar();
	crlfUnGetChar();
      }
    }

    // make sure bodylength doesn't overflow    
    bodylength = crlfoffset;
    if (bodylength > bodystartoffsetcrlf) {
      bodylength -= bodystartoffsetcrlf;
      if (bodylength > (unsigned int) boundarysize) {
	bodylength -= (unsigned int) boundarysize;
      } else {
	bodylength = 0;
      }
    } else {
      bodylength = 0;
    }
  }

  return (eof || foundendofpart) ? 1 : 0;
}
