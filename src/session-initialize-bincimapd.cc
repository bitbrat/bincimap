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
#include "maildir.h"
#include "depot.h"
#include "io-ssl.h"
#include "io.h"
#include "session.h"
#include "storage.h"
#include "tools.h"
#include "convert.h"
#include <string>
#include <map>
#include <signal.h>

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
  iofactory.assign(1, new IO(stdout));
  iofactory.assign(2, new IO(stderr));

  IO &com = iofactory.get(1);
  IO &logger = iofactory.get(2);
  logger.disable();

  Session &session = Session::getInstance();

  session.importFromEnv();

  // Read command line arguments
  if (!session.parseRequestLine(argc, argv))
    return false;

  // Assign command line arguments to global config.
  session.assignCommandLineArgs();

  // log settings
  string ipenv = session.globalconfig["Log"]["ip environment variable"];
  // Initialize logger
  string ip = getenv(ipenv.c_str()) ? getenv(ipenv.c_str()) :
    getenv("TCPREMOTEIP") ? getenv("TCPREMOTEIP") :
    getenv("REMOTEIP") ? getenv("REMOTEIP") :
    getenv("SSLREMOTEIP") ? getenv("SSLREMOTEIP") : "?";
  session.setIP(ip);

  if (session.globalconfig["Log"]["type"] == "multilog") {
    logger.setLogPrefix(session.getUserID() + "@" + ip + ":");
    logger.enableLogPrefix();
  } else if (session.globalconfig["Log"]["type"] == "stderr" ) {
    // stderr is the default
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

    logger.setModeSyslog("bincimapd", facility);
  }

  logger.enable();

  // Now that we know the log type, we can flush.
  logger.flushContent();
  logger.flushOnEndl();

  // Read command line arguments
  if (!session.parseRequestLine(argc, argv)) {
    logger << session.getLastError() << endl;
    return false;
  }

  // Show help if asked for it
  if (session.command.help) {
    usage(argv[0]);
    return false;
  }

  // Show help if asked for it
  if (session.command.version) {
    logger << "Binc IMAP v" << VERSION << endl;
    return false;
  }

  char *logindetails = getenv("BINCIMAP_LOGIN");
  if (logindetails == 0) {
    logger 
      << "BINCIMAP_LOGIN missing from environment (are you sure you invoked " 
      << argv[0] << " properly?)" << endl;
    return false;
  }

  //-----

  // try to read local settings.
  Storage lconfig(".bincimap", Storage::ReadOnly);
  string section, key, value;
  while (lconfig.get(&section, &key, &value))
    session.localconfig[section][key] = value;

  string tmp;
  if ((tmp = session.localconfig["Mailbox"]["depot"]) != "")
    session.globalconfig["Mailbox"]["depot"] = tmp;
  if ((tmp = session.localconfig["Mailbox"]["path"]) != "")
    session.globalconfig["Mailbox"]["path"] = tmp;
  if ((tmp = session.localconfig["Mailbox"]["type"]) != "")
    session.globalconfig["Mailbox"]["type"] = tmp;

  DepotFactory &depotfactory = DepotFactory::getInstance();
  depotfactory.assign(new IMAPdirDepot());
  depotfactory.assign(new MaildirPPDepot());

  string depottype = session.globalconfig["Mailbox"]["depot"];
  if (depottype == "") depottype = "Maildir++";

  if ((depot = depotfactory.get(depottype)) == 0) {
    logger << "Found no Depot for \"" << depottype
	   << "\". Please check "
      " your configurations file under the Mailbox section."
	   << endl;
    logger.flushContent();
    return false;
  }

  depot->assign(new Maildir());
  depot->setDefaultType("Maildir");

  BrokerFactory &brokerfactory = BrokerFactory::getInstance();

  brokerfactory.assign("APPEND", new AppendOperator());
  brokerfactory.assign("CAPABILITY", new CapabilityOperator());
  brokerfactory.assign("CHECK", new CheckOperator());
  brokerfactory.assign("CLOSE", new CloseOperator());
  brokerfactory.assign("COPY", new CopyOperator());
  brokerfactory.assign("CREATE", new CreateOperator());
  brokerfactory.assign("DELETE", new DeleteOperator());
  brokerfactory.assign("EXAMINE", new ExamineOperator());
  brokerfactory.assign("EXPUNGE", new ExpungeOperator());
  brokerfactory.assign("FETCH", new FetchOperator());
  brokerfactory.assign("LIST", new ListOperator());
  brokerfactory.assign("LOGOUT", new LogoutOperator());
  brokerfactory.assign("LSUB", new LsubOperator());
  brokerfactory.assign("NOOP", new NoopPendingOperator());
  brokerfactory.assign("RENAME", new RenameOperator());
  brokerfactory.assign("SEARCH", new SearchOperator());
  brokerfactory.assign("SELECT", new SelectOperator());
  brokerfactory.assign("STATUS", new StatusOperator());
  brokerfactory.assign("STORE", new StoreOperator());
  brokerfactory.assign("SUBSCRIBE", new SubscribeOperator());
  brokerfactory.assign("UNSUBSCRIBE", new UnsubscribeOperator());

  string path = session.globalconfig["Mailbox"]["path"];
  if (path == "") path = ".";
  else if (chdir(path.c_str()) != 0) {
    mkdir(path.c_str(), 0777);
    if (chdir(path.c_str()) != 0) {
      logger << "when entering depot " + toImapString(path) + ": "
	     << strerror(errno) << endl;
      return false;
    }
  }

  if (depot->get("INBOX") == 0) {
    if (session.globalconfig["Mailbox"]["auto create inbox"] == "yes")
      if (!depot->createMailbox("INBOX")) {
	logger << depot->getLastError() << endl;
	return false;
      }
  }

  // load subscription list
  session.loadSubscribes();

  session.setState(Session::AUTHENTICATED);

  // Read timeout settings from global config
  idletimeout = atoi(session.globalconfig["Session"]["idle timeout"]);
  transfertimeout = atoi(session.globalconfig["Session"]["transfer timeout"]);

  // Set transfer timeout
  com.setTransferTimeout(transfertimeout);
  logger.setTransferTimeout(transfertimeout);

  // Read transfer buffer size
  int buffersize = atoi(session.globalconfig["Session"]["transfer buffer size"]);
  com.setBufferSize(buffersize >= 0 ? buffersize : 0);

  // umask settings
  unsigned int mode;
  sscanf(session.globalconfig["Mailbox"]["umask"].c_str(), "%o", &mode);
  umask((mode_t) mode);

  const string details = logindetails;
  string::size_type det = details.find('+');
  if (det == string::npos) {
    logger << "invalid content of BINCIMAP_LOGIN - did you invoke "
       << argv[0] << " correctly?" << endl;
    return false;
  }

  const string tag = details.substr(det + 1);
  const string command = details.substr(0, det);
  com << tag << " OK " << command << " completed" << endl;
  com.flushContent();

  return true;
}
