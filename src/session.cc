/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    session.cc
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

#include "io.h"
#include "session.h"
#include "storage.h"
#include "tools.h"
#include "convert.h"
#include <string>
#include <map>

using namespace ::std;
using namespace Binc;
using namespace ArgParser;

extern char **environ;

//----------------------------------------------------------------------
Session::Session(void)
{
  readbytes = 0;
  writebytes = 0;
  statements = 0;
  bodies = 0;
  idletimeout = 0;
  authtimeout = 0;
  mailboxchanges = true;
  logfacility = LOG_DAEMON;
}

//----------------------------------------------------------------------
Session &Session::getInstance(void)
{
  static Session session;
  return session;
}

//----------------------------------------------------------------------
void Session::initConfig(void)
{
  args.assign();
}

//----------------------------------------------------------------------
const int Session::getState(void) const
{
  return state;
}

//----------------------------------------------------------------------
void Session::setState(int n)
{
  state = n;
}

//----------------------------------------------------------------------
const string &Session::getUserID(void) const
{
  return userid;
}

//----------------------------------------------------------------------
void Session::setUserID(const string &s)
{
  userid = s;
}

//----------------------------------------------------------------------
const string &Session::getIP(void) const
{
  return ip;
}

//----------------------------------------------------------------------
void Session::setIP(const string &s)
{
  ip = s;
}

//----------------------------------------------------------------------
void Session::setLogFacility(int facility)
{
  logfacility = facility;
}

//----------------------------------------------------------------------
int Session::getLogFacility(void) const
{
  return logfacility;
}

//----------------------------------------------------------------------
void Session::addBody(void)
{
  ++bodies;
}

//----------------------------------------------------------------------
void Session::addStatement(void)
{
  ++statements;
}

//----------------------------------------------------------------------
void Session::addReadBytes(int i)
{
  readbytes += i;
}

//----------------------------------------------------------------------
void Session::addWriteBytes(int i)
{
  writebytes += i;
}

//----------------------------------------------------------------------
int Session::getBodies(void) const
{
  return bodies;
}

//----------------------------------------------------------------------
int Session::getStatements(void) const
{
  return statements;
}

//----------------------------------------------------------------------
int Session::getWriteBytes(void) const
{
  return writebytes;
}

//----------------------------------------------------------------------
int Session::getReadBytes(void) const
{
  return readbytes;
}

bool Session::parseRequestLine(int argc, char * argv[])
{
  args = ArgParser::Args(argc, argv);

  args.addOptional("h|?|help", command.help);
  args.addOptional("v|version", command.version);
  args.addOptional("s|ssl", command.ssl);
  args.addOptional("c|conf", command.configfile);

  // Override config file settings with command line options
  string arg;
  args.addOptional("a|allow-plain", 
		   globalconfig["Authentication"]["allow plain auth in non ssl"]);
  args.addOptional("p|auth-penalty",
		   globalconfig["Authentication"]["auth penalty"]);
  args.addOptional("d|disable-starttls",
		   globalconfig["Authentication"]["disable starttls"]);

  args.addOptional("L|logtype", globalconfig["Log"]["type"]);
  args.addOptional("I|ip-variable",
		   globalconfig["Log"]["environment ip variable"]);

  args.addOptional("M|mailbox-type", globalconfig["Mailbox"]["type"]);
  args.addOptional("m|mailbox-path", globalconfig["Mailbox"]["path"]);
  args.addOptional("C|create-inbox", globalconfig["Mailbox"]["auto create inbox"]);
  args.addOptional("S|subscribe-mailboxes",
		   globalconfig["Mailbox"]["auto subscribe mailboxes"]);
  args.addOptional("u|umask", globalconfig["Mailbox"]["umask"]);

  args.addOptional("J|jail-path", globalconfig["Security"]["jail path"]);
  args.addOptional("x|jail-user", globalconfig["Security"]["jail user"]);
  args.addOptional("X|jail-group", globalconfig["Security"]["jail group"]);

  args.addOptional("i|idle-timeout", globalconfig["Session"]["idle timeout"]);
  args.addOptional("t|auth-timeout", globalconfig["Session"]["auth timeout"]);
  args.addOptional("T|transfer-timeout", 
		   globalconfig["Session"]["transfer timeout"]);
  args.addOptional("b|transfer-buffersize", 
		   globalconfig["Session"]["transfer buffer size"]);

  args.addOptional("p|pem-file", globalconfig["SSL"]["pem file"]);
  args.addOptional("P|ca-path", globalconfig["SSL"]["ca path"]);
  args.addOptional("f|ca-file", globalconfig["SSL"]["ca file"]);
  args.addOptional("l|cipher-list", globalconfig["SSL"]["cipher list"]);
  args.addOptional("V|verify-peer", globalconfig["SSL"]["verify peer"]);

  ArgumentError e(ArgParser::ArgumentError::NONE);
  vector<string> rest;

  int i = args.parse(rest, e);
  if (i == -1) {
    string err = e.getErrorStr();
    if (err == "") {
      switch (e.getErrorType()) {
      case ArgParser::ArgumentError::NONE:
	err = "Unknown error (none)";
	break;
      case ArgParser::ArgumentError::CHECK:
	err = "Syntax error";
	break;
      case ArgParser::ArgumentError::REQUIRED_COMMAND:
	err = "Required command";
	break;
      case ArgParser::ArgumentError::REQUIRED_ARGUMENT:
	err = "Required argument";
	break;
      case ArgParser::ArgumentError::UNKNOWN:
	err = "No such argument";
	break;
      case ArgParser::ArgumentError::MISSING:
	err = "Missing argument";
	break;
      case ArgParser::ArgumentError::INVALID:
	err = "Invalid argument";
	break;
      default:
	break;
      }
    }

    setLastError("Command line \"" + e.getName() + "\": " + err);
    return false;
  }

  unparsedArgs = argv + i;
  args.assign();

  return true;
}

//----------------------------------------------------------------------
void Session::add(const string &a, const string &b)
{
  attrs[a] = b;
}

//----------------------------------------------------------------------
const string &Session::operator [] (const string &s_in) const
{
  static const string NIL = "";
  if (attrs.find(s_in) == attrs.end())
    return NIL;
  else
    return attrs.find(s_in)->second;
}

//----------------------------------------------------------------------
void Session::exportToEnv(void)
{
  Tools &tools = Tools::getInstance();

  tools.setenv("BINCIMAP_STATE", toString(state));
  tools.setenv("BINCIMAP_USERID", userid);
  tools.setenv("BINCIMAP_IP", ip);

  for (map<string, string>::const_iterator i = attrs.begin();
       i != attrs.end(); ++i)
    tools.setenv("BINCIMAP_CONF_" + i->first, i->second);

  int n = 0;
  for (vector<string>::const_iterator i = subscribed.begin();
       i != subscribed.end(); ++i, ++n)
    tools.setenv("BINCIMAP_SUBSCRIBED_" + toString(n), *i);

  map<string, map<string, string> >::const_iterator gi = globalconfig.begin();
  for (; gi != globalconfig.end(); ++gi) {
    map<string, string>::const_iterator ji = gi->second.begin();
    for (; ji != gi->second.end(); ++ji)
      tools.setenv("BINCIMAP_GLOBALCONFIG_" 
		   + toHex(gi->first) + "::" + toHex(ji->first),
		   toHex(ji->second));
  }

  map<string, map<string, string> >::const_iterator li = localconfig.begin();
  for (; li != localconfig.end(); ++li) {
    map<string, string>::const_iterator ji = li->second.begin();
    for (; ji != li->second.end(); ++ji) {
      tools.setenv("BINCIMAP_GLOBALCONFIG_" 
		   + toHex(li->first) + "::" + toHex(ji->first),
		   toHex(ji->second));
    }
  }
}

//----------------------------------------------------------------------
void Session::importFromEnv(void)
{
  char *c;
  int cnt = 0;
  while ((c = environ[cnt]) != 0) {
    string s = c;

    if (s.substr(0, 14) == "BINCIMAP_STATE") state = atoi(s.substr(15));
    else if (s.substr(0, 15) == "BINCIMAP_USERID") userid = s.substr(16);
    else if (s.substr(0, 11) == "BINCIMAP_IP") ip = s.substr(12);
    else if (s.substr(0, 21) == "BINCIMAP_GLOBALCONFIG") {
      const string config = s.substr(22);
      if (config.find("::") != string::npos) {
	const string section = fromHex(config.substr(0, config.find("::")));
	const string data = config.substr(config.find("::") + 2);
	
	if (data.find("=") != string::npos) {
	  const string key = fromHex(data.substr(0, data.find("=")));
	  const string value = fromHex(data.substr(data.find("=") + 1));

	  globalconfig[section][key] = value;
	}
      }
    } else if (s.substr(0, 20) == "BINCIMAP_LOCALCONFIG") {
      const string config = s.substr(21);
      if (config.find("::") != string::npos) {
	const string section = fromHex(config.substr(0, config.find("::")));
	const string data = config.substr(config.find("::") + 2);
	
	if (data.find("=") != string::npos) {
	  const string key = fromHex(data.substr(0, data.find("=")));
	  const string value = fromHex(data.substr(data.find("=") + 1));

	  localconfig[section][key] = value;
	}
      }
    }

    ++cnt;
  }
}

//----------------------------------------------------------------------
const string &Session::getLastError(void) const
{
  return lastError;
}

//----------------------------------------------------------------------
void Session::setLastError(const string &error) const
{
  lastError = error;
}

//----------------------------------------------------------------------
const string &Session::getResponseCode(void) const
{
  return responseCode;
}

//----------------------------------------------------------------------
void Session::setResponseCode(const string &code) const
{
  responseCode = "[" + code + "] ";
}

//----------------------------------------------------------------------
void Session::clearResponseCode(void) const
{
  responseCode = "";
}

//----------------------------------------------------------------------
pid_t Session::getPid(void)
{
  if (pid == 0)
    pid = getpid();

  return pid;
}

//----------------------------------------------------------------------
const std::string &Session::getHostname(void)
{
  if (hostname == "") {
    char hostnamec[512];
    int hostnamelen = gethostname(hostnamec, sizeof(hostnamec));
    if (hostnamelen == -1 || hostnamelen == sizeof(hostnamec))
      strcpy(hostnamec, "localhost");
    hostnamec[511] = '\0';
    hostname = hostnamec;
  }

  return hostname;
}

//----------------------------------------------------------------------
void Session::subscribeTo(const std::string mailbox)
{
  subscribed.push_back(mailbox);
}

//----------------------------------------------------------------------
bool Session::unsubscribeTo(const std::string mailbox)
{
  for (vector<string>::iterator i = subscribed.begin();
       i != subscribed.end(); ++i) {
    if (*i == mailbox) {
      subscribed.erase(i);
      return true;
    }
  }

  return false;
}

//----------------------------------------------------------------------
void Session::loadSubscribes(void)
{
  // drop all existing subscribed folders.
  subscribed.clear();

  // try loading the .bincimap-subscribed file
  bool ok = false;
  FILE *fp = fopen(".bincimap-subscribed", "r");
  if (fp) {
    int c;
    string current;
    while ((c = fgetc(fp)) != EOF) {
      if (c == '\n') {
	trim(current);
	if (current != "") {
	  subscribed.push_back(current);
	  current = "";
	}
      } else
	current += c;
    }

    fclose(fp);
    ok = true;
  }

  bool deleteOld = false;
  if (!ok) {
    // load subscription list. if it doesn't exist - initialize it with
    // the values from the auto subscribe list.
    Storage subs("bincimap-subscribed", Storage::ReadOnly);
    string section, key, value;
   
    while (subs.get(&section, &key, &value))
      if (section == "subscribed")
	subscribed.push_back(value);
  
    if (subs.eof()) {
      ok = true;
      deleteOld = true;
    }
  }

  if (!ok) {
    string autosubmailboxes = globalconfig["Mailbox"]["auto subscribe mailboxes"];
    vector<string> mboxes;
    split(autosubmailboxes, ",", mboxes);
    
    for (vector<string>::const_iterator i = mboxes.begin(); i != mboxes.end();
	 ++i) {
      string tmp = *i;
      trim(tmp);
      subscribed.push_back(tmp);
    }
    
    saveSubscribes();
  }

  if (deleteOld) {
    if (saveSubscribes())
      unlink("bincimap-subscribed");
  }
}

//----------------------------------------------------------------------
bool Session::saveSubscribes(void) const
{
  IO &logger = IOFactory::getInstance().get(2);

  // create a safe file name
  string tpl = ".bincimap-subscribed-tmp-XXXXXX";
  char *ftemplate = new char[tpl.length() + 1];

  strcpy(ftemplate, tpl.c_str());
  int fd = mkstemp(ftemplate);
  if (fd == -1) {
    logger << "unable to create temporary file \""
	   << tpl << "\"" << endl;
    return false;
  }

  for (vector<string>::const_iterator i = subscribed.begin();
       i != subscribed.end(); ++i) {
    string w = (*i) + "\n";
    if (write(fd, w.c_str(), w.length()) != (ssize_t) w.length()) {
      logger << "failed to write to " << tpl << ": "
	     << strerror(errno) << endl;
      break;
    }
  }
  
  fsync(fd);
  close(fd);
  if (rename(ftemplate, ".bincimap-subscribed") != 0) {
      logger << "failed to rename " << ftemplate 
	     << " to .bincimap-subscribed: "
	     << strerror(errno) << endl;
      return false;
  }

  return true;
}

//----------------------------------------------------------------------
int Session::timeout() const
{
  return state == NONAUTHENTICATED ? authtimeout : idletimeout;
}
