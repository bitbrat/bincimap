/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    bincimapd-module.cc
 *  
 *  Description:
 *    Implementation of loadable modules
 *
 *  Authors:
 *    Eivind Kvedalen <eivind@kvedalen.name>
 *
 *  Bugs:
 *
 *  ChangeLog:
 *
 *  --------------------------------------------------------------------
 *  Copyright 2003 Eivind Kvedalen
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
#ifndef _BINCIMAPD_MODULE_H_
#define _BINCIMAPD_MODULE_H_

#include <string>
#include <vector>
#include "broker.h"

namespace Binc {

  class Module {
  private:
    std::string error;
    void * handle;
  protected:
    int * ref;
  public:
    Module(void);
    Module(const Module &module);
    Module &operator =(const Module &module);
    ~Module(void);

    bool open(const std::string &module);
    void * getSymbol(const std::string &s);

    const std::string &getLastError(void) const;
    void setLastError(const std::string &err);
  };
  
  class BincModule : public Module {
    bool loaded;

    typedef void (*loadFunc)(BrokerFactory &bf);
    typedef void (*unloadFunc)(void);

  public:
    BincModule(void);
    ~BincModule(void);

    void load(BrokerFactory & bf);
    void unload(void);
  };

}

#endif
