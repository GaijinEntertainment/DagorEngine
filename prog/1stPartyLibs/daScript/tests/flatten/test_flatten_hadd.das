options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten
require math

//! Differential net for the PHASE_FUSED horizontal-reduce re-pack (DEFAULT width>=3 policy): a sum of
//! independent scalars (no shared vector base, so the dot fold above can't reach them) collapses into
//! `hadd(floatN(...))` groups of width 3..4 via plan_groups — or `dot(floatN(s), floatN(weights))` when
//! the lanes carry const weights (`a*2 + b*3 + …`). A width-2 tail stays scalar (a bare pair is +1 node
//! on a node-graph backend); the width-2 path is exercised under the option in
//! no_aot/test_flatten_hadd_pairs.das. The reduce reorders the adds, so floats compare approx (same
//! contract as the dot/mad corpus). The structural FIRED half lives in no_aot/test_flatten_fold.das.

// ----- 2-way sum: below the default width-3 floor -> stays a scalar add -----
[flatten]
def h_two(a, b : float) : float {
    return a + b
}

// ----- 3-way sum -> hadd(float3(...)) -----
[flatten]
def h_three(a, b, c : float) : float {
    return a + b + c
}

// ----- pure 4-way sum -> hadd(float4(...)) -----
[flatten]
def h_four(a, b, c, d : float) : float {
    return a + b + c + d
}

// ----- 5 terms: plan_groups(5,3) = [4] -> hadd4, the 5th lane stays a scalar add (sub-width-3 tail) -----
[flatten]
def h_five(a, b, c, d, e : float) : float {
    return a + b + c + d + e
}

// ----- 7 terms: plan_groups splits [4,3] -> hadd4 + hadd3 -----
[flatten]
def h_seven(a, b, c, d, e, f, g : float) : float {
    return a + b + c + d + e + f + g
}

// ----- 8 terms: two groups of four -> two PARALLEL hadd4 (never a nested hadd) -----
[flatten]
def h_eight(a, b, c, d, e, f, g, h : float) : float {
    return a + b + c + d + e + f + g + h
}

// ----- one lane only: `a` is the sole reduce lane, `b*c` is a runtime*runtime product (mad, not a
// weighted lane) -> nothing to pack -----
[flatten]
def h_one_lane(a, b, c : float) : float {
    return a + b * c
}

// ----- negated tail: four positives pack, the `- e` stays -----
[flatten]
def h_neg(a, b, c, d, e : float) : float {
    return a + b + c + d - e
}

// ----- const term stays out of the ctor (it folds), four vars pack -----
[flatten]
def h_const(a, b, c, d : float) : float {
    return a + b + c + d + 0.5
}

// ----- horizontal sum buried inside a call argument -----
[flatten]
def h_buried(a, b, c, d : float) : float {
    return sin(a + b + c + d)
}

// ----- the plasma shape: `(sum) * s + k` -> mad(hadd(...), s, k) -----
[flatten]
def h_mad(a, b, c, d : float) : float {
    return (a + b + c + d) * 2.0 + 1.0
}

// ----- const-weighted lanes pack into a DOT, not hadd: `n*w` summed -> dot(floatN(n), floatN(w)) -----
// (the weighted reduction beats the mad-chain at width>=3: combine + dot vs N mads — the mandelbrot fBm)
[flatten]
def h_weighted(a, b, c, d : float) : float {
    return a * 2.0 + b * 3.0 + c * 4.0 + d * 5.0
}

// ----- mixed plain + weighted lanes share one dot with a unit weight: `a + b*2 + c*3` -> dot(.,(1,2,3)) -----
[flatten]
def h_wmixed(a, b, c : float) : float {
    return a + b * 2.0 + c * 3.0
}

// ----- producer-vectorization: same elementwise-unary lanes collapse the producer -> hadd(sin(float3(...))) -----
[flatten]
def h_trig(a, b, c : float) : float {
    return sin(a) + sin(b) + sin(c)
}

// ----- producer-vectorization at width 4 -> hadd(cos(float4(...))) -----
[flatten]
def h_trig4(a, b, c, d : float) : float {
    return cos(a) + cos(b) + cos(c) + cos(d)
}

// ----- mixed producers (sin / cos / sin) do NOT vectorize: leaf-pack hadd(float3(sin,cos,sin)) -----
[flatten]
def h_mixed(a, b, c : float) : float {
    return sin(a) + cos(b) + sin(c)
}

var GRID : array<float>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, 0.0, 1.0, 2.5, 7.0, 100.0]
}

def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "hadd-fused approx mismatch: {a} vs {b}")
}

[test]
def test_flatten_hadd(t : T?) {
    t |> run("two / three / four scalar sums — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, h_two_flat(v, 1.5), h_two(v, 1.5))
            fapprox(t, h_three_flat(v, 1.5, -v), h_three(v, 1.5, -v))
            fapprox(t, h_four_flat(v, 1.5, -v, 2.5), h_four(v, 1.5, -v, 2.5))
        }
    }
    t |> run("five / seven / eight scalar sums — multi-group + scalar tail, value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, h_five_flat(v, 1.5, -v, 2.5, 0.125), h_five(v, 1.5, -v, 2.5, 0.125))
            fapprox(t, h_seven_flat(v, 1.5, -v, 2.5, 0.125, -0.5, v * 0.25),
                       h_seven(v, 1.5, -v, 2.5, 0.125, -0.5, v * 0.25))
            fapprox(t, h_eight_flat(v, 1.5, -v, 2.5, 0.125, -0.5, v * 0.25, 3.0),
                       h_eight(v, 1.5, -v, 2.5, 0.125, -0.5, v * 0.25, 3.0))
        }
    }
    t |> run("one-lane / negated / const tail — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, h_one_lane_flat(v, 1.5, -v), h_one_lane(v, 1.5, -v))
            fapprox(t, h_neg_flat(v, 1.5, -v, 2.5, 0.75), h_neg(v, 1.5, -v, 2.5, 0.75))
            fapprox(t, h_const_flat(v, 1.5, -v, 2.5), h_const(v, 1.5, -v, 2.5))
        }
    }
    t |> run("buried sum / mad shape / weighted dot — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, h_buried_flat(v, 1.5, -v, 2.5), h_buried(v, 1.5, -v, 2.5))
            fapprox(t, h_mad_flat(v, 1.5, -v, 2.5), h_mad(v, 1.5, -v, 2.5))
            fapprox(t, h_weighted_flat(v, 1.5, -v, 2.5), h_weighted(v, 1.5, -v, 2.5))
            fapprox(t, h_wmixed_flat(v, 1.5, -v), h_wmixed(v, 1.5, -v))
        }
    }
    t |> run("producer-vectorization: shared / mixed elementwise unary — value approx") @(t : T?) {
        for (v in GRID) {
            fapprox(t, h_trig_flat(v, 1.5, -v), h_trig(v, 1.5, -v))
            fapprox(t, h_trig4_flat(v, 1.5, -v, 2.5), h_trig4(v, 1.5, -v, 2.5))
            fapprox(t, h_mixed_flat(v, 1.5, -v), h_mixed(v, 1.5, -v))
        }
    }
}
