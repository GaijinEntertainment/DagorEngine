options gen2

require dastest/testing_boost public
require dasllama/dasllama_tune

// Part 2 (final assembly): one source of truth, reconstituted into the real symbol.
def template sample_dot_template(a, b : array<float>) : float {
    var s = 0f
    for [tune = 1] (i in range(length(a))) {
        s += a[i] * b[i]
    }
    return s
}

// [tuned] clones sample_dot_template's body into sample_dot and stamps the
// "vec8_u2" permutation's hints. Callers just call sample_dot — untouched.
[tuned(perm = "vec8_u2")]
def sample_dot(a, b : array<float>) : float {}

// `fallback=` supplies a kernel's own hand hints when they aren't the global vec8_u2; the box
// profile still overrides it (precedence: perm > profile > fallback > vec8_u2). Validation proof:
// an unknown fallback name fails the compile with "tuned: unknown perm" (negative-probed), so this
// compiling proves the fallback names a valid grid perm — the one consulted whenever no profile
// entry / perm pin applies (and no profile carries this test-local kernel's key).
[tuned(fallback = "u4", src = "sample_dot_template")]
def sample_dot_u4(a, b : array<float>) : float {}

def private reference(a, b : array<float>) : float {
    var s = 0f
    for (i in range(length(a))) {
        s += a[i] * b[i]
    }
    return s
}

[test]
def test_tuned_reconstitutes(t : T?) {
    let a = [for (i in range(64)); float(i)]
    let b = [for (_i in range(64)); 1.0f]
    t |> equal(sample_dot(a, b), reference(a, b), "tuned sample_dot == reference")
    t |> equal(sample_dot_u4(a, b), reference(a, b), "tuned default=u4 sample_dot_u4 == reference")
}
