/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    storage.h
 *  
 *  Description:
 *    Declaration of the Binc::Storage class
 *
 *  Authors:
 *    Andreas Aardal Hanssen <andreas-binc@bincimap.org>
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
#ifndef STORAGEPARSER_H_INCLUDED
#define STORAGEPARSER_H_INCLUDED
#include <map>
#include <string>

#include <stdio.h>

namespace Binc {
  class Storage {
  public:
    enum Mode {
      ReadOnly, WriteOnly
    };

    Storage(const std::string &fileName, Mode mode);
    ~Storage(void);

    bool eof(void) const;

    bool get(std::string *section, std::string *key, 
	     std::string *value);

    bool put(const std::string &section, const std::string &key,
	     const std::string &value);

    bool commit(void);

    std::string getLastError(void) const;

  protected:
    void setLastError(const std::string &errorString) const;

    enum State {
      Searching, Section, Key, Value, 
      AliasKey, AliasColon, AliasValue, AliasEnd
    };

  private:
    FILE *fp;
    int fd;
    std::string fileName;
    State state;

    Mode mode;

    std::string lastError;
    std::string curSection;
    bool atEndOfFile;
    bool firstSection;
    std::map<std::string, std::string> aliases;
  };

  inline std::string Storage::getLastError(void) const
  {
    return lastError;
  }

  inline bool Storage::eof(void) const
  {
    return atEndOfFile;
  }
}

#endif
