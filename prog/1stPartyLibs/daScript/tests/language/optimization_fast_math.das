options gen2
options fast_math = true

require dastest/testing_boost public
require math

// behavioral coverage for the fast_math-gated folds: x*0 / x-x for FP, x/C → x*(1/C),
// NaN-compare flips, reassociation. values here are chosen so the transforms stay exact
// (or are compared with a tolerance where the reciprocal legitimately rounds).

var g_f = 3.5
var g_d = 2.25lf
var g_v = float3(1.0, 2.0, 4.0)
var g_calls = 0

def side_f : float {
    g_calls++
    return g_f
}

[test]
def test_fp_zero_folds(t : T?) {
    t |> equal(g_f * 0.0, 0.0)
    t |> equal(0.0 * g_f, 0.0)
    t |> equal(g_d * 0.0lf, 0.0lf)
    t |> equal(g_f - g_f, 0.0) // nolint:LINT007 -- deliberate: exercises the fast_math x-x fold
    t |> equal(g_d - g_d, 0.0lf) // nolint:LINT007
    g_calls = 0
    t |> equal(side_f() * 0.0, 0.0)
    t |> equal(g_calls, 1)    // impure operand must still run
}

[test]
def test_double_zero_sign_folds(t : T?) {
    // double x+0 / 0-x folds are fast_math-gated; values are identical for normal inputs
    t |> equal(g_d + 0.0lf, 2.25lf)
    t |> equal(g_d - 0.0lf, 2.25lf)
    t |> equal(0.0lf - g_d, -2.25lf)
}

[test]
def test_reciprocal_division(t : T?) {
    // powers of two: the reciprocal is exact
    t |> equal(g_f / 2.0, 1.75)
    t |> equal(g_f / 4.0, 0.875)
    t |> equal(g_d / 2.0lf, 1.125lf)
    t |> equal(g_v / 2.0, float3(0.5, 1.0, 2.0))
    // non-power-of-two: the reciprocal rounds — compare with tolerance
    t |> success(abs(g_f / 3.0 - 1.1666666) < 1e-5, "x/3 via reciprocal stays close")
}

[test]
def test_fp_compare_flip(t : T?) {
    t |> equal(!(g_f < 4.0), g_f >= 4.0)
    t |> equal(!(g_f == 3.5), false)
}

[test]
def test_reassociation(t : T?) {
    // (x ∘ C1) ∘ C2 → x ∘ K; constants chosen exactly representable
    t |> equal((g_f + 1.0) + 2.0, 6.5)
    t |> equal((g_f * 2.0) * 4.0, 28.0)
    t |> equal(2.0 * (g_f * 4.0), 28.0)
    t |> equal((2.0 + g_f) + 1.0, 6.5)
    t |> equal((g_d + 1.0lf) + 2.0lf, 5.25lf)
}
