/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/depot.h
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
#ifndef depot_h_included
#define depot_h_included
#include <map>
#include <string>
#include <vector>

#include <dirent.h>

namespace Binc {

  class Mailbox;
  class Depot;
  class Status;

  //------------------------------------------------------------------
  class DepotFactory {
  private:
    std::vector<Depot *> depots;
    DepotFactory(void);

  public:
    void assign(Depot *);
    Depot *get(const std::string &name) const;

    static DepotFactory &getInstance(void);
    ~DepotFactory(void);
  };

  //------------------------------------------------------------------
  class Depot {
  public:
    //--
    class iterator {
    public:      
      std::string operator * (void) const;
      void operator ++ (void);
      bool operator != (iterator) const;
      bool operator == (iterator) const;

      iterator(void);
      iterator(const iterator &copy);
      iterator(DIR *, struct dirent *);
      ~iterator(void);

      void deref(void);

      iterator &operator =(const iterator &copy);

      friend class Depot;

    private:
      DIR *dirp;
      struct dirent *direntp;
      int *ref;
    };

  private:
    iterator enditerator;
    std::vector<Mailbox *> backends;
    Mailbox *defaultmailbox;
    Mailbox *selectedmailbox;

  protected:
    mutable std::string lastError;
    std::string name;
    char delimiter;
    mutable std::map<std::string, Status> mailboxstatuses;

  public:
    iterator begin(const std::string &) const;
    const iterator &end(void) const;

    void setDelimiter(char);
    const char getDelimiter(void) const;

    void assign(Mailbox *);

    bool setDefaultType(const std::string &n) ;
    Mailbox *getDefault(void) const;
    Mailbox *get(const std::string &path) const;

    bool setSelected(Mailbox *);
    Mailbox *getSelected(void) const;

    bool getStatus(const std::string &s_in, Status &dest) const;

    const std::string &getName(void) const;

    virtual std::string mailboxToFilename(const std::string &m) const = 0;
    virtual std::string filenameToMailbox(const std::string &m) const = 0;

    virtual bool createMailbox(const std::string &m) const;
    virtual bool deleteMailbox(const std::string &m) const;
    virtual bool renameMailbox(const std::string &m, const std::string &n) const;

    const std::string &getLastError(void) const;
    void setLastError(const std::string &error) const;

    //--
    Depot(void);
    Depot(const std::string &name);
    virtual ~Depot(void);
  };

  //------------------------------------------------------------------
  class MaildirPPDepot : public Depot {
  public:
    std::string mailboxToFilename(const std::string &m) const;
    std::string filenameToMailbox(const std::string &m) const;

    //--
    MaildirPPDepot();
    ~MaildirPPDepot();
  };

  //------------------------------------------------------------------
  class IMAPdirDepot : public Depot {
  public:
    std::string mailboxToFilename(const std::string &m) const;
    std::string filenameToMailbox(const std::string &m) const;

    //--
    IMAPdirDepot();
    ~IMAPdirDepot();
  };

}

#endif
