options gen2
require math
require dastest/testing_boost public

options optimize = false

// Stage-2 math-shader-breadth: hyperbolic trig family (sinh/cosh/tanh + inverses) gets dasSpirv +
// llvm_jit dispatch coverage. C++ bindings already exist for float scalar + float2/3/4; this test
// pins the float-side correctness (identity-at-zero + small-x round-trips) across the three tiers
// under dastest --isolated-mode. Doubles already covered by tests/math/math_misc.das.

[test]
def test_hyperbolic_identity_at_zero(t : T?) {
    t |> success(abs(sinh(0.0f)) < 1.0e-6f, "sinh(0)=0")
    t |> success(abs(cosh(0.0f) - 1.0f) < 1.0e-6f, "cosh(0)=1")
    t |> success(abs(tanh(0.0f)) < 1.0e-6f, "tanh(0)=0")
    t |> success(abs(asinh(0.0f)) < 1.0e-6f, "asinh(0)=0")
    t |> success(abs(acosh(1.0f)) < 1.0e-6f, "acosh(1)=0")
    t |> success(abs(atanh(0.0f)) < 1.0e-6f, "atanh(0)=0")
}

[test]
def test_inverse_roundtrip(t : T?) {
    // asinh(sinh(x)) = x; tanh<->atanh and cosh<->acosh require |x|<1 / x>=1 domains.
    let x = 0.5f
    t |> success(abs(asinh(sinh(x)) - x) < 1.0e-5f, "asinh(sinh(0.5))=0.5")
    t |> success(abs(atanh(tanh(x)) - x) < 1.0e-5f, "atanh(tanh(0.5))=0.5")
    t |> success(abs(acosh(cosh(x)) - x) < 1.0e-5f, "acosh(cosh(0.5))=0.5")
}

[test]
def test_hyperbolic_vec(t : T?) {
    let s3 = sinh(float3(0.0, 0.0, 0.0))
    t |> success(abs(s3.x) < 1.0e-6f && abs(s3.y) < 1.0e-6f && abs(s3.z) < 1.0e-6f, "sinh(float3(0)) lanes")
    let c4 = cosh(float4(0.0, 0.0, 0.0, 0.0))
    t |> success(abs(c4.x - 1.0f) < 1.0e-6f && abs(c4.y - 1.0f) < 1.0e-6f && abs(c4.z - 1.0f) < 1.0e-6f && abs(c4.w - 1.0f) < 1.0e-6f, "cosh(float4(0)) lanes")
    let th2 = tanh(float2(0.0, 0.0))
    t |> success(abs(th2.x) < 1.0e-6f && abs(th2.y) < 1.0e-6f, "tanh(float2(0)) lanes")
}

[test]
def test_inverse_hyperbolic_vec(t : T?) {
    let as3 = asinh(float3(0.0, 0.0, 0.0))
    t |> success(abs(as3.x) < 1.0e-6f && abs(as3.y) < 1.0e-6f && abs(as3.z) < 1.0e-6f, "asinh(float3(0)) lanes")
    let ac4 = acosh(float4(1.0, 1.0, 1.0, 1.0))
    t |> success(abs(ac4.x) < 1.0e-5f && abs(ac4.y) < 1.0e-5f && abs(ac4.z) < 1.0e-5f && abs(ac4.w) < 1.0e-5f, "acosh(float4(1)) lanes")
    let at2 = atanh(float2(0.0, 0.0))
    t |> success(abs(at2.x) < 1.0e-6f && abs(at2.y) < 1.0e-6f, "atanh(float2(0)) lanes")
}
