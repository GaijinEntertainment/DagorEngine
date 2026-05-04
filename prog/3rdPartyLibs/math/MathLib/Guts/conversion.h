// © 2021 NVIDIA Corporation

#pragma once

// asfloat
ML_INLINE float asfloat(uint32_t x) {
    return *(float*)&x;
}

ML_INLINE float2 asfloat(const uint2& x) {
    return float2(asfloat(x.x), asfloat(x.y));
}

ML_INLINE float4 asfloat(const uint4& x) {
    return _mm_castsi128_ps(x.xmm);
}

// asuint
ML_INLINE uint32_t asuint(float x) {
    return *(uint32_t*)&x;
}

ML_INLINE uint2 asuint(const float2& x) {
    return uint2(asuint(x.x), asuint(x.y));
}

ML_INLINE uint4 asuint(const float4& x) {
    return _mm_castps_si128(x.xmm);
}

// bool2
ML_INLINE bool2::operator int2() const {
    return int2((mask & 0x1) ? 1 : 0, (mask & 0x2) ? 1 : 0);
}

ML_INLINE bool2::operator uint2() const {
    return uint2((mask & 0x1) ? 1 : 0, (mask & 0x2) ? 1 : 0);
}

ML_INLINE bool2::operator float2() const {
    return float2((mask & 0x1) ? 1.0f : 0.0f, (mask & 0x2) ? 1.0f : 0.0f);
}

ML_INLINE bool2::operator double2() const {
    return double2((mask & 0x1) ? 1.0 : 0.0, (mask & 0x2) ? 1.0 : 0.0);
}

// bool3
ML_INLINE bool3::operator int3() const {
    return int3((mask & 0x1) ? 1 : 0, (mask & 0x2) ? 1 : 0, (mask & 0x4) ? 1 : 0);
}

ML_INLINE bool3::operator uint3() const {
    return uint3((mask & 0x1) ? 1 : 0, (mask & 0x2) ? 1 : 0, (mask & 0x4) ? 1 : 0);
}

ML_INLINE bool3::operator float3() const {
    return float3((mask & 0x1) ? 1.0f : 0.0f, (mask & 0x2) ? 1.0f : 0.0f, (mask & 0x4) ? 1.0f : 0.0f);
}

ML_INLINE bool3::operator double3() const {
    return double3((mask & 0x1) ? 1.0 : 0.0, (mask & 0x2) ? 1.0 : 0.0, (mask & 0x4) ? 1.0 : 0.0);
}

// bool4
ML_INLINE bool4::operator int4() const {
    return int4((mask & 0x1) ? 1 : 0, (mask & 0x2) ? 1 : 0, (mask & 0x4) ? 1 : 0, (mask & 0x8) ? 1 : 0);
}

ML_INLINE bool4::operator uint4() const {
    return uint4((mask & 0x1) ? 1 : 0, (mask & 0x2) ? 1 : 0, (mask & 0x4) ? 1 : 0, (mask & 0x8) ? 1 : 0);
}

ML_INLINE bool4::operator float4() const {
    return float4((mask & 0x1) ? 1.0f : 0.0f, (mask & 0x2) ? 1.0f : 0.0f, (mask & 0x4) ? 1.0f : 0.0f, (mask & 0x8) ? 1.0f : 0.0f);
}

ML_INLINE bool4::operator double4() const {
    return double4((mask & 0x1) ? 1.0 : 0.0, (mask & 0x2) ? 1.0 : 0.0, (mask & 0x4) ? 1.0 : 0.0, (mask & 0x8) ? 1.0 : 0.0);
}

// int2
ML_INLINE int2::operator uint2() const {
    return uint2((uint)x, (uint)y);
}

ML_INLINE int2::operator float2() const {
    return float2((float)x, (float)y);
}

ML_INLINE int2::operator double2() const {
    return double2((double)x, (double)y);
}

// uint2
ML_INLINE uint2::operator int2() const {
    return int2((int32_t)x, (int32_t)y);
}

ML_INLINE uint2::operator float2() const {
    return float2((float)x, (float)y);
}

ML_INLINE uint2::operator double2() const {
    return double2((double)x, (double)y);
}

// float2
ML_INLINE float2::operator int2() const {
    return int2((int32_t)x, (int32_t)y);
}

ML_INLINE float2::operator uint2() const {
    return uint2((uint)x, (uint)y);
}

ML_INLINE float2::operator double2() const {
    return double2((double)x, (double)y);
}

ML_INLINE float2::operator float16_t2() const {
    return float16_t2(x, y);
}

// double2
ML_INLINE double2::operator int2() const {
    return int2((int32_t)x, (int32_t)y);
}

ML_INLINE double2::operator uint2() const {
    return uint2((uint)x, (uint)y);
}

ML_INLINE double2::operator float2() const {
    return float2((float)x, (float)y);
}

// int3
ML_INLINE int3::operator uint3() const {
    return xmm;
}

ML_INLINE int3::operator float3() const {
    return _mm_cvtepi32_ps(xmm);
}

ML_INLINE int3::operator double3() const {
    return _mm256_cvtepi32_pd(xmm);
}

// uint3
ML_INLINE uint3::operator int3() const {
    return xmm;
}

ML_INLINE uint3::operator float3() const {
    return _mm_cvtepi32_ps(xmm);
}

ML_INLINE uint3::operator double3() const {
    return _mm256_cvtepi32_pd(xmm);
}

// float3
ML_INLINE float3::operator int3() const {
    return _mm_cvtps_epi32(xmm);
}

ML_INLINE float3::operator uint3() const {
    return _mm_cvtps_epi32(xmm);
}

ML_INLINE float3::operator double3() const {
    return _mm256_cvtps_pd(xmm);
}

// double3
ML_INLINE double3::operator int3() const {
    return _mm256_cvtpd_epi32(ymm);
}

ML_INLINE double3::operator uint3() const {
    return _mm256_cvtpd_epi32(ymm);
}

ML_INLINE double3::operator float3() const {
    return _mm256_cvtpd_ps(ymm);
}

// int4
ML_INLINE int4::operator uint4() const {
    return xmm;
}

ML_INLINE int4::operator float4() const {
    return _mm_cvtepi32_ps(xmm);
}

ML_INLINE int4::operator double4() const {
    return _mm256_cvtepi32_pd(xmm);
}

// uint4
ML_INLINE uint4::operator int4() const {
    return xmm;
}

ML_INLINE uint4::operator float4() const {
    return _mm_cvtepi32_ps(xmm);
}

ML_INLINE uint4::operator double4() const {
    return _mm256_cvtepi32_pd(xmm);
}

// float4
ML_INLINE float4::operator int4() const {
    return _mm_cvtps_epi32(xmm);
}

ML_INLINE float4::operator uint4() const {
    return _mm_cvtps_epi32(xmm);
}

ML_INLINE float4::operator double4() const {
    return _mm256_cvtps_pd(xmm);
}

ML_INLINE float4::operator float16_t4() const {
    float16_t4 r;

#if (ML_INTRINSIC_LEVEL >= ML_INTRINSIC_AVX1 && ML_ALLOW_HW_FP16)
    v4i v = _mm_cvtps_ph(xmm, _MM_FROUND_TO_NEAREST_INT);
    r.xyzw = (uint64_t)_mm_cvtsi128_si64(v);
#else
    v4i v = ToSmallFloat4<fp16>(xmm);
    r.xyzw = v4i_to_u64(v);
#endif

    return r;
}

ML_INLINE float4::operator float8_e4m3_t4() const {
    v4i v = ToSmallFloat4<fp8_e4m3>(xmm);

    float8_e4m3_t4 r;
    r.xyzw = v4i_to_u32(v);

    return r;
}

ML_INLINE float4::operator float8_e5m2_t4() const {
    v4i v = ToSmallFloat4<fp8_e5m2>(xmm);

    float8_e5m2_t4 r;
    r.xyzw = v4i_to_u32(v);

    return r;
}

// double4
ML_INLINE double4::operator int4() const {
    return _mm256_cvtpd_epi32(ymm);
}

ML_INLINE double4::operator uint4() const {
    return _mm256_cvtpd_epi32(ymm);
}

ML_INLINE double4::operator float4() const {
    return _mm256_cvtpd_ps(ymm);
}

// float4x4
ML_INLINE float4x4::operator double4x4() const {
    double4x4 r;
    r.ca[0] = _mm256_cvtps_pd(ca[0]);
    r.ca[1] = _mm256_cvtps_pd(ca[1]);
    r.ca[2] = _mm256_cvtps_pd(ca[2]);
    r.ca[3] = _mm256_cvtps_pd(ca[3]);

    return r;
}

// double4x4
ML_INLINE double4x4::operator float4x4() const {
    float4x4 r;
    r.ca[0] = _mm256_cvtpd_ps(ca[0]);
    r.ca[1] = _mm256_cvtpd_ps(ca[1]);
    r.ca[2] = _mm256_cvtpd_ps(ca[2]);
    r.ca[3] = _mm256_cvtpd_ps(ca[3]);

    return r;
}

// float16
ML_INLINE float16_t::float16_t(float v) {
    x = (uint16_t)f32tof16(v);
}

ML_INLINE float16_t::operator float() const {
    return f16tof32(x);
}

ML_INLINE float16_t2::operator float2() const {
    return float2(float(x), float(y));
}

ML_INLINE float16_t4::operator float4() const {
#if (ML_INTRINSIC_LEVEL >= ML_INTRINSIC_AVX1 && ML_ALLOW_HW_FP16)
    v4i v = _mm_cvtsi64_si128(xyzw);
    return _mm_cvtph_ps(v);
#else
    v4i v = u64_to_v4i(xyzw);
    return FromSmallFloat4<fp16>(v);
#endif
}

ML_INLINE float16_t8::float16_t8(const float4& a, const float4& b) {
#if (ML_INTRINSIC_LEVEL >= ML_INTRINSIC_AVX1 && ML_ALLOW_HW_FP16)
    v8f v = _mm256_set_m128(b, a);
    A_B = _mm256_cvtps_ph(v, _MM_FROUND_TO_NEAREST_INT);
#elif (ML_INTRINSIC_LEVEL >= ML_INTRINSIC_AVX2)
    v8f v = _mm256_set_m128(b, a);
    v8i r = ToSmallFloat8<fp16>(v);

    A_B = v8i_to_u64x2(r);
#else
    A = float16_t4(a).xyzw;
    B = float16_t4(b).xyzw;
#endif
}

// float8_e4m3
ML_INLINE float8_e4m3_t::float8_e4m3_t(float v) {
    x = (uint8_t)ToSmallFloat<fp8_e4m3>(v);
}

ML_INLINE float8_e4m3_t::operator float() const {
    return FromSmallFloat<fp8_e4m3>(x);
}

ML_INLINE float8_e4m3_t2::operator float2() const {
    return float2(float(x), float(y));
}

ML_INLINE float8_e4m3_t4::operator float4() const {
    v4i v = u32_to_v4i(xyzw);

    return FromSmallFloat4<fp8_e4m3>(v);
}

ML_INLINE float8_e4m3_t8::float8_e4m3_t8(const float4& a, const float4& b) {
#if (ML_INTRINSIC_LEVEL >= ML_INTRINSIC_AVX2)
    v8f v = _mm256_set_m128(b, a);
    v8i r = ToSmallFloat8<fp8_e4m3>(v);

    A_B = v8i_to_u64(r);
#else
    A = float8_e4m3_t4(a).xyzw;
    B = float8_e4m3_t4(b).xyzw;
#endif
}

// float8_e5m2
ML_INLINE float8_e5m2_t::float8_e5m2_t(float v) {
    x = (uint8_t)ToSmallFloat<fp8_e5m2>(v);
}

ML_INLINE float8_e5m2_t::operator float() const {
    return FromSmallFloat<fp8_e5m2>(x);
}

ML_INLINE float8_e5m2_t2::operator float2() const {
    return float2(float(x), float(y));
}

ML_INLINE float8_e5m2_t4::operator float4() const {
    v4i v = u32_to_v4i(xyzw);

    return FromSmallFloat4<fp8_e5m2>(v);
}

ML_INLINE float8_e5m2_t8::float8_e5m2_t8(const float4& a, const float4& b) {
#if (ML_INTRINSIC_LEVEL >= ML_INTRINSIC_AVX2)
    v8f v = _mm256_set_m128(b, a);
    v8i r = ToSmallFloat8<fp8_e5m2>(v);

    A_B = v8i_to_u64(r);
#else
    A = float8_e5m2_t4(a).xyzw;
    B = float8_e5m2_t4(b).xyzw;
#endif
}
