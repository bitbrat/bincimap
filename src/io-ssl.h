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
#ifndef io_ssl_h_included
#define io_ssl_h_included
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WITH_SSL

#include <string>

#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "io.h"

namespace Binc {

  class SSLEnabledIO : public IO {
  public:
    enum {
      MODE_PLAIN = 0x01,
      MODE_SSL = 0x04,
      MODE_SYSLOG = 0x02
    };

    bool isModeSSL(void);
    bool setModeSSL(void);

    void writeStr(const std::string s);
    int fillBuffer(int timeout, bool retry);

    int pending(void) const;

    //--
    SSLEnabledIO(void);
    SSLEnabledIO(FILE *);
    ~SSLEnabledIO(void);

  private:
    SSL *ssl;
    SSL_CTX *ctx;

  };
}

#endif
#endif
