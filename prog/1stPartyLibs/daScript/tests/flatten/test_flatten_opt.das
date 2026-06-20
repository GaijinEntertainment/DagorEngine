options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten
require math

//! Differential net for the flatten OPTIMIZE arc (preshader extraction + CSE +
//! regroup-to-share). Hoist/dedup passes are VALUE-EXACT, so their twins compare
//! with exact `equal` (unlike the reassoc arc). Carve-outs compare approx: a twin
//! where PHASE_FUSED packs a mul-add into hardware-fused `mad` (p_vec, the r_*
//! returns), and the regroup twins themselves (re-pairing `C - x` moves the float
//! association). The FIRED proof (a `_preshader_` let on top, a shared `_cse_`
//! let) lives in `no_aot/test_flatten_fold.das` (empty `flatten_opt_residuals`)
//! and the cap_preshader / cap_cse node-count shaders; here we only assert the
//! optimization preserved the result.

// Module globals are UNIFORM (preshader-eligible); function params are VARYING.
var G_GAIN = 2.5
var G_BIAS = -0.75
var G_TINT = float3(0.3, 0.59, 0.11)
var G_SEED = 5

def gi(p : int) : int {
    return p * 3 + 1
}

// ----- preshader extraction: whole-uniform let -----
[flatten]
def p_whole(x : float) : float {
    let u = G_GAIN * 2.0 + 1.0
    return x * u + u
}

// ----- preshader: a uniform subtree BURIED under a varying parent -----
[flatten]
def p_buried(x : float3) : float3 {
    return x * (dot(G_TINT, float3(1.0, 1.0, 1.0)) + G_GAIN)
}

// ----- preshader: uniform local feeding another uniform local (topological order) -----
[flatten]
def p_chain(x : float) : float {
    let a = G_GAIN * 2.0
    let b = a + G_BIAS
    return x * b
}

// ----- preshader: vector-typed uniform -----
[flatten]
def p_vec(x : float3) : float3 {
    let tint = G_TINT * G_GAIN + float3(G_BIAS, G_BIAS, G_BIAS)
    return x + tint
}

// ----- CSE: a varying subtree computed twice -----
[flatten]
def c_var(x : float) : float {
    let a = (x * x + 1.0) * 2.0
    let b = (x * x + 1.0) * 3.0
    return a + b
}

// ----- CSE: commutative-reordered dup must match (reassoc canonical key) -----
[flatten]
def c_commute(x : float; y : float) : float {
    let a = x * y + 1.0
    let b = y * x + 2.0
    return a + b
}

// ----- CSE: a uniform dup → shared let in the preshader region -----
[flatten]
def c_uni(x : float) : float {
    let p = G_GAIN * G_GAIN + 1.0
    let q = (G_GAIN * G_GAIN + 1.0) * 2.0
    return x * p + x * q
}

// ----- alias elimination: a pure `let b = a` copy must collapse (b -> a), value unchanged -----
[flatten]
def c_alias(x : float) : float {
    let a = x * x + 1.0
    let b = a
    return a * 2.0 + b * 3.0
}

// ----- value coverage: a uniform helper call reused inside an unrolled loop. The inliner can emit
// a reassigned `var __flat_loc = _preshader` slot; alias elimination must leave it alone (collapsing
// a reassigned var onto its const source is a compile error — the deterministic repro is flatten-fuzz
// seed 3, fixed by the X-mutability guard in eliminate_aliases). -----
[flatten]
def c_loop_uni(x : int) : int {
    var acc = 0
    for (_i in range(2)) {
        acc += gi(G_SEED) + x
    }
    return acc
}

var PAL : array<float>

// ----- CSE under a wrapper node: the repeated index `(a*3+b)&3` sits under two ExprAt nodes.
// flatten preserves ExprAt/ExprCast/ExprMakeTuple/ExprPtr2Ref (see lift_expr), and the opt-pass
// recursions must descend through them — else the dup is tallied (universal visitor) but never
// replaced (manual dispatch), leaving a CSE residual and spinning the round loop. -----
[flatten]
def c_at(a : int; b : int) : float {
    return PAL[(a * 3 + b) & 3] + PAL[(a * 3 + b) & 3]
}

var G_BUF : int[8]

// ----- indexed STORE marks the base var mutable: G_BUF[...] reads must NOT be CSE-shared across
// the stores (a hoisted pre-store read returns the stale value). The ExprAt lvalue hides the base
// from a naive `ExprCopy.left is ExprVar` check. -----
[flatten]
def c_store(a : int; b : int) : int {
    G_BUF[(a * 3 + b) & 7] = a * 2
    G_BUF[(a * 3 + b) & 7] ++
    return G_BUF[(a * 3 + b) & 7]
}

// ----- regroup-to-share: reassoc canonicalizes `1 - b + e` to `(e - b) + 1` which splits the
// `1 - b` the albedo line carries whole; the regroup re-pairs it so CSE shares ONE node -----
[flatten]
def r_oneminus(b : float; e : float; c : float) : float {
    let albedo = c * (1.0 - b)
    let alpha = 1.0 - b + e
    return albedo + alpha * 2.0
}

// ----- regroup is keyed on the exact const, not just 1.0 -----
[flatten]
def r_anyconst(b : float; e : float; c : float) : float {
    let p = c * (0.5 - b)
    let q = 0.5 - b + e
    return p + q * 2.0
}

// ----- no grouped twin anywhere -> no regroup, canonical order stands (differential guard) -----
[flatten]
def r_notwin(b : float; e : float) : float {
    return 1.0 - b + e
}

var GRID : array<float>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, 0.0, 1.0, 2.5, 7.0, 100.0]
    PAL <- [1.0, 2.0, 3.0, 4.0]
}

def cmp3f(t : T?; a, b : float3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "fused-mad approx mismatch: {a} vs {b}")
}

def cmp3f_approx(t : T?; a, b : float3) {
    fapprox(t, a.x, b.x); fapprox(t, a.y, b.y); fapprox(t, a.z, b.z)
}

[test]
def test_flatten_opt(t : T?) {
    t |> run("preshader: whole-uniform let -> _preshader_") @(t : T?) {
        for (v in GRID) {
            t |> equal(p_whole_flat(v), p_whole(v))
        }
    }
    t |> run("preshader: buried uniform subtree in return") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, p_buried_flat(float3(v, v * 0.5, -v)), p_buried(float3(v, v * 0.5, -v)))
        }
    }
    t |> run("preshader: uniform local chain (topological order)") @(t : T?) {
        for (v in GRID) {
            t |> equal(p_chain_flat(v), p_chain(v))
        }
    }
    t |> run("preshader: vector-typed uniform (approx — fused mad in the preshader)") @(t : T?) {
        for (v in GRID) {
            cmp3f_approx(t, p_vec_flat(float3(v, -v, v * 2.0)), p_vec(float3(v, -v, v * 2.0)))
        }
    }
    t |> run("CSE: varying dup -> _cse_") @(t : T?) {
        for (v in GRID) {
            t |> equal(c_var_flat(v), c_var(v))
        }
    }
    t |> run("CSE: commutative-reordered dup matches") @(t : T?) {
        for (v in GRID) {
            t |> equal(c_commute_flat(v, v * 0.5 - 1.0), c_commute(v, v * 0.5 - 1.0))
        }
    }
    t |> run("CSE: uniform dup -> preshader region") @(t : T?) {
        for (v in GRID) {
            t |> equal(c_uni_flat(v), c_uni(v))
        }
    }
    t |> run("alias: pure copy let collapses") @(t : T?) {
        for (v in GRID) {
            t |> equal(c_alias_flat(v), c_alias(v))
        }
    }
    t |> run("alias: reassigned var from uniform is NOT collapsed") @(t : T?) {
        for (v in GRID) {
            let n = int(v)
            t |> equal(c_loop_uni_flat(n), c_loop_uni(n))
        }
    }
    t |> run("CSE: repeated index under ExprAt is deduped") @(t : T?) {
        for (v in GRID) {
            let n = int(v)
            t |> equal(c_at_flat(n, n + 1), c_at(n, n + 1))
        }
    }
    t |> run("regroup-to-share: const-minus re-pairs toward the grouped twin (approx — fused mad)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, r_oneminus_flat(v, v * 0.5, -v), r_oneminus(v, v * 0.5, -v))
            fapprox(t, r_anyconst_flat(v, v * 0.5, -v), r_anyconst(v, v * 0.5, -v))
            // no twin -> reassoc's canonical `(e - b) + 1` stands (approx: the association moved)
            fapprox(t, r_notwin_flat(v, v * 0.5), r_notwin(v, v * 0.5))
        }
    }
    t |> run("CSE: indexed-store base is mutable (no read shared across stores)") @(t : T?) {
        for (v in GRID) {
            let n = int(v)
            // Reset the buffer before EACH twin: a stale shared read would otherwise pick up the
            // previous twin's result and mask the bug.
            for (i in range(8)) {
                G_BUF[i] = 0
            }
            let want = c_store(n, n + 1)
            for (i in range(8)) {
                G_BUF[i] = 0
            }
            t |> equal(c_store_flat(n, n + 1), want)
        }
    }
}
