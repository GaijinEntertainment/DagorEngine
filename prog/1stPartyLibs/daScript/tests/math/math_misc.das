options gen2
require math
require dastest/testing_boost public

options optimize = false

[sideeffects]
def rightPolicyCeili(a : float) : int {
    return ceili(a)
}

[test]
def test_math_misc(t : T?) {
    t |> success(uint32_hash(1U) != 1U)
    var seed : uint = 1U
    let position : int = 1
    t |> equal(min(float2(0.5, -1.), float2(-1., 0.5)), float2(-1., -1.))
    t |> equal(min(float4(0.5, -1., 0.5, -1.), float4(-1., 0.5, -1., 0.5)), float4(-1, -1, -1, -1))
    t |> equal(reflect(float3(1, 1, 0), float3(-1, 0, 0)), float3(-1, 1, 0))
    t |> equal(reflect(float2(1, 1), float2(-1, 0)), float2(-1, 1))
    t |> equal(lerp(100., 0., 0.5), 50.)
    t |> equal(lerp(float2(100, 50), float2(), 0.5), float2(50, 25))
    t |> equal(lerp(float3(100, 50, 200), float3(), 0.5), float3(50, 25, 100))
    t |> equal(lerp(float4(100, 50, 200, 400), float4(), 0.5), float4(50, 25, 100, 200))
    t |> equal(lerp(float2(100, 50), float2(), float2(0.5)), float2(50, 25))
    t |> equal(lerp(float3(100, 50, 200), float3(), float3(0.5)), float3(50, 25, 100))
    t |> equal(lerp(float4(100, 50, 200, 400), float4(), float4(0.5)), float4(50, 25, 100, 200))
    //assert(clamp(acos(2.), 0., 0.) == 0.)  // removed due to exception on Win32
    t |> equal(clamp(100., 0., 200.), 100.)
    t |> equal(clamp(-100., 100., 200.), 100.)
    t |> equal(clamp(1000., 100., 200.), 200.)
    t |> success(uint_noise_1D(position, seed) != seed)
    t |> success(uint_noise_2D(int2(100, 100), seed) != seed)
    t |> success(uint_noise_3D(int3(100, 100, 100), seed) != seed)
    t |> equal(dot(float2(0, 1), float2(1, 0)), 0.)
    t |> equal(dot(float3(0, 1, 0), float3(1, 0, 1)), 0.)
    t |> equal(dot(float4(0, 1, 0, 0), float4(1, 0, 1, 1)), 0.)
    //
    t |> equal(length(float2(0, 0)), 0.)
    t |> equal(length(float3(0, 0, 0)), 0.)
    t |> equal(length(float4(0, 0, 0, 0)), 0.)
    t |> equal(length_sq(float2(0, 0)), 0.)
    t |> equal(length_sq(float3(0, 0, 0)), 0.)
    t |> equal(length_sq(float4(0, 0, 0, 0)), 0.)
    t |> equal(length(float2(0, 1)), 1.)
    t |> equal(length(float3(0, 1, 0)), 1.)
    t |> equal(length(float4(0, 0, 0, 1)), 1.)
    t |> equal(length_sq(float2(0, 1)), 1.)
    t |> equal(length_sq(float3(1, 0, 0)), 1.)
    t |> equal(length_sq(float4(0, 1, 0, 0)), 1.)
    t |> equal(cross(float3(0, 1, 0), float3(0, 0, 1)), float3(1, 0, 0))
    //
    t |> equal(sin(0.), 0.)
    t |> equal(cos(0.), 1.)
    t |> equal(tan(0.), 0.)
    t |> equal(asin(0.), 0.)
    t |> equal(acos(1.), 0.)
    t |> success(abs(asin(sin(0.5)) - 0.5) < 0.0001)
    t |> success(abs(acos(cos(0.5)) - 0.5) < 0.0001)
    t |> equal(sin(0.1) * sin(0.1) + cos(0.1) * cos(0.1), 1.) //cos^2 + sin^2 == 1
    //
    t |> equal(asin(1.0), safe_asin(200.0))
    t |> equal(asin(-1.0), safe_asin(-200.0))
    t |> equal(acos(1.0d), safe_acos(200.0d))
    t |> equal(acos(-1.0d), safe_acos(-200.0d))
    t |> equal(asin(float2(1.0)), safe_asin(float2(200.0)))
    t |> equal(asin(float3(1.0)), safe_asin(float3(200.0)))
    t |> equal(asin(float4(1.0)), safe_asin(float4(200.0)))
    t |> equal(asin(float2(-1.0)), safe_asin(float2(-200.0)))
    t |> equal(asin(float3(-1.0)), safe_asin(float3(-200.0)))
    t |> equal(asin(float4(-1.0)), safe_asin(float4(-200.0)))
    t |> equal(acos(float2(1.0)), safe_acos(float2(200.0)))
    t |> equal(acos(float3(1.0)), safe_acos(float3(200.0)))
    t |> equal(acos(float4(1.0)), safe_acos(float4(200.0)))
    t |> equal(acos(float2(-1.0)), safe_acos(float2(-200.0)))
    t |> equal(acos(float3(-1.0)), safe_acos(float3(-200.0)))
    t |> equal(acos(float4(-1.0)), safe_acos(float4(-200.0)))
    t |> equal(asin(0.75), safe_asin(0.75))
    t |> equal(asin(0.75d), safe_asin(0.75d))
    t |> equal(asin(-0.75), safe_asin(-0.75))
    t |> equal(asin(-0.75d), safe_asin(-0.75d))
    t |> equal(asin(float4(0.75, 0.76, 0.77, 0.78)), safe_asin(float4(0.75, 0.76, 0.77, 0.78)))
    t |> equal(asin(-float4(0.75, 0.76, 0.77, 0.78)), safe_asin(-float4(0.75, 0.76, 0.77, 0.78)))
    t |> equal(acos(0.75), safe_acos(0.75))
    t |> equal(acos(0.75d), safe_acos(0.75d))
    t |> equal(acos(-0.75), safe_acos(-0.75))
    t |> equal(acos(-0.75d), safe_acos(-0.75d))
    t |> equal(acos(float4(0.75, 0.76, 0.77, 0.78)), safe_acos(float4(0.75, 0.76, 0.77, 0.78)))
    t |> equal(acos(-float4(0.75, 0.76, 0.77, 0.78)), safe_acos(-float4(0.75, 0.76, 0.77, 0.78)))
    //
    t |> equal(atan2(0., 0.), 0.)
    t |> equal(sqrt(4.), 2.)
    t |> equal(sqrt(0.), 0.)
    t |> equal(pow(2., 2.), 4.)
    t |> equal(ceil(1.1), 2.)
    t |> equal(floor(1.1), 1.)
    t |> equal(ceil(-1.1), -1.)
    t |> equal(floor(-1.1), -2.)
    t |> equal(exp(0.), 1.)
    t |> success(abs(log(exp(1.)) - 1.) < 1e-6)
    t |> equal(pow(0., 0.), 1.)
    t |> equal(pow(2., 2.), 4.)
    t |> equal(exp2(2.), 4.)
    t |> equal(log2(4.), 2.)
    t |> success(abs(exp(1.) - 2.718281828459045f) < 1e-6)
    t |> success(abs(log(2.718281828459045f) - 1.) < 1e-6)
    t |> equal(ceili(float2(0.5, -0.1)), int2(1, 0))
    t |> equal(floori(float2(0.5, -0.1)), int2(0, -1))
    t |> equal(roundi(float2(0.501, -0.1)), int2(1, 0))
    t |> equal(trunci(float2(0.501, -0.1)), int2(0, 0))
    // and doubles
    t |> equal(ceili(0.5lf), 1)
    t |> equal(floori(0.5lf), 0)
    t |> equal(roundi(0.501lf), 1)
    t |> equal(trunci(0.501lf), 0)
    // policy AOT
    t |> equal(rightPolicyCeili(1.2), 2)
    // constants
    let mme = "{FLT_MIN} {FLT_MAX} {FLT_EPSILON}"
    t |> equal(mme, "1.1754944e-38 3.4028235e+38 1.1920929e-07")
}
