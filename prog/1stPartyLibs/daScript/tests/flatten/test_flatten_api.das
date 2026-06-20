options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require flat_api_macro
require math

//! Cross-module integration test for the PUBLIC `flatten_function` /
//! `flatten_fold` entries.
//!
//! `flat_api_macro` (a separate module, the real backend topology) calls
//! `flatten_function` then `flatten_fold` directly with its own whitelist
//! {max, min}. This proves both entries are callable from another module's macro,
//! honor a caller-supplied primitive set, and that the post-infer fold collapses
//! const selects. The flattened twin must match an unflattened reference.

// Unflattened reference (identical body) for differential comparison.
def ref_max_min(x : int) : int {
    return max(x, 0) + min(x, 10)
}

[flat_api]
def api_demo(x : int) : int {
    return max(x, 0) + min(x, 10)
}

// A recursive user function the backend whitelists as a leaf primitive. flatten
// must KEEP the call (whitelist precedence), not inline it — inlining a recursive
// function would trip the recursion boundary and fail the compile. That this
// compiles + matches the reference proves whitelisted calls survive un-inlined
// even though they have a (here recursive) body — the [hint]/[stub] shader case.
def myrec(n : int) : int {
    return n <= 0 ? 0 : n + myrec(n - 1)
}

def ref_rec(x : int) : int {
    return myrec(x) + 1
}

[flat_api]
def api_rec(x : int) : int {
    return myrec(x) + 1
}

// A loop-var-gated accumulator: each unrolled copy's `i >= 3` is a compile-time
// constant, so flatten_fold (run via the backend-style cycle 2 in flat_api_macro)
// must collapse the selects and drop the false-iteration self-assigns. Matching the
// reference proves the PUBLIC fold companion works cross-module.
def ref_loopcond(x : int) : int {
    var acc = 0
    for (i in range(5)) {
        if (i >= 3) {
            acc += x
        }
    }
    return acc
}

[flat_api]
def api_loopcond(x : int) : int {
    var acc = 0
    for (i in range(5)) {
        if (i >= 3) {
            acc += x
        }
    }
    return acc
}

// ----- ?: fold side-effect safety -----
// A whitelisted "primitive" WITH a side effect (global write). flatten keeps the
// call (whitelist precedence). The const conditions below arise from loop unrolling
// (i substituted to a literal), so `flatten_fold`'s `const ? a : b` collapse is what
// actually fires. The fold must keep the call in the LIVE arm and drop it from the
// DEAD arm (matching the source `if`, which never runs the dead branch).
var g_calls : int

def g_writer(x : int) : int {
    g_calls ++     // the observable side effect under test
    return x + 100
}

def ref_keep(x : int) : int {
    var r = -1
    for (i in range(1)) {
        if (i >= 0) {       // i=0 → true: g_writer IS called
            r = g_writer(x)
        }
    }
    return r
}

[flat_api]
def api_keep(x : int) : int {
    var r = -1
    for (i in range(1)) {
        if (i >= 0) {
            r = g_writer(x)
        }
    }
    return r
}

def ref_drop(x : int) : int {
    var r = -1
    for (i in range(1)) {
        if (i >= 1) {       // i=0 → false: g_writer is NOT called
            r = g_writer(x)
        }
    }
    return r
}

[flat_api]
def api_drop(x : int) : int {
    var r = -1
    for (i in range(1)) {
        if (i >= 1) {
            r = g_writer(x)
        }
    }
    return r
}

// `e * 0 → 0` is gated by has_sideeffects(e): a side-effecting `e` must NOT be
// dropped. i=0 makes `g_writer(x) * 0`; the call must survive (global incremented)
// even though the product is statically 0.
def ref_mul0(x : int) : int {
    var acc = 0
    for (i in range(1)) {
        acc += g_writer(x) * i      // i=0 → g_writer(x) * 0
    }
    return acc
}

[flat_api]
def api_mul0(x : int) : int {
    var acc = 0
    for (i in range(1)) {
        acc += g_writer(x) * i
    }
    return acc
}

var GRID : array<int>

[init]
def fill_grid {
    GRID <- [-5, -1, 0, 1, 5, 10, 20]
}

[test]
def test_flatten_api(t : T?) {
    t |> run("flatten_function callable cross-module with custom whitelist") @(t : T?) {
        for (v in GRID) {
            t |> equal(api_demo_apiflat(v), ref_max_min(v))
        }
    }
    t |> run("whitelisted recursive user fn is kept, not inlined") @(t : T?) {
        for (v in GRID) {
            t |> equal(api_rec_apiflat(v), ref_rec(v))
        }
    }
    t |> run("flatten_fold collapses const selects cross-module") @(t : T?) {
        for (v in GRID) {
            t |> equal(api_loopcond_apiflat(v), ref_loopcond(v))
        }
    }
    t |> run("?: fold KEEPS a side-effecting call in the const-true arm") @(t : T?) {
        for (v in GRID) {
            g_calls = 0
            let refv = ref_keep(v)
            let refCalls = g_calls          // 1 — source runs g_writer
            g_calls = 0
            let flatv = api_keep_apiflat(v)
            t |> equal(flatv, refv)
            t |> equal(g_calls, refCalls)   // the twin must ALSO call g_writer
        }
    }
    t |> run("?: fold DROPS a side-effecting call from the dead arm (matches source)") @(t : T?) {
        for (v in GRID) {
            g_calls = 0
            let refv = ref_drop(v)
            let refCalls = g_calls          // 0 — source's `if (false)` never calls g_writer
            g_calls = 0
            let flatv = api_drop_apiflat(v)
            t |> equal(flatv, refv)
            t |> equal(g_calls, refCalls)   // the twin must ALSO not call it
        }
    }
    t |> run("e*0 fold is side-effect-gated: g_writer(x)*0 keeps the call") @(t : T?) {
        for (v in GRID) {
            g_calls = 0
            let refv = ref_mul0(v)
            let refCalls = g_calls          // 1 — g_writer runs even though *0
            g_calls = 0
            let flatv = api_mul0_apiflat(v)
            t |> equal(flatv, refv)
            t |> equal(g_calls, refCalls)   // the twin must ALSO call g_writer (not fold *0 away)
        }
    }
}
