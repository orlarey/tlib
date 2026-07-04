/*
 * TLIB : tree library
 * Copyright (C) 2003-2026 GRAME, Centre National de Creation Musicale
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * A small benchmark of the hash-consing machinery : builds a large population
 * of trees (forcing several hash table rehashes), then measures the cache-hit
 * path (rebuilding already existing trees) and property traffic.
 */

#include <chrono>
#include <iostream>

#include "tlib.hh"

using clk = std::chrono::steady_clock;

static double ms(clk::time_point t0, clk::time_point t1)
{
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// Build a balanced binary tree of the given depth over distinct leaves,
// with enough sharing to be representative of compiler workloads.
static Tree build(int depth, int salt)
{
    if (depth == 0) {
        return tree(salt % 1000);  // bounded leaf alphabet => heavy sharing
    }
    return tree(symbol("op"), build(depth - 1, salt * 2 + 1), build(depth - 1, salt * 2 + 7));
}

int main()
{
    const int kCreations = 2000000;
    const int kDepth     = 18;

    // 1. creation : many small distinct trees, forcing table growth
    auto t0 = clk::now();
    Tree acc = nil();
    for (int i = 0; i < kCreations; i++) {
        acc = cons(tree(i), acc);
    }
    auto t1 = clk::now();
    std::cout << "creation  : " << kCreations << " list cells in " << ms(t0, t1) << " ms\n";

    // 2. reuse : rebuilding the same deep tree only hits the cache
    t0        = clk::now();
    Tree big1 = build(kDepth, 1);
    t1        = clk::now();
    std::cout << "build     : depth-" << kDepth << " tree in " << ms(t0, t1) << " ms\n";

    t0        = clk::now();
    Tree big2 = build(kDepth, 1);
    t1        = clk::now();
    std::cout << "rebuild   : same tree (pure cache hits) in " << ms(t0, t1) << " ms\n";
    std::cout << "identity  : " << (big1 == big2 ? "shared (OK)" : "NOT SHARED (BUG)") << "\n";

    // 3. properties : annotate then re-read a chain of nodes
    Tree key = tree(symbol("benchkey"));
    t0       = clk::now();
    Tree l   = acc;
    while (isList(l)) {
        hd(l)->setProperty(key, l);
        l = tl(l);
    }
    t1 = clk::now();
    std::cout << "prop set  : " << kCreations << " setProperty in " << ms(t0, t1) << " ms\n";

    t0       = clk::now();
    size_t n = 0;
    l        = acc;
    while (isList(l)) {
        if (hd(l)->getProperty(key)) {
            n++;
        }
        l = tl(l);
    }
    t1 = clk::now();
    std::cout << "prop get  : " << n << " getProperty in " << ms(t0, t1) << " ms\n";

    tlib::cleanup();
    return 0;
}
