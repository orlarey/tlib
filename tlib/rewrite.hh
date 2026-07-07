/*
 * TLIB : tree library
 * Copyright (C) 2003-2026 GRAME, Centre National de Creation Musicale
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef __REWRITE__
#define __REWRITE__

#include <unordered_map>

#include "symbol.hh"
#include "tlib-error.hh"
#include "tree.hh"

/**
 * Bottom-up rewriting of symbolic trees (see REWRITE-SPEC.md).
 *
 * treeRewrite(root, rule) applies 'rule' bottom-up to every node of the DAG,
 * with a memo local to the call :
 *  - sharing is preserved : a subtree seen several times is transformed once;
 *  - reconstruction is minimal : an unchanged node returns the same pointer;
 *  - no property is attached to the trees by the traversal;
 *  - the rule is never applied to SYMREC nodes : a recursive definition
 *    rec(var, body) is traversed through its body (RECDEF property), and
 *    every other occurrence of the same SYMREC pointer (recursive reference
 *    or shared occurrence) resolves through the memo.
 *
 * The rule is any callable of signature Tree(Tree), like tmap's tfun. It
 * receives a node whose branches are already transformed, and returns either
 * the same pointer ("no local change") or the replacement tree.
 *
 * treeRewrite() creates a fresh variable for every rec(var, body) : pure, the
 * old tree keeps its RECDEF untouched, but under the identity rule the
 * result is only alpha-equivalent to the input (areEquiv, not ==).
 * treeRewriteInPlace() reuses the same variable : rec(var, newBody) updates
 * RECDEF on the shared SYMREC(var) node. Destructive on the old tree, but
 * pointer-stable : treeRewriteInPlace(t, identity) == t.
 */

template <class Rule>
Tree treeRewriteMemo(Tree t, Rule& rule, std::unordered_map<Tree, Tree>& memo)
{
    auto it = memo.find(t);
    if (it != memo.end()) {
        return it->second;
    }

    Tree var  = nullptr;
    Tree body = nullptr;
    if (isRec(t, var, body)) {
        // a symbolic reference whose variable was never defined by a
        // rec(var, body) reaches this point with a null body : caller error
        TLIB_ASSERT(body != nullptr);
        Tree newVar = tree(unique("W"));
        // resolves recursive references and shared occurrences of t during
        // the body traversal; ref(newVar) and rec(newVar, newBody) are the
        // same hash-consed pointer, so this memo entry is already final
        memo[t]      = ref(newVar);
        Tree newBody = treeRewriteMemo(body, rule, memo);
        return rec(newVar, newBody);
    }

    int  ar = t->arity();
    Tree r  = t;
    if (ar > 0) {
        bool changed = false;
        tvec br(ar);
        for (int i = 0; i < ar; i++) {
            br[i]   = treeRewriteMemo(t->branch(i), rule, memo);
            changed = changed || (br[i] != t->branch(i));
        }
        if (changed) {
            r = tree(t->node(), br);
        }
    }
    Tree result = rule(r);
    memo[t]     = result;
    return result;
}

template <class Rule>
Tree treeRewrite(Tree root, Rule&& rule)
{
    std::unordered_map<Tree, Tree> memo;
    return treeRewriteMemo(root, rule, memo);
}

template <class Rule>
Tree treeRewriteInPlaceMemo(Tree t, Rule& rule, std::unordered_map<Tree, Tree>& memo)
{
    auto it = memo.find(t);
    if (it != memo.end()) {
        return it->second;
    }

    Tree var  = nullptr;
    Tree body = nullptr;
    if (isRec(t, var, body)) {
        TLIB_ASSERT(body != nullptr);
        // self-mapping : recursive references and shared occurrences of t
        // resolve to t itself; rec(var, newBody) below returns this same
        // pointer, so the memo entry is already final
        memo[t]      = t;
        Tree newBody = treeRewriteInPlaceMemo(body, rule, memo);
        return rec(var, newBody);
    }

    int  ar = t->arity();
    Tree r  = t;
    if (ar > 0) {
        bool changed = false;
        tvec br(ar);
        for (int i = 0; i < ar; i++) {
            br[i]   = treeRewriteInPlaceMemo(t->branch(i), rule, memo);
            changed = changed || (br[i] != t->branch(i));
        }
        if (changed) {
            r = tree(t->node(), br);
        }
    }
    Tree result = rule(r);
    memo[t]     = result;
    return result;
}

template <class Rule>
Tree treeRewriteInPlace(Tree root, Rule&& rule)
{
    std::unordered_map<Tree, Tree> memo;
    return treeRewriteInPlaceMemo(root, rule, memo);
}

#endif
