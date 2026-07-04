# tlib — a tree library with hash-consing

Standalone version of the tree library used at the heart of the
[Faust](https://faust.grame.fr) compiler since 2002, in the spirit of the
ATerm library.

`tlib` provides immutable trees with **maximal sharing** (hash-consing): two
trees with the same content are always the *same* pointer, so structural
equality is pointer equality, and memoization comes for free by attaching
properties to trees. On top of this core it provides symbols, lists, sets,
environments, recursive trees (de Bruijn ↔ symbolic, with alpha-equivalence
by construction), and typed memoization primitives.

## Quick example

```cpp
#include "tlib.hh"

int main()
{
    // maximal sharing : same content => same pointer
    Tree a = tree(symbol("+"), tree(1), tree(2));
    Tree b = tree(symbol("+"), tree(1), tree(2));
    assert(a == b);

    // lists, sets, environments
    Tree l = list3(tree(1), tree(2), tree(3));
    assert(len(l) == 3 && hd(l) == tree(1));

    // memoization via properties
    property<int> depth;
    depth.set(a, 1);

    // recursive trees : alpha-equivalent recursions are shared
    Tree r1 = rec(tree(symbol("f"), ref(1)));
    Tree r2 = rec(tree(symbol("f"), ref(1)));
    assert(r1 == r2 && isClosed(r1));

    // end of session : frees every tree and symbol in one sweep
    tlib::cleanup();
}
```

## Contents

| Files | Layer |
| :--- | :--- |
| `tlib/tlib.hh` | single entry point: aggregated API + `tlib::init/cleanup` lifecycle |
| `tlib/tree.hh/.cpp` | `CTree`: hash-consing core, properties, aperture |
| `tlib/node.hh/.cpp` | `Node`: tagged union int / int64 / double / symbol / pointer |
| `tlib/symbol.hh/.cpp` | `Symbol`: interned symbol table, `unique()` fresh names |
| `tlib/property.hh` | `property<P>` (unary) and `property2<Tree>` (binary) memoization |
| `tlib/list.hh/.cpp` | lists, sets (canonical ordered lists), environments |
| `tlib/recursive-tree.cpp` | `rec` / `ref`: de Bruijn and symbolic recursive trees |
| `tlib/dcond.hh/.cpp` | boolean conditions in DNF/CNF (optional module) |
| `tlib/occur.hh/.cpp` | subtree occurrence counting (optional module) |
| `tlib/garbageable.hh/.cpp` | session memory model: allocate freely, free all at cleanup |
| `tlib/tlib-error.hh/.cpp` | pluggable error handler (defaults to `std::runtime_error`) |

## Design notes

- **One session per process.** The library keeps its state in static tables
  (like the Faust compiler does). `tlib::cleanup()` frees every tree and
  symbol at once and leaves the library ready for a new session; any
  `Tree`/`Sym` obtained before it is invalid after.
- **Deterministic ordering.** `std::less<CTree*>` is specialized to compare
  stable serial numbers, not addresses, so anything iterated in tree order is
  reproducible from run to run.
- **Growing hash tables.** The CTree/Symbol tables start small and rehash
  when the load factor exceeds 0.7 (tunable with `tlib::setHashLoadFactor`);
  rehashing never moves a tree, so held pointers stay valid.
- **Host-pluggable errors.** Internal errors go through a single handler
  (`tlib::setErrorHandler`); the default throws `std::runtime_error`. A host
  application can throw its own exception type instead.

## Build and test

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tlib-tests
./build/tlib-benchmark
```

The library builds as a static library `libtlib.a`; add `tlib/` to your
include path and link against it. Requires C++17, no external dependencies.

## Origin

Detached from `compiler/tlib` in the Faust compiler (Y. Orlarey, GRAME,
2002-2026), following the same approach as
[DirectedGraph](https://github.com/orlarey/DirectedGraph). Compared to the
in-compiler version, the only changes are the removal of the compiler-global
couplings (symbols and tuning flags now owned by the library, pluggable error
handling) — the data structures and algorithms are identical.
