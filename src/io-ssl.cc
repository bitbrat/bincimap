/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    io-ssl.cc
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

#ifdef WITH_SSL

#include <string>

#include <openssl/ssl.h>

#include "session.h"
#include "io-ssl.h"

using namespace ::std;
using namespace Binc;

//------------------------------------------------------------------------
SSLEnabledIO::SSLEnabledIO(void)
{
}

//------------------------------------------------------------------------
SSLEnabledIO::SSLEnabledIO(FILE *fp) : IO(fp)
{
}

//------------------------------------------------------------------------
SSLEnabledIO::~SSLEnabledIO(void)
{
  switch (mode) {
  case MODE_SSL:
    SSL_shutdown(ssl);
    SSL_shutdown(ssl);

    SSL_free(ssl);
    SSL_CTX_free(ctx);

    break;
  case MODE_SYSLOG:
    closelog();
    break;
  default:
    break;
  }
}


//------------------------------------------------------------------------
bool SSLEnabledIO::setModeSSL(void)
{
  SSL_library_init();
  SSL_load_error_strings();

  OpenSSL_add_ssl_algorithms();
  
  if ((ctx = SSL_CTX_new(SSLv23_server_method())) == 0) {
    setLastError("SSL error: internal error when creating CTX: " 
		 + string(ERR_error_string(ERR_get_error(), 0)));
    return false;
  }
  
  SSL_CTX_set_options(ctx, SSL_OP_ALL);

  Session &session = Session::getInstance();

  string clist = session.globalconfig["SSL"]["cipher list"];
  if (clist != "" && !SSL_CTX_set_cipher_list(ctx, clist.c_str())) {
    setLastError("SSL error: cannot load cipher list " + clist);
    return false;
  }

  string CAfile = session.globalconfig["SSL"]["ca file"];
  if (CAfile == "") CAfile == "/usr/share/ssl/certs/.crt";

  string CApath = session.globalconfig["SSL"]["ca path"];
  if (CApath == "") CApath == "/usr/share/ssl/certs/";

  SSL_CTX_set_default_verify_paths(ctx);

  string pemname = session.globalconfig["SSL"]["pem file"];
  if (pemname == "") pemname = "/usr/share/ssl/certs/stunnel.pem";

  if (!SSL_CTX_use_certificate_file(ctx, pemname.c_str(), SSL_FILETYPE_PEM)) {
    setLastError("SSL error: unable to use certificate in PEM file: "
		 + pemname + ": "
		 + string(ERR_error_string(ERR_get_error(), 0)));
    return false;
  }

  if (!SSL_CTX_use_PrivateKey_file(ctx, pemname.c_str(), SSL_FILETYPE_PEM)) {
    setLastError("SSL error: unable to use private key in PEM file: "
		 + pemname + ": "
		 + string(ERR_error_string(ERR_get_error(), 0)));
    return false;
  }

  if (!SSL_CTX_check_private_key(ctx)) {
    setLastError("SSL error: public and private key in PEM file"
		 " don't match: " 
		 + pemname + ": "
		 + string(ERR_error_string(ERR_get_error(), 0)));
    return false;
  }

  if (!SSL_CTX_load_verify_locations(ctx, CAfile.c_str(), CApath.c_str())) {
    setLastError("SSL error: unable to load CA file or path: "
		 + string(ERR_error_string(ERR_get_error(), 0)));
    return false;
  }

  if (session.globalconfig["SSL"]["verify peer"] == "yes")
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, 0);
  else
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);

  SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(CAfile.c_str()));

  SSL *ssl = SSL_new(ctx);
  if (ssl == 0) {
    setLastError("SSL error: when creating SSL object: "
		 + string(ERR_error_string(ERR_get_error(), 0)));
    return false;
  }

  SSL_clear(ssl);

  SSL_set_rfd(ssl, 0);
  SSL_set_wfd(ssl, 1);
  SSL_set_accept_state(ssl);
   
  fflush(stdout);

  int result = SSL_accept(ssl);
  if (result <= 0) {
    string errstr;
    switch (SSL_get_error(ssl, 0)) {
    case SSL_ERROR_NONE: errstr = "Unknown error"; break;
    case SSL_ERROR_ZERO_RETURN: errstr = "Connection closed"; break;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_CONNECT: errstr = "Operation did not complete"; break;
    case SSL_ERROR_WANT_X509_LOOKUP: errstr = "X509 lookup requested"; break;
    case SSL_ERROR_SYSCALL: errstr = "Unexpected EOF"; break;
    case SSL_ERROR_SSL: errstr = "Internal SSL error: ";
      errstr += string(ERR_error_string(ERR_get_error(), 0)); break;
    }

    setLastError(errstr);
    return false;
  }

  this->ssl = ssl;
  mode = MODE_SSL;
  return true;
}

//------------------------------------------------------------------------
bool SSLEnabledIO::isModeSSL(void)
{
  return mode == MODE_SSL;
}

//------------------------------------------------------------------------
void SSLEnabledIO::writeStr(const string s)
{
  if (s == "")
    return;
  
  if (mode == MODE_PLAIN) {
    // Set transfer timeout
    alarm(transfertimeout);
    int n = fwrite(s.c_str(), 1, s.length(), fpout);
    fflush(fpout);
    alarm(0);
    
    Session::getInstance().addWriteBytes(n);

    if (n != (int) s.length()) {
      setLastError("Output error: fwrite wrote too few bytes");
      return;
    }
  } else if (mode == MODE_SYSLOG) {
    // Set transfer timeout
    alarm(transfertimeout);
    syslog(LOG_INFO, "%s", s.c_str());
    alarm(0);
  } else if (mode == MODE_SSL) {
    do {
      alarm(transfertimeout);
      int retval = SSL_write(ssl, s.c_str(), s.length());
      alarm(0);
      
      if (retval > 0) {
	Session::getInstance().addWriteBytes(retval);
	break;
      } else if (retval == 0) {
	/* call get_error */
	setLastError("SSL_write returned 0");
	return;
      } else if (retval < 0) {
	int err = SSL_get_error(ssl, retval);
	
	if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
	  setLastError("SSL_write returned < 0");
	  return;
	} else {
	  // retry when we get this error.
	} 
      }
    } while (1);
  }
}

//------------------------------------------------------------------------
int SSLEnabledIO::fillBuffer(int timeout, bool retry)
{
  if (mode == MODE_PLAIN)
    return IO::fillBuffer(timeout, retry);

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  
  for (;;) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    int r = ::select(fileno(stdin) + 1, &rfds, 0, 0, timeout ? &tv : 0);
    if (r == 0) {
      setLastError("Reading from client timed out.");
      return -2;
    }

    if (r < 0) {
      setLastError("Error when reading from client");
      return -1;
    }

    char buf[1024];
    unsigned int readBytes = SSL_read(ssl, buf, sizeof(buf));      
    if (readBytes > 0) {

      Session::getInstance().addReadBytes(readBytes);

      for (unsigned int i = 0; i < readBytes; ++i)
	inputBuffer.push_front(buf[i]);

      return readBytes;
    }
    
    if (readBytes == 0) {
      setLastError("Client disconnected");
      return -1;
    }
    
    if (readBytes < 0) {
      int err = SSL_get_error(ssl, readBytes);
      if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
	setLastError("SSL error: read returned " 
		     + toString(readBytes) + ": " 
		     + string(ERR_error_string(ERR_get_error(), 0)));
	return -1;
      }
      
      if (timeout == 0 || !retry) {
	setLastError("Reading from client timed out.");
	return -2;
      }
    }
  }
}

//------------------------------------------------------------------------
int SSLEnabledIO::pending(void) const
{
  int tmp = (mode == MODE_PLAIN ? 0 : SSL_pending(ssl));
  if (tmp)
    return tmp;
  else
    return inputBuffer.size();
}

#endif
