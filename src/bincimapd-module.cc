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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_DLSUPPORT
#include <dlfcn.h>
#endif

#include "session.h"
#include "bincimapd-module.h"

using namespace std;
using namespace Binc;

//----------------------------------------------------------------------
Module::Module(void) : handle(0), ref(0)
{
  ref = new int;
  *ref = 1;
}

//----------------------------------------------------------------------
Module::Module(const Module & module)
{
  handle = module.handle;
  ref = module.ref;
  ++(*ref);
}

//----------------------------------------------------------------------
Module::Module &Module::operator=(const Module &module)
{
  if (&module != this) {
    handle = module.handle;
    ref = module.ref;
    ++(*ref);
  }

  return *this;
}

//----------------------------------------------------------------------
bool Module::open(const string &module)
{
#ifdef HAVE_DLSUPPORT
  handle = dlopen (module.size() == 0 ? 0 : module.c_str(), RTLD_NOW|RTLD_GLOBAL);
#endif
  return (handle != 0);

}

//----------------------------------------------------------------------
Module::~Module(void)
{
  --(*ref);
  if (*ref == 0) {
#ifdef HAVE_DLSUPPORT
    if (handle != 0)
      dlclose(handle);
#endif
    delete ref;
  }
}

//----------------------------------------------------------------------
void *Module::getSymbol(const string &s)
{
  if (handle == 0) {
    setLastError("Module not opened, dynamic loading unsupported.");
    return 0;
  }

  void *symbol = 0;
#ifdef HAVE_DLSUPPORT
  symbol = dlsym(handle, s.c_str());
  if ((error = dlerror()) != 0) {
    session.setLastError("Symbol " + s + " not found");
    return 0;
  }
#endif

  return symbol;
}

//----------------------------------------------------------------------
void Module::setLastError(const string &err)
{
  error = err;
}

//----------------------------------------------------------------------
const string &Module::getLastError(void) const
{
  return error;
}

//----------------------------------------------------------------------
BincModule::BincModule(void) : Module(), loaded(false)
{
}

//----------------------------------------------------------------------
BincModule::~BincModule(void)
{
  if (*ref == 1)
    unload();
}

//----------------------------------------------------------------------
void BincModule::load(BrokerFactory & bf)
{
  loadFunc load = (loadFunc)getSymbol("bincimap_module_load");
  load(bf);
  loaded = true;
}


//----------------------------------------------------------------------
void BincModule::unload(void)
{
  unloadFunc unload = (unloadFunc)getSymbol("bincimap_module_unload");
  unload();
}
