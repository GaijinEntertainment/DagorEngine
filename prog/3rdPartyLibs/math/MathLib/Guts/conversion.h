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

// From bool2
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

// From bool3
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

// From bool4
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

// From int2
ML_INLINE int2::operator uint2() const {
    return uint2((uint)x, (uint)y);
}

ML_INLINE int2::operator float2() const {
    return float2((float)x, (float)y);
}

ML_INLINE int2::operator double2() const {
    return double2((double)x, (double)y);
}

// From uint2
ML_INLINE uint2::operator int2() const {
    return int2((int32_t)x, (int32_t)y);
}

ML_INLINE uint2::operator float2() const {
    return float2((float)x, (float)y);
}

ML_INLINE uint2::operator double2() const {
    return double2((double)x, (double)y);
}

// From float2
ML_INLINE float2::operator int2() const {
    return int2((int32_t)x, (int32_t)y);
}

ML_INLINE float2::operator uint2() const {
    return uint2((uint)x, (uint)y);
}

ML_INLINE float2::operator double2() const {
    return double2((double)x, (double)y);
}

// From double2
ML_INLINE double2::operator int2() const {
    return int2((int32_t)x, (int32_t)y);
}

ML_INLINE double2::operator uint2() const {
    return uint2((uint)x, (uint)y);
}

ML_INLINE double2::operator float2() const {
    return float2((float)x, (float)y);
}

// From int3
ML_INLINE int3::operator uint3() const {
    return xmm;
}

ML_INLINE int3::operator float3() const {
    return _mm_cvtepi32_ps(xmm);
}

ML_INLINE int3::operator double3() const {
    return _mm256_cvtepi32_pd(xmm);
}

// From uint3
ML_INLINE uint3::operator int3() const {
    return xmm;
}

ML_INLINE uint3::operator float3() const {
    return _mm_cvtepi32_ps(xmm);
}

ML_INLINE uint3::operator double3() const {
    return _mm256_cvtepi32_pd(xmm);
}

// From float3
ML_INLINE float3::operator int3() const {
    return _mm_cvtps_epi32(xmm);
}

ML_INLINE float3::operator uint3() const {
    return _mm_cvtps_epi32(xmm);
}

ML_INLINE float3::operator double3() const {
    return _mm256_cvtps_pd(xmm);
}

// From double3
ML_INLINE double3::operator int3() const {
    return _mm256_cvtpd_epi32(ymm);
}

ML_INLINE double3::operator uint3() const {
    return _mm256_cvtpd_epi32(ymm);
}

ML_INLINE double3::operator float3() const {
    return _mm256_cvtpd_ps(ymm);
}

// From int4
ML_INLINE int4::operator uint4() const {
    return xmm;
}

ML_INLINE int4::operator float4() const {
    return _mm_cvtepi32_ps(xmm);
}

ML_INLINE int4::operator double4() const {
    return _mm256_cvtepi32_pd(xmm);
}

// From uint4
ML_INLINE uint4::operator int4() const {
    return xmm;
}

ML_INLINE uint4::operator float4() const {
    return _mm_cvtepi32_ps(xmm);
}

ML_INLINE uint4::operator double4() const {
    return _mm256_cvtepi32_pd(xmm);
}

// From float4
ML_INLINE float4::operator int4() const {
    return _mm_cvtps_epi32(xmm);
}

ML_INLINE float4::operator uint4() const {
    return _mm_cvtps_epi32(xmm);
}

ML_INLINE float4::operator double4() const {
    return _mm256_cvtps_pd(xmm);
}

// From double4
ML_INLINE double4::operator int4() const {
    return _mm256_cvtpd_epi32(ymm);
}

ML_INLINE double4::operator uint4() const {
    return _mm256_cvtpd_epi32(ymm);
}

ML_INLINE double4::operator float4() const {
    return _mm256_cvtpd_ps(ymm);
}

// From float4x4
ML_INLINE float4x4::operator double4x4() const {
    double4x4 r;
    r.col0 = _mm256_cvtps_pd(col0);
    r.col1 = _mm256_cvtps_pd(col1);
    r.col2 = _mm256_cvtps_pd(col2);
    r.col3 = _mm256_cvtps_pd(col3);

    return r;
}

// From double4x4
ML_INLINE double4x4::operator float4x4() const {
    float4x4 r;
    r.col0 = _mm256_cvtpd_ps(col0);
    r.col1 = _mm256_cvtpd_ps(col1);
    r.col2 = _mm256_cvtpd_ps(col2);
    r.col3 = _mm256_cvtpd_ps(col3);

    return r;
}
