options gen2
require dastest/testing_boost
require daslib/math_boost

let inf = 1.0 / 0.0
let neg_inf = -1.0 / 0.0
let neg_nan = inf / inf
let nan = -(inf / inf)

[sideeffects]
def eq(a, b) {
    return a == b
}

[sideeffects]
def neq(a, b) {
    return a != b
}

[sideeffects]
def less(a, b) {
    return a < b
}

[sideeffects]
def greater(a, b) {
    return a > b
}

[sideeffects]
def leq(a, b) {
    return a <= b
}

[sideeffects]
def geq(a, b) {
    return a >= b
}

[test]
def test_inf_and_nan(t : T?) {
    t |> run("inf and neg inf") @@(t : T?) {
        t |> strictEqual(100.0 / 0.0, inf)
        t |> strictEqual(-100.0 / 0.0, neg_inf)
        t |> strictEqual(inf / inf, neg_nan)
        t |> strictEqual(-(inf / inf), nan)
    }

    t |> run("is_finite and is_nan") @@(t : T?) {
        t |> success(!is_finite(inf))
        t |> success(!is_finite(neg_inf))
        t |> success(!is_finite(nan))
        t |> success(!is_finite(neg_nan))
        t |> success(is_finite(1.))
        t |> success(is_finite(0.))
        t |> success(is_nan(nan))
        t |> success(is_nan(neg_nan))
        t |> success(!is_nan(inf))
        t |> success(!is_nan(neg_inf))
        t |> success(!is_nan(1.))
        t |> success(!is_nan(0.))
    }

    t |> run("inf and nan comparisons") @@(t : T?) {
        t |> success(!eq(nan, nan))
        t |> success(!eq(1., nan))
        t |> success(!eq(nan, 1.))
        t |> success(!eq(inf, nan))
        t |> success(!eq(nan, inf))

        t |> success(neq(nan, nan))
        t |> success(neq(1., nan))
        t |> success(neq(nan, 1.))
        t |> success(neq(inf, nan))
        t |> success(neq(nan, inf))

        t |> success(!less(nan, nan))
        t |> success(!less(1., nan))
        t |> success(!less(nan, 1.))
        t |> success(!less(inf, nan))
        t |> success(!less(nan, inf))

        t |> success(!leq(nan, nan))
        t |> success(!leq(1., nan))
        t |> success(!leq(nan, 1.))
        t |> success(!leq(inf, nan))
        t |> success(!leq(nan, inf))

        t |> success(!greater(nan, nan))
        t |> success(!greater(1., nan))
        t |> success(!greater(nan, 1.))
        t |> success(!greater(inf, nan))
        t |> success(!greater(nan, inf))

        t |> success(!geq(nan, nan))
        t |> success(!geq(1., nan))
        t |> success(!geq(nan, 1.))
        t |> success(!geq(inf, nan))
        t |> success(!geq(nan, inf))
    }
}


