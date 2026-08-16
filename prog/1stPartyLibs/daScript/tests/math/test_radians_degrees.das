options gen2
require math
require dastest/testing_boost public

options optimize = false

// Stage-2 math-shader-breadth: radians + degrees promoted from daslib/math_boost to the C++ math
// module so dasSpirv can lower them in shaders. Scalar + float2/3/4 correctness across the three
// execution tiers (interpreter / JIT / AOT) under dastest --isolated-mode.

let PI_F = 3.14159265358979323846f

[test]
def test_radians_scalar(t : T?) {
    t |> equal(radians(0.0f), 0.0f)
    t |> success(abs(radians(180.0f) - PI_F) < 1.0e-5f, "radians(180) ~= pi")
    t |> success(abs(radians(90.0f) - PI_F * 0.5f) < 1.0e-5f, "radians(90) ~= pi/2")
    t |> success(abs(radians(-180.0f) + PI_F) < 1.0e-5f, "radians(-180) ~= -pi")
}

[test]
def test_degrees_scalar(t : T?) {
    t |> equal(degrees(0.0f), 0.0f)
    t |> success(abs(degrees(PI_F) - 180.0f) < 1.0e-3f, "degrees(pi) ~= 180")
    t |> success(abs(degrees(PI_F * 0.5f) - 90.0f) < 1.0e-3f, "degrees(pi/2) ~= 90")
}

[test]
def test_radians_vec(t : T?) {
    let r2 = radians(float2(0.0, 180.0))
    t |> success(abs(r2.x) < 1.0e-5f && abs(r2.y - PI_F) < 1.0e-5f, "radians(float2) per-lane")
    let r3 = radians(float3(0.0, 90.0, 180.0))
    t |> success(abs(r3.x) < 1.0e-5f && abs(r3.y - PI_F * 0.5f) < 1.0e-5f && abs(r3.z - PI_F) < 1.0e-5f, "radians(float3) per-lane")
    let r4 = radians(float4(0.0, 45.0, 90.0, 180.0))
    t |> success(abs(r4.x) < 1.0e-5f && abs(r4.y - PI_F * 0.25f) < 1.0e-5f && abs(r4.z - PI_F * 0.5f) < 1.0e-5f && abs(r4.w - PI_F) < 1.0e-5f, "radians(float4) per-lane")
}

[test]
def test_degrees_vec(t : T?) {
    let d2 = degrees(float2(0.0, PI_F))
    t |> success(abs(d2.x) < 1.0e-3f && abs(d2.y - 180.0f) < 1.0e-3f, "degrees(float2) per-lane")
    let d3 = degrees(float3(0.0, PI_F * 0.5f, PI_F))
    t |> success(abs(d3.x) < 1.0e-3f && abs(d3.y - 90.0f) < 1.0e-3f && abs(d3.z - 180.0f) < 1.0e-3f, "degrees(float3) per-lane")
    let d4 = degrees(float4(0.0, PI_F * 0.25f, PI_F * 0.5f, PI_F))
    t |> success(abs(d4.x) < 1.0e-3f && abs(d4.y - 45.0f) < 1.0e-3f && abs(d4.z - 90.0f) < 1.0e-3f && abs(d4.w - 180.0f) < 1.0e-3f, "degrees(float4) per-lane")
}
