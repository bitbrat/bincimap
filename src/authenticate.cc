/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    authenticate.cc
 *  
 *  Description:
 *    Implementation of the common authentication mechanism.
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
#include <vector>

#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>

#ifndef HAVE_SYS_WAIT_H
#include <wait.h>
#else
#include <sys/wait.h>
#endif

#include "authenticate.h"
#include "io.h"
#include "session.h"
#include "storage.h"
#include "convert.h"

using namespace ::std;
using namespace Binc;

namespace {

  bool enteredjail = false;

  void enterJail(void)
  {
    Session &session = Session::getInstance();
    IO &logger = IOFactory::getInstance().get(2);

    // drop all privileges that we can drop and enter chroot jail
    const string &jailpath = session.globalconfig["Security"]["jail path"];
    const string &jailuser = session.globalconfig["Security"]["jail user"];
    const string &jailgroup = session.globalconfig["Security"]["jail group"];
    struct group *gr = getgrnam(jailgroup.c_str());
    struct passwd *pw = getpwnam(jailuser.c_str());

    if (jailgroup != "" && !gr)
      logger << "invalid jail group <" << jailgroup 
	     << "> - check the Security section of bincimap.conf"
	     << endl;

    if (jailuser != "" && !pw)
      logger << "invalid jail user <" << jailuser
	     << "> - check the Security section of bincimap.conf" 
	     << endl;

    setgroups(0, 0);

    if (jailpath != "") {
      if (chroot(jailpath.c_str()) != 0)
	logger << "unable to enter jail path "
	       << toImapString(jailpath) << endl;
      else
	chdir("/");
    }

    if (gr) setgid(gr->gr_gid);
    if (pw) setuid(pw->pw_uid);

    umask(0);
  }
}

// 0 = ok
// 1 = internal error
// 2 = failed
// 3 = timeout
// -1 = abort
//------------------------------------------------------------------------
int Binc::authenticate(Depot &depot, const string &username,
		       const string &password)
{
  Session &session = Session::getInstance();
  IO &com = IOFactory::getInstance().get(1);
  IO &logger = IOFactory::getInstance().get(2);

  // read auth penalty from global config
  string authpenalty = session.globalconfig["Authentication"]["auth penalty"];

  session.setUserID(username);

  // export the session data
  session.exportToEnv();
  
  // The information supplied on descriptor 3 is a login name
  // terminated by \0, a password terminated by \0, a timestamp
  // terminated by \0, and possibly more data. There are no other
  // restrictions on the form of the login name, password, and
  // timestamp.
  int authintercom[2];
  int intercomw[2];
  int intercomr[2];
  bool authenticated = false;

  if (pipe(authintercom) == -1) {
    session.setLastError("An error occurred when creating pipes: " 
			 + string(strerror(errno)));
    return -1;
  }

  if (pipe(intercomw) == -1) {
    session.setLastError("An error occurred when creating pipes: " 
			 + string(strerror(errno)));
    close(authintercom[0]);
    close(authintercom[1]);
    return -1;
  }

  if (pipe(intercomr) == -1) {
    session.setLastError("An error occurred when creating pipes: " 
			 + string(strerror(errno)));
    close(intercomw[0]);
    close(intercomr[0]);
    close(authintercom[0]);
    close(authintercom[1]);
    return -1;
  }

  string timestamp;
  time_t t;
  char *c;
  t = time(0);
  if ((c = ctime(&t)) != 0)
    timestamp = c;
  else
    timestamp = "unknown timestamp";

  // execute authentication module
  int result;
  int childspid = fork();
  if (childspid == -1) {
    logger << "fork failed: " << strerror(errno) << endl;
    return 1;
  }

  if (childspid == 0) {

    close(authintercom[1]);
    close(intercomr[0]);
    close(intercomw[1]);

    if (dup2(intercomr[1], 1) == -1) {
      logger << "[auth module] dup2 failed: "
	     << strerror(errno) << endl;
      logger.flushContent();
      exit(111);
    }

    if (dup2(intercomw[0], 0) == -1) {
      logger << "[auth module] dup2 failed: "
	     << strerror(errno) << endl;
      logger.flushContent();
      exit(111);
    }

    if (dup2(authintercom[0], 3) == -1) {
      logger << "[auth module] dup2 failed: "
	     << strerror(errno) << endl;
      logger.flushContent();
      exit(111);
    }

    session.exportToEnv();

    if (session.unparsedArgs[0] != 0) {
      execv(session.unparsedArgs[0], &session.unparsedArgs[0]);
      logger << "[auth module] invocation of " << session.unparsedArgs[0] 
	     << " failed: " << strerror(errno)
	     << endl;
      logger.flushContent();
      exit(111);
    }
    
    logger << "[auth module] Missing mandatory -- in arg list,"
      " after bincimap-up + arguments, before authenticator."
      " Please check your run scripts and the man pages for"
      " more on how to invoke Binc IMAP." << endl;
    logger.flushContent();
    exit(111);
  }

  if (authintercom[0] != -1)
    close(authintercom[0]);

  bool error = false;

  // write the userid
  if (write(authintercom[1],
	    username.c_str(),
	    username.length()) != (int) username.length()) 
    error = true;

  // terminate with a \0
  if (!error && write(authintercom[1], "", 1) != 1)
    error = true;

  // write the password
  if (!error && write(authintercom[1],
		      password.c_str(),
		      password.length()) != (int) password.length())
    error = true;

  // terminate with a \0
  if (!error && write(authintercom[1], "", 1) != 1)
    error = true;

  // write the timestamp
  if (!error && write(authintercom[1],
		      timestamp.c_str(),
		      timestamp.length()) != (int) timestamp.length()) 
    error = true;

  // terminate with a \0
  if (!error && write(authintercom[1], "", 1) != 1)
    error = true;

  if (error) {
    logger << "error writing to authenticator " 
	   << session.unparsedArgs[0] << ": "
	   << strerror(errno) << endl;
    return 1;
  }

  // close the write channel. this is necessary for the checkpassword
  // module to see an EOF.
  close(authintercom[1]);
  close(intercomr[1]);
  close(intercomw[0]);

  fd_set rmask;
  FD_ZERO(&rmask);
  FD_SET(fileno(stdin), &rmask);
  FD_SET(intercomr[0], &rmask);
  
  int maxfd = intercomr[0];
  bool disconnected = false;
  bool timedout = false;
  com.disableInputLimit();

  bool eof = false;
  while (!eof) {
    fd_set rtmp = rmask;
    struct timeval timeout;

    // time out 5 minutes after the idle timeout. we expect the main
    // server to time out at the right time, but will shut down at
    // T+5m in case of a server lockup.
    timeout.tv_sec = session.idletimeout + 5*60;
    timeout.tv_usec = 0;
    
    int n = select(maxfd + 1, &rtmp, 0, 0, &timeout);
    if (n < 0) {
      if (errno == EINTR) {
	logger << "warning: zero data from select: " 
	       << strerror(errno) << endl;
	break;
      }

      logger << "error: invalid exit from select, "
	     << strerror(errno) << endl;
      break;
    }

    if (n == 0) {
      logger << "lock-up: server timed out after " 
	     << session.idletimeout << " seconds" << endl;
      timedout = true;
      break;
    }

    if (FD_ISSET(fileno(stdin), &rtmp)) {
      authenticated = true;

      do {
	string data;
	int ret = com.readStr(data);
	if (ret == 0 || ret == -1) {
	  session.setLastError("client disconnected");
	  eof = true;
	  disconnected = true;
	  break;
	}

	if (ret == -2) {
	  // Fall through. Triggered when there was no data
	  // to read, even though no error has occurred
	  continue;
	}

	for (;;) {
	  int w = write(intercomw[1], data.c_str(), data.length());
	  if (w > 0)
	    Session::getInstance().addReadBytes(w);

	  if (w == (int) data.length())
	    break;

	  if (w == -1 && (errno == EINTR))
	      continue;
	  
	  logger << "error writing to server: "
		 << strerror(errno) << endl;
	  eof = true;
	  break;
	}
      } while (com.pending());
    }

    if (FD_ISSET(intercomr[0], &rtmp)) {
      char buf[1024];
      int ret = read(intercomr[0], buf, sizeof(buf));
      if (ret == 0) {
	// Main server has shut down
	eof = true;
	break;
      } else if (ret == -1) {
	logger << "error reading from server: " << strerror(errno)
	       << endl;
	eof = true;
	break;
      } else {
	if (enteredjail == false) {
	  enterJail();
	  enteredjail = true;
	}

	Session::getInstance().addWriteBytes(ret);
	
	com << string(buf, ret);
	com.flushContent();
      }
    }
  }

  close(intercomr[0]);
  close(intercomw[1]);

   // catch the dead baby
  if (waitpid(childspid, &result, 0) != childspid) {
    logger << "<" << username << "> authentication failed: "
	   << (authenticated ? "server " : session.unparsedArgs[0])
	   << " waitpid returned unexpected value" << endl;
    string tmp = strerror(errno);

    return -1;
  }

  // if the server died because we closed the sockets after a timeout,
  // exit 3.
  if (timedout)
    return 3;

  if (disconnected)
    return 0;

  if (WIFSIGNALED(result)) {
    logger << "<" << username <<  "> authentication failed: "
	   << (authenticated ? "server" : session.unparsedArgs[0])
	   << " died by signal "
	   << WTERMSIG(result) << endl;

    sleep(atoi(authpenalty));
    session.setState(Session::LOGOUT);
    return -1;
  }

  switch (WEXITSTATUS(result)) {
  case 0:  break;
  case 1:
    // authentication failed - sleep
    logger << "<" << username 
	   << "> authentication failed: wrong userid or password" << endl;
    sleep(atoi(authpenalty));
    return 2;
  case 111:
  case 2:
    // internal error
    logger << "<" << username << "> authentication failed: "
	   << (authenticated ? "authenticator " : "server ")
	   << " returned "
	   << WEXITSTATUS(result) << " (internal error)" << endl;
    return -1;
  default:
    break;
  }

  return 0;
}
