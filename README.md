# tlib — a tree library with hash-consing

Standalone version of the tree library used at the heart of the
[Faust](https://faust.grame.fr).

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

`tlib-benchmark` runs a small performance suite inspired by Faust compiler
workloads: low/high-sharing tree construction, logical and unique-node
traversals, occurrence annotations, tree properties, `property2<Tree>`
memoization, and recursive tree conversion. Pass an integer scale factor to
increase the workload:

```bash
./build/tlib-benchmark 3
```

The output columns are:

- `work`: number of logical operations for the scenario (usually logical tree
  nodes visited or property operations attempted).
- `ms`: wall-clock time for the measured block.
- `Mops/s`: `work / ms / 1000`, useful for comparing runs on the same machine.
- `note`: scenario-specific sanity data, such as number of distinct shared
  nodes, cache hits, or occurrence counts.

Benchmark groups:

- `build-low-sharing`: builds a full binary expression tree whose leaves are
  mostly distinct. This stresses raw tree creation, hash-table growth, and the
  low-cache-hit path.
- `build-high-sharing`: builds a much larger logical binary tree from a small
  state space, so many subtrees collapse to the same `Tree`. This measures
  hash-consing under heavy sharing.
- `rebuild-high-sharing`: rebuilds the same high-sharing tree immediately. The
  result should be pointer-identical to the previous one; this measures the
  cache-hit lookup path.
- `walk-logical-occurrences`: recursively walks every logical occurrence in a
  shared tree, revisiting shared subtrees every time they appear.
- `walk-unique-shared-nodes`: walks the same tree but uses `CTree::gVisitTime`
  to visit each shared node once. This approximates compiler passes that avoid
  repeated work on DAGs.
- `Occur-all-visits`: uses `Occur` to count subtree occurrences in a tree with
  extreme sharing. This measures recursive occurrence annotation when every
  logical occurrence is counted.
- `sharing-first-visit-annotate`: annotates a large mostly-unshared tree with a
  fresh property key, visiting each node once. This is close to Faust sharing
  analysis (`shlysis`) first-visit behavior.
- `property-set-many-hosts` / `property-get-many-hosts`: sets and reads one
  property key across many different trees, representative of compiler passes
  that attach one analysis result per node.
- `property-set-one-host` / `property-get-one-host`: sets and reads many
  distinct property keys on the same tree, representative of worst-case
  property-map growth on a hot node.
- `property2-set-one-box` / `property2-get-one-box`: memoizes one tree under
  many environment trees using `property2<Tree>`, matching the Faust
  `eval(box, env)` and pattern-matcher use case.
- `build-debruijn-rec`: builds a deep de Bruijn recursive tree and checks that
  the enclosing `rec` closes it.
- `debruijn-to-symbolic`: converts that recursive tree to symbolic form,
  exercising substitution, recursive-tree properties, and hash-consing.
- `debruijn-to-symbolic-hit`: converts the same tree again and should hit the
  memoized result.
- `lift-open-rec-body`: applies `lift` to the open recursive body, measuring
  recursive-tree traversal and memoization for free de Bruijn references.

The library builds as a static library `libtlib.a`; add `tlib/` to your
include path and link against it. Requires C++17, no external dependencies.

## Origin

Detached from `compiler/tlib` in the Faust compiler (Y. Orlarey, GRAME,
2002-2026), following the same approach as
[DirectedGraph](https://github.com/orlarey/DirectedGraph). Compared to the
in-compiler version, the only changes are the removal of the compiler-global
couplings (symbols and tuning flags now owned by the library, pluggable error
handling) — the data structures and algorithms are identical.

## License

GNU Lesser General Public License version 2.1 or later — see
[LICENSE](LICENSE).
