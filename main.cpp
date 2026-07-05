/*
 * TLIB : tree library
 * Copyright (C) 2003-2026 GRAME, Centre National de Creation Musicale
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <iostream>

#include "tests.hh"
#include "tlib.hh"

int main(int, const char**)
{
    std::cout << "Tests tlib library\n";

    tlib::init();

    bool r = true;

    r &= checkSymbols();
    r &= checkNodes();
    r &= checkHashConsing();
    r &= checkAccessors();
    r &= checkConversions();
    r &= checkProperties();
    r &= checkTypedProperties();
    r &= checkLists();
    r &= checkSets();
    r &= checkEnvironments();
    r &= checkRecursiveTrees();
    r &= checkDnfCnf();
    r &= checkOccurrences();
    r &= checkErrorHandler();
    r &= checkHashTableGrowth();
    r &= checkLifecycle();  // last : it wipes the tree population

    if (r) {
        std::cout << "All tests passed\n";
        return 0;
    }
    std::cout << "SOME TESTS FAILED\n";
    return 1;
}
