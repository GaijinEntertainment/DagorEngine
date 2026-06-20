options gen2
options indenting = 4
options _flatten_expand_lerp = true

require dastest/testing_boost public
require daslib/flatten
require math

//! Twin corpus for `options _flatten_expand_lerp` — the opt-in lerp disassembly for backends
//! WITHOUT a native lerp instruction. Under the option the fold expands `lerp(a,b,t)` into
//! `(b-a)*t + a` so reassoc/CSE/preshader see through it, and PHASE_FUSED re-packs the
//! surviving `mad(b-a, t, a)` shape back into one `lerp` call — the expand → re-fuse round
//! trip that is OFF by default. Float differentials compare approx (the das lerp builtin is
//! FMA-contracted, the expanded form is raw interp ops — same contract as test_flatten_mad.das).
//! The structural half (round trip survives, the merge shape proves expansion FIRED, uniform
//! bounds split into a preshader mad) lives in no_aot/test_flatten_fold.das.

var G_A = 0.25
var G_B = 4.0

// ----- round trip: expand -> nothing folds -> re-fuse to the same call -----
[flatten]
def le_round(a : float; b : float; t : float) : float {
    return lerp(a, b, t)
}

[flatten]
def le_vec(a : float3; b : float3; t : float) : float3 {
    return lerp(a, b, t)
}

// ----- const selectors: fire on the call before any expansion -----
[flatten]
def le_t0(a : float; b : float) : float {
    return lerp(a, b, 0.0)
}

[flatten]
def le_t1(a : float; b : float) : float {
    return lerp(a, b, 1.0)
}

// ----- the smin shape: expansion hands the pieces to reassoc/mad fusion; whether the lerp
// ----- re-fuses depends on the canonical term order, so this twin is differential-only
[flatten]
def le_merge(d : float; ds : float; h : float; k : float) : float {
    return lerp(d, ds, h) + k * h * (h - 1.0)
}

// ----- uniform bounds: expansion exposes `G_B - G_A` to the preshader split -----
[flatten]
def le_preshader(x : float) : float {
    return lerp(G_A, G_B, x)
}

var GRID : array<float>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, 0.0, 1.0, 2.5, 7.0, 100.0]
}

def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "expanded-lerp approx mismatch: {a} vs {b}")
}

def cmp3f_approx(t : T?; a, b : float3) {
    fapprox(t, a.x, b.x); fapprox(t, a.y, b.y); fapprox(t, a.z, b.z)
}

[test]
def test_flatten_lerp_expand(t : T?) {
    t |> run("round trip — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, le_round_flat(v, 7.5, 0.3), le_round(v, 7.5, 0.3))
        }
    }
    t |> run("vector round trip — value approx") @(t : T?) {
        for (v in GRID) {
            cmp3f_approx(t, le_vec_flat(float3(v, -v, 1.0), float3(2.0, 3.0, 4.0), 0.3),
                le_vec(float3(v, -v, 1.0), float3(2.0, 3.0, 4.0), 0.3))
        }
    }
    t |> run("const selectors still collapse") @(t : T?) {
        for (v in GRID) {
            t |> equal(le_t0_flat(v, 7.5), le_t0(v, 7.5))
            t |> equal(le_t1_flat(v, 7.5), le_t1(v, 7.5))
        }
    }
    t |> run("merge shape — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, le_merge_flat(2.0, v, 0.4, 0.22), le_merge(2.0, v, 0.4, 0.22))
        }
    }
    t |> run("uniform-bound lerp — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, le_preshader_flat(v), le_preshader(v))
        }
    }
}
