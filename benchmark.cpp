/*
 * TLIB : tree library
 * Copyright (C) 2003-2026 GRAME, Centre National de Creation Musicale
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Performance scenarios inspired by the Faust compiler use of tlib:
 *   - build large hash-consed expression DAGs with low/high sharing,
 *   - traverse trees by logical occurrences and by unique shared nodes,
 *   - annotate subtrees with occurrence counts,
 *   - stress property maps and binary memoization property2<Tree>,
 *   - build and convert recursive de Bruijn trees.
 *
 * Usage:
 *   ./build/tlib-benchmark [scale]
 *
 * scale is a positive integer. The default scale=1 keeps the suite quick
 * enough for interactive development; larger values increase loop counts.
 */

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "occur.hh"
#include "tlib.hh"

using Clock = std::chrono::steady_clock;

static volatile std::uintptr_t gPtrSink   = 0;
static volatile std::size_t    gCountSink = 0;

static double ms(Clock::time_point t0, Clock::time_point t1)
{ return std::chrono::duration<double, std::milli>(t1 - t0).count(); }

static std::size_t fullBinaryNodes(int depth)
{ return (std::size_t(1) << (depth + 1)) - 1; }

static int scaled(int base, int scale)
{ return base * scale; }

static void report(const std::string& name, std::size_t work, double elapsedMs,
                   const std::string& note = "")
{
    const double rate = (elapsedMs > 0.0) ? (double(work) / elapsedMs / 1000.0) : 0.0;
    std::cout << std::left << std::setw(30) << name << std::right << std::setw(14) << work
              << std::setw(12) << std::fixed << std::setprecision(3) << elapsedMs << std::setw(12)
              << std::setprecision(3) << rate;
    if (!note.empty()) {
        std::cout << "  " << note;
    }
    std::cout << "\n";
}

static Tree makeLowSharingTree(int depth, int seed)
{
    if (depth == 0) {
        return tree(seed);
    }
    return tree(symbol("bin"), makeLowSharingTree(depth - 1, seed * 2 + 1),
                makeLowSharingTree(depth - 1, seed * 2 + 2));
}

static Tree makeHighSharingTree(int depth, int seed, int states)
{
    seed %= states;
    if (depth == 0) {
        return tree(seed % 64);
    }
    return tree(symbol("bin"), makeHighSharingTree(depth - 1, seed * 3 + 1, states),
                makeHighSharingTree(depth - 1, seed * 5 + 7, states));
}

static Tree makeRepeatedTree(int depth, Tree leaf)
{
    if (depth == 0) {
        return leaf;
    }
    Tree x = makeRepeatedTree(depth - 1, leaf);
    return tree(symbol("repeat"), x, x);
}

static Tree makeRecursiveBody(int depth)
{
    Tree body = ref(1);
    for (int i = 0; i < depth; ++i) {
        body = tree(symbol("recop"), body, ref(1), tree(i & 63));
    }
    return body;
}

static std::size_t countLogical(Tree t)
{
    std::size_t n = 1;
    for (int i = 0; i < t->arity(); ++i) {
        n += countLogical(t->branch(i));
    }
    return n;
}

static std::size_t countUnique(Tree t)
{
    if (t->isAlreadyVisited()) {
        return 0;
    }
    t->setVisited();
    std::size_t n = 1;
    for (int i = 0; i < t->arity(); ++i) {
        n += countUnique(t->branch(i));
    }
    return n;
}

static std::size_t distinctNodes(Tree t)
{
    std::set<Tree>    seen;
    std::vector<Tree> stack;
    stack.push_back(t);
    while (!stack.empty()) {
        Tree x = stack.back();
        stack.pop_back();
        if (!seen.insert(x).second) {
            continue;
        }
        for (int i = 0; i < x->arity(); ++i) {
            stack.push_back(x->branch(i));
        }
    }
    return seen.size();
}

static void annotateSharing(Tree key, Tree t)
{
    Tree countTree = t->getProperty(key);
    int  count     = countTree ? countTree->node().getInt() : 0;
    if (count == 0) {
        for (int i = 0; i < t->arity(); ++i) {
            annotateSharing(key, t->branch(i));
        }
    }
    t->setProperty(key, tree(count + 1));
}

static void benchConstruction(int scale)
{
    std::cout << "\n[construction]\n";
    const int lowDepth  = 18 + (scale > 1 ? 1 : 0);
    const int highDepth = 20 + (scale > 2 ? 1 : 0);

    tlib::cleanup();
    auto t0  = Clock::now();
    Tree low = makeLowSharingTree(lowDepth, 1);
    auto t1  = Clock::now();
    gPtrSink = reinterpret_cast<std::uintptr_t>(low);
    report("build-low-sharing", fullBinaryNodes(lowDepth), ms(t0, t1),
           "unique=" + std::to_string(distinctNodes(low)));

    tlib::cleanup();
    t0        = Clock::now();
    Tree high = makeHighSharingTree(highDepth, 1, 128);
    t1        = Clock::now();
    gPtrSink  = reinterpret_cast<std::uintptr_t>(high);
    report("build-high-sharing", fullBinaryNodes(highDepth), ms(t0, t1),
           "unique=" + std::to_string(distinctNodes(high)));

    t0               = Clock::now();
    Tree high2       = makeHighSharingTree(highDepth, 1, 128);
    t1               = Clock::now();
    gPtrSink         = reinterpret_cast<std::uintptr_t>(high2);
    std::string note = (high == high2) ? "cache-hit shared" : "NOT-SHARED";
    report("rebuild-high-sharing", fullBinaryNodes(highDepth), ms(t0, t1), note);
    tlib::cleanup();
}

static void benchTraversal(int scale)
{
    std::cout << "\n[traversal]\n";
    const int depth = 21 + (scale > 2 ? 1 : 0);

    tlib::cleanup();
    Tree        root  = makeHighSharingTree(depth, 1, 96);
    std::size_t logic = fullBinaryNodes(depth);
    std::size_t uniq  = distinctNodes(root);

    auto        t0      = Clock::now();
    std::size_t logical = countLogical(root);
    auto        t1      = Clock::now();
    gCountSink          = logical;
    report("walk-logical-occurrences", logic, ms(t0, t1), "visited=" + std::to_string(logical));

    CTree::startNewVisit();
    t0                 = Clock::now();
    std::size_t unique = countUnique(root);
    t1                 = Clock::now();
    gCountSink         = unique;
    report("walk-unique-shared-nodes", uniq, ms(t0, t1), "visited=" + std::to_string(unique));
    tlib::cleanup();
}

static void benchOccurrences(int scale)
{
    std::cout << "\n[occurrences]\n";
    const int depth = 20 + (scale > 2 ? 1 : 0);

    tlib::cleanup();
    Tree  leaf = tree(symbol("x"));
    Tree  root = makeRepeatedTree(depth, leaf);
    auto  t0   = Clock::now();
    Occur occ(root);
    auto  t1 = Clock::now();
    report("Occur-all-visits", fullBinaryNodes(depth), ms(t0, t1),
           "leaf-count=" + std::to_string(occ.getCount(leaf)));

    Tree sharingRoot = makeLowSharingTree(18, 1);
    Tree key         = tree(unique("sharing-count"));
    t0               = Clock::now();
    annotateSharing(key, sharingRoot);
    t1     = Clock::now();
    Tree c = sharingRoot->getProperty(key);
    report("sharing-first-visit-annotate", fullBinaryNodes(18), ms(t0, t1),
           "root-count=" + std::to_string(c ? c->node().getInt() : 0));
    tlib::cleanup();
}

static void benchProperties(int scale)
{
    std::cout << "\n[properties]\n";
    const int manyHosts = scaled(500000, scale);
    const int hotKeys   = scaled(60000, scale);
    const int envCount  = scaled(80000, scale);

    tlib::cleanup();
    Tree list = nil();
    for (int i = 0; i < manyHosts; ++i) {
        list = cons(tree(i), list);
    }
    Tree key = tree(symbol("many-hosts-key"));

    auto t0 = Clock::now();
    for (Tree l = list; isList(l); l = tl(l)) {
        hd(l)->setProperty(key, l);
    }
    auto t1 = Clock::now();
    report("property-set-many-hosts", manyHosts, ms(t0, t1));

    t0            = Clock::now();
    std::size_t n = 0;
    for (Tree l = list; isList(l); l = tl(l)) {
        n += (hd(l)->getProperty(key) != nullptr);
    }
    t1         = Clock::now();
    gCountSink = n;
    report("property-get-many-hosts", manyHosts, ms(t0, t1), "hits=" + std::to_string(n));
    tlib::cleanup();

    Tree              host = tree(symbol("hot-property-host"));
    std::vector<Tree> keys;
    keys.reserve(hotKeys);
    for (int i = 0; i < hotKeys; ++i) {
        keys.push_back(tree(symbol("hot-key"), tree(i)));
    }

    t0 = Clock::now();
    for (int i = 0; i < hotKeys; ++i) {
        host->setProperty(keys[i], tree(i));
    }
    t1 = Clock::now();
    report("property-set-one-host", hotKeys, ms(t0, t1));

    t0 = Clock::now();
    n  = 0;
    for (int i = 0; i < hotKeys; ++i) {
        n += (host->getProperty(keys[i]) != nullptr);
    }
    t1         = Clock::now();
    gCountSink = n;
    report("property-get-one-host", hotKeys, ms(t0, t1), "hits=" + std::to_string(n));
    tlib::cleanup();

    property2<Tree>   memo;
    Tree              box = tree(symbol("eval-box"), tree(0));
    std::vector<Tree> envs;
    envs.reserve(envCount);
    for (int i = 0; i < envCount; ++i) {
        envs.push_back(tree(symbol("env"), tree(i)));
    }

    t0 = Clock::now();
    for (int i = 0; i < envCount; ++i) {
        memo.set(box, envs[i], tree(i));
    }
    t1 = Clock::now();
    report("property2-set-one-box", envCount, ms(t0, t1));

    t0 = Clock::now();
    n  = 0;
    for (int i = 0; i < envCount; ++i) {
        Tree value = nullptr;
        n += memo.get(box, envs[i], value) && value == tree(i);
    }
    t1         = Clock::now();
    gCountSink = n;
    report("property2-get-one-box", envCount, ms(t0, t1), "hits=" + std::to_string(n));
    tlib::cleanup();
}

static void benchRecursiveTrees(int scale)
{
    std::cout << "\n[recursive-trees]\n";
    const int depth = scaled(1200, scale);

    tlib::cleanup();
    auto t0   = Clock::now();
    Tree body = makeRecursiveBody(depth);
    Tree r    = rec(body);
    auto t1   = Clock::now();
    gPtrSink  = reinterpret_cast<std::uintptr_t>(r);
    report("build-debruijn-rec", depth, ms(t0, t1),
           std::string("closed=") + (isClosed(r) ? "yes" : "no"));

    t0       = Clock::now();
    Tree sym = deBruijn2Sym(r);
    t1       = Clock::now();
    gPtrSink = reinterpret_cast<std::uintptr_t>(sym);
    report("debruijn-to-symbolic", depth, ms(t0, t1));

    t0        = Clock::now();
    Tree sym2 = deBruijn2Sym(r);
    t1        = Clock::now();
    gPtrSink  = reinterpret_cast<std::uintptr_t>(sym2);
    report("debruijn-to-symbolic-hit", depth, ms(t0, t1),
           (sym == sym2) ? "cache-hit" : "NOT-SHARED");

    t0          = Clock::now();
    Tree lifted = lift(body);
    t1          = Clock::now();
    gPtrSink    = reinterpret_cast<std::uintptr_t>(lifted);
    report("lift-open-rec-body", depth, ms(t0, t1));
    tlib::cleanup();
}

int main(int argc, const char* argv[])
{
    int scale = 1;
    if (argc > 1) {
        scale = std::atoi(argv[1]);
        if (scale < 1) {
            scale = 1;
        }
    }

    std::cout << "tlib performance scenarios (scale=" << scale << ")\n";
    std::cout << std::left << std::setw(30) << "case" << std::right << std::setw(14) << "work"
              << std::setw(12) << "ms" << std::setw(12) << "Mops/s"
              << "  note\n";
    std::cout << std::string(82, '-') << "\n";

    benchConstruction(scale);
    benchTraversal(scale);
    benchOccurrences(scale);
    benchProperties(scale);
    benchRecursiveTrees(scale);

    // Keep the sinks observable.
    if (gPtrSink == 0 && gCountSink == 0) {
        std::cerr << "unexpected empty benchmark sinks\n";
        return 1;
    }
    return 0;
}
