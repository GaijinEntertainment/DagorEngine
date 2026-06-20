options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten
require math

//! Differential net for the PHASE_FUSED horizontal-add → dot re-pack: a sum of same-vector lane
//! reads — bare `v.x`, const-weighted `v.x * c`, negated, repeated — collapses into `dot(v, mask)`
//! with missing lanes masked by 0. The dot reorders the adds and runs hardware-dotted, so floats
//! compare approx (same contract as the mad corpus). The structural FIRED half (a `dot(` call in
//! the twin) lives in no_aot/test_flatten_fold.das.

// ----- bare horizontal adds, every width -----
[flatten]
def d_hadd2(v : float2) : float {
    return v.x + v.y
}

[flatten]
def d_hadd3(v : float3) : float {
    return v.x + v.y + v.z
}

[flatten]
def d_hadd4(v : float4) : float {
    return v.x + v.y + v.z + v.w
}

// ----- partial lanes of a wider vector: the skipped lane rides as mask 0 -----
[flatten]
def d_partial4(v : float4) : float {
    return v.x + v.y + v.z
}

// ----- const-weighted lanes (the luma idiom) -----
[flatten]
def d_luma(c : float3) : float {
    return c.x * 0.299 + c.y * 0.587 + c.z * 0.114
}

// ----- negated lane -> negative mask weight -----
[flatten]
def d_neg(v : float3) : float {
    return v.x - v.y
}

// ----- non-lane leftover term + negated lanes -----
[flatten]
def d_mixed(v : float3; e : float) : float {
    return e - v.x - v.y
}

// ----- repeated lane accumulates its weight: (2, 1) -----
[flatten]
def d_dup(v : float2) : float {
    return v.x + v.x + v.y
}

// ----- weighted AND negated lanes in one chain -> dot(a, (2, 3, -4)) -----
[flatten]
def d_wneg(a : float3) : float {
    return 2.0 * a.x + 3.0 * a.y - 4.0 * a.z
}

// ----- two base vectors in one sum -> two independent dots -----
[flatten]
def d_two(a : float2; b : float2) : float {
    return a.x + a.y + b.x + b.y
}

// ----- lane sum buried inside a call argument -----
[flatten]
def d_nested(v : float2) : float {
    return sin(v.x + v.y)
}

// ----- variable weight is NOT a lane term: one lane left -> no dot, mad fuses instead -----
[flatten]
def d_varweight(v : float2; w : float) : float {
    return v.x * w + v.y
}

var GRID : array<float>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, 0.0, 1.0, 2.5, 7.0, 100.0]
}

def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "dot-fused approx mismatch: {a} vs {b}")
}

[test]
def test_flatten_dot(t : T?) {
    t |> run("bare horizontal adds — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, d_hadd2_flat(float2(v, 7.5)), d_hadd2(float2(v, 7.5)))
            fapprox(t, d_hadd3_flat(float3(v, -v, 2.5)), d_hadd3(float3(v, -v, 2.5)))
            fapprox(t, d_hadd4_flat(float4(v, -v, 2.5, 0.125)), d_hadd4(float4(v, -v, 2.5, 0.125)))
        }
    }
    t |> run("partial lanes — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, d_partial4_flat(float4(v, -v, 2.5, 1.0e9)), d_partial4(float4(v, -v, 2.5, 1.0e9)))
        }
    }
    t |> run("weighted lanes (luma) — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, d_luma_flat(float3(v, v * 0.5, -v)), d_luma(float3(v, v * 0.5, -v)))
        }
    }
    t |> run("negated / mixed / repeated / weighted-negated lanes — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, d_neg_flat(float3(v, 7.5, 1.0)), d_neg(float3(v, 7.5, 1.0)))
            fapprox(t, d_mixed_flat(float3(v, -v, 1.0), v * 0.25), d_mixed(float3(v, -v, 1.0), v * 0.25))
            fapprox(t, d_dup_flat(float2(v, 7.5)), d_dup(float2(v, 7.5)))
            fapprox(t, d_wneg_flat(float3(v, -v, 2.5)), d_wneg(float3(v, -v, 2.5)))
        }
    }
    t |> run("two base vectors — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, d_two_flat(float2(v, 2.0), float2(-v, 3.0)), d_two(float2(v, 2.0), float2(-v, 3.0)))
        }
    }
    t |> run("lane sum inside a call arg — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, d_nested_flat(float2(v, 0.75)), d_nested(float2(v, 0.75)))
        }
    }
    t |> run("variable weight stays out — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, d_varweight_flat(float2(v, 7.5), v * 0.5), d_varweight(float2(v, 7.5), v * 0.5))
        }
    }
}
