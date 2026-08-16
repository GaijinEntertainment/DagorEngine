options gen2
require math
require dastest/testing_boost public

options optimize = false

// Math correctness for step + smoothstep: scalar GLSL semantics plus float2/3/4 per-lane sanity.
// dastest --isolated-mode exercises interpreter / JIT / AOT; one test file covers all three.

[test]
def test_step_scalar(t : T?) {
    // GLSL: step(edge, x) = x < edge ? 0 : 1. At x == edge the result is 1.
    t |> equal(step(0.5f, 0.3f), 0.0f)
    t |> equal(step(0.5f, 0.7f), 1.0f)
    t |> equal(step(0.5f, 0.5f), 1.0f)
    t |> equal(step(0.0f, -0.1f), 0.0f)
    t |> equal(step(0.0f, 0.0f), 1.0f)
}

[test]
def test_step_vec(t : T?) {
    t |> equal(step(float2(0.5, 0.5), float2(0.3, 0.7)), float2(0.0, 1.0))
    t |> equal(step(float3(0.0, 1.0, -1.0), float3(0.5, 1.0, -2.0)), float3(1.0, 1.0, 0.0))
    t |> equal(step(float4(0.0, 0.0, 0.0, 0.0), float4(-1.0, 0.0, 1.0, 2.0)), float4(0.0, 1.0, 1.0, 1.0))
}

[test]
def test_smoothstep_scalar(t : T?) {
    // t = clamp((x-e0)/(e1-e0), 0, 1); return t*t*(3-2t).
    // smoothstep(0, 1, 0.5) -> t=0.5 -> 0.25 * (3-1) = 0.5
    t |> equal(smoothstep(0.0f, 1.0f, 0.5f), 0.5f)
    // Outside the band clamps: below e0 -> 0, above e1 -> 1.
    t |> equal(smoothstep(0.0f, 1.0f, -1.0f), 0.0f)
    t |> equal(smoothstep(0.0f, 1.0f, 2.0f), 1.0f)
    // Endpoints: at e0 t=0 -> 0; at e1 t=1 -> 1*1*(3-2) = 1.
    t |> equal(smoothstep(0.0f, 1.0f, 0.0f), 0.0f)
    t |> equal(smoothstep(0.0f, 1.0f, 1.0f), 1.0f)
    // Off-zero edge band: smoothstep(2,4,3) -> t=0.5 -> 0.5
    t |> equal(smoothstep(2.0f, 4.0f, 3.0f), 0.5f)
}

[test]
def test_smoothstep_vec(t : T?) {
    let s2 = smoothstep(float2(0.0, 0.0), float2(1.0, 1.0), float2(0.5, -1.0))
    t |> equal(s2, float2(0.5, 0.0))
    let s3 = smoothstep(float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0), float3(0.5, 2.0, 1.0))
    t |> equal(s3, float3(0.5, 1.0, 1.0))
    let s4 = smoothstep(float4(0.0, 0.0, 0.0, 0.0), float4(1.0, 1.0, 1.0, 1.0), float4(0.5, 0.0, 1.0, -1.0))
    t |> equal(s4, float4(0.5, 0.0, 1.0, 0.0))
}
