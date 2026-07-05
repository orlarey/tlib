/*
 * TLIB : tree library
 * Copyright (C) 2003-2026 GRAME, Centre National de Creation Musicale
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <iostream>
#include <string>

#include "tlib.hh"

static void printCheck(const char* label, bool ok, bool& allOk)
{
    allOk = allOk && ok;
    std::cout << "check " << label << ": " << (ok ? "yes" : "NO") << "\n";
}

int main()
{
    tlib::init();

    bool ok = true;

    Tree E = rec(tree(symbol("mix"), ref(1),
                      rec(tree(symbol("tap"), ref(1), ref(2),
                               rec(tree(symbol("hold"), ref(1), ref(2), ref(3))))),
                      tree(symbol("sum"), ref(1), rec(tree(symbol("echo"), ref(1), ref(2))))));
    Tree G = tree(symbol("foo"), E, E, E);

    std::cout << "E = " << toDeBruijnString(E) << "\n\n";
    std::cout << "G = " << toDeBruijnString(G) << "\n\n";

    Tree S1 = deBruijn2Sym(E);
    Tree S2 = deBruijn2Sym(E);
    Tree S3 = deBruijn2Sym(E);

    std::cout << "S1 = deBruijn2Sym(E)\n" << toSymbolicString(S1) << "\n\n";
    std::cout << "S2 = deBruijn2Sym(E)\n" << toSymbolicString(S2) << "\n\n";
    std::cout << "S3 = deBruijn2Sym(E)\n" << toSymbolicString(S3) << "\n\n";

    printCheck("S1 != S2", S1 != S2, ok);
    printCheck("S1 != S3", S1 != S3, ok);
    printCheck("S2 != S3", S2 != S3, ok);
    std::cout << "\n";

    Tree C1 = tree(symbol("foo"), S1, S2, S3);
    std::cout << "C1 = foo(S1, S2, S3)\n" << toSymbolicString(C1) << "\n\n";

    Tree C1db = sym2deBruijn(C1);
    std::cout << "sym2deBruijn(C1) = " << toDeBruijnString(C1db) << "\n\n";
    printCheck("sym2deBruijn(C1) == G", C1db == G, ok);
    std::cout << "\n";

    Tree GS = deBruijn2Sym(G);
    std::cout << "deBruijn2Sym(G)\n" << toSymbolicString(GS) << "\n\n";
    printCheck("areEquiv(deBruijn2Sym(G), C1)", areEquiv(GS, C1), ok);
    printCheck("deBruijn2Sym(G) preserves G branch sharing",
               GS->arity() == 3 && GS->branch(0) == GS->branch(1) &&
                   GS->branch(1) == GS->branch(2),
               ok);

    tlib::cleanup();
    return ok ? 0 : 1;
}
