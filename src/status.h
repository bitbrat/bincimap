/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/mailbox/status.h
 *  
 *  Description:
 *    Declaration of the Status class.
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

#ifndef status_h_included
#define status_h_included

namespace Binc {

  //------------------------------------------------------------------------
  class Status {

    //--
    int recent;
    int messages;
    int unseen;
    int uidvalidity;
    int uidnext;

    //--
    int statusid;

  public:

    //--
    inline void setMessages(int i)  { messages = i; }
    inline void setRecent(int i) { recent = i; }
    inline void setStatusID(int i) { statusid = i; }
    inline void setUnseen(int i) { unseen = i; }
    inline void setUidValidity(int i) { uidvalidity = i; }
    inline void setUidNext(int i) { uidnext = i; }
    
    //--
    inline int getMessages(void) const { return messages; }
    inline int getRecent(void) const { return recent; }
    inline int getStatusID(void) const { return statusid; }
    inline int getUnseen(void) const { return unseen; }
    inline int getUidValidity(void) const { return uidvalidity; }
    inline int getUidNext(void) const { return uidnext; }


    //--
    Status(void);
    ~Status(void);
  };
}

#endif
