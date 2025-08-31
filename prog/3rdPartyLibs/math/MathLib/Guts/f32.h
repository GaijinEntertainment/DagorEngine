// © 2021 NVIDIA Corporation

#pragma once

//======================================================================================================================
// float2
//======================================================================================================================

union float2 {
    v2i mm;

    struct {
        float a[COORD_2D];
    };

    struct {
        float x, y;
    };

    ML_SWIZZLE_2(float2, float);

public:
    ML_INLINE float2()
        : mm(0) {
    }

    ML_INLINE float2(float c)
        : x(c), y(c) {
    }

    ML_INLINE float2(float _x, float _y)
        : x(_x), y(_y) {
    }

    ML_INLINE float2(const float2& v) = default;

    // Set

    ML_INLINE void operator=(const float2& v) {
        mm = v.mm;
    }

    // Conversion

    ML_INLINE operator int2() const;
    ML_INLINE operator uint2() const;
    ML_INLINE operator double2() const;

    // Compare

    ML_COMPARE_UNOPT(bool2, float2, <)
    ML_COMPARE_UNOPT(bool2, float2, <=)
    ML_COMPARE_UNOPT(bool2, float2, ==)
    ML_COMPARE_UNOPT(bool2, float2, >=)
    ML_COMPARE_UNOPT(bool2, float2, >)
    ML_COMPARE_UNOPT(bool2, float2, !=)

    // Ops

    ML_INLINE float2 operator-() const {
        return float2(-x, -y);
    }

    ML_OP_UNOPT(float2, float, -, -=)
    ML_OP_UNOPT(float2, float, +, +=)
    ML_OP_UNOPT(float2, float, *, *=)
    ML_OP_UNOPT(float2, float, /, /=)
};

ML_INLINE float2 degrees(const float2& x) {
    return x * (180.0f / acosf(-1.0f));
}

ML_INLINE float2 radians(const float2& x) {
    return x * (acosf(-1.0f) / 180.0f);
}

ML_INLINE float2 sign(const float2& x) {
    return float2(sign(x.x), sign(x.y));
}

ML_INLINE float2 abs(const float2& x) {
    return float2(abs(x.x), abs(x.y));
}

ML_INLINE float2 floor(const float2& x) {
    return float2(floor(x.x), floor(x.y));
}

ML_INLINE float2 round(const float2& x) {
    return float2(round(x.x), round(x.y));
}

ML_INLINE float2 ceil(const float2& x) {
    return float2(ceil(x.x), ceil(x.y));
}

ML_INLINE float2 frac(const float2& x) {
    return float2(frac(x.x), frac(x.y));
}

ML_INLINE float2 fmod(const float2& x, const float2& y) {
    return float2(fmod(x.x, y.x), fmod(x.y, y.y));
}

ML_INLINE float2 min(const float2& x, const float2& y) {
    return float2(min(x.x, y.x), min(x.y, y.y));
}

ML_INLINE float2 max(const float2& x, const float2& y) {
    return float2(max(x.x, y.x), max(x.y, y.y));
}

ML_INLINE float2 clamp(const float2& x, const float2& a, const float2& b) {
    return float2(clamp(x.x, a.x, b.x), clamp(x.y, a.y, b.y));
}

ML_INLINE float2 saturate(const float2& x) {
    return float2(clamp(x.x, 0.0f, 1.0f), clamp(x.y, 0.0f, 1.0f));
}

ML_INLINE float2 lerp(const float2& a, const float2& b, const float2& x) {
    return a + (b - a) * x;
}

ML_INLINE float2 linearstep(const float2& a, const float2& b, const float2& x) {
    return saturate((x - a) / (b - a));
}

ML_INLINE float2 smoothstep(const float2& a, const float2& b, const float2& x) {
    float2 t = linearstep(a, b, x);

    return t * t * (3.0f - 2.0f * t);
}

ML_INLINE float2 step(const float2& edge, const float2& x) {
    return float2(step(edge.x, x.x), step(edge.y, x.y));
}

ML_INLINE float2 sin(const float2& x) {
    return float2(sin(x.x), sin(x.y));
}

ML_INLINE float2 cos(const float2& x) {
    return float2(cos(x.x), cos(x.y));
}

ML_INLINE float2 tan(const float2& x) {
    return float2(tan(x.x), tan(x.y));
}

ML_INLINE float2 asin(const float2& x) {
    return float2(asin(x.x), asin(x.y));
}

ML_INLINE float2 acos(const float2& x) {
    return float2(acos(x.x), acos(x.y));
}

ML_INLINE float2 atan(const float2& x) {
    return float2(atan(x.x), atan(x.y));
}

ML_INLINE float2 atan2(const float2& y, const float2& x) {
    return float2(atan2(y.x, x.x), atan2(y.y, x.y));
}

ML_INLINE float2 sqrt(const float2& x) {
    return float2(sqrt(x.x), sqrt(x.y));
}

ML_INLINE float2 rsqrt(const float2& x) {
    return float2(rsqrt(x.x), rsqrt(x.y));
}

ML_INLINE float2 rcp(const float2& x) {
    return float2(rcp(x.x), rcp(x.y));
}

ML_INLINE float2 pow(const float2& x, const float2& y) {
    return float2(pow(x.x, y.x), pow(x.y, y.y));
}

ML_INLINE float2 log(const float2& x) {
    return float2(log(x.x), log(x.y));
}

ML_INLINE float2 log2(const float2& x) {
    return float2(log2(x.x), log2(x.y));
}

ML_INLINE float2 exp(const float2& x) {
    return float2(exp(x.x), exp(x.y));
}

ML_INLINE float2 exp2(const float2& x) {
    return float2(exp2(x.x), exp2(x.y));
}

ML_INLINE float2 madd(const float2& a, const float2& b, const float2& c) {
    return a * b + c;
}

ML_INLINE float dot(const float2& a, const float2& b) {
    return a.x * b.x + a.y * b.y;
}

ML_INLINE float length(const float2& x) {
    return sqrt(dot(x, x));
}

ML_INLINE float2 normalize(const float2& x) {
    return x / length(x);
}

// non-HLSL

ML_INLINE float2 Pi(const float2& mul) {
    return mul * acosf(-1.0f);
}

ML_INLINE float2 GetPerpendicularVector(const float2& a) {
    return float2(-a.y, a.x);
}

ML_INLINE float2 Snap(const float2& x, const float2& step) {
    return round(x / step) * step;
}

ML_INLINE float2 Rotate(const float2& v, float angle) {
    float sa = sin(angle);
    float ca = cos(angle);

    float2 p;
    p.x = ca * v.x + sa * v.y;
    p.y = ca * v.y - sa * v.x;

    return p;
}

//======================================================================================================================
// float3
//======================================================================================================================

union float3 {
    v4f xmm;

    struct {
        float a[COORD_3D];
    };

    struct {
        float x, y, z;
    };

    ML_SWIZZLE_3(v4f_swizzle2, float2, v4f_swizzle3, float3);

public:
    ML_INLINE float3()
        : xmm(_mm_setzero_ps()) {
    }

    ML_INLINE float3(float c)
        : xmm(_mm_set1_ps(c)) {
    }

    ML_INLINE float3(float _x, float _y, float _z)
        : xmm(v4f_set(_x, _y, _z, 0.0f)) {
    }

    ML_INLINE float3(const float2& v, float _z)
        : xmm(v4f_set(v.x, v.y, _z, 0.0f)) {
    }

    ML_INLINE float3(float _x, const float2& v)
        : xmm(v4f_set(_x, v.x, v.y, 0.0f)) {
    }

    ML_INLINE float3(const v4f& v)
        : xmm(v) {
    }

    ML_INLINE float3(const float* v3)
        : xmm(v4f_set(v3[0], v3[1], v3[2], 0.0f)) {
    }

    ML_INLINE float3(const float3& v) = default;

    // Set

    ML_INLINE void operator=(const float3& v) {
        xmm = v.xmm;
    }

    // Conversion

    ML_INLINE operator int3() const;
    ML_INLINE operator uint3() const;
    ML_INLINE operator double3() const;

    // Compare

    ML_COMPARE(bool3, float3, <, _mm_cmplt_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool3, float3, <=, _mm_cmple_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool3, float3, ==, _mm_cmpeq_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool3, float3, >, _mm_cmpgt_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool3, float3, >=, _mm_cmpge_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool3, float3, !=, _mm_cmpneq_ps, _mm_movemask_ps, xmm)

    // Ops

    ML_INLINE float3 operator-() const {
        return v4f_negate(xmm);
    }

    ML_OP(float3, float, -, -=, _mm_sub_ps, _mm_set1_ps, xmm)
    ML_OP(float3, float, +, +=, _mm_add_ps, _mm_set1_ps, xmm)
    ML_OP(float3, float, *, *=, _mm_mul_ps, _mm_set1_ps, xmm)
    ML_OP(float3, float, /, /=, _mm_div_ps, _mm_set1_ps, xmm)

    // Misc

    ML_INLINE operator v4f() const {
        return xmm;
    }

    static ML_INLINE float3 Zero() {
        return _mm_setzero_ps();
    }
};

ML_INLINE float3 degrees(const float3& x) {
    return x * (180.0f / acosf(-1.0f));
}

ML_INLINE float3 radians(const float3& x) {
    return x * (acosf(-1.0f) / 180.0f);
}

ML_INLINE float3 sign(const float3& x) {
    return v4f_sign(x.xmm);
}

ML_INLINE float3 abs(const float3& x) {
    return v4f_abs(x.xmm);
}

ML_INLINE float3 floor(const float3& x) {
    return v4f_floor(x.xmm);
}

ML_INLINE float3 round(const float3& x) {
    return v4f_round(x.xmm);
}

ML_INLINE float3 ceil(const float3& x) {
    return v4f_ceil(x.xmm);
}

ML_INLINE float3 frac(const float3& x) {
    return v4f_frac(x.xmm);
}

ML_INLINE float3 fmod(const float3& x, const float3& y) {
    return v4f_mod(x.xmm, y.xmm);
}

ML_INLINE float3 min(const float3& x, const float3& y) {
    return _mm_min_ps(x.xmm, y.xmm);
}

ML_INLINE float3 max(const float3& x, const float3& y) {
    return _mm_max_ps(x.xmm, y.xmm);
}

ML_INLINE float3 clamp(const float3& x, const float3& a, const float3& b) {
    return v4f_clamp(x.xmm, a.xmm, b.xmm);
}

ML_INLINE float3 saturate(const float3& x) {
    return v4f_saturate(x.xmm);
}

ML_INLINE float3 lerp(const float3& a, const float3& b, const float3& x) {
    return v4f_mix(a.xmm, b.xmm, x.xmm);
}

ML_INLINE float3 linearstep(const float3& a, const float3& b, const float3& x) {
    return v4f_linearstep(a.xmm, b.xmm, x.xmm);
}

ML_INLINE float3 smoothstep(const float3& a, const float3& b, const float3& x) {
    return v4f_smoothstep(a.xmm, b.xmm, x.xmm);
}

ML_INLINE float3 step(const float3& edge, const float3& x) {
    return v4f_step(edge.xmm, x.xmm);
}

ML_INLINE float3 sin(const float3& x) {
    return _mm_sin_ps(x.xmm);
}

ML_INLINE float3 cos(const float3& x) {
    return _mm_cos_ps(x.xmm);
}

ML_INLINE float3 tan(const float3& x) {
    return _mm_tan_ps(x.xmm);
}

ML_INLINE float3 asin(const float3& x) {
    ML_Assert(all(x >= float3(-1.0f)) && all(x <= float3(1.0f)));

    return _mm_asin_ps(x.xmm);
}

ML_INLINE float3 acos(const float3& x) {
    ML_Assert(all(x >= float3(-1.0f)) && all(x <= float3(1.0f)));

    return _mm_acos_ps(x.xmm);
}

ML_INLINE float3 atan(const float3& x) {
    return _mm_atan_ps(x.xmm);
}

ML_INLINE float3 atan2(const float3& y, const float3& x) {
    return _mm_atan2_ps(y.xmm, x.xmm);
}

ML_INLINE float3 sqrt(const float3& x) {
    return _mm_sqrt_ps(x.xmm);
}

ML_INLINE float3 rsqrt(const float3& x) {
    return v4f_rsqrt(x.xmm);
}

ML_INLINE float3 rcp(const float3& x) {
    return v4f_rcp(v4f_setw1(x.xmm));
}

ML_INLINE float3 pow(const float3& x, const float3& y) {
    return _mm_pow_ps(x.xmm, y.xmm);
}

ML_INLINE float3 log(const float3& x) {
    return _mm_log_ps(x.xmm);
}

ML_INLINE float3 log2(const float3& x) {
    return _mm_log2_ps(x.xmm);
}

ML_INLINE float3 exp(const float3& x) {
    return _mm_exp_ps(x.xmm);
}

ML_INLINE float3 exp2(const float3& x) {
    return _mm_exp2_ps(x.xmm);
}

ML_INLINE float3 madd(const float3& a, const float3& b, const float3& c) {
    return _mm_fmadd_ps(a.xmm, b.xmm, c.xmm);
}

ML_INLINE float dot(const float3& a, const float3& b) {
    v4f r = v4f_dot33(a.xmm, b.xmm);

    return _mm_cvtss_f32(r);
}

ML_INLINE float length(const float3& x) {
    v4f r = v4f_length(x.xmm);

    return _mm_cvtss_f32(r);
}

ML_INLINE float3 normalize(const float3& x) {
    return v4f_normalize(x.xmm);
}

ML_INLINE float3 cross(const float3& x, const float3& y) {
    return v4f_cross(x.xmm, y.xmm);
}

ML_INLINE float3 reflect(const float3& v, const float3& n) {
    // NOTE: slow
    // return v - n * dot(n, v) * 2;

    v4f dot0 = v4f_dot33(n.xmm, v.xmm);
    dot0 = _mm_mul_ps(dot0, _mm_set1_ps(2.0f));

    return _mm_fnmadd_ps(n.xmm, dot0, v.xmm);
}

ML_INLINE float3 refract(const float3& v, const float3& n, float eta) {
    // NOTE: slow
    /*
    float dot = dot(v, n);
    float k = 1 - eta * eta * (1 - dot * dot);

    if( k < 0 )
        return 0

    return v * eta - n * (eta * dot + Sqrt(k));
    */

    v4f eta0 = _mm_set1_ps(eta);
    v4f dot0 = v4f_dot33(n.xmm, v.xmm);
    v4f mul0 = _mm_mul_ps(eta0, eta0);
    v4f sub0 = _mm_fnmadd_ps(dot0, dot0, c_v4f_1111);
    v4f sub1 = _mm_fnmadd_ps(mul0, sub0, c_v4f_1111);

    if (v4f_isnegative4_all(sub1))
        return _mm_setzero_ps();

    v4f mul5 = _mm_mul_ps(eta0, v.xmm);
    v4f mul3 = _mm_mul_ps(eta0, dot0);
    v4f sqt0 = _mm_sqrt_ps(sub1);
    v4f add0 = _mm_add_ps(mul3, sqt0);

    return _mm_fnmadd_ps(add0, n.xmm, mul5);
}

// non-HLSL

ML_INLINE float3 Pi(const float3& mul) {
    return mul * acosf(-1.0f);
}

ML_INLINE float3 GetPerpendicularVector(const float3& N) {
    float3 T = float3(N.z, -N.x, N.y);
    T -= N * dot(T, N);

    return normalize(T);
}

ML_INLINE float3 SinCos(const float3& x, float3* pCos) {
    return _mm_sincos_ps(&pCos->xmm, x.xmm);
}

ML_INLINE float3 Snap(const float3& x, const float3& step) {
    return round(x / step) * step;
}

ML_INLINE bool IsPointsNear(const float3& p1, const float3& p2, float eps) {
    v4f r = _mm_sub_ps(p1.xmm, p2.xmm);
    r = v4f_abs(r);
    r = _mm_cmple_ps(r, _mm_set1_ps(eps));

    return v4f_test3_all(r);
}

//======================================================================================================================
// float4
//======================================================================================================================

union float4 {
    v4f xmm;

    struct {
        float a[COORD_4D];
    };

    struct {
        float x, y, z, w;
    };

    ML_SWIZZLE_4(v4f_swizzle2, float2, v4f_swizzle3, float3, v4f_swizzle4, float4);

public:
    ML_INLINE float4()
        : xmm(_mm_setzero_ps()) {
    }

    ML_INLINE float4(float c)
        : xmm(_mm_set1_ps(c)) {
    }

    ML_INLINE float4(float _x, float _y, float _z, float _w)
        : xmm(v4f_set(_x, _y, _z, _w)) {
    }

    ML_INLINE float4(const float3& v, float _w)
        : xmm(v4f_set(v.x, v.y, v.z, _w)) {
    }

    ML_INLINE float4(const float2& a, const float2& b)
        : xmm(v4f_set(a.x, a.y, b.x, b.y)) {
    }

    ML_INLINE float4(float _x, const float3& v)
        : xmm(v4f_set(_x, v.x, v.y, v.z)) {
    }

    ML_INLINE float4(const v4f& v)
        : xmm(v) {
    }

    ML_INLINE float4(const float* v4)
        : xmm(_mm_loadu_ps(v4)) {
    }

    ML_INLINE float4(const float4& v) = default;

    // Set

    ML_INLINE void operator=(const float4& v) {
        xmm = v.xmm;
    }

    // Conversion

    ML_INLINE operator int4() const;
    ML_INLINE operator uint4() const;
    ML_INLINE operator double4() const;

    // Compare

    ML_COMPARE(bool4, float4, <, _mm_cmplt_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool4, float4, <=, _mm_cmple_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool4, float4, ==, _mm_cmpeq_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool4, float4, >, _mm_cmpgt_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool4, float4, >=, _mm_cmpge_ps, _mm_movemask_ps, xmm)
    ML_COMPARE(bool4, float4, !=, _mm_cmpneq_ps, _mm_movemask_ps, xmm)

    // Ops

    ML_INLINE float4 operator-() const {
        return v4f_negate(xmm);
    }

    ML_OP(float4, float, -, -=, _mm_sub_ps, _mm_set1_ps, xmm)
    ML_OP(float4, float, +, +=, _mm_add_ps, _mm_set1_ps, xmm)
    ML_OP(float4, float, *, *=, _mm_mul_ps, _mm_set1_ps, xmm)
    ML_OP(float4, float, /, /=, _mm_div_ps, _mm_set1_ps, xmm)

    // Misc

    ML_INLINE operator v4f() const {
        return xmm;
    }

    static ML_INLINE float4 Zero() {
        return _mm_setzero_ps();
    }
};

ML_INLINE float4 degrees(const float4& x) {
    return x * (180.0f / acosf(-1.0f));
}

ML_INLINE float4 radians(const float4& x) {
    return x * (acosf(-1.0f) / 180.0f);
}

ML_INLINE float4 sign(const float4& x) {
    return v4f_sign(x.xmm);
}

ML_INLINE float4 abs(const float4& x) {
    return v4f_abs(x.xmm);
}

ML_INLINE float4 floor(const float4& x) {
    return v4f_floor(x.xmm);
}

ML_INLINE float4 round(const float4& x) {
    return v4f_round(x.xmm);
}

ML_INLINE float4 ceil(const float4& x) {
    return v4f_ceil(x.xmm);
}

ML_INLINE float4 frac(const float4& x) {
    return v4f_frac(x.xmm);
}

ML_INLINE float4 fmod(const float4& x, const float4& y) {
    return v4f_mod(x.xmm, y.xmm);
}

ML_INLINE float4 min(const float4& x, const float4& y) {
    return _mm_min_ps(x.xmm, y.xmm);
}

ML_INLINE float4 max(const float4& x, const float4& y) {
    return _mm_max_ps(x.xmm, y.xmm);
}

ML_INLINE float4 clamp(const float4& x, const float4& a, const float4& b) {
    return v4f_clamp(x.xmm, a.xmm, b.xmm);
}

ML_INLINE float4 saturate(const float4& x) {
    return v4f_saturate(x.xmm);
}

ML_INLINE float4 lerp(const float4& a, const float4& b, const float4& x) {
    return v4f_mix(a.xmm, b.xmm, x.xmm);
}

ML_INLINE float4 linearstep(const float4& a, const float4& b, const float4& x) {
    return v4f_linearstep(a.xmm, b.xmm, x.xmm);
}

ML_INLINE float4 smoothstep(const float4& a, const float4& b, const float4& x) {
    return v4f_smoothstep(a.xmm, b.xmm, x.xmm);
}

ML_INLINE float4 step(const float4& edge, const float4& x) {
    return v4f_step(edge.xmm, x.xmm);
}

ML_INLINE float4 sin(const float4& x) {
    return _mm_sin_ps(x.xmm);
}

ML_INLINE float4 cos(const float4& x) {
    return _mm_cos_ps(x.xmm);
}

ML_INLINE float4 tan(const float4& x) {
    return _mm_tan_ps(x.xmm);
}

ML_INLINE float4 asin(const float4& x) {
    ML_Assert(all(x >= float4(-1.0f)) && all(x <= float4(1.0f)));

    return _mm_asin_ps(x.xmm);
}

ML_INLINE float4 acos(const float4& x) {
    ML_Assert(all(x >= float4(-1.0f)) && all(x <= float4(1.0f)));

    return _mm_acos_ps(x.xmm);
}

ML_INLINE float4 atan(const float4& x) {
    return _mm_atan_ps(x.xmm);
}

ML_INLINE float4 atan2(const float4& y, const float4& x) {
    return _mm_atan2_ps(y.xmm, x.xmm);
}

ML_INLINE float4 sqrt(const float4& x) {
    return _mm_sqrt_ps(x.xmm);
}

ML_INLINE float4 rsqrt(const float4& x) {
    return v4f_rsqrt(x.xmm);
}

ML_INLINE float4 rcp(const float4& x) {
    return v4f_rcp(x.xmm);
}

ML_INLINE float4 pow(const float4& x, const float4& y) {
    return _mm_pow_ps(x.xmm, y.xmm);
}

ML_INLINE float4 log(const float4& x) {
    return _mm_log_ps(x.xmm);
}

ML_INLINE float4 log2(const float4& x) {
    return _mm_log2_ps(x.xmm);
}

ML_INLINE float4 exp(const float4& x) {
    return _mm_exp_ps(x.xmm);
}

ML_INLINE float4 exp2(const float4& x) {
    return _mm_exp2_ps(x.xmm);
}

ML_INLINE float4 madd(const float4& a, const float4& b, const float4& c) {
    return _mm_fmadd_ps(a.xmm, b.xmm, c.xmm);
}

ML_INLINE float dot(const float4& a, const float4& b) {
    v4f r = v4f_dot44(a.xmm, b.xmm);

    return _mm_cvtss_f32(r);
}

// Non-HLSL

ML_INLINE float4 Pi(const float4& mul) {
    return mul * acosf(-1.0f);
}

ML_INLINE float Dot43(const float4& a, const float3& b) {
    v4f r = v4f_dot43(a.xmm, b.xmm);

    return _mm_cvtss_f32(r);
}

ML_INLINE float4 Snap(const float4& x, const float4& step) {
    return round(x / step) * step;
}

ML_INLINE float4 SinCos(const float4& x, float4* pCos) {
    return _mm_sincos_ps(&pCos->xmm, x.xmm);
}

// TODO: add "Quaternion"
ML_INLINE float4 Slerp(const float4& a, const float4& b, float x) {
    ML_Assert(x >= 0.0f && x <= 1.0f);
    ML_Assert(abs(dot(a, a) - 1.0f) < 1e-5f);
    ML_Assert(abs(dot(b, b) - 1.0f) < 1e-5f);

    float4 r;

    float theta = dot(a, b);
    if (theta > 0.9995f)
        r = lerp(a, b, x);
    else {
        theta = acos(theta);

        float3 s = sin(theta * float3(1.0f, 1.0f - x, x));
        float sn = 1.0f / s.x;
        float wa = s.y * sn;
        float wb = s.z * sn;

        r = a * wa + b * wb;
    }

    r *= rsqrt(dot(r, r));

    return r;
}

//======================================================================================================================
// float4x4
//======================================================================================================================

// IMPORTANT: store - "column-major", usage - "row-major" (vector is a column)
union float4x4 {
#if defined(__GNUC__)
    struct {
        v4f col0;
        v4f col1;
        v4f col2;
        v4f col3;
    };
#else
    struct {
        float4 col0;
        float4 col1;
        float4 col2;
        float4 col3;
    };
#endif

    struct {
        float4 cols[4];
    };

    struct {
        float a[16];
    };

    struct {
        struct {
            float a00, a10, a20, a30;
        };

        struct {
            float a01, a11, a21, a31;
        };

        struct {
            float a02, a12, a22, a32;
        };

        struct {
            float a03, a13, a23, a33;
        };
    };

public:
    ML_INLINE float4x4() {
        col0 = _mm_setzero_ps();
        col1 = _mm_setzero_ps();
        col2 = _mm_setzero_ps();
        col3 = _mm_setzero_ps();
    }

    ML_INLINE float4x4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31,
        float m32, float m33) {
        col0 = v4f_set(m00, m10, m20, m30);
        col1 = v4f_set(m01, m11, m21, m31);
        col2 = v4f_set(m02, m12, m22, m32);
        col3 = v4f_set(m03, m13, m23, m33);
    }

    ML_INLINE float4x4(const float4& c0, const float4& c1, const float4& c2, const float4& c3) {
        col0 = c0;
        col1 = c1;
        col2 = c2;
        col3 = c3;
    }

    ML_INLINE float4x4(const float4x4& m) = default;

    // Set

    ML_INLINE void operator=(const float4x4& m) {
        col0 = m.col0;
        col1 = m.col1;
        col2 = m.col2;
        col3 = m.col3;
    }

    // Conversion

    ML_INLINE operator double4x4() const;

    // Compare

    ML_INLINE bool operator==(const float4x4& m) const {
        return all(float4(col0) == float4(m.col0)) && all(float4(col1) == float4(m.col1)) && all(float4(col2) == float4(m.col2)) && all(float4(col3) == float4(m.col3));
    }

    ML_INLINE bool operator!=(const float4x4& m) const {
        return any(float4(col0) != float4(m.col0)) || any(float4(col1) != float4(m.col1)) || any(float4(col2) != float4(m.col2)) || any(float4(col3) != float4(m.col3));
    }

    // NOTE: *

    ML_INLINE float4x4 operator*(const float4x4& m) const {
        float4x4 r;

        v4f r1 = _mm_mul_ps(v4f_swizzle(m.col0, 0, 0, 0, 0), col0);
        v4f r2 = _mm_mul_ps(v4f_swizzle(m.col1, 0, 0, 0, 0), col0);

        r1 = _mm_fmadd_ps(v4f_swizzle(m.col0, 1, 1, 1, 1), col1, r1);
        r2 = _mm_fmadd_ps(v4f_swizzle(m.col1, 1, 1, 1, 1), col1, r2);
        r1 = _mm_fmadd_ps(v4f_swizzle(m.col0, 2, 2, 2, 2), col2, r1);
        r2 = _mm_fmadd_ps(v4f_swizzle(m.col1, 2, 2, 2, 2), col2, r2);
        r1 = _mm_fmadd_ps(v4f_swizzle(m.col0, 3, 3, 3, 3), col3, r1);
        r2 = _mm_fmadd_ps(v4f_swizzle(m.col1, 3, 3, 3, 3), col3, r2);

        r.col0 = r1;
        r.col1 = r2;

        r1 = _mm_mul_ps(v4f_swizzle(m.col2, 0, 0, 0, 0), col0);
        r2 = _mm_mul_ps(v4f_swizzle(m.col3, 0, 0, 0, 0), col0);

        r1 = _mm_fmadd_ps(v4f_swizzle(m.col2, 1, 1, 1, 1), col1, r1);
        r2 = _mm_fmadd_ps(v4f_swizzle(m.col3, 1, 1, 1, 1), col1, r2);
        r1 = _mm_fmadd_ps(v4f_swizzle(m.col2, 2, 2, 2, 2), col2, r1);
        r2 = _mm_fmadd_ps(v4f_swizzle(m.col3, 2, 2, 2, 2), col2, r2);
        r1 = _mm_fmadd_ps(v4f_swizzle(m.col2, 3, 3, 3, 3), col3, r1);
        r2 = _mm_fmadd_ps(v4f_swizzle(m.col3, 3, 3, 3, 3), col3, r2);

        r.col2 = r1;
        r.col3 = r2;

        return r;
    }

    ML_INLINE float4 operator*(const float4& v) const {
        v4f r = _mm_mul_ps(v4f_swizzle(v.xmm, 0, 0, 0, 0), col0);
        r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 1, 1, 1, 1), col1, r);
        r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 2, 2, 2, 2), col2, r);
        r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 3, 3, 3, 3), col3, r);

        return r;
    }

    ML_INLINE float3 operator*(const float3& v) const {
        v4f r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 0, 0, 0, 0), col0, col3);
        r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 1, 1, 1, 1), col1, r);
        r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 2, 2, 2, 2), col2, r);

        return r;
    }

    // NOTE: other

    static ML_INLINE float4x4 Identity() {
        return float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    float4& operator[](uint32_t col) {
        ML_Assert(col < 4);

        return cols[col];
    }

    const float4& operator[](uint32_t col) const {
        ML_Assert(col < 4);

        return cols[col];
    }

    ML_INLINE float4 GetRow0() const {
        return float4(a00, a01, a02, a03);
    }

    ML_INLINE float4 GetRow1() const {
        return float4(a10, a11, a12, a13);
    }

    ML_INLINE float4 GetRow2() const {
        return float4(a20, a21, a22, a23);
    }

    ML_INLINE float4 GetRow3() const {
        return float4(a30, a31, a32, a33);
    }

    ML_INLINE float GetNdcDepth(float z) const {
        float c = a22 * z + a23;
        float d = a32 * z + a33;

        return c / d;
    }

    ML_INLINE float3 GetRotationYPR() const {
        float3 r;
        r.x = atan2(-a01, a11);
        r.y = asin(a21);
        r.z = atan2(-a20, a22);

        return r;
    }

    ML_INLINE float4 GetQuaternion() const {
        float4 q;
        float t;

        if (a22 < 0.0f) {
            if (a00 > a11) {
                t = 1.0f + a00 - a11 - a22;
                q = float4(t, a10 + a01, a02 + a20, a21 - a12);
            } else {
                t = 1.0f - a00 + a11 - a22;
                q = float4(a10 + a01, t, a21 + a12, a02 - a20);
            }
        } else {
            if (a00 < -a11) {
                t = 1.0f - a00 - a11 + a22;
                q = float4(a02 + a20, a21 + a12, t, a10 - a01);
            } else {
                t = 1.0f + a00 + a11 + a22;
                q = float4(a21 - a12, a02 - a20, a10 - a01, t);
            }
        }

        q *= 0.5f / sqrt(t);

        return q;
    }

    ML_INLINE float3 GetScale() const {
        float3 scale = float3(_mm_cvtss_f32(v4f_length(col0)), _mm_cvtss_f32(v4f_length(col1)), _mm_cvtss_f32(v4f_length(col2)));

        return scale;
    }

    ML_INLINE void SetTranslation(const float3& p) {
        col3 = v4f_setw1(p.xmm);
    }

    ML_INLINE void AddTranslation(const float3& p) {
        col3 = _mm_add_ps(col3, v4f_setw0(p.xmm));
    }

    ML_INLINE void PreTranslation(const float3& p);

    ML_INLINE void AddScale(const float3& scale) {
        col0 = _mm_mul_ps(col0, scale.xmm);
        col1 = _mm_mul_ps(col1, scale.xmm);
        col2 = _mm_mul_ps(col2, scale.xmm);
    }

    ML_INLINE void WorldToView(uint32_t uiProjFlags = 0) {
        /*
        float4x4 rot;
        rot.SetupByRotationX(c_fHalfPi);
        *this = (*this) * rot;
        InvertOrtho();
        */

        Swap(col1, col2);

        if ((uiProjFlags & PROJ_LEFT_HANDED) == 0)
            col2 = v4f_negate(col2);

        Transpose3x4();
    }

    ML_INLINE void ViewToWorld(uint32_t uiProjFlags = 0) {
        Transpose3x4();

        if ((uiProjFlags & PROJ_LEFT_HANDED) == 0)
            col2 = v4f_negate(col2);

        Swap(col1, col2);
    }

    ML_INLINE bool IsLeftHanded() const {
        float3 v1 = cross(float3(col0), float3(col1));

        return dot(v1, float3(col2)) < 0.0f;
    }

    ML_INLINE void TransposeTo(float4x4& m) const {
        v4f xmm0 = v4f_Ax_Bx_Ay_By(col0, col1);
        v4f xmm1 = v4f_Ax_Bx_Ay_By(col2, col3);
        v4f xmm2 = v4f_Az_Bz_Aw_Bw(col0, col1);
        v4f xmm3 = v4f_Az_Bz_Aw_Bw(col2, col3);

        m.col0 = v4f_Axy_Bxy(xmm0, xmm1);
        m.col1 = v4f_Azw_Bzw(xmm1, xmm0);
        m.col2 = v4f_Axy_Bxy(xmm2, xmm3);
        m.col3 = v4f_Azw_Bzw(xmm3, xmm2);
    }

    ML_INLINE void Transpose() {
        v4f xmm0 = v4f_Ax_Bx_Ay_By(col0, col1);
        v4f xmm1 = v4f_Ax_Bx_Ay_By(col2, col3);
        v4f xmm2 = v4f_Az_Bz_Aw_Bw(col0, col1);
        v4f xmm3 = v4f_Az_Bz_Aw_Bw(col2, col3);

        col0 = v4f_Axy_Bxy(xmm0, xmm1);
        col1 = v4f_Azw_Bzw(xmm1, xmm0);
        col2 = v4f_Axy_Bxy(xmm2, xmm3);
        col3 = v4f_Azw_Bzw(xmm3, xmm2);
    }

    ML_INLINE void Transpose3x4() {
        v4f xmm0 = v4f_Ax_Bx_Ay_By(col0, col1);
        v4f xmm1 = v4f_Ax_Bx_Ay_By(col2, col3);
        v4f xmm2 = v4f_Az_Bz_Aw_Bw(col0, col1);
        v4f xmm3 = v4f_Az_Bz_Aw_Bw(col2, col3);

        col0 = v4f_Axy_Bxy(xmm0, xmm1);
        col1 = v4f_Azw_Bzw(xmm1, xmm0);
        col2 = v4f_Axy_Bxy(xmm2, xmm3);
    }

    ML_INLINE void Invert() {
        // NOTE: http://forum.devmaster.net/t/sse-mat4-inverse/16799

        v4f Fac0;
        {
            v4f Swp0a = v4f_shuffle(col3, col2, 3, 3, 3, 3);
            v4f Swp0b = v4f_shuffle(col3, col2, 2, 2, 2, 2);

            v4f Swp00 = v4f_shuffle(col2, col1, 2, 2, 2, 2);
            v4f Swp01 = v4f_swizzle(Swp0a, 0, 0, 0, 2);
            v4f Swp02 = v4f_swizzle(Swp0b, 0, 0, 0, 2);
            v4f Swp03 = v4f_shuffle(col2, col1, 3, 3, 3, 3);

            v4f Mul00 = _mm_mul_ps(Swp00, Swp01);

            Fac0 = _mm_fnmadd_ps(Swp02, Swp03, Mul00);
        }

        v4f Fac1;
        {
            v4f Swp0a = v4f_shuffle(col3, col2, 3, 3, 3, 3);
            v4f Swp0b = v4f_shuffle(col3, col2, 1, 1, 1, 1);

            v4f Swp00 = v4f_shuffle(col2, col1, 1, 1, 1, 1);
            v4f Swp01 = v4f_swizzle(Swp0a, 0, 0, 0, 2);
            v4f Swp02 = v4f_swizzle(Swp0b, 0, 0, 0, 2);
            v4f Swp03 = v4f_shuffle(col2, col1, 3, 3, 3, 3);

            v4f Mul00 = _mm_mul_ps(Swp00, Swp01);

            Fac1 = _mm_fnmadd_ps(Swp02, Swp03, Mul00);
        }

        v4f Fac2;
        {
            v4f Swp0a = v4f_shuffle(col3, col2, 2, 2, 2, 2);
            v4f Swp0b = v4f_shuffle(col3, col2, 1, 1, 1, 1);

            v4f Swp00 = v4f_shuffle(col2, col1, 1, 1, 1, 1);
            v4f Swp01 = v4f_swizzle(Swp0a, 0, 0, 0, 2);
            v4f Swp02 = v4f_swizzle(Swp0b, 0, 0, 0, 2);
            v4f Swp03 = v4f_shuffle(col2, col1, 2, 2, 2, 2);

            v4f Mul00 = _mm_mul_ps(Swp00, Swp01);

            Fac2 = _mm_fnmadd_ps(Swp02, Swp03, Mul00);
        }

        v4f Fac3;
        {
            v4f Swp0a = v4f_shuffle(col3, col2, 3, 3, 3, 3);
            v4f Swp0b = v4f_shuffle(col3, col2, 0, 0, 0, 0);

            v4f Swp00 = v4f_shuffle(col2, col1, 0, 0, 0, 0);
            v4f Swp01 = v4f_swizzle(Swp0a, 0, 0, 0, 2);
            v4f Swp02 = v4f_swizzle(Swp0b, 0, 0, 0, 2);
            v4f Swp03 = v4f_shuffle(col2, col1, 3, 3, 3, 3);

            v4f Mul00 = _mm_mul_ps(Swp00, Swp01);

            Fac3 = _mm_fnmadd_ps(Swp02, Swp03, Mul00);
        }

        v4f Fac4;
        {
            v4f Swp0a = v4f_shuffle(col3, col2, 2, 2, 2, 2);
            v4f Swp0b = v4f_shuffle(col3, col2, 0, 0, 0, 0);

            v4f Swp00 = v4f_shuffle(col2, col1, 0, 0, 0, 0);
            v4f Swp01 = v4f_swizzle(Swp0a, 0, 0, 0, 2);
            v4f Swp02 = v4f_swizzle(Swp0b, 0, 0, 0, 2);
            v4f Swp03 = v4f_shuffle(col2, col1, 2, 2, 2, 2);

            v4f Mul00 = _mm_mul_ps(Swp00, Swp01);

            Fac4 = _mm_fnmadd_ps(Swp02, Swp03, Mul00);
        }

        v4f Fac5;
        {
            v4f Swp0a = v4f_shuffle(col3, col2, 1, 1, 1, 1);
            v4f Swp0b = v4f_shuffle(col3, col2, 0, 0, 0, 0);

            v4f Swp00 = v4f_shuffle(col2, col1, 0, 0, 0, 0);
            v4f Swp01 = v4f_swizzle(Swp0a, 0, 0, 0, 2);
            v4f Swp02 = v4f_swizzle(Swp0b, 0, 0, 0, 2);
            v4f Swp03 = v4f_shuffle(col2, col1, 1, 1, 1, 1);

            v4f Mul00 = _mm_mul_ps(Swp00, Swp01);

            Fac5 = _mm_fnmadd_ps(Swp02, Swp03, Mul00);
        }

        v4f SignA = _mm_set_ps(1.0f, -1.0f, 1.0f, -1.0f);
        v4f SignB = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);

        v4f Temp0 = v4f_shuffle(col1, col0, 0, 0, 0, 0);
        v4f Vec0 = v4f_swizzle(Temp0, 0, 2, 2, 2);

        v4f Temp1 = v4f_shuffle(col1, col0, 1, 1, 1, 1);
        v4f Vec1 = v4f_swizzle(Temp1, 0, 2, 2, 2);

        v4f Temp2 = v4f_shuffle(col1, col0, 2, 2, 2, 2);
        v4f Vec2 = v4f_swizzle(Temp2, 0, 2, 2, 2);

        v4f Temp3 = v4f_shuffle(col1, col0, 3, 3, 3, 3);
        v4f Vec3 = v4f_swizzle(Temp3, 0, 2, 2, 2);

        v4f Mul0 = _mm_mul_ps(Vec1, Fac0);
        v4f Mul1 = _mm_mul_ps(Vec0, Fac0);
        v4f Mul2 = _mm_mul_ps(Vec0, Fac1);
        v4f Mul3 = _mm_mul_ps(Vec0, Fac2);

        v4f Sub0 = _mm_fnmadd_ps(Vec2, Fac1, Mul0);
        v4f Sub1 = _mm_fnmadd_ps(Vec2, Fac3, Mul1);
        v4f Sub2 = _mm_fnmadd_ps(Vec1, Fac3, Mul2);
        v4f Sub3 = _mm_fnmadd_ps(Vec1, Fac4, Mul3);

        v4f Add0 = _mm_fmadd_ps(Vec3, Fac2, Sub0);
        v4f Add1 = _mm_fmadd_ps(Vec3, Fac4, Sub1);
        v4f Add2 = _mm_fmadd_ps(Vec3, Fac5, Sub2);
        v4f Add3 = _mm_fmadd_ps(Vec2, Fac5, Sub3);

        v4f Inv0 = _mm_mul_ps(SignB, Add0);
        v4f Inv1 = _mm_mul_ps(SignA, Add1);
        v4f Inv2 = _mm_mul_ps(SignB, Add2);
        v4f Inv3 = _mm_mul_ps(SignA, Add3);

        v4f Row0 = v4f_shuffle(Inv0, Inv1, 0, 0, 0, 0);
        v4f Row1 = v4f_shuffle(Inv2, Inv3, 0, 0, 0, 0);
        v4f Row2 = v4f_shuffle(Row0, Row1, 0, 2, 0, 2);

        v4f Det0 = v4f_dot44(col0, Row2);
        v4f Rcp0 = v4f_rcp(Det0);

        col0 = _mm_mul_ps(Inv0, Rcp0);
        col1 = _mm_mul_ps(Inv1, Rcp0);
        col2 = _mm_mul_ps(Inv2, Rcp0);
        col3 = _mm_mul_ps(Inv3, Rcp0);
    }

    ML_INLINE void InvertOrtho();

    // NOTE: special sets

    ML_INLINE void SetupByQuaternion(const float4& q) {
        // NOTE: assuming the quaternion is normalized
        float x2 = q.x + q.x;
        float y2 = q.y + q.y;
        float z2 = q.z + q.z;
        float xx2 = q.x * x2;
        float xy2 = q.x * y2;
        float xz2 = q.x * z2;
        float yy2 = q.y * y2;
        float yz2 = q.y * z2;
        float zz2 = q.z * z2;
        float wx2 = q.w * x2;
        float wy2 = q.w * y2;
        float wz2 = q.w * z2;

        col0 = float4(1.0f - (yy2 + zz2), xy2 + wz2, xz2 - wy2, 0.0f).xmm;
        col1 = float4(xy2 - wz2, 1.0f - (xx2 + zz2), yz2 + wx2, 0.0f).xmm;
        col2 = float4(xz2 + wy2, yz2 - wx2, 1.0f - (xx2 + yy2), 0.0f).xmm;
        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByRotationX(float angleX) {
        float ct = cos(angleX);
        float st = sin(angleX);

        col0 = float4(1.0f, 0.0f, 0.0f, 0.0f);
        col1 = float4(0.0f, ct, st, 0.0f);
        col2 = float4(0.0f, -st, ct, 0.0f);
        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByRotationY(float angleY) {
        float ct = cos(angleY);
        float st = sin(angleY);

        col0 = float4(ct, 0.0f, -st, 0.0f);
        col1 = float4(0.0f, 1.0f, 0.0f, 0.0f);
        col2 = float4(st, 0.0f, ct, 0.0f);
        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByRotationZ(float angleZ) {
        float ct = cos(angleZ);
        float st = sin(angleZ);

        col0 = float4(ct, st, 0.0f, 0.0f);
        col1 = float4(-st, ct, 0.0f, 0.0f);
        col2 = float4(0.0f, 0.0f, 1.0f, 0.0f);
        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByRotationYPR(float fYaw, float fPitch, float fRoll) {
        // NOTE: "yaw-pitch-roll" rotation
        //       yaw - around Z (object "down-up" axis)
        //       pitch - around X (object "left-right" axis)
        //       roll - around Y (object "backward-forward" axis)

        /*
        float4x4 rot;
        rot.SetupByRotationY(fRoll);
        *this = rot;
        rot.SetupByRotationX(fPitch);
        *this = rot * (*this);
        rot.SetupByRotationZ(fYaw);
        *this = rot * (*this);
        */

        float4 angles(fYaw, fPitch, fRoll, 0.0f);

        float4 c;
        float4 s = _mm_sincos_ps(&c.xmm, angles.xmm);

        a00 = c.x * c.z - s.x * s.y * s.z;
        a10 = s.x * c.z + c.x * s.y * s.z;
        a20 = -c.y * s.z;
        a30 = 0.0f;

        a01 = -s.x * c.y;
        a11 = c.x * c.y;
        a21 = s.y;
        a31 = 0.0f;

        a02 = c.x * s.z + c.z * s.x * s.y;
        a12 = s.x * s.z - c.x * s.y * c.z;
        a22 = c.y * c.z;
        a32 = 0.0f;

        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByRotation(float theta, const float3& v) {
        float ct = cos(theta);
        float st = sin(theta);

        SetupByRotation(st, ct, v);
    }

    ML_INLINE void SetupByRotation(float st, float ct, const float3& v) {
        float xx = v.x * v.x;
        float yy = v.y * v.y;
        float zz = v.z * v.z;
        float xy = v.x * v.y;
        float xz = v.x * v.z;
        float yz = v.y * v.z;
        float ctxy = ct * xy;
        float ctxz = ct * xz;
        float ctyz = ct * yz;
        float sty = st * v.y;
        float stx = st * v.x;
        float stz = st * v.z;

        a00 = xx + ct * (1.0f - xx);
        a01 = xy - ctxy - stz;
        a02 = xz - ctxz + sty;

        a10 = xy - ctxy + stz;
        a11 = yy + ct * (1.0f - yy);
        a12 = yz - ctyz - stx;

        a20 = xz - ctxz - sty;
        a21 = yz - ctyz + stx;
        a22 = zz + ct * (1.0f - zz);

        a30 = 0.0f;
        a31 = 0.0f;
        a32 = 0.0f;

        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByRotation(const float3& z, const float3& d) {
        /*
        // NOTE: same as

        float3 axis = cross(z, d);
        float angle = Acos( dot(z, d) );

        SetupByRotation(angle, axis);
        */

        float3 w = cross(z, d);
        float c = dot(z, d);
        float k = (1.0f - c) / (1.0f - c * c);

        float hxy = w.x * w.y * k;
        float hxz = w.x * w.z * k;
        float hyz = w.y * w.z * k;

        a00 = c + w.x * w.x * k;
        a01 = hxy - w.z;
        a02 = hxz + w.y;

        a10 = hxy + w.z;
        a11 = c + w.y * w.y * k;
        a12 = hyz - w.x;

        a20 = hxz - w.y;
        a21 = hyz + w.x;
        a22 = c + w.z * w.z * k;

        a30 = 0.0f;
        a31 = 0.0f;
        a32 = 0.0f;

        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByTranslation(const float3& p) {
        col0 = float4(1.0f, 0.0f, 0.0f, 0.0f);
        col1 = float4(0.0f, 1.0f, 0.0f, 0.0f);
        col2 = float4(0.0f, 0.0f, 1.0f, 0.0f);
        col3 = v4f_setw1(p);
    }

    ML_INLINE void SetupByScale(const float3& scale) {
        col0 = float4(scale.x, 0.0f, 0.0f, 0.0f);
        col1 = float4(0.0f, scale.y, 0.0f, 0.0f);
        col2 = float4(0.0f, 0.0f, scale.z, 0.0f);
        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByLookAt(const float3& vForward) {
        float3 y = normalize(vForward);
        float3 z = GetPerpendicularVector(y);
        float3 x = cross(y, z);

        col0 = v4f_setw0(x);
        col1 = v4f_setw0(y);
        col2 = v4f_setw0(z);
        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByLookAt(const float3& vForward, const float3& vRight) {
        float3 y = normalize(vForward);
        float3 z = normalize(cross(vRight, y));
        float3 x = cross(y, z);

        col0 = v4f_setw0(x);
        col1 = v4f_setw0(y);
        col2 = v4f_setw0(z);
        col3 = c_v4f_0001;
    }

    ML_INLINE void SetupByOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar, uint32_t uiProjFlags = 0) {
        ML_Assert(left < right);
        ML_Assert(bottom < top);

        float rWidth = 1.0f / (right - left);
        float rHeight = 1.0f / (top - bottom);
        float rDepth = 1.0f / (zFar - zNear);

        a00 = 2.0f * rWidth;
        a01 = 0.0f;
        a02 = 0.0f;
        a03 = -(right + left) * rWidth;

        a10 = 0.0f;
        a11 = 2.0f * rHeight;
        a12 = 0.0f;
        a13 = -(top + bottom) * rHeight;

        a20 = 0.0f;
        a21 = 0.0f;
        a22 = -2.0f * rDepth;
        a23 = -(zFar + zNear) * rDepth;

        a30 = 0.0f;
        a31 = 0.0f;
        a32 = 0.0f;
        a33 = 1.0f;

        bool bReverseZ = (uiProjFlags & PROJ_REVERSED_Z) != 0;

        a22 = ML_ModifyProjZ(bReverseZ, a22, a32);
        a23 = ML_ModifyProjZ(bReverseZ, a23, a33);

        if (uiProjFlags & PROJ_LEFT_HANDED)
            col2 = v4f_negate(col2);
    }

    ML_INLINE void SetupByFrustum(float left, float right, float bottom, float top, float zNear, float zFar, uint32_t uiProjFlags = 0) {
        ML_Assert(left < right);
        ML_Assert(bottom < top);

        float rWidth = 1.0f / (right - left);
        float rHeight = 1.0f / (top - bottom);
        float rDepth = 1.0f / (zNear - zFar);

        a00 = 2.0f * zNear * rWidth;
        a01 = 0.0f;
        a02 = (right + left) * rWidth;
        a03 = 0.0f;

        a10 = 0.0f;
        a11 = 2.0f * zNear * rHeight;
        a12 = (top + bottom) * rHeight;
        a13 = 0.0f;

        a20 = 0.0f;
        a21 = 0.0f;
        a22 = (zFar + zNear) * rDepth;
        a23 = 2.0f * zFar * zNear * rDepth;

        a30 = 0.0f;
        a31 = 0.0f;
        a32 = -1.0f;
        a33 = 0.0f;

        bool bReverseZ = (uiProjFlags & PROJ_REVERSED_Z) != 0;

        a22 = ML_ModifyProjZ(bReverseZ, a22, a32);
        a23 = ML_ModifyProjZ(bReverseZ, a23, a33);

        if (uiProjFlags & PROJ_LEFT_HANDED)
            col2 = v4f_negate(col2);
    }

    ML_INLINE void SetupByFrustumInf(float left, float right, float bottom, float top, float zNear, uint32_t uiProjFlags = 0) {
        ML_Assert(left < right);
        ML_Assert(bottom < top);

        float rWidth = 1.0f / (right - left);
        float rHeight = 1.0f / (top - bottom);

        a00 = 2.0f * zNear * rWidth;
        a01 = 0.0f;
        a02 = (right + left) * rWidth;
        a03 = 0.0f;

        a10 = 0.0f;
        a11 = 2.0f * zNear * rHeight;
        a12 = (top + bottom) * rHeight;
        a13 = 0.0f;

        a20 = 0.0f;
        a21 = 0.0f;
        a22 = -1.0f;
        a23 = -2.0f * zNear;

        a30 = 0.0f;
        a31 = 0.0f;
        a32 = -1.0f;
        a33 = 0.0f;

        bool bReverseZ = (uiProjFlags & PROJ_REVERSED_Z) != 0;

        a22 = ML_ModifyProjZ(bReverseZ, a22, a32);
        a23 = ML_ModifyProjZ(bReverseZ, a23, a33);

        if (uiProjFlags & PROJ_LEFT_HANDED)
            col2 = v4f_negate(col2);
    }

    ML_INLINE void SetupByHalfFovy(float halfFovy, float aspect, float zNear, float zFar, uint32_t uiProjFlags = 0) {
        float ymax = zNear * tan(halfFovy);
        float xmax = ymax * aspect;

        SetupByFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar, uiProjFlags);
    }

    ML_INLINE void SetupByHalfFovyInf(float halfFovy, float aspect, float zNear, uint32_t uiProjFlags = 0) {
        float ymax = zNear * tan(halfFovy);
        float xmax = ymax * aspect;

        SetupByFrustumInf(-xmax, xmax, -ymax, ymax, zNear, uiProjFlags);
    }

    ML_INLINE void SetupByHalfFovx(float halfFovx, float aspect, float zNear, float zFar, uint32_t uiProjFlags = 0) {
        float xmax = zNear * tan(halfFovx);
        float ymax = xmax / aspect;

        SetupByFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar, uiProjFlags);
    }

    ML_INLINE void SetupByHalfFovxInf(float halfFovx, float aspect, float zNear, uint32_t uiProjFlags = 0) {
        float xmax = zNear * tan(halfFovx);
        float ymax = xmax / aspect;

        SetupByFrustumInf(-xmax, xmax, -ymax, ymax, zNear, uiProjFlags);
    }

    ML_INLINE void SetupByAngles(float angleMinx, float angleMaxx, float angleMiny, float angleMaxy, float zNear, float zFar, uint32_t uiProjFlags = 0) {
        float xmin = tan(angleMinx) * zNear;
        float xmax = tan(angleMaxx) * zNear;
        float ymin = tan(angleMiny) * zNear;
        float ymax = tan(angleMaxy) * zNear;

        SetupByFrustum(xmin, xmax, ymin, ymax, zNear, zFar, uiProjFlags);
    }

    ML_INLINE void SetupByAnglesInf(float angleMinx, float angleMaxx, float angleMiny, float angleMaxy, float zNear, uint32_t uiProjFlags = 0) {
        float xmin = tan(angleMinx) * zNear;
        float xmax = tan(angleMaxx) * zNear;
        float ymin = tan(angleMiny) * zNear;
        float ymax = tan(angleMaxy) * zNear;

        SetupByFrustumInf(xmin, xmax, ymin, ymax, zNear, uiProjFlags);
    }

    ML_INLINE void SubsampleProjection(float dx, float dy, uint32_t viewportWidth, uint32_t viewportHeight) {
        // NOTE: dx/dy in range [-1; 1]

        a02 += dx / float(viewportWidth);
        a12 += dy / float(viewportHeight);
    }

    ML_INLINE bool IsProjectionValid() const {
        // Do not check a20 and a21 to allow off-centered projections
        // Do not check a22 to allow reverse infinite projections

        return ((a00 != 0.0f && a10 == 0.0f && a20 == 0.0f && a30 == 0.0f) && (a01 == 0.0f && a11 != 0.0f && a21 == 0.0f && a31 == 0.0f) && (a32 == 1.0f || a32 == -1.0f) && (a03 == 0.0f && a13 == 0.0f && a23 != 0.0f && a33 == 0.0f));
    }
};

ML_INLINE float4 mul(const float4x4& m, const float4& v) {
    return m * v;
}

ML_INLINE float4x4 transpose(const float4x4& m) {
    float4x4 res;
    m.TransposeTo(res);

    return res;
}

// non-HLSL

ML_INLINE float3 Rotate(const float4x4& m, const float3& v) {
    v4f r = _mm_mul_ps(v4f_swizzle(v.xmm, 0, 0, 0, 0), m.col0);
    r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 1, 1, 1, 1), m.col1, r);
    r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 2, 2, 2, 2), m.col2, r);
    r = v4f_setw0(r);

    return r;
}

ML_INLINE float3 RotateAbs(const float4x4& m, const float3& v) {
    v4f col0_abs = v4f_abs(m.col0);
    v4f col1_abs = v4f_abs(m.col1);
    v4f col2_abs = v4f_abs(m.col2);

    v4f r = _mm_mul_ps(v4f_swizzle(v.xmm, 0, 0, 0, 0), col0_abs);
    r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 1, 1, 1, 1), col1_abs, r);
    r = _mm_fmadd_ps(v4f_swizzle(v.xmm, 2, 2, 2, 2), col2_abs, r);

    return r;
}

ML_INLINE float3 Project(const float3& v, const float4x4& m) {
    float4 clip = (m * v).xmm;
    clip /= clip.w;

    return clip.xyz;
}

ML_INLINE void float4x4::PreTranslation(const float3& p) {
    v4f r = Rotate(*this, p.xmm).xmm;
    col3 = _mm_add_ps(col3, r);
}

ML_INLINE void float4x4::InvertOrtho() {
    Transpose3x4();

    col3 = Rotate(*this, float3(col3)).xmm;
    col3 = v4f_negate(col3);

    col0 = v4f_setw0(col0);
    col1 = v4f_setw0(col1);
    col2 = v4f_setw0(col2);
    col3 = v4f_setw1(col3);
}

//======================================================================================================================
// cBoxf
//======================================================================================================================

struct cBoxf {
    float3 vMin;
    float3 vMax;

public:
    ML_INLINE cBoxf() {
        Clear();
    }

    ML_INLINE cBoxf(const float3& v) {
        vMin = v;
        vMax = v;
    }

    ML_INLINE cBoxf(const float3& minv, const float3& maxv) {
        vMin = minv;
        vMax = maxv;
    }

    ML_INLINE void Clear() {
        vMin = float3(c_v4f_Inf);
        vMax = float3(c_v4f_InfMinus);
    }

    ML_INLINE bool IsValid() const {
        v4f r = _mm_cmplt_ps(vMin.xmm, vMax.xmm);

        return v4f_test3_all(r);
    }

    ML_INLINE float3 GetCenter() const {
        return (vMin + vMax) * 0.5f;
    }

    ML_INLINE float GetRadius() const {
        return length(vMax - vMin) * 0.5f;
    }

    ML_INLINE void Scale(float fScale) {
        fScale *= 0.5f;

        float k1 = 0.5f + fScale;
        float k2 = 0.5f - fScale;

        float3 a = vMin * k1 + vMax * k2;
        float3 b = vMax * k1 + vMin * k2;

        vMin = a;
        vMax = b;
    }

    ML_INLINE void Enlarge(const float3& vBorder) {
        vMin -= vBorder;
        vMax += vBorder;
    }

    ML_INLINE void Add(const float3& v) {
        vMin = _mm_min_ps(vMin.xmm, v.xmm);
        vMax = _mm_max_ps(vMax.xmm, v.xmm);
    }

    ML_INLINE void Add(const cBoxf& b) {
        vMin = _mm_min_ps(vMin.xmm, b.vMin.xmm);
        vMax = _mm_max_ps(vMax.xmm, b.vMax.xmm);
    }

    ML_INLINE float DistanceSquared(const float3& from) const {
        v4f p = v4f_clamp(from.xmm, vMin.xmm, vMax.xmm);
        p = _mm_sub_ps(p, from.xmm);
        p = v4f_dot33(p, p);

        return _mm_cvtss_f32(p);
    }

    ML_INLINE float Distance(const float3& from) const {
        v4f p = v4f_clamp(from.xmm, vMin.xmm, vMax.xmm);
        p = _mm_sub_ps(p, from.xmm);
        p = v4f_length(p);

        return _mm_cvtss_f32(p);
    }

    ML_INLINE bool IsIntersectWith(const cBoxf& b) const {
        v4f r = _mm_cmplt_ps(vMax.xmm, b.vMin.xmm);
        r = _mm_or_ps(r, _mm_cmpgt_ps(vMin.xmm, b.vMax.xmm));

        return v4f_test3_none(r);
    }

    // NOTE: intersection state 'b' vs 'this'

    ML_INLINE eClip GetIntersectionState(const cBoxf& b) const {
        if (!IsIntersectWith(b))
            return CLIP_OUT;

        v4f r = _mm_cmplt_ps(vMin.xmm, b.vMin.xmm);
        r = _mm_and_ps(r, _mm_cmpgt_ps(vMax.xmm, b.vMax.xmm));

        return v4f_test3_all(r) ? CLIP_IN : CLIP_PARTIAL;
    }

    ML_INLINE bool IsContain(const float3& p) const {
        v4f r = _mm_cmplt_ps(p.xmm, vMin.xmm);
        r = _mm_or_ps(r, _mm_cmpgt_ps(p.xmm, vMax.xmm));

        return v4f_test3_none(r);
    }

    ML_INLINE bool IsContainSphere(const float3& center, float radius) const {
        v4f r = _mm_set1_ps(radius);
        v4f t = _mm_sub_ps(vMin.xmm, r);
        t = _mm_cmplt_ps(center.xmm, t);

        if (v4f_test3_any(t))
            return false;

        t = _mm_add_ps(vMax.xmm, r);
        t = _mm_cmpgt_ps(center.xmm, t);

        if (v4f_test3_any(t))
            return false;

        return true;
    }

    ML_INLINE uint32_t GetIntersectionBits(const cBoxf& b) const {
        v4f r = _mm_cmpge_ps(b.vMin.xmm, vMin.xmm);
        uint32_t bits = (_mm_movemask_ps(r) & ML_Mask(1, 1, 1, 0));

        r = _mm_cmple_ps(b.vMax.xmm, vMax.xmm);
        bits |= (_mm_movemask_ps(r) & ML_Mask(1, 1, 1, 0)) << 3;

        return bits;
    }

    ML_INLINE uint32_t IsContain(const float3& p, uint32_t bits) const {
        v4f r = _mm_cmpge_ps(p.xmm, vMin.xmm);
        bits |= (_mm_movemask_ps(r) & ML_Mask(1, 1, 1, 0));

        r = _mm_cmple_ps(p.xmm, vMax.xmm);
        bits |= (_mm_movemask_ps(r) & ML_Mask(1, 1, 1, 0)) << 3;

        return bits;
    }

    ML_INLINE bool IsIntersectWith(const float3& vRayPos, const float3& vRayDir, float* out_fTmin, float* out_fTmax) const {
        // NOTE: http://tavianator.com/2011/05/fast-branchless-raybounding-box-intersections/

        // IMPORTANT: store '1 / ray_dir' and filter INFs out!

        v4f t1 = _mm_div_ps(_mm_sub_ps(vMin.xmm, vRayPos.xmm), vRayDir.xmm);
        v4f t2 = _mm_div_ps(_mm_sub_ps(vMax.xmm, vRayPos.xmm), vRayDir.xmm);

        v4f vmin = _mm_min_ps(t1, t2);
        v4f vmax = _mm_max_ps(t1, t2);

        // NOTE: hmax.xxx
        v4f tmin = _mm_max_ps(vmin, v4f_swizzle(vmin, _Y, _Z, _X, 0));
        tmin = _mm_max_ps(tmin, v4f_swizzle(vmin, _Z, _X, _Y, 0));

        // NOTE: hmin.xxx
        v4f tmax = _mm_min_ps(vmax, v4f_swizzle(vmax, _Y, _Z, _X, 0));
        tmax = _mm_min_ps(tmax, v4f_swizzle(vmax, _Z, _X, _Y, 0));

        v4f_store_x(out_fTmin, tmin);
        v4f_store_x(out_fTmax, tmax);

        v4f cmp = _mm_cmpge_ps(tmax, tmin);

        return (_mm_movemask_ps(cmp) & ML_Mask(1, 0, 0, 0)) == ML_Mask(1, 0, 0, 0);
    }
};

ML_INLINE void TransformAabb(const float4x4& mTransform, const cBoxf& src, cBoxf& dst) {
    float3 center = (src.vMin + src.vMax) * 0.5f;
    float3 extends = src.vMax - center;

    center = mTransform * center;
    extends = RotateAbs(mTransform, extends);

    dst.vMin = center - extends;
    dst.vMax = center + extends;
}
