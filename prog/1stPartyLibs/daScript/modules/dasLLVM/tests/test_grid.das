options gen2

require dastest/testing_boost public
require dasllama/dasllama_tune
require math

// [dasllama_grid] emits, from a single `def template`, one clone per grid permutation plus a
// <name>_variants() : array<tuple<string; <sig>>> fn-ptr registry — the boilerplate the
// profiler harness used to hand-write (per-perm [tuned] stubs + a variants() array).
def template grid_dot_template(a, b : array<float>) : float {
    var s = 0f
    for [tune = 1] (i in range(length(a))) {
        s += a[i] * b[i]
    }
    return s
}

[dasllama_grid(src = "grid_dot_template")]
struct GridDot {}

def private reference(a, b : array<float>) : float {
    var s = 0f
    for (i in range(length(a))) {
        s += a[i] * b[i]
    }
    return s
}

[test]
def test_grid_registry(t : T?) {
    let a = [for (i in range(64)); float(i)]
    let b = [for (_i in range(64)); 1.0f]
    let want = reference(a, b)

    let vs <- grid_dot_template_variants()
    t |> equal(length(vs), 20, "grid emits all 20 permutations")

    // Every emitted variant is callable and numerically correct (vectorize reassociates FP,
    // so compare within tolerance rather than bit-exact).
    var seen_default = false
    for (v in vs) {
        let f = v._1
        t |> success(abs(f(a, b) - want) < 1.0e-2f, "variant {v._0} == reference")
        if (v._0 == "vec8_u2") { seen_default = true }
    }
    t |> success(seen_default, "grid includes the default vec8_u2 permutation")
}

// A VOID-returning template must also produce a working registry: function_to_type carries the
// declared `: void` through, so the emitted variants() tuple type is a concrete function<...:void>.
// Regression — an undeclared/auto return produced a non-concrete function<...:auto> registry type
// that failed to compile (the in-place kernels axpy/add/mul/scale/copy hit exactly this).
def template grid_scale_template(var d : float ?; k : float; n : int64) : void {
    for [tune = 1] (i in range64(n)) {
        unsafe {
            d[i] *= k
        }
    }
}

[dasllama_grid(src = "grid_scale_template")]
struct GridScale {}

[test]
def test_grid_void_registry(t : T?) {
    let vs <- grid_scale_template_variants()
    t |> equal(length(vs), 20, "void-template grid emits all 20 permutations")

    var d : array<float>
    d |> resize(8)
    for (i in range(8)) {
        d[i] = float(i)
    }
    var dp = unsafe(addr(d[0]))
    let f = vs[0]._1               // "plain" perm — scales d in place by k
    invoke(f, dp, 2.0f, 8l)
    t |> equal(d[3], 6.0f, "void variant {vs[0]._0} scaled d[3] (3*2) in place")
}
