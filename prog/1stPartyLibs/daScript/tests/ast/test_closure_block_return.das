options gen2
options indenting = 4
options rtti

require dastest/testing_boost public
require daslib/ast

//! A bare `return` inside a block exits THE BLOCK only. The interpreter's invokeEx code0 fast
//! path handed enumerators (for_each_function et al.) the closure body's single statement WITHOUT
//! the SimNode_ClosureBlock epilogue that clears stopForReturn — so the first firing return
//! killed the remaining iterations AND leaked the stop flag into the calling function, silently
//! skipping its tail (rst.das documents() lost every Uncategorized function this way). The
//! single-statement and multi-statement closures below are semantically identical; they must
//! count the same. JIT and AOT were always correct — this is an interpreter-only regression rail.

// single-statement closure body (one IfThenElse) — rides the code0 fast path
def private count_single(mod : rtti::Module?) : int {
    var n = 0
    for_each_function(mod, "") $(func) {
        if (func.flags.generated) {
            return
        } else {
            n++
        }
    }
    return n
}

// multi-statement closure body — never rode the fast path, always correct
def private count_multi(mod : rtti::Module?; var fired : int&) : int {
    var n = 0
    for_each_function(mod, "") $(func) {
        if (func.flags.generated) {
            fired++
            return
        }
        n++
    }
    return n
}

// tail sentinel: a leaked stop flag would skip everything after the enumeration
def private tail_runs(mod : rtti::Module?) : bool {
    for_each_function(mod, "") $(func) {
        if (func.flags.generated) {
            return
        }
    }
    return true
}

[test]
def test_closure_block_return(t : T?) {
    ast_gc_guard() {
        var mod = get_module("$")
        var fired = 0
        let multi = count_multi(mod, fired)
        t |> success(fired > 0, "the early return must actually fire (module $ has generated fns)")
        t |> success(multi > 0, "enumeration continues past a block return")
        t |> equal(count_single(mod), multi)
        t |> success(tail_runs(mod), "block return must not leak past the enumeration")
    }
}
