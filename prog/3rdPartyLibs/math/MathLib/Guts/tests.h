// © 2021 NVIDIA Corporation

#define Test_Eps         0.00001
#define Test_ConstantEps 0.001 // hack for "fmod"

#define TestEqual_x2(C, T) ML_Assert(all(C((T)x1, (T)y1) == C((T)x1, (T)y1)))
#define TestEqual_x3(C, T) ML_Assert(all(C((T)x1, (T)y1, (T)z1) == C((T)x1, (T)y1, (T)z1)))
#define TestEqual_x4(C, T) ML_Assert(all(C((T)x1, (T)y1, (T)z1, (T)w1) == C((T)x1, (T)y1, (T)z1, (T)w1)))

#define TestNotEqual_x2(C, T) ML_Assert(any(C((T)x1, (T)y1) != C((T)y1, (T)x1)))
#define TestNotEqual_x3(C, T) ML_Assert(any(C((T)x1, (T)y1, (T)z1) != C((T)z1, (T)x1, (T)y1)))
#define TestNotEqual_x4(C, T) ML_Assert(any(C((T)x1, (T)y1, (T)z1, (T)w1) != C((T)w1, (T)z1, (T)y1, (T)x1)))

#define TestOp_x2(C, T, op) ML_Assert(all((C((T)x1, (T)y1) op C((T)x2, (T)y2)) == C((T)x1 op(T) x2, (T)y1 op(T) y2)))
#define TestOp_x3(C, T, op) ML_Assert(all((C((T)x1, (T)y1, (T)z1) op C((T)x2, (T)y2, (T)z2)) == C((T)x1 op(T) x2, (T)y1 op(T) y2, (T)z1 op(T) z2)))
#define TestOp_x4(C, T, op) ML_Assert(all((C((T)x1, (T)y1, (T)z1, (T)w1) op C((T)x2, (T)y2, (T)z2, (T)w2)) == C((T)x1 op(T) x2, (T)y1 op(T) y2, (T)z1 op(T) z2, (T)w1 op(T) w2)))

#define Test1_x2(C, T, func) ML_Assert(all(func(C((T)x1, (T)y1)) == C(func((T)x1), func((T)y1))))
#define Test1_x3(C, T, func) ML_Assert(all(func(C((T)x1, (T)y1, (T)z1)) == C(func((T)x1), func((T)y1), func((T)z1))))
#define Test1_x4(C, T, func) ML_Assert(all(func(C((T)x1, (T)y1, (T)z1, (T)w1)) == C(func((T)x1), func((T)y1), func((T)z1), func((T)w1))))

#define Test2_x2(C, T, func) ML_Assert(all(func(C((T)x1, (T)y1), C((T)x2, (T)y2)) == C(func((T)x1, (T)x2), func((T)y1, (T)y2))))
#define Test2_x3(C, T, func) ML_Assert(all(func(C((T)x1, (T)y1, (T)z1), C((T)x2, (T)y2, (T)z2)) == C(func((T)x1, (T)x2), func((T)y1, (T)y2), func((T)z1, (T)z2))))

#define Test2_x4(C, T, func) \
    ML_Assert(all(func(C((T)x1, (T)y1, (T)z1, (T)w1), C((T)x2, (T)y2, (T)z2, (T)w2)) == C(func((T)x1, (T)x2), func((T)y1, (T)y2), func((T)z1, (T)z2), func((T)w1, (T)w2))))

#define Test1_x3_eps(C, T, func) \
    ML_Assert(all(abs(func(C((T)x1, (T)y1, (T)z1)) - C(func((T)x1), func((T)y1), func((T)z1))) <= abs(C(func((T)x1), func((T)y1), func((T)z1))) * (T)Test_Eps))

#define Test1_x4_eps(C, T, func) \
    ML_Assert(all(abs(func(C((T)x1, (T)y1, (T)z1, (T)w1)) - C(func((T)x1), func((T)y1), func((T)z1), func((T)w1))) <= abs(C(func((T)x1), func((T)y1), func((T)z1), func((T)w1))) * (T)Test_Eps))

#define Test2_x3_eps(C, T, func) \
    ML_Assert(all(abs(func(C((T)x1, (T)y1, (T)z1), C((T)x2, (T)y2, (T)z2)) - C(func((T)x1, (T)x2), func((T)y1, (T)y2), func((T)z1, (T)z2))) <= abs(C(func((T)x1, (T)x2), func((T)y1, (T)y2), func((T)z1, (T)z2))) * (T)Test_Eps))
#define Test2_x4_eps(C, T, func) \
    ML_Assert(all(abs(func(C((T)x1, (T)y1, (T)z1, (T)w1), C((T)x2, (T)y2, (T)z2, (T)w2)) - C(func((T)x1, (T)x2), func((T)y1, (T)y2), func((T)z1, (T)z2), func((T)w1, (T)w2))) <= abs(C(func((T)x1, (T)x2), func((T)y1, (T)y2), func((T)z1, (T)z2), func((T)w1, (T)w2))) * (T)Test_Eps))

#define Test1_x3_ceps(C, T, func) \
    ML_Assert(all(abs(func(C((T)x1, (T)y1, (T)z1)) - C(func((T)x1), func((T)y1), func((T)z1))) <= abs(C(func((T)x1), func((T)y1), func((T)z1))) * (T)Test_Eps))

#define Test1_x4_ceps(C, T, func) \
    ML_Assert(all(abs(func(C((T)x1, (T)y1, (T)z1, (T)w1)) - C(func((T)x1), func((T)y1), func((T)z1), func((T)w1))) <= abs(C(func((T)x1), func((T)y1), func((T)z1), func((T)w1))) * (T)Test_Eps))

#define Test2_x3_ceps(C, T, func) \
    ML_Assert(all(abs(func(C((T)x1, (T)y1, (T)z1), C((T)x2, (T)y2, (T)z2)) - C(func((T)x1, (T)x2), func((T)y1, (T)y2), func((T)z1, (T)z2))) <= (T)Test_ConstantEps))
#define Test2_x4_ceps(C, T, func) \
    ML_Assert(all(abs(func(C((T)x1, (T)y1, (T)z1, (T)w1), C((T)x2, (T)y2, (T)z2, (T)w2)) - C(func((T)x1, (T)x2), func((T)y1, (T)y2), func((T)z1, (T)z2), func((T)w1, (T)w2))) <= (T)Test_ConstantEps))

#include "../ml.hlsli"

#ifdef ML_NAMESPACE
namespace ml {
#endif

void ML_Tests() {
    const uint32_t N = 10000;
    const float R = 10000.0f;

    uint32_t rngState = 1983;
    Rng::Hash::Initialize(rngState, 0, 0);

    for (uint32_t i = 0; i < N; i++) {
        { // Ops
            float x1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float y1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float z1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float w1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;

            float x2 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float y2 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float z2 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float w2 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;

            TestOp_x2(float2, float, -);
            TestOp_x2(float2, float, +);
            TestOp_x2(float2, float, *);
            TestOp_x2(float2, float, /);

            TestOp_x3(float3, float, -);
            TestOp_x3(float3, float, +);
            TestOp_x3(float3, float, *);
            TestOp_x3(float3, float, /);

            TestOp_x4(float4, float, -);
            TestOp_x4(float4, float, +);
            TestOp_x4(float4, float, *);
            TestOp_x4(float4, float, /);

            TestOp_x2(double2, double, -);
            TestOp_x2(double2, double, +);
            TestOp_x2(double2, double, *);
            TestOp_x2(double2, double, /);

            TestOp_x3(double3, double, -);
            TestOp_x3(double3, double, +);
            TestOp_x3(double3, double, *);
            TestOp_x3(double3, double, /);

            TestOp_x4(double4, double, +);
            TestOp_x4(double4, double, -);
            TestOp_x4(double4, double, *);
            TestOp_x4(double4, double, /);

            // Avoid division by "0" for integers
            if (x2 > -1 && x2 < 1)
                x2 = 1;

            if (y2 > -1 && y2 < 1)
                y2 = 1;

            if (z2 > -1 && z2 < 1)
                z2 = 1;

            if (w2 > -1 && w2 < 1)
                w2 = 1;

            TestOp_x2(int2, int32_t, -);
            TestOp_x2(int2, int32_t, +);
            TestOp_x2(int2, int32_t, *);
            TestOp_x2(int2, int32_t, /);

            TestOp_x3(int3, int32_t, -);
            TestOp_x3(int3, int32_t, +);
            TestOp_x3(int3, int32_t, *);
            TestOp_x3(int3, int32_t, /);

            TestOp_x4(int4, int32_t, -);
            TestOp_x4(int4, int32_t, +);
            TestOp_x4(int4, int32_t, *);
            TestOp_x4(int4, int32_t, /);

            TestOp_x2(uint2, uint32_t, -);
            TestOp_x2(uint2, uint32_t, +);
            TestOp_x2(uint2, uint32_t, *);
            TestOp_x2(uint2, uint32_t, /);

            TestOp_x3(uint3, uint32_t, -);
            TestOp_x3(uint3, uint32_t, +);
            TestOp_x3(uint3, uint32_t, *);
            TestOp_x3(uint3, uint32_t, /);

            TestOp_x4(uint4, uint32_t, -);
            TestOp_x4(uint4, uint32_t, +);
            TestOp_x4(uint4, uint32_t, *);
            TestOp_x4(uint4, uint32_t, /);
        }

        { // Integer ops
            uint32_t x1 = Rng::Hash::GetUint(rngState);
            uint32_t y1 = Rng::Hash::GetUint(rngState);
            uint32_t z1 = Rng::Hash::GetUint(rngState);
            uint32_t w1 = Rng::Hash::GetUint(rngState);

            uint32_t x2 = Rng::Hash::GetUint(rngState);
            uint32_t y2 = Rng::Hash::GetUint(rngState);
            uint32_t z2 = Rng::Hash::GetUint(rngState);
            uint32_t w2 = Rng::Hash::GetUint(rngState);

            TestOp_x2(int2, int32_t, &);
            TestOp_x2(int2, int32_t, |);
            TestOp_x2(int2, int32_t, ^);

            TestOp_x3(int3, int32_t, &);
            TestOp_x3(int3, int32_t, |);
            TestOp_x3(int3, int32_t, ^);

            TestOp_x4(int4, int32_t, &);
            TestOp_x4(int4, int32_t, |);
            TestOp_x4(int4, int32_t, ^);

            TestOp_x2(uint2, uint32_t, &);
            TestOp_x2(uint2, uint32_t, |);
            TestOp_x2(uint2, uint32_t, ^);

            TestOp_x3(uint3, uint32_t, &);
            TestOp_x3(uint3, uint32_t, |);
            TestOp_x3(uint3, uint32_t, ^);

            TestOp_x4(uint4, uint32_t, &);
            TestOp_x4(uint4, uint32_t, |);
            TestOp_x4(uint4, uint32_t, ^);

            // Shifts and mod: use sane 2nd arg
            x1 &= 0x7FFFFFFF;
            y1 &= 0x7FFFFFFF;
            z1 &= 0x7FFFFFFF;
            w1 &= 0x7FFFFFFF;

            x2 &= 31;
            y2 &= 31;
            z2 &= 31;
            w2 &= 31;

            TestOp_x2(int2, int32_t, <<);
            TestOp_x2(int2, int32_t, >>);

            TestOp_x3(int3, int32_t, <<);
            TestOp_x3(int3, int32_t, >>);

            TestOp_x4(int4, int32_t, <<);
            TestOp_x4(int4, int32_t, >>);

            TestOp_x2(uint2, uint32_t, <<);
            TestOp_x2(uint2, uint32_t, >>);

            TestOp_x3(uint3, uint32_t, <<);
            TestOp_x3(uint3, uint32_t, >>);

            TestOp_x4(uint4, uint32_t, <<);
            TestOp_x4(uint4, uint32_t, >>);

            // Avoid division by "0"
            if (!x2)
                x2 = 1;

            if (!y2)
                y2 = 1;

            if (!z2)
                z2 = 1;

            if (!w2)
                w2 = 1;

            TestOp_x2(int2, int32_t, %);
            TestOp_x3(int3, int32_t, %);
            TestOp_x4(int4, int32_t, %);
            TestOp_x2(uint2, uint32_t, %);
            TestOp_x3(uint3, uint32_t, %);
            TestOp_x4(uint4, uint32_t, %);
        }

        { // Math [-INF, INF]
            float x1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float y1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float z1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float w1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;

            float x2 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float y2 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float z2 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
            float w2 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;

            Test1_x2(float2, float, degrees);
            Test1_x2(float2, float, radians);
            Test1_x2(float2, float, sign);
            Test1_x2(float2, float, abs);
            Test1_x2(float2, float, floor);
            Test1_x2(float2, float, ceil);
            Test1_x2(float2, float, frac);
            Test1_x2(float2, float, saturate);
            Test2_x2(float2, float, min);
            Test2_x2(float2, float, max);
            Test2_x2(float2, float, step);

            Test1_x2(float2, float, rcp);
            Test1_x2(float2, float, sin);
            Test1_x2(float2, float, cos);
            Test1_x2(float2, float, tan);
            Test1_x2(float2, float, atan);
            Test2_x2(float2, float, fmod);
            Test2_x2(float2, float, atan2);

            Test1_x2(double2, double, degrees);
            Test1_x2(double2, double, radians);
            Test1_x2(double2, double, sign);
            Test1_x2(double2, double, abs);
            Test1_x2(double2, double, floor);
            Test1_x2(double2, double, ceil);
            Test1_x2(double2, double, frac);
            Test1_x2(double2, double, saturate);
            Test2_x2(double2, double, min);
            Test2_x2(double2, double, max);
            Test2_x2(double2, double, step);

            Test1_x2(double2, double, rcp);
            Test1_x2(double2, double, sin);
            Test1_x2(double2, double, cos);
            Test1_x2(double2, double, tan);
            Test1_x2(double2, double, atan);
            Test2_x2(double2, double, fmod);
            Test2_x2(double2, double, atan2);

            Test1_x3(float3, float, degrees);
            Test1_x3(float3, float, radians);
            Test1_x3(float3, float, sign);
            Test1_x3(float3, float, abs);
            Test1_x3(float3, float, floor);
            Test1_x3(float3, float, ceil);
            Test1_x3(float3, float, frac);
            Test1_x3(float3, float, saturate);
            Test2_x3(float3, float, min);
            Test2_x3(float3, float, max);
            Test2_x3(float3, float, step);

            Test1_x3_eps(float3, float, rcp);
            Test1_x3_eps(float3, float, sin);
            Test1_x3_eps(float3, float, cos);
            Test1_x3_eps(float3, float, tan);
            Test1_x3_eps(float3, float, atan);
            Test2_x3_ceps(float3, float, fmod);
            Test2_x3_eps(float3, float, atan2);

            Test1_x3(double3, double, degrees);
            Test1_x3(double3, double, radians);
            Test1_x3(double3, double, sign);
            Test1_x3(double3, double, abs);
            Test1_x3(double3, double, floor);
            Test1_x3(double3, double, ceil);
            Test1_x3(double3, double, frac);
            Test1_x3(double3, double, saturate);
            Test2_x3(double3, double, min);
            Test2_x3(double3, double, max);
            Test2_x3(double3, double, step);

            Test1_x3_eps(double3, double, rcp);
            Test1_x3_eps(double3, double, sin);
            Test1_x3_eps(double3, double, cos);
            Test1_x3_eps(double3, double, tan);
            Test1_x3_eps(double3, double, atan);
            Test2_x3_ceps(double3, double, fmod);
            Test2_x3_eps(double3, double, atan2);

            Test1_x4(float4, float, degrees);
            Test1_x4(float4, float, radians);
            Test1_x4(float4, float, sign);
            Test1_x4(float4, float, abs);
            Test1_x4(float4, float, floor);
            Test1_x4(float4, float, ceil);
            Test1_x4(float4, float, frac);
            Test1_x4(float4, float, saturate);
            Test2_x4(float4, float, min);
            Test2_x4(float4, float, max);
            Test2_x4(float4, float, step);

            Test1_x4_eps(float4, float, rcp);
            Test1_x4_eps(float4, float, sin);
            Test1_x4_eps(float4, float, cos);
            Test1_x4_eps(float4, float, tan);
            Test1_x4_eps(float4, float, atan);
            Test2_x4_ceps(float4, float, fmod);
            Test2_x4_eps(float4, float, atan2);

            Test1_x4(double4, double, degrees);
            Test1_x4(double4, double, radians);
            Test1_x4(double4, double, sign);
            Test1_x4(double4, double, abs);
            Test1_x4(double4, double, floor);
            Test1_x4(double4, double, ceil);
            Test1_x4(double4, double, frac);
            Test1_x4(double4, double, saturate);
            Test2_x4(double4, double, min);
            Test2_x4(double4, double, max);
            Test2_x4(double4, double, step);

            Test1_x4_eps(double4, double, rcp);
            Test1_x4_eps(double4, double, sin);
            Test1_x4_eps(double4, double, cos);
            Test1_x4_eps(double4, double, tan);
            Test1_x4_eps(double4, double, atan);
            Test2_x4_ceps(double4, double, fmod);
            Test2_x4_eps(double4, double, atan2);

            // round: avoid fractional part = 0.5
            if (frac(x1) == 0.5f)
                x1 = uFloat(uFloat(x1).i + 1).f;

            if (frac(y1) == 0.5f)
                y1 = uFloat(uFloat(y1).i + 1).f;

            if (frac(z1) == 0.5f)
                z1 = uFloat(uFloat(z1).i + 1).f;

            if (frac(w1) == 0.5f)
                w1 = uFloat(uFloat(w1).i + 1).f;

            Test1_x2(float2, float, round);
            Test1_x3(float3, float, round);
            Test1_x4(float4, float, round);

            Test1_x2(double2, double, round);
            Test1_x3(double3, double, round);
            Test1_x4(double4, double, round);

            // pow/exp/exp2: do not use to large "x" and "y" to avoid "INF"
            x1 *= 32.0f / R;
            y1 *= 32.0f / R;
            z1 *= 32.0f / R;
            w1 *= 32.0f / R;

            x2 *= 32.0f / R;
            y2 *= 32.0f / R;
            z2 *= 32.0f / R;
            w2 *= 32.0f / R;

            Test1_x2(float2, float, exp);
            Test1_x3_eps(float3, float, exp);
            Test1_x4_eps(float4, float, exp);

            Test1_x2(double2, double, exp);
            Test1_x3_eps(double3, double, exp);
            Test1_x4_eps(double4, double, exp);

            Test1_x2(float2, float, exp2);
            Test1_x3_eps(float3, float, exp2);
            Test1_x4_eps(float4, float, exp2);

            Test1_x2(double2, double, exp2);
            Test1_x3_eps(double3, double, exp2);
            Test1_x4_eps(double4, double, exp2);

            // pow: "x" must be positive
            x1 = abs(x1);
            y1 = abs(y1);
            z1 = abs(z1);
            w1 = abs(w1);

            Test2_x2(float2, float, pow);
            Test2_x3_eps(float3, float, pow);
            Test2_x4_eps(float4, float, pow);

            Test2_x2(double2, double, pow);
            Test2_x3_eps(double3, double, pow);
            Test2_x4_eps(double4, double, pow);
        }

        { // Math (> 0)
            float x1 = Rng::Hash::GetFloat(rngState) * R;
            float y1 = Rng::Hash::GetFloat(rngState) * R;
            float z1 = Rng::Hash::GetFloat(rngState) * R;
            float w1 = Rng::Hash::GetFloat(rngState) * R;

            Test1_x2(float2, float, rsqrt);
            Test1_x2(float2, float, sqrt);
            Test1_x2(float2, float, log);
            Test1_x2(float2, float, log2);

            Test1_x2(double2, double, rsqrt);
            Test1_x2(double2, double, sqrt);
            Test1_x2(double2, double, log);
            Test1_x2(double2, double, log2);

            Test1_x3_eps(float3, float, rsqrt);
            Test1_x3_eps(float3, float, sqrt);
            Test1_x3_eps(float3, float, log);
            Test1_x3_eps(float3, float, log2);

            Test1_x3_eps(double3, double, rsqrt);
            Test1_x3_eps(double3, double, sqrt);
            Test1_x3_eps(double3, double, log);
            Test1_x3_eps(double3, double, log2);

            Test1_x4_eps(float4, float, rsqrt);
            Test1_x4_eps(float4, float, sqrt);
            Test1_x4_eps(float4, float, log);
            Test1_x4_eps(float4, float, log2);

            Test1_x4_eps(double4, double, rsqrt);
            Test1_x4_eps(double4, double, sqrt);
            Test1_x4_eps(double4, double, log);
            Test1_x4_eps(double4, double, log2);
        }

        { // Math [-1; 1]
            float x1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * 2.0f;
            float y1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * 2.0f;
            float z1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * 2.0f;
            float w1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * 2.0f;

            Test1_x2(float2, float, asin);
            Test1_x2(float2, float, acos);

            Test1_x2(double2, double, asin);
            Test1_x2(double2, double, acos);

            Test1_x3_eps(float3, float, asin);
            Test1_x3_eps(float3, float, acos);

            Test1_x3_eps(double3, double, asin);
            Test1_x3_eps(double3, double, acos);

            Test1_x4_eps(float4, float, asin);
            Test1_x4_eps(float4, float, acos);

            Test1_x4_eps(double4, double, asin);
            Test1_x4_eps(double4, double, acos);
        }
    }

    { // == and !=
        float x1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
        float y1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
        float z1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;
        float w1 = (Rng::Hash::GetFloat(rngState) - 0.5f) * R;

        TestEqual_x2(int2, int32_t);
        TestEqual_x3(int3, int32_t);
        TestEqual_x4(int4, int32_t);

        TestEqual_x2(uint2, uint32_t);
        TestEqual_x3(uint3, uint32_t);
        TestEqual_x4(uint4, uint32_t);

        TestEqual_x2(float2, float);
        TestEqual_x3(float3, float);
        TestEqual_x4(float4, float);

        TestEqual_x2(double2, double);
        TestEqual_x3(double3, double);
        TestEqual_x4(double4, double);

        TestNotEqual_x2(int2, int32_t);
        TestNotEqual_x3(int3, int32_t);
        TestNotEqual_x4(int4, int32_t);

        TestNotEqual_x2(uint2, uint32_t);
        TestNotEqual_x3(uint3, uint32_t);
        TestNotEqual_x4(uint4, uint32_t);

        TestNotEqual_x2(float2, float);
        TestNotEqual_x3(float3, float);
        TestNotEqual_x4(float4, float);

        TestNotEqual_x2(double2, double);
        TestNotEqual_x3(double3, double);
        TestNotEqual_x4(double4, double);
    }

    { // +0 == -0
        ML_Assert(all(+0 == -0));
        ML_Assert(all(int2(+0) == int2(-0)));
        ML_Assert(all(int3(+0) == int3(-0)));
        ML_Assert(all(int4(+0) == int4(-0)));

        ML_Assert(all(+0.0f == -0.0f));
        ML_Assert(all(float2(+0.0f) == float2(-0.0f)));
        ML_Assert(all(float3(+0.0f) == float3(-0.0f)));
        ML_Assert(all(float4(+0.0f) == float4(-0.0f)));

        ML_Assert(all(+0.0 == -0.0));
        ML_Assert(all(float2(+0.0) == float2(-0.0)));
        ML_Assert(all(float3(+0.0) == float3(-0.0)));
        ML_Assert(all(float4(+0.0) == float4(-0.0)));
    }

    { // NAN != NAN
        float myNanf = log(0.0f) * 0.0f;
        ML_Assert(myNanf != myNanf);
        ML_Assert(any(float2(myNanf) != float2(myNanf)));
        ML_Assert(any(float3(myNanf) != float3(myNanf)));
        ML_Assert(any(float4(myNanf) != float4(myNanf)));

        double myNan = log(0.0) * 0.0;
        ML_Assert(myNan != myNan);
        ML_Assert(any(double2(myNan) != double2(myNan)));
        ML_Assert(any(double3(myNan) != double3(myNan)));
        ML_Assert(any(double4(myNan) != double4(myNan)));
    }
}

#ifdef ML_NAMESPACE
}
#endif
