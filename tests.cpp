/*
 * TLIB : tree library
 * Copyright (C) 2003-2026 GRAME, Centre National de Creation Musicale
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "dcond.hh"
#include "occur.hh"
#include "tests.hh"
#include "tlib.hh"

// Minimal check helper : prints the failing expression with its location.
static bool checkAux(bool cond, const char* expr, const char* file, int line)
{
    if (!cond) {
        std::cerr << "FAILED : " << expr << " (" << file << ":" << line << ")\n";
    }
    return cond;
}
#define CHECK(cond) ok &= checkAux((cond), #cond, __FILE__, __LINE__)

//-----------------------------------------------------------------------------
// Symbols
//-----------------------------------------------------------------------------

bool checkSymbols()
{
    bool ok = true;

    // interning : same name => same symbol, different name => different symbol
    CHECK(symbol("foo") == symbol("foo"));
    CHECK(symbol("foo") != symbol("bar"));
    CHECK(std::string(name(symbol("foo"))) == "foo");

    // unique() mints fresh names
    Sym u1 = unique("fresh");
    Sym u2 = unique("fresh");
    CHECK(u1 != u2);
    CHECK(std::string(name(u1)) != std::string(name(u2)));

    // user data
    int dummy = 42;
    setUserData(symbol("foo"), &dummy);
    CHECK(getUserData(symbol("foo")) == &dummy);
    setUserData(symbol("foo"), nullptr);

    return ok;
}

//-----------------------------------------------------------------------------
// Nodes
//-----------------------------------------------------------------------------

bool checkNodes()
{
    bool ok = true;

    int    i = 0;
    double d = 0;
    Sym    s = nullptr;
    void*  p = nullptr;

    CHECK(isInt(Node(5), &i) && i == 5);
    CHECK(isDouble(Node(2.5), &d) && d == 2.5);
    CHECK(isSym(Node(symbol("n")), &s) && s == symbol("n"));
    CHECK(isPointer(Node((void*)&i), &p) && p == &i);

    CHECK(Node(5) == Node(5));
    CHECK(Node(5) != Node(6));
    CHECK(Node(5) != Node(5.0));  // int and double are distinct node types

    // arithmetic on nodes, with int/double promotion
    CHECK(addNode(Node(2), Node(3)) == Node(5));
    CHECK(addNode(Node(2), Node(0.5)) == Node(2.5));
    CHECK(mulNode(Node(4), Node(4)) == Node(16));
    CHECK(isZero(Node(0)) && isZero(Node(0.0)) && !isZero(Node(1)));
    CHECK(isOne(Node(1)) && isMinusOne(Node(-1)));
    CHECK(sameMagnitude(Node(-3), Node(3.0)));

    return ok;
}

//-----------------------------------------------------------------------------
// Hash-consing : the central invariant, p != q <=> *p != *q
//-----------------------------------------------------------------------------

bool checkHashConsing()
{
    bool ok = true;

    // same content => same pointer
    CHECK(tree(1) == tree(1));
    CHECK(tree(symbol("x")) == tree(symbol("x")));
    CHECK(tree(symbol("+"), tree(1), tree(2)) == tree(symbol("+"), tree(1), tree(2)));

    // different content => different pointer
    CHECK(tree(1) != tree(2));
    CHECK(tree(symbol("+"), tree(1), tree(2)) != tree(symbol("+"), tree(2), tree(1)));
    CHECK(tree(symbol("+"), tree(1), tree(2)) != tree(symbol("*"), tree(1), tree(2)));

    // shared subtrees are physically shared
    Tree a = tree(symbol("+"), tree(1), tree(2));
    Tree b = tree(symbol("*"), a, a);
    CHECK(b->branch(0) == b->branch(1));

    // serial numbers give a stable deterministic order
    Tree t1 = tree(symbol("first"), tree(101));
    Tree t2 = tree(symbol("second"), tree(102));
    CHECK(t1->serial() < t2->serial());
    CHECK(std::less<CTree*>()(t1, t2));

    return ok;
}

//-----------------------------------------------------------------------------
// Accessors and pattern matching
//-----------------------------------------------------------------------------

bool checkAccessors()
{
    bool ok = true;

    Tree t = tree(symbol("op"), tree(1), tree(2), tree(3));
    CHECK(t->arity() == 3);
    CHECK(t->branch(0) == tree(1));
    CHECK(t->branch(2) == tree(3));
    CHECK(t->node() == Node(symbol("op")));

    Tree x = nullptr, y = nullptr;
    CHECK(isTree(tree(symbol("pair"), tree(7), tree(8)), symbol("pair"), x, y));
    CHECK(x == tree(7) && y == tree(8));
    CHECK(!isTree(tree(symbol("pair"), tree(7), tree(8)), symbol("other"), x, y));

    return ok;
}

//-----------------------------------------------------------------------------
// Conversions
//-----------------------------------------------------------------------------

bool checkConversions()
{
    bool ok = true;

    CHECK(tree2int(tree(42)) == 42);
    CHECK(tree2int(tree(42.0)) == 42);  // double casted to int
    CHECK(tree2double(tree(2.5)) == 2.5);
    CHECK(tree2double(tree(2)) == 2.0);  // int promoted to double
    CHECK(strcmp(tree2str(tree(symbol("hello"))), "hello") == 0);

    int dummy = 0;
    CHECK(tree2ptr(tree(Node((void*)&dummy))) == &dummy);

    return ok;
}

//-----------------------------------------------------------------------------
// Raw properties on trees
//-----------------------------------------------------------------------------

bool checkProperties()
{
    bool ok = true;

    Tree t   = tree(symbol("host"), tree(1));
    Tree key = tree(symbol("mykey"));

    CHECK(t->getProperty(key) == nullptr);
    t->setProperty(key, tree(99));
    CHECK(t->getProperty(key) == tree(99));

    // hash-consing means the property is visible through any alias of t
    CHECK(tree(symbol("host"), tree(1))->getProperty(key) == tree(99));

    t->clearProperty(key);
    CHECK(t->getProperty(key) == nullptr);

    // the fast-property slot is independent from the property list
    t->setFastProperty(tree(7));
    CHECK(t->getFastProperty() == tree(7));
    CHECK(t->getProperty(key) == nullptr);

    return ok;
}

//-----------------------------------------------------------------------------
// Typed property<P> and binary property2<Tree>
//-----------------------------------------------------------------------------

bool checkTypedProperties()
{
    bool ok = true;

    property<int> counts;
    Tree          t = tree(symbol("node"), tree(3));
    int           v = 0;
    CHECK(!counts.get(t, v));
    counts.set(t, 5);
    CHECK(counts.get(t, v) && v == 5);
    counts.clear(t);
    CHECK(!counts.get(t, v));

    // property2 : memoize f(a, b) with 'a' revisited under several 'b'
    property2<Tree> memo;
    Tree            a  = tree(symbol("stable"));
    Tree            b1 = tree(symbol("env1"));
    Tree            b2 = tree(symbol("env2"));
    Tree            r  = nullptr;

    CHECK(!memo.get(a, b1, r));
    memo.set(a, b1, tree(10));
    memo.set(a, b2, tree(20));  // second distinct 'b' : inline slot promoted to map
    CHECK(memo.get(a, b1, r) && r == tree(10));
    CHECK(memo.get(a, b2, r) && r == tree(20));
    CHECK(!memo.get(a, tree(symbol("env3")), r));
    memo.set(a, b1, tree(11));  // overwrite
    CHECK(memo.get(a, b1, r) && r == tree(11));
    memo.clear(a);
    CHECK(!memo.get(a, b1, r));

    return ok;
}

//-----------------------------------------------------------------------------
// Lists
//-----------------------------------------------------------------------------

static Tree double_(Tree t)
{
    return tree(t->node().getInt() * 2);
}

bool checkLists()
{
    bool ok = true;

    CHECK(isNil(nil()));
    CHECK(!isList(nil()));
    CHECK(len(nil()) == 0);

    Tree l = list3(tree(1), tree(2), tree(3));
    CHECK(isList(l) && !isNil(l));
    CHECK(len(l) == 3);
    CHECK(hd(l) == tree(1));
    CHECK(hd(tl(l)) == tree(2));
    CHECK(nth(l, 2) == tree(3));
    CHECK(isNil(nth(l, 10)));

    CHECK(reverse(l) == list3(tree(3), tree(2), tree(1)));
    CHECK(concat(list2(tree(1), tree(2)), list1(tree(3))) == l);
    CHECK(replace(l, 1, tree(9)) == list3(tree(1), tree(9), tree(3)));
    CHECK(lmap(double_, l) == list3(tree(2), tree(4), tree(6)));
    CHECK(lrange(l, 1, 3) == list2(tree(2), tree(3)));

    return ok;
}

//-----------------------------------------------------------------------------
// Sets (ordered lists without duplicates)
//-----------------------------------------------------------------------------

bool checkSets()
{
    bool ok = true;

    Tree s = list2set(list3(tree(1), tree(2), tree(1)));
    CHECK(len(s) == 2);
    CHECK(isElement(tree(1), s) && isElement(tree(2), s) && !isElement(tree(3), s));

    Tree s1 = addElement(tree(1), addElement(tree(2), nil()));
    Tree s2 = addElement(tree(2), addElement(tree(3), nil()));

    // sets are canonical : same elements => same tree, whatever insertion order
    CHECK(s1 == addElement(tree(2), addElement(tree(1), nil())));

    Tree u = setUnion(s1, s2);
    CHECK(len(u) == 3);
    Tree i = setIntersection(s1, s2);
    CHECK(len(i) == 1 && isElement(tree(2), i));
    Tree d = setDifference(s1, s2);
    CHECK(len(d) == 1 && isElement(tree(1), d));

    return ok;
}

//-----------------------------------------------------------------------------
// Environments
//-----------------------------------------------------------------------------

bool checkEnvironments()
{
    bool ok = true;

    Tree env = pushEnv(tree(symbol("x")), tree(1), nil());
    env      = pushEnv(tree(symbol("y")), tree(2), env);
    env      = pushEnv(tree(symbol("x")), tree(3), env);  // shadows the first x

    Tree v = nullptr;
    CHECK(searchEnv(tree(symbol("x")), v, env) && v == tree(3));
    CHECK(searchEnv(tree(symbol("y")), v, env) && v == tree(2));
    CHECK(!searchEnv(tree(symbol("z")), v, env));

    return ok;
}

//-----------------------------------------------------------------------------
// Recursive trees : de Bruijn <-> symbolic
//-----------------------------------------------------------------------------

bool checkRecursiveTrees()
{
    bool ok = true;

    // alpha-equivalence for free : two identical de Bruijn recursions share
    Tree r1 = rec(tree(symbol("f"), ref(1)));
    Tree r2 = rec(tree(symbol("f"), ref(1)));
    CHECK(r1 == r2);

    // aperture : a lone reference is open, the enclosing rec closes it
    CHECK(isOpen(ref(1)));
    CHECK(isClosed(r1));

    Tree body = nullptr;
    CHECK(isRec(r1, body));
    int level = 0;
    CHECK(isRef(ref(2), level) && level == 2);

    // lift increments free references only
    Tree l = lift(ref(1));
    CHECK(isRef(l, level) && level == 2);
    CHECK(lift(r1) == r1);  // closed tree : nothing to lift

    // conversion to symbolic representation
    Tree s = deBruijn2Sym(r1);
    Tree var = nullptr, sbody = nullptr;
    CHECK(isRec(s, var, sbody));
    CHECK(isTree(sbody, symbol("f")));
    CHECK(toDeBruijnString(r1) == "rec(f(ref(1)))");
    Tree back = sym2deBruijn(s);
    CHECK(back == r1);
    CHECK(sym2deBruijn(s) == back);  // deterministic : hash-consing gives the same tree
    CHECK(areEquiv(r1, s));

    // symbolic references
    Tree id = tree(unique("R"));
    CHECK(isRef(ref(id), var) && var == id);

    Tree x = tree(unique("X"));
    Tree y = tree(unique("Y"));
    Tree z = tree(unique("Z"));
    Tree sx = rec(x, tree(symbol("f"), ref(x)));
    Tree sy = rec(y, tree(symbol("f"), ref(y)));
    Tree sz = rec(z, tree(symbol("g"), ref(z)));
    CHECK(sx != sy);
    CHECK(areEquiv(sx, sy));
    CHECK(!areEquiv(sx, sz));

    Tree px = tree(symbol("x"));
    Tree py = tree(symbol("y"));
    Tree psx = rec(px, tree(symbol("f"), ref(px)));
    Tree psy = rec(py, tree(symbol("f"), ref(py)));
    (void)psy;
    CHECK(toSymbolicString(psx) == "x\nwith {\n  x := f(x)\n}");

    Tree sharedRoot = tree(symbol("h"), ref(px), ref(py), ref(px));
    CHECK(toSymbolicString(sharedRoot) ==
          "h(x, y, x)\nwith {\n  x := f(x)\n  y := f(y)\n}");

    // nested recursion : ref(1) points to the inner rec, ref(2) to the outer rec
    Tree nested = rec(tree(symbol("outer"), ref(1), rec(tree(symbol("inner"), ref(1), ref(2)))));
    Tree snested = deBruijn2Sym(nested);
    CHECK(sym2deBruijn(snested) == nested);
    CHECK(areEquiv(nested, snested));

    // Symbolic conversion demo:
    // E is a closed de Bruijn recursion with two recursive nesting levels.
    // G shares the same E three times, while C1 uses three symbolic copies of E
    // produced by separate deBruijn2Sym calls.
    Tree E  = rec(tree(symbol("mix"), ref(1),
                       rec(tree(symbol("tap"), ref(1), ref(2),
                                rec(tree(symbol("hold"), ref(1), ref(2), ref(3))))),
                       tree(symbol("sum"), ref(1), rec(tree(symbol("echo"), ref(1), ref(2))))));
    Tree G  = tree(symbol("foo"), E, E, E);
    Tree S1 = deBruijn2Sym(E);
    Tree S2 = deBruijn2Sym(E);
    Tree S3 = deBruijn2Sym(E);
    CHECK(S1 != S2 && S1 != S3 && S2 != S3);
    CHECK(isClosed(E));
    CHECK(toDeBruijnString(E).find("ref(3)") != std::string::npos);

    Tree C1 = tree(symbol("foo"), S1, S2, S3);
    CHECK(sym2deBruijn(C1) == G);
    CHECK(areEquiv(deBruijn2Sym(G), C1));

    Tree sharedSym = deBruijn2Sym(G);
    CHECK(sharedSym->arity() == 3);
    CHECK(sharedSym->branch(0) == sharedSym->branch(1));
    CHECK(sharedSym->branch(1) == sharedSym->branch(2));
    CHECK(areEquiv(sharedSym, C1));

    return ok;
}

//-----------------------------------------------------------------------------
// DNF/CNF conditions
//-----------------------------------------------------------------------------

bool checkDnfCnf()
{
    bool ok = true;

    Tree a = tree(symbol("condA"));
    Tree b = tree(symbol("condB"));

    Tree da = dnfCond(a);
    Tree db = dnfCond(b);

    // a AND a = a ; a OR a = a
    CHECK(dnfAnd(da, da) == da);
    CHECK(dnfOr(da, da) == da);

    // a is less specific than a AND b
    Tree dab = dnfAnd(da, db);
    CHECK(dnfLess(da, dab));
    CHECK(dnfAnd(da, db) == dnfAnd(db, da));  // commutativity

    Tree ca = cnfCond(a);
    Tree cb = cnfCond(b);
    CHECK(cnfAnd(ca, ca) == ca);
    CHECK(cnfOr(ca, ca) == ca);
    CHECK(cnfLess(ca, ca));
    CHECK(cnfAnd(ca, cb) == cnfAnd(cb, ca));  // commutativity
    CHECK(cnfOr(ca, cb) == cnfOr(cb, ca));

    return ok;
}

//-----------------------------------------------------------------------------
// Occurrences counting
//-----------------------------------------------------------------------------

bool checkOccurrences()
{
    bool ok = true;

    Tree x    = tree(symbol("shared"));
    Tree root = tree(symbol("op"), tree(symbol("f"), x), tree(symbol("g"), x, x));

    Occur occ(root);
    CHECK(occ.getCount(x) == 3);
    CHECK(occ.getCount(root) == 0);  // by convention the root doesn't count itself
    CHECK(occ.getCount(tree(symbol("unrelated#"))) == 0);

    return ok;
}

//-----------------------------------------------------------------------------
// Error handler hook
//-----------------------------------------------------------------------------

static std::string gLastError;
[[noreturn]] static void recordingHandler(const std::string& msg)
{
    gLastError = msg;
    throw std::runtime_error(msg);
}

bool checkErrorHandler()
{
    bool ok = true;

    // default handler : std::runtime_error
    bool caught = false;
    try {
        tree2int(tree(symbol("not_a_number")));
    } catch (std::runtime_error& e) {
        caught = true;
        CHECK(std::string(e.what()).find("integer") != std::string::npos);
    }
    CHECK(caught);

    // custom handler : sees the message, previous handler is returned
    tlib::ErrorHandler previous = tlib::setErrorHandler(recordingHandler);
    caught                      = false;
    try {
        tree2ptr(tree(1));
    } catch (std::runtime_error&) {
        caught = true;
    }
    CHECK(caught);
    CHECK(gLastError.find("pointer") != std::string::npos);
    tlib::setErrorHandler(previous);

    return ok;
}

//-----------------------------------------------------------------------------
// Lifecycle : cleanup() ends a session, the library is reusable right after
//-----------------------------------------------------------------------------

bool checkLifecycle()
{
    bool ok = true;

    Tree before = tree(symbol("survivor"), tree(1), tree(2));
    (void)before;  // any use of 'before' after cleanup() would be invalid

    tlib::cleanup();

    // a fresh session works, including the library-owned symbols (nil, rec...)
    CHECK(isNil(nil()));
    Tree after = tree(symbol("survivor"), tree(1), tree(2));
    CHECK(after->arity() == 2);
    CHECK(isClosed(rec(tree(symbol("f"), ref(1)))));
    CHECK(len(list2(tree(1), tree(2))) == 2);

    return ok;
}

//-----------------------------------------------------------------------------
// Hash table growth : identity must survive many insertions (rehashing)
//-----------------------------------------------------------------------------

bool checkHashTableGrowth()
{
    bool ok = true;

    const int  n     = 100000;  // well past the initial table sizes
    static Sym leafS = symbol("leaf");

    // record a sample of pointers, then verify identity after the table grew
    Tree first = tree(leafS, tree(0));
    for (int i = 0; i < n; i++) {
        (void)tree(leafS, tree(i));
    }
    CHECK(first == tree(leafS, tree(0)));            // pointer identity preserved
    CHECK(tree(leafS, tree(n / 2)) == tree(leafS, tree(n / 2)));
    CHECK(tree(leafS, tree(n - 1)) != tree(leafS, tree(n - 2)));

    return ok;
}
