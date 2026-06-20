options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten
require math

//! Differential net for the swizzle lane fold: `ctor(args…).mask` resolves every output lane to
//! its provenance (scalar arg / lane of an opaque vector / const) and re-emits the cheapest
//! equivalent — the base itself, one swizzle, a scalar, a splat, or a narrower ctor. Pure lane
//! selection is bit-exact, so every compare here is EXACT equal. The structural FIRED half lives
//! in no_aot/test_flatten_fold.das.

// ----- full leading vector arg selected whole: the ctor and the 1f die -----
[flatten]
def s_ctor3(v : float3) : float3 {
    return float4(v, 1.0).xyz
}

// ----- nested swizzle composes through the ctor: float4(v.zyx, 1).xz -> v.zx -----
[flatten]
def s_nestedxz(v : float3) : float2 {
    return float4(v.zyx, 1.0).xz
}

// ----- single lane kills the whole ctor -----
[flatten]
def s_lane(a : float; b : float; c : float) : float {
    return float3(a, b, c).y
}

// ----- swizzle-of-swizzle composes to one mask -----
[flatten]
def s_swsw(v : float4) : float2 {
    return v.zyx.xz
}

// ----- mixed ctor: selected scalar + const re-pack narrower -----
[flatten]
def s_mixed(v : float2; s : float) : float2 {
    return float4(v, s, 2.0).zw
}

// ----- dropped lanes' compute dies with them -----
[flatten]
def s_dropdead(x : float) : float2 {
    return float4(sin(x), x, 3.0, cos(x)).yz
}

// ----- splat ctor narrows to a splat -----
[flatten]
def s_splat(s : float) : float2 {
    return float4(s).xy
}

// ----- identity selection just drops the swizzle node -----
[flatten]
def s_identity(v : float2; s : float) : float3 {
    return float3(v, s).xyz
}

// ----- all-const lanes collapse to a vector literal -----
[flatten]
def s_const(x : float) : float2 {
    return float4(1.0, 2.0, 3.0, 4.0).wy + float2(x)
}

// ----- component-read args re-vectorize to the base itself -----
[flatten]
def s_revec(v : float3) : float3 {
    return float3(v.x, v.y, v.z).xyz
}

// ----- permutation re-pack: the extract dies, args reorder -----
[flatten]
def s_perm(a : float; b : float; c : float) : float3 {
    return float3(a, b, c).zyx
}

// ----- whole-base run + scalar ride into a narrower ctor -----
[flatten]
def s_widen(v : float2; s : float) : float3 {
    return float4(v, s, 4.0).xyz
}

// ----- repeated source lane: the duplicate clones -----
[flatten]
def s_repeat(a : float; b : float; c : float; d : float) : float3 {
    return float4(a, b, c, d).xyx
}

// ----- partial base run stays: re-packing would pay extracts the original didn't -----
[flatten]
def s_partial(v : float2; s : float) : float2 {
    return float3(v, s).yz
}

var GRID : array<float>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, 0.0, 1.0, 2.5, 7.0, 100.0]
}

def eq2(t : T?; a, b : float2) {
    t |> equal(a.x, b.x)
    t |> equal(a.y, b.y)
}

def eq3(t : T?; a, b : float3) {
    t |> equal(a.x, b.x)
    t |> equal(a.y, b.y)
    t |> equal(a.z, b.z)
}

[test]
def test_flatten_swizzle(t : T?) {
    t |> run("whole-base / nested / swizzle-of-swizzle — exact") @(t : T?) {
        for (v in GRID) {
            eq3(t, s_ctor3_flat(float3(v, -v, 2.5)), s_ctor3(float3(v, -v, 2.5)))
            eq2(t, s_nestedxz_flat(float3(v, -v, 2.5)), s_nestedxz(float3(v, -v, 2.5)))
            eq2(t, s_swsw_flat(float4(v, -v, 2.5, 0.125)), s_swsw(float4(v, -v, 2.5, 0.125)))
        }
    }
    t |> run("single lane / mixed re-pack / dead lanes — exact") @(t : T?) {
        for (v in GRID) {
            t |> equal(s_lane_flat(v, -v, 2.5), s_lane(v, -v, 2.5))
            eq2(t, s_mixed_flat(float2(v, -v), v * 0.5), s_mixed(float2(v, -v), v * 0.5))
            eq2(t, s_dropdead_flat(v), s_dropdead(v))
        }
    }
    t |> run("splat / identity / const literal — exact") @(t : T?) {
        for (v in GRID) {
            eq2(t, s_splat_flat(v), s_splat(v))
            eq3(t, s_identity_flat(float2(v, -v), v * 0.5), s_identity(float2(v, -v), v * 0.5))
            eq2(t, s_const_flat(v), s_const(v))
        }
    }
    t |> run("re-vectorize / permutation / narrower ctor / repeat — exact") @(t : T?) {
        for (v in GRID) {
            eq3(t, s_revec_flat(float3(v, -v, 2.5)), s_revec(float3(v, -v, 2.5)))
            eq3(t, s_perm_flat(v, -v, 2.5), s_perm(v, -v, 2.5))
            eq3(t, s_widen_flat(float2(v, -v), v * 0.5), s_widen(float2(v, -v), v * 0.5))
            eq3(t, s_repeat_flat(v, -v, 2.5, 0.125), s_repeat(v, -v, 2.5, 0.125))
        }
    }
    t |> run("partial base run stays put — exact") @(t : T?) {
        for (v in GRID) {
            eq2(t, s_partial_flat(float2(v, -v), v * 0.5), s_partial(float2(v, -v), v * 0.5))
        }
    }
}
