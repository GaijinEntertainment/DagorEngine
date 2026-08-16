options gen2

require dastest/testing_boost public
require dasllama/dasllama_tune

// Source-of-truth kernel. `def template` keeps it out of codegen — its [tune=1]
// marker would otherwise trip the JIT (unknown loop hint). The emitted variants
// have the marker stripped and the real hints stamped in its place.
def template sample_dot(a, b : array<float>) : float {
    var s = 0f
    for [tune = 1] (i in range(length(a))) {
        s += a[i] * b[i]
    }
    return s
}

// [dasllama_tune] scans this module for [tune]-marked templates and emits
// sample_dot__plain / __u2 / __vec8 / __vec8_u2 (one per grid permutation).
[dasllama_tune]
struct private Tuner {}

def private reference(a, b : array<float>) : float {
    var s = 0f
    for (i in range(length(a))) {
        s += a[i] * b[i]
    }
    return s
}

// b is all-ones so the dot is sum(a) — exactly representable and
// order-independent, so vectorize/unroll reassociation stays bit-exact.
[test]
def test_variants_match_reference(t : T?) {
    let a = [for (i in range(64)); float(i)]
    let b = [for (_i in range(64)); 1.0f]
    let want = reference(a, b)
    t |> equal(want, 2016.0f, "sum(0..63)")
    t |> equal(sample_dot__plain(a, b), want, "plain")
    t |> equal(sample_dot__u2(a, b), want, "u2")
    t |> equal(sample_dot__vec8(a, b), want, "vec8")
    t |> equal(sample_dot__vec8_u2(a, b), want, "vec8_u2")
}
