/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/pendingupdates.h
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

#include <map>
#include <vector>

#ifndef pendingupdates_h_included
#define pendingupdates_h_included

namespace Binc {
  class Mailbox;

  //------------------------------------------------------------------------
  class PendingUpdates {
  public:
    enum {
      EXPUNGE = 0x01,
      FLAGS   = 0x02,
      EXISTS  = 0x04,
      RECENT  = 0x08
    };

    //----------------------------------------------------------------------
    class expunged_const_iterator {
    private:
      std::vector<unsigned int>::iterator internal;

    public:
      unsigned int operator * (void) const;
      void operator ++ (void);
      bool operator != (expunged_const_iterator) const;
      bool operator == (expunged_const_iterator) const;

      //--
      expunged_const_iterator(void);
      expunged_const_iterator(std::vector<unsigned int>::iterator i);
    };

    //--
    expunged_const_iterator beginExpunged(void);
    expunged_const_iterator endExpunged(void);

    //----------------------------------------------------------------------
    class flagupdates_const_iterator {
    private:
      std::map<unsigned int, unsigned int>::iterator internal;

    public:
      unsigned int first(void) const;
      unsigned int second(void) const;

      void operator ++ (void);
      bool operator != (flagupdates_const_iterator) const;

      //--
      flagupdates_const_iterator(void);
      flagupdates_const_iterator(std::map<unsigned int, unsigned int>::iterator i);
    };

    //--
    flagupdates_const_iterator beginFlagUpdates(void);
    flagupdates_const_iterator endFlagUpdates(void);

    //--
    void addExpunged(unsigned int uid);
    void addFlagUpdates(unsigned int uid, unsigned int flags);
    void setExists(unsigned int n);
    void setRecent(unsigned int n);
    unsigned int getExists(void) const;
    unsigned int getRecent(void) const;
    bool newExists(void) const;
    bool newRecent(void) const;

    //--
    PendingUpdates(void);
    ~PendingUpdates(void);

  private:
    std::vector<unsigned int> expunges;
    std::map<unsigned int, unsigned int> flagupdates;

    unsigned int exists;
    unsigned int recent;
    bool newexists;
    bool newrecent;
  };

  bool Binc::pendingUpdates(Mailbox *, int type, bool rescan, bool showAll = false);
}

#endif
