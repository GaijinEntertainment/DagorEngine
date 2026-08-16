options gen2
options indenting = 4
options _flatten_hadd_pack_pairs = true

require dastest/testing_boost public
require daslib/flatten
require math

//! The width-2 hadd path, opt-in via `options _flatten_hadd_pack_pairs = true`: with the floor lowered
//! to 2, a bare pair packs into `hadd2` and plan_groups emits width-2 remainders (5 -> [3,2]). This is
//! +1 node on a node-graph backend but a win in cycles on a SIMD CPU backend — hence opt-in, default
//! width>=3 (see test_flatten_hadd.das). Value approx (the hadd reorders the adds); the structural
//! FIRED proof (`hadd(float2(` / `hadd(float3(` emission) lives in test_flatten_fold.das.

// ----- 2-way sum -> hadd(float2(...)) (the opt-in pays for the combine here) -----
[flatten]
def hp_two(a, b : float) : float {
    return a + b
}

// ----- 5 terms: plan_groups(5,2) = [3,2] -> hadd3 + hadd2 (the width-2 remainder emits) -----
[flatten]
def hp_five(a, b, c, d, e : float) : float {
    return a + b + c + d + e
}

var GRID : array<float>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, 0.0, 1.0, 2.5, 7.0, 100.0]
}

def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "hadd-pairs approx mismatch: {a} vs {b}")
}

[test]
def test_flatten_hadd_pairs(t : T?) {
    t |> run("width-2 pack + [3,2] split — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, hp_two_flat(v, 1.5), hp_two(v, 1.5))
            fapprox(t, hp_five_flat(v, 1.5, -v, 2.5, 0.125), hp_five(v, 1.5, -v, 2.5, 0.125))
        }
    }
}
