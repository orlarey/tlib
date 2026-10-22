// A property declared at namespace scope : its constructor makes a symbol (unique) during the
// static initialization, before main and possibly before the tables of symbol.cpp are built.
// The program is linked before the library, the order in which those tables come last.
#include <iostream>
#include "tlib.hh"

static property<int> gDepth;

int main()
{
    tlib::init();
    gDepth.set(tree(1), 3);
    int d = 0;
    const bool found = gDepth.get(tree(1), d);
    std::cout << "static-init : a namespace-scope property " << ((found && d == 3) ? "works" : "FAILS") << std::endl;
    return (found && d == 3) ? 0 : 1;
}
