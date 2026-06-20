options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten
require math

//! Twin corpus for the fast-math division rewrites: `x / C → x * (1/C)` (fold arm) and
//! `x / U → x * _preshader(1/U)` (extraction, U uniform). Exactness tiers: a power-of-two
//! reciprocal is exact (`equal`); any other reciprocal rounds, so those twins compare
//! `fapprox` — the rounding is the fast-math contract, `options _flatten_no_fast_math`
//! turns it off (see test_flatten_nofastmath.das). A zero divisor keeps the division
//! (no inf literal is minted); integer division is out of scope entirely. The structural
//! FIRED half (no un-rcp'd const/uniform division residual) lives in
//! tests/flatten/no_aot/test_flatten_fold.das via the option-aware oracles; the backend
//! node proof is capability/cap_rcp.shader (one shared per-draw reciprocal).

var G_RCP = 3.0

[flatten]
def r_pow2(x : float) : float {
    return x / 4.0
}

[flatten]
def r_npow2(x : float) : float {
    return x / 3.0
}

[flatten]
def r_vecdiv(v : float3) : float3 {
    return v / 2.0
}

[flatten]
def r_vconst(v : float3) : float3 {
    return v / float3(2.0, 4.0, 8.0)
}

[flatten]
def r_uni(x : float) : float {
    return x / G_RCP
}

[flatten]
def r_two(x, y : float) : float {
    return (x / G_RCP) + (y / G_RCP)
}

[flatten]
def r_zero(x : float) : float {
    return x / 0.0 // nolint:LINT006 — the zero-divisor bail (no inf literal) is what's under test
}

[flatten]
def i_div(n : int) : int {
    return n / 3
}

// ----- differential driver -----

var GRID : array<float>
var NZGRID : array<float>
var IGRID : array<int>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, -0.0, 0.0, 1.0, 2.5, 7.0, 100.0]
    NZGRID <- [-100.0, -3.5, -1.0, 1.0, 2.5, 7.0, 100.0]   // x/0 is inf (inf==inf) but 0/0 is NaN
    IGRID <- [-100, -7, -1, 0, 1, 3, 42, 1000]
}

def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "rcp approx mismatch: {a} vs {b}")
}

def cmp3f(t : T?; a, b : float3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

def cmp3f_approx(t : T?; a, b : float3) {
    fapprox(t, a.x, b.x); fapprox(t, a.y, b.y); fapprox(t, a.z, b.z)
}

[test]
def test_flatten_rcp(t : T?) {
    t |> run("x / 4.0 -> x * 0.25 (pow2 reciprocal, exact)") @(t : T?) {
        for (v in GRID) {
            t |> equal(r_pow2_flat(v), r_pow2(v))
        }
    }
    t |> run("x / 3.0 -> x * (1/3) (rounds, approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, r_npow2_flat(v), r_npow2(v))
        }
    }
    t |> run("v / 2.0 -> v * 0.5 (vector by pow2 scalar, exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, r_vecdiv_flat(float3(v, -v, 1.0)), r_vecdiv(float3(v, -v, 1.0)))
        }
    }
    t |> run("v / float3(2,4,8) -> v * float3(.5,.25,.125) (pow2 lanes, exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, r_vconst_flat(float3(v, -v, 1.0)), r_vconst(float3(v, -v, 1.0)))
        }
    }
    t |> run("x / G -> x * _preshader(1/G) (uniform divisor, approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, r_uni_flat(v), r_uni(v))
        }
    }
    t |> run("two / G share one reciprocal (approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, r_two_flat(v, -v), r_two(v, -v))
        }
    }
    t |> run("x / 0.0 keeps the division (no inf literal)") @(t : T?) {
        for (v in NZGRID) {
            t |> equal(r_zero_flat(v), r_zero(v))
        }
    }
    t |> run("int n / 3 untouched (exact)") @(t : T?) {
        for (n in IGRID) {
            t |> equal(i_div_flat(n), i_div(n))
        }
    }
}
