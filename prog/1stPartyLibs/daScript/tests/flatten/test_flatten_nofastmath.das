options gen2
options indenting = 4
options no_unused_function_arguments = false
options _flatten_no_fast_math = true

require dastest/testing_boost public
require daslib/flatten
require math

//! Twin corpus for `options _flatten_no_fast_math` — the user opt-out of flatten's
//! value-changing rewrites. Under the option every gated rewrite (float `x*0`/`x-x`,
//! float reassoc, lerp const-selectors) is SKIPPED, so each twin executes the same op
//! sequence as its original and every differential below compares EXACT `equal` —
//! including the reassoc shapes that need `fapprox` in the fast-math corpus. The
//! bit-exact passes (flattening, predication, preshader extraction, CSE, mad fusion)
//! must still run; the int twins prove the integer folds (exact) still fire. The
//! structural FIRED/SKIPPED half lives in tests/flatten/no_aot/test_flatten_fold.das,
//! which compiles this file as a unit and asserts the gated shapes SURVIVE in the
//! twins while the option-aware oracle (`flatten_fold_residuals` called with the unit's
//! detected `_flatten_no_fast_math` flag) stays clean.

var G_NF = 2.5

// ----- gated rewrites: skipped under the option, value bit-exact -----

[flatten]
def nf_mul0(x : float) : float {
    return x * 0.0
}

[flatten]
def nf_vmul0(v : float3) : float3 {
    return v * float3(0.0, 0.0, 0.0)
}

[flatten]
def nf_subself(x : float) : float {
    return (x * 3.0) - (x * 3.0) // nolint:LINT007 — the x-x gate is exactly what's under test
}

[flatten]
def nf_reassoc(x : float) : float {
    return 0.25 + x + 0.5
}

[flatten]
def nf_lerp0(a, b : float) : float {
    return lerp(a, b, 0.0)
}

[flatten]
def nf_div(x : float) : float {
    return x / 3.0
}

[flatten]
def nf_unidiv(x : float) : float {
    return x / G_NF
}

[flatten]
def nf_zkill(x : float) : float2 {
    return float2(sin(x) * 12.0, 13.0) * float2(0.0, 123.0)
}

[flatten]
def nf_mj(x, y, z : float) : float3 {
    return float3(x * 13.0, y * 13.0, z * 12.0)
}

// ----- exact rewrites: still fire under the option -----

[flatten]
def nf_imul0(n : int) : int {
    return n * 0
}

[flatten]
def nf_ireassoc(n : int) : int {
    return 2 + n + 3
}

[flatten]
def nf_mad(x, y, z : float) : float {
    return x * y + z
}

[flatten]
def nf_preshader(x : float) : float {
    let u = G_NF * 2.0 + 1.0
    return x * u + u
}

// ----- differential driver -----

var GRID : array<float>
var IGRID : array<int>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, -0.0, 0.0, 1.0, 2.5, 7.0, 100.0]
    IGRID <- [-100, -7, -1, 0, 1, 3, 42, 1000]
}

def cmp3f(t : T?; a, b : float3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

[test]
def test_flatten_nofastmath(t : T?) {
    t |> run("x * 0.0 NOT folded — bit-exact") @(t : T?) {
        for (v in GRID) {
            t |> equal(nf_mul0_flat(v), nf_mul0(v))
        }
    }
    t |> run("v * vec0 NOT folded — bit-exact") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, nf_vmul0_flat(float3(v, -v, 1.0)), nf_vmul0(float3(v, -v, 1.0)))
        }
    }
    t |> run("x - x NOT folded — bit-exact") @(t : T?) {
        for (v in GRID) {
            t |> equal(nf_subself_flat(v), nf_subself(v))
        }
    }
    t |> run("float reassoc skipped — EXACT equal (no fapprox needed)") @(t : T?) {
        for (v in GRID) {
            t |> equal(nf_reassoc_flat(v), nf_reassoc(v))
        }
    }
    t |> run("lerp const-selector skipped — bit-exact") @(t : T?) {
        for (a in GRID) {
            t |> equal(nf_lerp0_flat(a, 7.5), nf_lerp0(a, 7.5))
        }
    }
    t |> run("x / C NOT rewritten to *(1/C) — bit-exact") @(t : T?) {
        for (v in GRID) {
            t |> equal(nf_div_flat(v), nf_div(v))
        }
    }
    t |> run("x / uniform NOT rcp'd — bit-exact") @(t : T?) {
        for (v in GRID) {
            t |> equal(nf_unidiv_flat(v), nf_unidiv(v))
        }
    }
    t |> run("float zero-lane kill skipped — bit-exact") @(t : T?) {
        for (v in GRID) {
            let r = nf_zkill_flat(v)
            let o = nf_zkill(v)
            t |> equal(r.x, o.x)
            t |> equal(r.y, o.y)
        }
    }
    t |> run("majority-factor compensation skipped — bit-exact") @(t : T?) {
        for (v in GRID) {
            let r = nf_mj_flat(v, -v, 1.0)
            let o = nf_mj(v, -v, 1.0)
            t |> equal(r.x, o.x)
            t |> equal(r.y, o.y)
            t |> equal(r.z, o.z)
        }
    }
    t |> run("int n * 0 still folds (exact)") @(t : T?) {
        for (n in IGRID) {
            t |> equal(nf_imul0_flat(n), nf_imul0(n))
        }
    }
    t |> run("int reassoc still fires (exact)") @(t : T?) {
        for (n in IGRID) {
            t |> equal(nf_ireassoc_flat(n), nf_ireassoc(n))
        }
    }
    t |> run("mad fusion still fires — das-exact") @(t : T?) {
        for (v in GRID) {
            t |> equal(nf_mad_flat(v, 3.0, 1.5), nf_mad(v, 3.0, 1.5))
        }
    }
    t |> run("preshader extraction still runs — bit-exact") @(t : T?) {
        for (v in GRID) {
            t |> equal(nf_preshader_flat(v), nf_preshader(v))
        }
    }
}
