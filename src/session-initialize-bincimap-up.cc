/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    session-initialize-bincimap-up.cc
 *  
 *  Description:
 *    <--->
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

#include <syslog.h>

#include "broker.h"
#include "io-ssl.h"
#include "io.h"
#include "session.h"
#include "storage.h"
#include "tools.h"
#include "convert.h"
#include <string>
#include <map>

using namespace ::std;
using namespace Binc;

extern char **environ;

namespace {
  //------------------------------------------------------------------------
  void usage(char *name)
  {
    IO &logger = IOFactory::getInstance().get(2);

    logger << "Please refer to the man pages for bincimap-up and bincimapd"
	   << endl;
    logger << "for more information about how to invoke Binc IMAP." << endl;
    logger.flushContent();
  }
}

//----------------------------------------------------------------------
bool Session::initialize(int argc, char *argv[])
{
  IOFactory &iofactory = IOFactory::getInstance();

#ifdef WITH_SSL
  SSLEnabledIO *sslio = new SSLEnabledIO(stdout);
  iofactory.assign(1, sslio);
#else
  iofactory.assign(1, new IO(stdout));
#endif

  iofactory.assign(2, new IO(stderr));

  IO &com = iofactory.get(1);
  IO &logger = iofactory.get(2);

  Session &session = Session::getInstance();

  logger.noFlushOnEndl();

  // Read command line arguments
  if (!session.parseRequestLine(argc, argv))
    return false;

  // Show help if asked for it
  if (session.command.help) {
    usage(argv[0]);
    logger.flushContent();
    return false;
  }

  // Show help if asked for it
  if (session.command.version) {
    logger << "Binc IMAP v" << VERSION << endl;
    logger.flushContent();
    return false;
  }

  // Grab config file name
  string configfile = session.command.configfile;

  // Read configuration
  if (configfile != "") {

    // try to read global settings.
    Storage gconfig(configfile, Storage::ReadOnly);
    string section, key, value;
    while (gconfig.get(&section, &key, &value))
      session.globalconfig[section][key] = value;

    if (!gconfig.eof()) {
      logger << "error reading Binc IMAP's config file "
	     << configfile << ". Default values will be used instead: "
	     << gconfig.getLastError() << endl;
      // don't flush!
    }
  }

  // Read configuration settings
  session.initConfig();

  // log settings
  string ipenv = session.globalconfig["Log"]["ip environment variable"];
  // Initialize logger
  string ip = getenv(ipenv.c_str()) ? getenv(ipenv.c_str()) :
    getenv("TCPREMOTEIP") ? getenv("TCPREMOTEIP") :
    getenv("REMOTE_HOST") ? getenv("REMOTE_HOST") :
    getenv("REMOTEIP") ? getenv("REMOTEIP") :
    getenv("SSLREMOTEIP") ? getenv("SSLREMOTEIP") : "?";
  session.setIP(ip);

  if (session.globalconfig["Log"]["type"] == "multilog" 
      || session.globalconfig["Log"]["type"] == "stderr") {
    logger.setLogPrefix("unknown@" + ip + ":");
    logger.enableLogPrefix();
  } else if (session.globalconfig["Log"]["type"] == "" 
	     || session.globalconfig["Log"]["type"] == "syslog") {
    const string f = session.globalconfig["Log"]["syslog facility"];
    const string fn = session.globalconfig["Log"]["syslog facility number"];

    int facility;

    if (fn != "") facility = atoi(fn);
    else {
      if (f == "LOG_USER") facility = LOG_USER;
      else if (f == "LOG_LOCAL0") facility = LOG_LOCAL0;
      else if (f == "LOG_LOCAL1") facility = LOG_LOCAL1;
      else if (f == "LOG_LOCAL2") facility = LOG_LOCAL2;
      else if (f == "LOG_LOCAL3") facility = LOG_LOCAL3;
      else if (f == "LOG_LOCAL4") facility = LOG_LOCAL4;
      else if (f == "LOG_LOCAL5") facility = LOG_LOCAL5;
      else if (f == "LOG_LOCAL6") facility = LOG_LOCAL6;
      else if (f == "LOG_LOCAL7") facility = LOG_LOCAL7;
      else facility = LOG_DAEMON;
    }

    session.globalconfig["Log"]["syslog facility number"] = toString(facility);

    logger.setModeSyslog("bincimap-up", facility);
  }

  // Now that we know the log type, we can flush.
  logger.flushContent();
  logger.flushOnEndl();

  BrokerFactory &brokerfactory = BrokerFactory::getInstance();

  brokerfactory.assign("AUTHENTICATE", new AuthenticateOperator());
  brokerfactory.assign("CAPABILITY", new CapabilityOperator());
  brokerfactory.assign("LOGIN", new LoginOperator());
  brokerfactory.assign("LOGOUT", new LogoutOperator());
  brokerfactory.assign("NOOP", new NoopOperator());
#ifdef WITH_SSL
  brokerfactory.assign("STARTTLS", new StarttlsOperator());
#endif

#ifdef WITH_SSL
  // Set SSL mode if --ssl is passed
  if (session.command.ssl)
     if (sslio->setModeSSL()) {
       session.add("sslmode", "true");
     } else {
       session.setLastError("SSL negotiation failed: " 
			    + sslio->getLastError());
       return false;
     }
#endif

  session.setState(Session::NONAUTHENTICATED);

  // Read timeout settings from global config
  idletimeout = atoi(session.globalconfig["Session"]["idle timeout"]);
  authtimeout = atoi(session.globalconfig["Session"]["auth timeout"]);
  transfertimeout = atoi(session.globalconfig["Session"]["transfer timeout"]);

  // Settings are not allowed to break the IMAP protocol
  if (idletimeout < (30 * 60)) idletimeout = 30 * 60;
  
  // No auth timeout is not allowed.
  if (authtimeout < 30) authtimeout = 30;

  // Set transfer timeout
  com.setTransferTimeout(transfertimeout);
  logger.setTransferTimeout(transfertimeout);

  // Read transfer buffer size
  int buffersize = atoi(session.globalconfig["Session"]["transfer buffer size"]);
  com.setBufferSize(buffersize >= 0 ? buffersize : 0);

  // umask settings
  string umsk = session.globalconfig["Mailbox"]["umask"];
  if (umsk == "" || !isdigit(umsk[0])) umsk = "0777";
  unsigned int mode;
  sscanf(session.globalconfig["Mailbox"]["umask"].c_str(), "%o", &mode);
  umask((mode_t) mode);

  // If the depot was not initialized properly, return false.
  return true;
}
