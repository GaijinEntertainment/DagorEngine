options gen2
options indenting = 4

require math
require dastest/testing_boost public

// A module-qualified builtin call answers exactly what the unqualified spelling answers, in
// every tier - interpreter, JIT and AOT. This file pins that, one call per assert, qualified
// against unqualified, over every builtin family a JIT intrinsic emitter keys by name, and
// pins the arm identity of the calls whose vector form rides an inline polynomial rail, where
// agreeing spellings are not enough.

struct Vec2 {
    x : float
    y : float
}

//! the file's own sqrt overload - it is what makes the math:: qualifier necessary on the builtin calls
def sqrt(v : Vec2) : Vec2 {
    return Vec2(x = math::sqrt(v.x), y = math::sqrt(v.y))
}

// the operands stay mutable module globals - a literal or a `let` const-folds the call
// away before any backend sees it
var g_nine = 9.0f
var g_frac = 2.5f
var g_small = 0.5f
var g_lo = 3.0f
var g_hi = 7.0f
var g_two = 2.0f
var g_v4 = float4(4.0f, 1.0f, 9.0f, 2.0f)
var g_zero4 = float4(0.0f, 0.0f, 0.0f, 0.0f)
var g_one4 = float4(1.0f, 1.0f, 1.0f, 1.0f)
var g_zero = 0.0f
var g_bits = 0x7fffffffu

[test]
def test_qualified_unary_float(t : T?) {
    t |> equal(math::sqrt(g_nine), sqrt(g_nine), "math::sqrt")
    t |> equal(math::floor(g_frac), floor(g_frac), "math::floor")
    t |> equal(math::ceil(g_frac), ceil(g_frac), "math::ceil")
    t |> equal(math::round(g_frac), round(g_frac), "math::round")
    t |> equal(math::sin(g_small), sin(g_small), "math::sin")
    t |> equal(math::cos(g_small), cos(g_small), "math::cos")
    t |> equal(math::log(g_nine), log(g_nine), "math::log")
    t |> equal(math::exp2(g_small), exp2(g_small), "math::exp2")
    t |> equal(math::log2(g_nine), log2(g_nine), "math::log2")
    t |> equal(math::sinh(g_small), sinh(g_small), "math::sinh")
    t |> equal(math::cosh(g_small), cosh(g_small), "math::cosh")
    t |> equal(math::tanh(g_small), tanh(g_small), "math::tanh")
}

[test]
def test_qualified_float_to_int(t : T?) {
    t |> equal(math::floori(g_frac), floori(g_frac), "math::floori")
    t |> equal(math::ceili(g_frac), ceili(g_frac), "math::ceili")
}

[test]
def test_qualified_two_operand(t : T?) {
    t |> equal(math::pow(g_lo, g_two), pow(g_lo, g_two), "math::pow")
    t |> equal(math::fmod(g_hi, g_lo), fmod(g_hi, g_lo), "math::fmod")
}

// clz/ctz/popcnt live in the builtin module, whose qualified spelling is `_::`
[test]
def test_qualified_builtin_bits(t : T?) {
    t |> equal(_::popcnt(g_bits), popcnt(g_bits), "_::popcnt")
    t |> equal(_::clz(g_bits), clz(g_bits), "_::clz")
    t |> equal(_::ctz(g_bits), ctz(g_bits), "_::ctz")
}

// the half8 lane accessors are keyed by name the same way, and the qualified spelling used to
// answer the hi lanes for a lo call
[test]
def test_qualified_half8_lanes(t : T?) {
    let p = half8(g_v4, g_v4 + g_one4)
    t |> equal(_::half8_lo(p), half8_lo(p), "_::half8_lo")
    t |> equal(_::half8_hi(p), half8_hi(p), "_::half8_hi")
    t |> equal(_::half8_lo(p), g_v4, "_::half8_lo answers the lo lanes")
}

[test]
def test_qualified_minmax(t : T?) {
    t |> equal(math::min(g_lo, g_hi), min(g_lo, g_hi), "math::min")
    t |> equal(math::max(g_lo, g_hi), max(g_lo, g_hi), "math::max")
    t |> equal(math::hmin(g_v4), hmin(g_v4), "math::hmin")
    t |> equal(math::hmax(g_v4), hmax(g_v4), "math::hmax")
}

[test]
def test_qualified_on_vectors(t : T?) {
    t |> equal(math::sqrt(g_v4), sqrt(g_v4), "math::sqrt(float4)")
    t |> equal(math::sin(g_v4), sin(g_v4), "math::sin(float4)")
    t |> equal(math::cos(g_v4), cos(g_v4), "math::cos(float4)")
    t |> equal(math::tanh(g_v4), tanh(g_v4), "math::tanh(float4)")
}

[test]
def test_qualified_call_inside_own_overload(t : T?) {
    let s = sqrt(Vec2(x = g_nine, y = g_frac * g_frac))
    t |> equal(s.x, math::sqrt(g_nine), "own sqrt(Vec2).x")
    t |> equal(s.y, g_frac, "own sqrt(Vec2).y")
}

[test]
def test_qualified_vector_arm_identity(t : T?) {
    // cos/cosh/tanh reach an inline polynomial rail whose arm is picked by the call's name,
    // and that rail only runs on aarch64. Comparing spellings there proves nothing if both
    // pick the same wrong arm, so these compare against the value the named function has:
    // the neighbouring arm answers a different number at zero.
    t |> equal(math::cos(g_zero4), g_one4, "math::cos(0) is 1, not sin(0)")
    t |> equal(math::sin(g_zero4), g_zero4, "math::sin(0) is 0")
    t |> equal(math::cosh(g_zero4), g_one4, "math::cosh(0) is 1, not sinh(0)")
    t |> equal(math::sinh(g_zero4), g_zero4, "math::sinh(0) is 0")
    t |> equal(math::tanh(g_zero4), g_zero4, "math::tanh(0) is 0")
    let th = math::tanh(g_v4)
    t |> success(abs(th.x) <= 1.0f && abs(th.y) <= 1.0f && abs(th.z) <= 1.0f && abs(th.w) <= 1.0f,
        "math::tanh is bounded by 1 - sinh is not")
    t |> equal(th, tanh(g_v4), "math::tanh matches the unqualified spelling")
}

[test]
def test_qualified_nan_lanes(t : T?) {
    // both spellings answer NaN in the same lanes; is_nan reads IEEE on a host that folds
    // nan compares, so this cell holds wherever it runs
    let nan = g_zero / g_zero   // nolint:LINT007 - 0/0 is how this cell makes its nan
    let nan4 = float4(nan, g_zero, nan, g_zero)
    let q = math::cos(nan4)
    let u = cos(nan4)
    t |> equal(is_nan(q.x), is_nan(u.x), "lane 0")
    t |> equal(is_nan(q.y), is_nan(u.y), "lane 1")
    t |> equal(is_nan(q.z), is_nan(u.z), "lane 2")
    t |> equal(is_nan(q.w), is_nan(u.w), "lane 3")
}
