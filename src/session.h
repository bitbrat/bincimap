/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/session.h
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

#ifndef session_h_included
#define session_h_included
#include <string>
#include <vector>
#include <map>

namespace Binc {

  class Depot;
  
  //--------------------------------------------------------------------
  class Session {
  public:
    std::map<std::string, std::string> attrs;

    char **unparsedArgs;

    struct {
      bool help;
      bool version;
      bool ssl;
      std::string configfile;
    } command;

    int idletimeout;
    int authtimeout;
    int transfertimeout;

    bool mailboxchanges;

    enum State {
      NONAUTHENTICATED = 0x01,
      AUTHENTICATED = 0x02,
      SELECTED = 0x04,
      LOGOUT = 0x00
    };

    std::map<std::string, std::map<std::string, std::string> > globalconfig;
    std::map<std::string, std::map<std::string, std::string> > localconfig;

    int timeout() const;

    std::vector<std::string> subscribed;
    void subscribeTo(const std::string mailbox);
    bool unsubscribeTo(const std::string mailbox);
    void loadSubscribes(void);
    bool saveSubscribes(void) const;

    const int getState(void) const;
    void setState(int n);
    void exportToEnv(void);
    void importFromEnv(void);
    bool parseRequestLine(int argc, char * argv[]);
    int getWriteBytes(void) const;
    int getReadBytes(void) const;
    void addWriteBytes(int);
    void addReadBytes(int);
    int getBodies(void) const;
    int getStatements(void) const;
    void addBody(void);
    void addStatement(void);
    void setLogFacility(int facility);
    int getLogFacility(void) const;

    const std::string &getLastError(void) const;
    const std::string &getResponseCode(void) const;
    const std::string &getIP(void) const;
    const std::string &getUserID() const;
    pid_t getPid(void);
    const std::string &getHostname(void);
    void setLastError(const std::string &error) const;
    void setResponseCode(const std::string &error) const;
    void clearResponseCode(void) const;
    void setIP(const std::string &ip);
    void setUserID(const std::string &s);

    inline Depot *getDepot(void) const;

    const std::string &operator [] (const std::string &) const;

    //--
    void add(const std::string &, const std::string &);

    //--
    static Session &getInstance(void);
    void initConfig(void);

    bool initialize(int argc, char *argv[]);

  private:
    //--
    int state;
    std::string userid;
    std::string ip;
    char **argv;
    int argc;

    int logfacility;

    int readbytes;
    int writebytes;
    int statements;
    int bodies;

    Depot *depot;

    mutable std::string lastError;
    mutable std::string responseCode;

    pid_t pid;
    std::string hostname;

    Session(void);
  };

  inline Depot *Session::getDepot(void) const
  {
    return depot;
  }
}

#endif
