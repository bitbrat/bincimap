/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/io/io.h
 *  
 *  Description:
 *    Declaration of the IO class.
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
#ifndef io_h_included
#define io_h_included
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <deque>
#include <string>
#include <iostream>
#include <iomanip>
#include <map>

#include <errno.h>
#include <stdio.h>
#include <syslog.h>

#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "convert.h"

// #define DEBUG

namespace Binc {

  //----------------------------------------------------------------------
  class IO {
  public:
    enum {
      MODE_PLAIN = 0x01,
      MODE_SYSLOG = 0x02
    };

  public:

    //--
    void clear(void);
    void noFlushOnEndl(void);
    void flushOnEndl(void);
    void flushContent(void);
    void resetInput(void);
    void disableInputLimit(void);
    void enableInputLimit(void);

    void enable(void);
    void disable(void);

    void enableLogPrefix(void);
    void setModeSyslog(const std::string &servicename, int facility);
    void setFd(FILE *fpout);

    virtual void writeStr(const std::string s);
    virtual int readChar(int timeout = 0, bool retry = true);
    virtual int readStr(std::string &data, int bytes = -1, int timeout = 0, bool retry = true);
    virtual int fillBuffer(int timeout, bool retry);

    void unReadChar(int c_in);
    void unReadChar(const std::string &s_in);
    virtual int pending(void) const;

    inline void setBufferSize(int s) { buffersize = s; }
    inline void setTransferTimeout(int s) { transfertimeout = s; }
    inline void setLogPrefix(const std::string s_in) { logprefix = s_in; }

    template <class T> IO &operator << (const T &o);
    IO &operator << (std::ostream &(*man)(std::ostream &));

    const std::string &getLastError(void) const;
    void setLastError(const std::string &e);

    //--
    IO();
    IO(FILE *fp);
    virtual ~IO();

  protected:
    BincStream outputBuffer;

    std::deque<char> inputBuffer;
    std::string logprefix;
    int buffersize;
    int transfertimeout;
    int seqnr;
    int pid;
    int mode;

    int inputsize;
    bool inputlimit;

    bool flushesOnEndl;
    bool useLogPrefix;
    bool enabled;

    std::string lastError;

    FILE *fpout;

    //--
    int select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, int &timeout);
  };

  //----------------------------------------------------------------------
  class IOFactory {
  private:
    std::map<int, IO *> ioers;

    //--
    IOFactory(void);

  public:
    void assign(int, IO *);
    IO &get(int);

    //--
    static IOFactory &getInstance(void);
    ~IOFactory(void);
  };
}

//------------------------------------------------------------------------
template <class T> Binc::IO &Binc::IO::operator << (const T &o)
{
  using namespace ::std;
  
  if (useLogPrefix && outputBuffer.getSize() == 0) {
    outputBuffer << pid;
    outputBuffer << " ";
    outputBuffer << seqnr++;
    if (logprefix != "")
      outputBuffer << " [" << logprefix << "]";
    outputBuffer << " ";
  }
  
  outputBuffer << o;
  
  if ((int) outputBuffer.getSize() >= buffersize)
    flushContent();
  
  return *this;
}

#endif
