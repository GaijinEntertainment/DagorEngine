options gen2
require math
require dastest/testing_boost public

options optimize = false

// Stage-2 math-shader-breadth: fmod + mad get dasSpirv + llvm_jit dispatch coverage. fmod maps to
// SPIR-V core OpFRem (GLSL.std.450 has no FMod sub-opcode; OpFRem matches C fmod sign-of-dividend
// semantics) / LLVM frem; mad maps to GLSLstd450.Fma (= a*b + c per sim_policy). C++ bindings
// already exist for float scalar + float2/3/4; this test pins per-tier float correctness.

[test]
def test_fmod_scalar(t : T?) {
    t |> equal(fmod(5.5f, 2.0f), 1.5f)
    t |> equal(fmod(-5.5f, 2.0f), -1.5f)
    t |> equal(fmod(5.5f, -2.0f), 1.5f)
    t |> equal(fmod(0.0f, 1.0f), 0.0f)
}

[test]
def test_fmod_vec(t : T?) {
    let r2 = fmod(float2(5.5, -5.5), float2(2.0, 2.0))
    t |> success(abs(r2.x - 1.5f) < 1.0e-5f && abs(r2.y + 1.5f) < 1.0e-5f, "fmod(float2) per-lane")
    let r3 = fmod(float3(5.5, 0.0, 7.25), float3(2.0, 1.0, 2.5))
    t |> success(abs(r3.x - 1.5f) < 1.0e-5f && abs(r3.y) < 1.0e-5f && abs(r3.z - 2.25f) < 1.0e-5f, "fmod(float3) per-lane")
    let r4 = fmod(float4(5.5, 5.5, -5.5, 10.0), float4(2.0, -2.0, -2.0, 3.0))
    t |> success(abs(r4.x - 1.5f) < 1.0e-5f && abs(r4.y - 1.5f) < 1.0e-5f && abs(r4.z + 1.5f) < 1.0e-5f && abs(r4.w - 1.0f) < 1.0e-5f, "fmod(float4) per-lane")
}

[test]
def test_mad_scalar(t : T?) {
    // mad(a, b, c) = a*b + c
    t |> equal(mad(2.0f, 3.0f, 4.0f), 10.0f)
    t |> equal(mad(0.0f, 0.0f, 5.0f), 5.0f)
    t |> equal(mad(-1.0f, 2.0f, 3.0f), 1.0f)
}

[test]
def test_mad_vec(t : T?) {
    // float2 vec form: mad(a : float2, b : float, c : float2) = a*b + c
    let r2 = mad(float2(1.0, 2.0), 3.0f, float2(4.0, 5.0))
    t |> success(abs(r2.x - 7.0f) < 1.0e-5f && abs(r2.y - 11.0f) < 1.0e-5f, "mad(float2) lanes")
    let r3 = mad(float3(1.0, 2.0, 3.0), 2.0f, float3(0.0, 1.0, -1.0))
    t |> success(abs(r3.x - 2.0f) < 1.0e-5f && abs(r3.y - 5.0f) < 1.0e-5f && abs(r3.z - 5.0f) < 1.0e-5f, "mad(float3) lanes")
    let r4 = mad(float4(1.0, 2.0, 3.0, 4.0), 1.5f, float4(0.5, 0.5, 0.5, 0.5))
    t |> success(abs(r4.x - 2.0f) < 1.0e-5f && abs(r4.y - 3.5f) < 1.0e-5f && abs(r4.z - 5.0f) < 1.0e-5f && abs(r4.w - 6.5f) < 1.0e-5f, "mad(float4) lanes")
}
