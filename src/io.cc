/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/io/io.h
 *  
 *  Description:
 *    Implementation of the IO class.
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

#include <string>

#include "session.h"
#include "io.h"


using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
IO::IO()
{
  pid = getpid();
  enabled = true;
  flushesOnEndl = true;
  mode = MODE_PLAIN;
  useLogPrefix = false;
  seqnr = 0;
  buffersize = 8192;
  transfertimeout = 0;
  inputsize = 0;
  inputlimit = true;
}

//------------------------------------------------------------------------
IO::IO(FILE *fp)
{
  pid = getpid();
  setFd(fp);
  enabled = true;
  flushesOnEndl = true;
  mode = MODE_PLAIN;
  useLogPrefix = false;
  seqnr = 0;
  buffersize = 8192;
  transfertimeout = 0;
  inputsize = 0;
  inputlimit = true;
}

//------------------------------------------------------------------------
IO::~IO(void)
{
  switch (mode) {
  case MODE_SYSLOG:
    closelog();
    break;
  default:
    break;
  }
}

//------------------------------------------------------------------------
void IO::enable(void)
{
  enabled = true;
}

//------------------------------------------------------------------------
void IO::enableLogPrefix(void)
{
  useLogPrefix = true;
}

//------------------------------------------------------------------------
void IO::disable(void)
{
  enabled = false;
}

//------------------------------------------------------------------------
void IO::flushContent(void)
{
  if (!enabled) return;

  string s = outputBuffer.str();
  string tmpstr;
  if (mode == MODE_SYSLOG) {
    tmpstr = "";
    for (string::iterator i = s.begin(); i != s.end(); ++i)
      if (*i != '\r' && *i != '\n') {
	tmpstr += *i;
      } else {
	writeStr(tmpstr);
	tmpstr = "";
      }

    if (tmpstr != "")
      writeStr(tmpstr);
  } else
    writeStr(s);


  outputBuffer.clear();
}

//------------------------------------------------------------------------
void IO::flushOnEndl(void)
{
  flushesOnEndl = true;
}

//------------------------------------------------------------------------
void IO::noFlushOnEndl(void)
{
  flushesOnEndl = false;
}

//------------------------------------------------------------------------
void IO::clear(void)
{
  if (!enabled) return;

  outputBuffer.clear();
}

//------------------------------------------------------------------------
void IO::setModeSyslog(const string &servicename, int facility = LOG_DAEMON)
{
  static string sname;
  sname = servicename;
  if (mode != MODE_SYSLOG) {
    openlog(sname.c_str(), LOG_PID, facility);
    mode = MODE_SYSLOG;
  }
}

//------------------------------------------------------------------------
void IO::setFd(FILE *fp)
{
  fpout = fp;
}

//------------------------------------------------------------------------
void IO::resetInput(void)
{
  inputsize = 0;
}

//------------------------------------------------------------------------
void IO::enableInputLimit(void)
{
  inputlimit = true;
}

//------------------------------------------------------------------------
void IO::disableInputLimit(void)
{
  inputlimit = false;
}

//------------------------------------------------------------------------
IO &IO::operator << (std::ostream &(*man)(std::ostream &))
{ 
  if (!enabled) return *this;
  
  if (useLogPrefix && outputBuffer.getSize() == 0) {
    outputBuffer << pid;
    outputBuffer << " ";
    outputBuffer << seqnr++;
    if (logprefix != "")
      outputBuffer << " [" << logprefix << "]";
    outputBuffer << " ";
  }

  static std::ostream &(*endl_funcpt)(std::ostream&) = std::endl;
  
  if (man == endl_funcpt) {
    outputBuffer << "\r\n";
    
    if (flushesOnEndl)
      flushContent();
  } else
    outputBuffer << man;
  
  if ((int) outputBuffer.getSize() >= buffersize)
    flushContent();

  return *this;
}

//------------------------------------------------------------------------
int IO::select(int maxfd,
	       fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	       int &timeout)
{
  if (timeout == -1) return 1;
  struct timeval t1;
  struct timeval t2;
  int res;

  if (gettimeofday(&t1, 0) == -1) {
    setLastError("Failure in select: gettimeofday failed.");
    return -1;
  }

  do {
    struct timeval t;
    t.tv_sec = timeout;
    t.tv_usec = 0;
    res = ::select(maxfd, readfds, writefds, exceptfds, timeout ? &t : 0);

    if (gettimeofday(&t2, 0) == -1) {
      setLastError("Failure in select: gettimeofday failed.");
      return -1;
    }

    timeout = 1000*(t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec)/1000;

    if (timeout < 0) timeout = 0;
    if (res == 0) return 0;
    if (res > 0) return res;
  } while (res == -1 && errno == EINTR);
  return -1;
}

//------------------------------------------------------------------------
void IO::writeStr(const string s)
{
  if (s == "")
    return;
  
  if (mode == MODE_PLAIN) {
    alarm(transfertimeout);
    int n = fwrite(s.c_str(), 1, s.length(), fpout);
    fflush(fpout);
    alarm(0);
    
    if (n != (int) s.length()) {
      // ignore error
      return;
    }

    Session::getInstance().addWriteBytes(n);

  } else if (mode == MODE_SYSLOG) {
    alarm(transfertimeout);
    syslog(LOG_INFO, "%s", s.c_str());
    alarm(0);
  } else {
    // ignore error
    return;
  }
}

//------------------------------------------------------------------------
int IO::readChar(int timeout, bool retry)
{
  string s;
  int ret = readStr(s, 1, timeout, retry);
  if (ret == 1)
    return s[0];

  return ret;
}

//------------------------------------------------------------------------
int IO::fillBuffer(int timeout, bool retry)
{
  // Fill the input buffer with data
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);

  int r = select(fileno(stdin) + 1, &rfds, 0, 0, timeout);
  if (r == 0) {
    setLastError("client timed out");
    return -2;
  } else if (r == -1) {
    setLastError("error reading from client");
    return -1;
  }
  
  char buf[1024];
  int readBytes = read(fileno(stdin), buf, sizeof(buf));
  if (readBytes <= 0) {
    setLastError("client disconnected");
    return -1;
  }

  Session::getInstance().addReadBytes(readBytes);
  
  for (int i = 0; i < readBytes; ++i)
    inputBuffer.push_front(buf[i]);
  
  return readBytes;
}

//------------------------------------------------------------------------
int IO::readStr(string &data, int bytes, int timeout, bool retry)
{
  data = "";

  bool second = false;
  for (;;) {
    // First, empty data from the input buffer
    while (inputBuffer.size() > 0) {
      if (bytes != -1 && data.length() == (unsigned int) bytes)
	break;

      data += inputBuffer.back();
      inputBuffer.pop_back();
    }

    // If bytes == -1, this means we want to fill the buffer once
    // more, then empty this over in data, then finally return.
    if (bytes == -1 && second)
      return data.length();

    // If we got as much as we wanted, return.
    if (bytes != -1 && data.length() == (unsigned int) bytes)
      return bytes;

    // If we reached the input limit, abort.
    if (inputlimit && data.length() >= 8192) {
      setLastError("input limit is 8192 characters.");
      return -1;
    }

    // Fill the buffer with data, 
    int ret = fillBuffer(timeout, retry);
    if (ret < 0)
      return ret;

    second = true;
  }

  return data.length();
}

//------------------------------------------------------------------------
void IO::unReadChar(int c_in) 
{
  inputBuffer.push_back(c_in);
}

//------------------------------------------------------------------------
void IO::unReadChar(const string &s_in)
{
  if (s_in.length() != 0) {
    int c = s_in.length() - 1;
    while (c >= 0) {
      unReadChar(s_in[c]);
      --c;
    }
  }
}

//------------------------------------------------------------------------
int IO::pending(void) const
{
  return 0;
}

//------------------------------------------------------------------------
const string &IO::getLastError(void) const
{
  return lastError;
}

//------------------------------------------------------------------------
void IO::setLastError(const string &e)
{
  lastError = e;
}

//------------------------------------------------------------------------
IOFactory::IOFactory(void)
{
}

//------------------------------------------------------------------------
IOFactory::~IOFactory(void)
{
}

//------------------------------------------------------------------------
IOFactory &IOFactory::getInstance(void)
{
  static IOFactory iofactory;
  return iofactory;
}

//------------------------------------------------------------------------
void IOFactory::assign(int id, IO *io)
{
  ioers[id] = io;
}

//------------------------------------------------------------------------
IO &IOFactory::get(int id)
{
  if (ioers.find(id) == ioers.end()) {
    static IO NIL;
    return NIL;
  }

  return *ioers[id];
}
