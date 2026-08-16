options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten
require math

//! Differential tests for the same-condition select-accumulation fusion (flatten_opt
//! fuse_select_chains). An unrolled loop with a LOOP-INVARIANT guard (`if (uniform) { acc OP= tap
//! }`) emits a chain of selects all on ONE condition; the fusion hoists that condition out of all
//! N selects (strip every link but the last, retarget the tail's false arm to the base). It is
//! bit-exact (the op tree is preserved, not reassociated), so it fires for ANY op in the chain
//! (+/*/min/max/clamp/mad), for non-zero bases, and is NOT fast-math gated. Each twin runs against
//! its original over a grid spanning the guard-false and guard-true sides. The control
//! (`sf_pervar`, guard depends on the loop counter → distinct masks) must NOT fuse but stays
//! value-correct. The structural FIRED proof (select count collapses) lives in
//! tests/flatten/no_aot/test_flatten_fold.das via twin_bodies.

// ----- additive, base 0 -----
[flatten]
def sf_add(s, n : float) : float {
    var acc = 0.0
    for (i in range(4)) {
        if (s > 0.5) {
            acc += n * float(i)
        }
    }
    return acc
}

// ----- additive, NON-zero base -----
[flatten]
def sf_add_base(s, n : float) : float {
    var acc = 5.0
    for (i in range(4)) {
        if (s > 0.5) {
            acc += n * float(i + 1)
        }
    }
    return acc
}

// ----- multiplicative, base 1 -----
[flatten]
def sf_mul(s, n : float) : float {
    var prod = 1.0
    for (i in range(4)) {
        if (s > 0.5) {
            prod *= 1.0 + n * float(i)
        }
    }
    return prod
}

// ----- min reduction (exact; reassoc-free fusion makes the gate moot) -----
[flatten]
def sf_min(s, n : float) : float {
    var acc = 1000.0
    for (i in range(4)) {
        if (s > 0.5) {
            acc = min(acc, n * float(i + 1))
        }
    }
    return acc
}

// ----- nested op in the true arm (clamp): Tk references acc once, non-trivially -----
[flatten]
def sf_clamp(s, n : float) : float {
    var acc = 0.0
    for (i in range(5)) {
        if (s > 0.5) {
            acc = clamp(acc + n * float(i + 1), 0.0, 50.0)
        }
    }
    return acc
}

// ----- vector accumulate (a real tap loop shape) -----
[flatten]
def sf_vec(s : float; c : float3) : float3 {
    var acc = float3(0.0, 0.0, 0.0)
    for (i in range(3)) {
        if (s > 0.5) {
            acc += c * float(i + 1)
        }
    }
    return acc
}

// ----- CONTROL: guard depends on the loop counter → per-iteration distinct masks, must NOT fuse -----
[flatten]
def sf_pervar(s, n : float) : float {
    var acc = 0.0
    for (i in range(4)) {
        if (float(i) > s) {
            acc += n
        }
    }
    return acc
}

// ----- CONTROL: fault-capable true arm (INTEGER division) must NOT fuse — stripping the select
// would run `acc / d` unconditionally, panicking on d == 0 when the guard is false (the original
// short-circuits). The eval-safety gate keeps the selects. -----
[flatten]
def sf_intdiv(s : float; d : int) : int {
    var acc = 100
    for (_i in range(4)) {
        if (s > 0.5) {
            acc = acc / d
        }
    }
    return acc
}

// ----- driver -----

var SG : array<float>
var NG : array<float>

[init]
def fill_grids {
    SG <- [-1.0, 0.0, 0.4, 0.5, 0.6, 1.0, 2.5]   // straddles the guard threshold 0.5
    NG <- [-3.0, -0.5, 0.0, 1.0, 2.5, 7.0]
}

def cmp3(t : T?; a, b : float3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

[test]
def test_flatten_select_fusion(t : T?) {
    t |> run("additive (base 0)") @(t : T?) {
        for (s in SG) { for (n in NG) { t |> equal(sf_add_flat(s, n), sf_add(s, n)) } }
    }
    t |> run("additive (non-zero base)") @(t : T?) {
        for (s in SG) { for (n in NG) { t |> equal(sf_add_base_flat(s, n), sf_add_base(s, n)) } }
    }
    t |> run("multiplicative (base 1)") @(t : T?) {
        for (s in SG) { for (n in NG) { t |> equal(sf_mul_flat(s, n), sf_mul(s, n)) } }
    }
    t |> run("min reduction") @(t : T?) {
        for (s in SG) { for (n in NG) { t |> equal(sf_min_flat(s, n), sf_min(s, n)) } }
    }
    t |> run("nested op in true arm (clamp)") @(t : T?) {
        for (s in SG) { for (n in NG) { t |> equal(sf_clamp_flat(s, n), sf_clamp(s, n)) } }
    }
    t |> run("vector accumulate") @(t : T?) {
        for (s in SG) { for (n in NG) { cmp3(t, sf_vec_flat(s, float3(n, -n, 1.0)), sf_vec(s, float3(n, -n, 1.0))) } }
    }
    t |> run("control: per-iteration guard stays value-correct (un-fused)") @(t : T?) {
        for (s in SG) { for (n in NG) { t |> equal(sf_pervar_flat(s, n), sf_pervar(s, n)) } }
    }
    t |> run("control: integer-division true arm value-correct (un-fused)") @(t : T?) {
        for (s in SG) { for (d in [1, 2, 5, -3]) { t |> equal(sf_intdiv_flat(s, d), sf_intdiv(s, d)) } }
    }
    t |> run("safety: guard-false with a zero divisor must NOT divide (gate kept the short-circuit)") @(t : T?) {
        // s <= 0.5 so the guard is false; had the gate fused `acc / d`, the twin would run it
        // unconditionally and panic on d == 0. Un-fused, it short-circuits to the base (100).
        for (s in [-1.0, 0.0, 0.4]) {
            t |> equal(sf_intdiv_flat(s, 0), 100)
            t |> equal(sf_intdiv(s, 0), 100)
        }
    }
}
