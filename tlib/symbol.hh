/************************************************************************
 ************************************************************************
    FAUST compiler
    Copyright (C) 2003-2018 GRAME, Centre National de Creation Musicale
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

/** \file symbol.hh
 * A library of functions to create and manipulate symbols with a unique name.
 *
 *  <b>API:</b>
 *
 *   - Sym q = symbol("abcd");     <i>returns the symbol of name "abcd"</i>
 *   - const char* s = name(q);    <i>returns the name of symbol q</i>
 *   - Sym q = unique("TOTO");	 <i>returns a new unique symbol of name "TOTOnnn"</i>
 *
 *  <b>Properties:</b>
 *
 *     If p and q are two symbols then :
 *  	   p != q  <=>  name(p) != name(q)
 *
 */

#ifndef __SYMBOL__
#define __SYMBOL__

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "export.hh"
#include "garbageable.hh"

//--------------------------------SYMBOL-------------------------------------

class Symbol;
typedef Symbol* Sym;

/**
 * Optional constructor identity attached to an interned symbol.
 *
 * The domain identifies a constructor signature and the opcode identifies one
 * constructor within that signature. A null domain represents the absence of
 * a tag and is never a valid registered domain.
 */
struct SymbolTag {
    Sym           domain = nullptr;
    std::uint16_t opcode = 0;
};

/**
 * Symbols are unique objects with a name stored in a hash table.
 */
class Symbol : public Garbageable {
   private:
    static const std::size_t kInitialHashTableSize = 511;  ///< initial size of the hash table (prime)
    static std::size_t        gHashTableSize;   ///< current size of the hash table (grows as needed)
    static std::size_t        gHashTableCount;  ///< number of symbols currently stored in the table
    static Symbol**       gSymbolTable;     ///< hash table used to store the symbols (grows by rehashing)
    static std::map<std::string, std::size_t> gPrefixCounters;

    static double gHashLoadFactor;  ///< load factor triggering table growth (see setHashLoadFactor)

    ///< cheap check, called on every get()/isnew() : lazily allocates the table on first use
    static void ensureHashTableAllocated();
    ///< called only right before inserting a new symbol ; rehash into a larger table once the
    ///< load factor is too high. Not called on a lookup that finds an existing symbol.
    static void growHashTableIfNeeded();

    // Fields
    std::string fName;  ///< Name of the symbol
    std::size_t fHash;  ///< Hash key computed from the name and used to determine the hash table entry
    Sym         fNext;  ///< Next symbol in the hash table entry
    void*       fData;  ///< Field to user disposal to store additional data
    Sym         fDomain;  ///< Optional signature domain, null while the symbol is untagged
    std::uint16_t fOpcode;  ///< Constructor identity within fDomain; meaningful only when tagged

    // Constructors & destructors
    Symbol(const std::string&, std::size_t hsh,
           Sym nxt);  ///< Constructs a new symbol ready to be placed in the hash table
    ~Symbol();        ///< The destructor is never used

    // Others
    bool equiv(std::size_t hash, const std::string& str)
        const;  ///< Check if the name of the symbol is equal to string \p str
    static std::size_t calcHashKey(
        const std::string& str);  ///< Compute the 32-bits hash key of string \p str

    // Static methods
    static Sym get(const std::string& str);     ///< Get the symbol of name \p str
    static Sym prefix(const std::string& str);  ///< Creates a new symbol of name prefixed by \p str
    static bool isnew(
        const std::string& str);  ///< Returns \b true if no symbol of name \p str exists

   public:
    std::ostream& print(std::ostream& fout) const;  ///< print a symbol on a stream

    friend Sym         symbol(const char* str);
    friend Sym         symbol(const std::string& str);
    friend Sym         unique(const char* str);
    friend const char* name(Sym sym);

    friend void* getUserData(Sym sym);
    friend void  setUserData(Sym sym, void* d);
    friend bool  getSymbolTag(Sym sym, SymbolTag& tag);
    friend void  registerSymbolTag(Sym sym, Sym domain, std::uint16_t opcode);

    static void init();

    ///< Set the load factor that triggers hash table growth (default 0.7).
    ///< A pure performance knob : it never changes the symbols created.
    static void setHashLoadFactor(double f) { gHashLoadFactor = f; }
};

inline Sym symbol(const char* str)
{
    return Symbol::get(str);
}  ///< Returns (and creates if new) the symbol of name \p str
inline Sym symbol(const std::string& str)
{
    return Symbol::get(str);
}  ///< Returns (and creates if new) the symbol of name \p str
inline Sym unique(const char* str)
{
    return Symbol::prefix(str);
}  ///< Returns a new unique symbol of name strxxx
inline const char* name(Sym sym)
{
    return sym->fName.c_str();
}  ///< Returns the name of a symbol

inline void* getUserData(Sym sym)
{
    return sym->fData;
}  ///< Returns user data
inline void setUserData(Sym sym, void* d)
{
    sym->fData = d;
}  ///< Set user data

//--------------------------------------------------------------------------
// Public API: optional domain/opcode tags
//--------------------------------------------------------------------------

/**
 * Read the constructor tag registered on \p sym.
 *
 * Return true and copy the complete tag to \p tag when one is present.
 * Return false without modifying \p tag when the symbol is untagged. A null
 * symbol is invalid and is reported through the TLIB error handler.
 */
TLIB_API bool getSymbolTag(Sym sym, SymbolTag& tag);

/**
 * Register \p sym as opcode \p opcode in \p domain.
 *
 * Repeating the same registration is a no-op, which lets independent
 * initialization paths safely declare the same constructor. A null symbol, a
 * null domain, or a conflicting registration is reported through the TLIB
 * error handler; an existing tag is never overwritten. This storage is
 * independent from getUserData()/setUserData().
 */
TLIB_API void registerSymbolTag(Sym sym, Sym domain, std::uint16_t opcode);

inline std::ostream& operator<<(std::ostream& s, const Symbol& n)
{
    return n.print(s);
}

#endif
