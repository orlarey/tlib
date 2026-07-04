/************************************************************************
 ************************************************************************
    TLIB : tree library
    Copyright (C) 2003-2026 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/

#include "garbageable.hh"

#include <list>

// The allocation registries. In the Faust compiler these lived in the
// 'global' singleton; the library owns them directly so it has no
// dependency on any host application state.
static std::list<Garbageable*> gRawObjectTable;
static std::list<Garbageable*> gArrayObjectTable;

// True while cleanup() is running: individual deletes then skip the
// (pointless and slow) removal from the registry.
static bool gHeapCleanup = false;

void Garbageable::cleanup()
{
    gHeapCleanup = true;

    for (Garbageable* obj : gRawObjectTable) {
        delete obj;
    }
    gRawObjectTable.clear();

    for (Garbageable* obj : gArrayObjectTable) {
        delete[] obj;
    }
    gArrayObjectTable.clear();

    gHeapCleanup = false;
}

void* Garbageable::operator new(size_t size)
{
    Garbageable* res = static_cast<Garbageable*>(::operator new(size));
    gRawObjectTable.push_front(res);
    return res;
}

void Garbageable::operator delete(void* ptr)
{
    // An object may be deleted individually during a session: it then has to
    // be removed from the registry so cleanup() doesn't free it twice.
    if (!gHeapCleanup) {
        gRawObjectTable.remove(static_cast<Garbageable*>(ptr));
    }
    ::operator delete(ptr);
}

void* Garbageable::operator new[](size_t size)
{
    Garbageable* res = static_cast<Garbageable*>(::operator new[](size));
    gArrayObjectTable.push_front(res);
    return res;
}

void Garbageable::operator delete[](void* ptr)
{
    if (!gHeapCleanup) {
        gArrayObjectTable.remove(static_cast<Garbageable*>(ptr));
    }
    ::operator delete[](ptr);
}
