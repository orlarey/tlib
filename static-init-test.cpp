// A property declared at namespace scope, built during the static initialization, before
// main and before tlib::init(). The program is linked before the library, the order in which
// the tables of symbol.cpp come last. The property makes its key on first use : its key is
// not mixed up with the first key of the session (whose serial init() would issue again),
// and it is made anew after a cleanup().
#include <iostream>
#include "tlib.hh"

static property<int> gDepth;

int main()
{
    tlib::init();
    property<int> local;  // the first key of the session
    Tree          t = tree(42);
    gDepth.set(t, 3);
    local.set(t, 4);
    int        d        = 0;
    int        e        = 0;
    const bool distinct = gDepth.get(t, d) && local.get(t, e) && d == 3 && e == 4;

    tlib::cleanup();
    tlib::init();
    Tree u = tree(42);
    gDepth.set(u, 5);
    int        f     = 0;
    const bool again = gDepth.get(u, f) && f == 5;

    const bool ok = distinct && again;
    std::cout << "static-init : a namespace-scope property " << (ok ? "works" : "FAILS") << " (" << d << ", " << e
              << ", " << f << ")" << std::endl;
    return ok ? 0 : 1;
}
