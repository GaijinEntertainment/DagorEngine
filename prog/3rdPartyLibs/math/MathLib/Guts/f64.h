// © 2021 NVIDIA Corporation

#pragma once

//======================================================================================================================
// double2
//======================================================================================================================

union double2 {
    v2d xmm;

    struct {
        double a[COORD_2D];
    };

    struct {
        double x, y;
    };

    ML_SWIZZLE_2(double2, double);

public:
    ML_INLINE double2()
        : x(0.0), y(0.0) {
    }

    ML_INLINE double2(double c)
        : x(c), y(c) {
    }

    ML_INLINE double2(double _x, double _y)
        : x(_x), y(_y) {
    }

    ML_INLINE double2(const v2d& v)
        : xmm(v) {
    }

    ML_INLINE double2(const double2& v) = default;

    // Set

    ML_INLINE void operator=(const double2& v) {
        xmm = v.xmm;
    }

    // Conversion

    ML_INLINE operator int2() const;
    ML_INLINE operator uint2() const;
    ML_INLINE operator float2() const;

    // Compare

    ML_COMPARE(bool2, double2, <, _mm_cmplt_pd, _mm_movemask_pd, xmm)
    ML_COMPARE(bool2, double2, <=, _mm_cmple_pd, _mm_movemask_pd, xmm)
    ML_COMPARE(bool2, double2, ==, _mm_cmpeq_pd, _mm_movemask_pd, xmm)
    ML_COMPARE(bool2, double2, >, _mm_cmpgt_pd, _mm_movemask_pd, xmm)
    ML_COMPARE(bool2, double2, >=, _mm_cmpge_pd, _mm_movemask_pd, xmm)
    ML_COMPARE(bool2, double2, !=, _mm_cmpneq_pd, _mm_movemask_pd, xmm)

    // Ops

    ML_INLINE double2 operator-() const {
        return double2(-x, -y);
    }

    ML_OP(double2, double, -, -=, _mm_sub_pd, _mm_set1_pd, xmm)
    ML_OP(double2, double, +, +=, _mm_add_pd, _mm_set1_pd, xmm)
    ML_OP(double2, double, *, *=, _mm_mul_pd, _mm_set1_pd, xmm)
    ML_OP(double2, double, /, /=, _mm_div_pd, _mm_set1_pd, xmm)
};

ML_INLINE double2 degrees(const double2& x) {
    return x * (180.0 / acos(-1.0));
}

ML_INLINE double2 radians(const double2& x) {
    return x * (acos(-1.0) / 180.0);
}

ML_INLINE double2 sign(const double2& x) {
    return double2(sign(x.x), sign(x.y));
}

ML_INLINE double2 abs(const double2& x) {
    return double2(abs(x.x), abs(x.y));
}

ML_INLINE double2 floor(const double2& x) {
    return double2(floor(x.x), floor(x.y));
}

ML_INLINE double2 round(const double2& x) {
    return double2(round(x.x), round(x.y));
}

ML_INLINE double2 ceil(const double2& x) {
    return double2(ceil(x.x), ceil(x.y));
}

ML_INLINE double2 frac(const double2& x) {
    return double2(frac(x.x), frac(x.y));
}

ML_INLINE double2 fmod(const double2& x, const double2& y) {
    return double2(fmod(x.x, y.x), fmod(x.y, y.y));
}

ML_INLINE double2 min(const double2& x, const double2& y) {
    return double2(min(x.x, y.x), min(x.y, y.y));
}

ML_INLINE double2 max(const double2& x, const double2& y) {
    return double2(max(x.x, y.x), max(x.y, y.y));
}

ML_INLINE double2 clamp(const double2& x, const double2& a, const double2& b) {
    return double2(clamp(x.x, a.x, b.x), clamp(x.y, a.y, b.y));
}

ML_INLINE double2 saturate(const double2& x) {
    return double2(clamp(x.x, 0.0, 1.0), clamp(x.y, 0.0, 1.0));
}

ML_INLINE double2 lerp(const double2& a, const double2& b, const double2& x) {
    return a + (b - a) * x;
}

ML_INLINE double2 linearstep(const double2& a, const double2& b, const double2& x) {
    return saturate((x - a) / (b - a));
}

ML_INLINE double2 smoothstep(const double2& a, const double2& b, const double2& x) {
    double2 t = linearstep(a, b, x);

    return t * t * (3.0 - 2.0 * t);
}

ML_INLINE double2 step(const double2& edge, const double2& x) {
    return double2(step(edge.x, x.x), step(edge.y, x.y));
}

ML_INLINE double2 sin(const double2& x) {
    return double2(sin(x.x), sin(x.y));
}

ML_INLINE double2 cos(const double2& x) {
    return double2(cos(x.x), cos(x.y));
}

ML_INLINE double2 tan(const double2& x) {
    return double2(tan(x.x), tan(x.y));
}

ML_INLINE double2 asin(const double2& x) {
    return double2(asin(x.x), asin(x.y));
}

ML_INLINE double2 acos(const double2& x) {
    return double2(acos(x.x), acos(x.y));
}

ML_INLINE double2 atan(const double2& x) {
    return double2(atan(x.x), atan(x.y));
}

ML_INLINE double2 atan2(const double2& y, const double2& x) {
    return double2(atan2(y.x, x.x), atan2(y.y, x.y));
}

ML_INLINE double2 sqrt(const double2& x) {
    return double2(sqrt(x.x), sqrt(x.y));
}

ML_INLINE double2 rsqrt(const double2& x) {
    return double2(rsqrt(x.x), rsqrt(x.y));
}

ML_INLINE double2 rcp(const double2& x) {
    return double2(rcp(x.x), rcp(x.y));
}

ML_INLINE double2 pow(const double2& x, const double2& y) {
    return double2(pow(x.x, y.x), pow(x.y, y.y));
}

ML_INLINE double2 log(const double2& x) {
    return double2(log(x.x), log(x.y));
}

ML_INLINE double2 log2(const double2& x) {
    return double2(log2(x.x), log2(x.y));
}

ML_INLINE double2 exp(const double2& x) {
    return double2(exp(x.x), exp(x.y));
}

ML_INLINE double2 exp2(const double2& x) {
    return double2(exp2(x.x), exp2(x.y));
}

ML_INLINE double2 madd(const double2& a, const double2& b, const double2& c) {
    return a * b + c;
}

ML_INLINE double dot(const double2& a, const double2& b) {
    return a.x * b.x + a.y * b.y;
}

ML_INLINE double length(const double2& x) {
    return sqrt(dot(x, x));
}

ML_INLINE double2 normalize(const double2& x) {
    return x / length(x);
}

// non-HLSL

ML_INLINE double2 Pi(const double2& mul) {
    return mul * acos(-1.0);
}

ML_INLINE double2 GetPerpendicularVector(const double2& a) {
    return double2(-a.y, a.x);
}

ML_INLINE double2 Snap(const double2& x, const double2& step) {
    return round(x / step) * step;
}

ML_INLINE double2 Rotate(const double2& v, double angle) {
    double sa = sin(angle);
    double ca = cos(angle);

    double2 p;
    p.x = ca * v.x + sa * v.y;
    p.y = ca * v.y - sa * v.x;

    return p;
}

//======================================================================================================================
// double3
//======================================================================================================================

union double3 {
    v4d ymm;

    struct {
        double a[COORD_3D];
    };

    struct {
        double x, y, z;
    };

    ML_SWIZZLE_3(v4d_swizzle2, double2, v4d_swizzle3, double3);

public:
    ML_INLINE double3()
        : ymm(_mm256_setzero_pd()) {
    }

    ML_INLINE double3(double c)
        : ymm(_mm256_set1_pd(c)) {
    }

    ML_INLINE double3(double _x, double _y, double _z)
        : ymm(v4d_set(_x, _y, _z, 0.0)) {
    }

    ML_INLINE double3(const double2& v, double _z)
        : ymm(v4d_set(v.x, v.y, _z, 0.0)) {
    }

    ML_INLINE double3(double _x, const double2& v)
        : ymm(v4d_set(_x, v.x, v.y, 0.0)) {
    }

    ML_INLINE double3(const v4d& v)
        : ymm(v) {
    }

    ML_INLINE double3(const double* v3)
        : ymm(v4d_set(v3[0], v3[1], v3[2], 0.0)) {
    }

    ML_INLINE double3(const double3& v) = default;

    // Set

    ML_INLINE void operator=(const double3& v) {
        ymm = v.ymm;
    }

    // Conversion

    ML_INLINE operator int3() const;
    ML_INLINE operator uint3() const;
    ML_INLINE operator float3() const;

    // Compare

    ML_COMPARE(bool3, double3, <, _mm256_cmplt_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool3, double3, <=, _mm256_cmple_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool3, double3, ==, _mm256_cmpeq_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool3, double3, >, _mm256_cmpgt_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool3, double3, >=, _mm256_cmpge_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool3, double3, !=, _mm256_cmpneq_pd, _mm256_movemask_pd, ymm)

    // Ops

    ML_INLINE double3 operator-() const {
        return v4d_negate(ymm);
    }

    ML_OP(double3, double, -, -=, _mm256_sub_pd, _mm256_set1_pd, ymm)
    ML_OP(double3, double, +, +=, _mm256_add_pd, _mm256_set1_pd, ymm)
    ML_OP(double3, double, *, *=, _mm256_mul_pd, _mm256_set1_pd, ymm)
    ML_OP(double3, double, /, /=, _mm256_div_pd, _mm256_set1_pd, ymm)

    // Misc

    ML_INLINE operator v4d() const {
        return ymm;
    }

    static ML_INLINE double3 Zero() {
        return _mm256_setzero_pd();
    }
};

ML_INLINE double3 degrees(const double3& x) {
    return x * (180.0 / acos(-1.0));
}

ML_INLINE double3 radians(const double3& x) {
    return x * (acos(-1.0) / 180.0);
}

ML_INLINE double3 sign(const double3& x) {
    return v4d_sign(x.ymm);
}

ML_INLINE double3 abs(const double3& x) {
    return v4d_abs(x.ymm);
}

ML_INLINE double3 floor(const double3& x) {
    return v4d_floor(x.ymm);
}

ML_INLINE double3 round(const double3& x) {
    return v4d_round(x.ymm);
}

ML_INLINE double3 ceil(const double3& x) {
    return v4d_ceil(x.ymm);
}

ML_INLINE double3 frac(const double3& x) {
    return v4d_frac(x.ymm);
}

ML_INLINE double3 fmod(const double3& x, const double3& y) {
    return v4d_mod(x.ymm, y.ymm);
}

ML_INLINE double3 min(const double3& x, const double3& y) {
    return _mm256_min_pd(x.ymm, y.ymm);
}

ML_INLINE double3 max(const double3& x, const double3& y) {
    return _mm256_max_pd(x.ymm, y.ymm);
}

ML_INLINE double3 clamp(const double3& x, const double3& a, const double3& b) {
    return v4d_clamp(x.ymm, a.ymm, b.ymm);
}

ML_INLINE double3 saturate(const double3& x) {
    return v4d_saturate(x.ymm);
}

ML_INLINE double3 lerp(const double3& a, const double3& b, const double3& x) {
    return v4d_mix(a.ymm, b.ymm, x.ymm);
}

ML_INLINE double3 linearstep(const double3& a, const double3& b, const double3& x) {
    return v4d_linearstep(a.ymm, b.ymm, x.ymm);
}

ML_INLINE double3 smoothstep(const double3& a, const double3& b, const double3& x) {
    return v4d_smoothstep(a.ymm, b.ymm, x.ymm);
}

ML_INLINE double3 step(const double3& edge, const double3& x) {
    return v4d_step(edge.ymm, x.ymm);
}

ML_INLINE double3 sin(const double3& x) {
    return _mm256_sin_pd(x.ymm);
}

ML_INLINE double3 cos(const double3& x) {
    return _mm256_cos_pd(x.ymm);
}

ML_INLINE double3 tan(const double3& x) {
    return _mm256_tan_pd(x.ymm);
}

ML_INLINE double3 asin(const double3& x) {
    ML_Assert(all(x >= double3(-1.0)) && all(x <= double3(1.0)));

    return _mm256_asin_pd(x.ymm);
}

ML_INLINE double3 acos(const double3& x) {
    ML_Assert(all(x >= double3(-1.0)) && all(x <= double3(1.0)));

    return _mm256_acos_pd(x.ymm);
}

ML_INLINE double3 atan(const double3& x) {
    return _mm256_atan_pd(x.ymm);
}

ML_INLINE double3 atan2(const double3& y, const double3& x) {
    return _mm256_atan2_pd(y.ymm, x.ymm);
}

ML_INLINE double3 sqrt(const double3& x) {
    return _mm256_sqrt_pd(x.ymm);
}

ML_INLINE double3 rsqrt(const double3& x) {
    return v4d_rsqrt(x.ymm);
}

ML_INLINE double3 rcp(const double3& x) {
    return v4d_rcp(v4d_setw1(x.ymm));
}

ML_INLINE double3 pow(const double3& x, const double3& y) {
    return _mm256_pow_pd(x.ymm, y.ymm);
}

ML_INLINE double3 log(const double3& x) {
    return _mm256_log_pd(x.ymm);
}

ML_INLINE double3 log2(const double3& x) {
    return _mm256_log2_pd(x.ymm);
}

ML_INLINE double3 exp(const double3& x) {
    return _mm256_exp_pd(x.ymm);
}

ML_INLINE double3 exp2(const double3& x) {
    return _mm256_exp2_pd(x.ymm);
}

ML_INLINE double3 madd(const double3& a, const double3& b, const double3& c) {
    return _mm256_fmadd_pd(a.ymm, b.ymm, c.ymm);
}

ML_INLINE double dot(const double3& a, const double3& b) {
    v4d r = v4d_dot33(a.ymm, b.ymm);

    return _mm256_cvtsd_f64(r);
}

ML_INLINE double length(const double3& x) {
    v4d r = v4d_length(x.ymm);

    return _mm256_cvtsd_f64(r);
}

ML_INLINE double3 normalize(const double3& x) {
    return v4d_normalize(x.ymm);
}

ML_INLINE double3 cross(const double3& x, const double3& y) {
    return v4d_cross(x.ymm, y.ymm);
}

ML_INLINE double3 reflect(const double3& v, const double3& n) {
    // NOTE: slow
    // return v - n * dot(n, v) * 2;

    v4d dot0 = v4d_dot33(n.ymm, v.ymm);
    dot0 = _mm256_mul_pd(dot0, _mm256_set1_pd(2.0));

    return _mm256_fnmadd_pd(n.ymm, dot0, v.ymm);
}

ML_INLINE double3 refract(const double3& v, const double3& n, double eta) {
    // NOTE: slow
    /*
    double dot = dot(v, n);
    double k = 1 - eta * eta * (1 - dot * dot);

    if( k < 0 )
    return 0

    return v * eta - n * (eta * dot + Sqrt(k));
    */

    v4d eta0 = _mm256_set1_pd(eta);
    v4d dot0 = v4d_dot33(n.ymm, v.ymm);
    v4d mul0 = _mm256_mul_pd(eta0, eta0);
    v4d sub0 = _mm256_fnmadd_pd(dot0, dot0, c_v4d_1111);
    v4d sub1 = _mm256_fnmadd_pd(mul0, sub0, c_v4d_1111);

    if (v4d_isnegative4_all(sub1))
        return _mm256_setzero_pd();

    v4d mul5 = _mm256_mul_pd(eta0, v.ymm);
    v4d mul3 = _mm256_mul_pd(eta0, dot0);
    v4d sqt0 = _mm256_sqrt_pd(sub1);
    v4d add0 = _mm256_add_pd(mul3, sqt0);

    return _mm256_fnmadd_pd(add0, n.ymm, mul5);
}

// non-HLSL

ML_INLINE double3 Pi(const double3& mul) {
    return mul * acos(-1.0);
}

ML_INLINE double3 GetPerpendicularVector(const double3& N) {
    double3 T = double3(N.z, -N.x, N.y);
    T -= N * dot(T, N);

    return normalize(T);
}

ML_INLINE double3 SinCos(const double3& x, double3* pCos) {
    return _mm256_sincos_pd(&pCos->ymm, x.ymm);
}

ML_INLINE double3 Snap(const double3& x, const double3& step) {
    return round(x / step) * step;
}

ML_INLINE bool IsPointsNear(const double3& p1, const double3& p2, double eps) {
    v4d r = _mm256_sub_pd(p1.ymm, p2.ymm);
    r = v4d_abs(r);
    r = _mm256_cmple_pd(r, _mm256_set1_pd(eps));

    return v4d_test3_all(r);
}

//======================================================================================================================
// double4
//======================================================================================================================

union double4 {
    v4d ymm;

    struct {
        double a[COORD_4D];
    };

    struct {
        double x, y, z, w;
    };

    ML_SWIZZLE_4(v4d_swizzle2, double2, v4d_swizzle3, double3, v4d_swizzle4, double4);

public:
    ML_INLINE double4()
        : ymm(_mm256_setzero_pd()) {
    }

    ML_INLINE double4(double c)
        : ymm(_mm256_set1_pd(c)) {
    }

    ML_INLINE double4(double _x, double _y, double _z, double _w)
        : ymm(v4d_set(_x, _y, _z, _w)) {
    }

    ML_INLINE double4(const float3& v, double _w)
        : ymm(v4d_set(v.x, v.y, v.z, _w)) {
    }

    ML_INLINE double4(const float2& a, const float2& b)
        : ymm(v4d_set(a.x, a.y, b.x, b.y)) {
    }

    ML_INLINE double4(double _x, const float3& v)
        : ymm(v4d_set(_x, v.x, v.y, v.z)) {
    }

    ML_INLINE double4(const v4d& v)
        : ymm(v) {
    }

    ML_INLINE double4(const double* v4)
        : ymm(_mm256_loadu_pd(v4)) {
    }

    ML_INLINE double4(const double4& v) = default;

    // Set

    ML_INLINE void operator=(const double4& v) {
        ymm = v.ymm;
    }

    // Conversion

    ML_INLINE operator int4() const;
    ML_INLINE operator uint4() const;
    ML_INLINE operator float4() const;

    // Compare

    ML_COMPARE(bool4, double4, <, _mm256_cmplt_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool4, double4, <=, _mm256_cmple_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool4, double4, ==, _mm256_cmpeq_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool4, double4, >, _mm256_cmpgt_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool4, double4, >=, _mm256_cmpge_pd, _mm256_movemask_pd, ymm)
    ML_COMPARE(bool4, double4, !=, _mm256_cmpneq_pd, _mm256_movemask_pd, ymm)

    // Ops

    ML_INLINE double4 operator-() const {
        return v4d_negate(ymm);
    }

    ML_OP(double4, double, -, -=, _mm256_sub_pd, _mm256_set1_pd, ymm)
    ML_OP(double4, double, +, +=, _mm256_add_pd, _mm256_set1_pd, ymm)
    ML_OP(double4, double, *, *=, _mm256_mul_pd, _mm256_set1_pd, ymm)
    ML_OP(double4, double, /, /=, _mm256_div_pd, _mm256_set1_pd, ymm)

    // Misc

    ML_INLINE operator v4d() const {
        return ymm;
    }

    static ML_INLINE double4 Zero() {
        return _mm256_setzero_pd();
    }
};

ML_INLINE double4 degrees(const double4& x) {
    return x * (180.0 / acos(-1.0));
}

ML_INLINE double4 radians(const double4& x) {
    return x * (acos(-1.0) / 180.0);
}

ML_INLINE double4 sign(const double4& x) {
    return v4d_sign(x.ymm);
}

ML_INLINE double4 abs(const double4& x) {
    return v4d_abs(x.ymm);
}

ML_INLINE double4 floor(const double4& x) {
    return v4d_floor(x.ymm);
}

ML_INLINE double4 round(const double4& x) {
    return v4d_round(x.ymm);
}

ML_INLINE double4 ceil(const double4& x) {
    return v4d_ceil(x.ymm);
}

ML_INLINE double4 frac(const double4& x) {
    return v4d_frac(x.ymm);
}

ML_INLINE double4 fmod(const double4& x, const double4& y) {
    return v4d_mod(x.ymm, y.ymm);
}

ML_INLINE double4 min(const double4& x, const double4& y) {
    return _mm256_min_pd(x.ymm, y.ymm);
}

ML_INLINE double4 max(const double4& x, const double4& y) {
    return _mm256_max_pd(x.ymm, y.ymm);
}

ML_INLINE double4 clamp(const double4& x, const double4& a, const double4& b) {
    return v4d_clamp(x.ymm, a.ymm, b.ymm);
}

ML_INLINE double4 saturate(const double4& x) {
    return v4d_saturate(x.ymm);
}

ML_INLINE double4 lerp(const double4& a, const double4& b, const double4& x) {
    return v4d_mix(a.ymm, b.ymm, x.ymm);
}

ML_INLINE double4 linearstep(const double4& a, const double4& b, const double4& x) {
    return v4d_linearstep(a.ymm, b.ymm, x.ymm);
}

ML_INLINE double4 smoothstep(const double4& a, const double4& b, const double4& x) {
    return v4d_smoothstep(a.ymm, b.ymm, x.ymm);
}

ML_INLINE double4 step(const double4& edge, const double4& x) {
    return v4d_step(edge.ymm, x.ymm);
}

ML_INLINE double4 sin(const double4& x) {
    return _mm256_sin_pd(x.ymm);
}

ML_INLINE double4 cos(const double4& x) {
    return _mm256_cos_pd(x.ymm);
}

ML_INLINE double4 tan(const double4& x) {
    return _mm256_tan_pd(x.ymm);
}

ML_INLINE double4 asin(const double4& x) {
    ML_Assert(all(x >= double4(-1.0)) && all(x <= double4(1.0)));

    return _mm256_asin_pd(x.ymm);
}

ML_INLINE double4 acos(const double4& x) {
    ML_Assert(all(x >= double4(-1.0)) && all(x <= double4(1.0)));

    return _mm256_acos_pd(x.ymm);
}

ML_INLINE double4 atan(const double4& x) {
    return _mm256_atan_pd(x.ymm);
}

ML_INLINE double4 atan2(const double4& y, const double4& x) {
    return _mm256_atan2_pd(y.ymm, x.ymm);
}

ML_INLINE double4 sqrt(const double4& x) {
    return _mm256_sqrt_pd(x.ymm);
}

ML_INLINE double4 rsqrt(const double4& x) {
    return v4d_rsqrt(x.ymm);
}

ML_INLINE double4 rcp(const double4& x) {
    return v4d_rcp(x.ymm);
}

ML_INLINE double4 pow(const double4& x, const double4& y) {
    return _mm256_pow_pd(x.ymm, y.ymm);
}

ML_INLINE double4 log(const double4& x) {
    return _mm256_log_pd(x.ymm);
}

ML_INLINE double4 log2(const double4& x) {
    return _mm256_log2_pd(x.ymm);
}

ML_INLINE double4 exp(const double4& x) {
    return _mm256_exp_pd(x.ymm);
}

ML_INLINE double4 exp2(const double4& x) {
    return _mm256_exp2_pd(x.ymm);
}

ML_INLINE double4 madd(const double4& a, const double4& b, const double4& c) {
    return _mm256_fmadd_pd(a.ymm, b.ymm, c.ymm);
}

ML_INLINE double dot(const double4& a, const double4& b) {
    v4d r = v4d_dot44(a.ymm, b.ymm);

    return _mm256_cvtsd_f64(r);
}

// non-HLSL

ML_INLINE double4 Pi(const double4& mul) {
    return mul * acos(-1.0);
}

ML_INLINE double Dot43(const double4& a, const double3& b) {
    v4d r = v4d_dot43(a.ymm, b.ymm);

    return _mm256_cvtsd_f64(r);
}

ML_INLINE double4 Snap(const double4& x, const double4& step) {
    return round(x / step) * step;
}

ML_INLINE double4 SinCos(const double4& x, double4* pCos) {
    return _mm256_sincos_pd(&pCos->ymm, x.ymm);
}

// TODO: add "Quaternion"
ML_INLINE double4 Slerp(const double4& a, const double4& b, double x) {
    ML_Assert(x >= 0.0 && x <= 1.0);
    ML_Assert(abs(dot(a, a) - 1.0) < 1e-5);
    ML_Assert(abs(dot(b, b) - 1.0) < 1e-5);

    double4 r;

    double theta = dot(a, b);
    if (theta > 0.9995)
        r = lerp(a, b, x);
    else {
        theta = acos(theta);

        double3 s = sin(theta * double3(1.0, 1.0 - x, x));
        double sn = 1.0 / s.x;
        double wa = s.y * sn;
        double wb = s.z * sn;

        r = a * wa + b * wb;
    }

    r *= rsqrt(dot(r, r));

    return r;
}

//======================================================================================================================
// double4x4
//======================================================================================================================

// IMPORTANT: store - "column-major", math - "row-major" (vector is column)
union double4x4 {
    // Column array
    struct {
        double4 ca[4];

        /*
        TODO: at least older GCC version don't allow this:

        float4 c0;
        float4 c1;
        float4 c2;
        float4 c3;

        because of this errors:
         - member with constructor not allowed in anonymous aggregate
         - member with copy assignment operator not allowed in anonymous aggregate
        */
    };

    // Element array
    struct {
        double a[16];
    };

    // Elements aXY, where X - row, Y - column
    struct {
        double a00, a10, a20, a30;
        double a01, a11, a21, a31;
        double a02, a12, a22, a32;
        double a03, a13, a23, a33;
    };

public:
    ML_INLINE double4x4() {
        ca[0] = _mm256_setzero_pd();
        ca[1] = _mm256_setzero_pd();
        ca[2] = _mm256_setzero_pd();
        ca[3] = _mm256_setzero_pd();
    }

    ML_INLINE double4x4(double m00, double m01, double m02, double m03, double m10, double m11, double m12, double m13, double m20, double m21, double m22, double m23, double m30,
        double m31, double m32, double m33) {
        ca[0] = v4d_set(m00, m10, m20, m30);
        ca[1] = v4d_set(m01, m11, m21, m31);
        ca[2] = v4d_set(m02, m12, m22, m32);
        ca[3] = v4d_set(m03, m13, m23, m33);
    }

    ML_INLINE double4x4(const double4& c0, const double4& c1, const double4& c2, const double4& c3) {
        ca[0] = c0.ymm;
        ca[1] = c1.ymm;
        ca[2] = c2.ymm;
        ca[3] = c3.ymm;
    }

    ML_INLINE double4x4(const double4x4& m) = default;

    // Set

    ML_INLINE void operator=(const double4x4& m) {
        ca[0] = m.ca[0];
        ca[1] = m.ca[1];
        ca[2] = m.ca[2];
        ca[3] = m.ca[3];
    }

    // Conversion

    ML_INLINE operator float4x4() const;

    // Compare

    ML_INLINE bool operator==(const double4x4& m) const {
        return all(ca[0] == m.ca[0]) && all(ca[1] == m.ca[1]) && all(ca[2] == m.ca[2]) && all(ca[3] == m.ca[3]);
    }

    ML_INLINE bool operator!=(const double4x4& m) const {
        return any(ca[0] != m.ca[0]) || any(ca[1] != m.ca[1]) || any(ca[2] != m.ca[2]) || any(ca[3] != m.ca[3]);
    }

    // NOTE: *

    ML_INLINE double4x4 operator*(const double4x4& m) const {
        double4x4 r;

        v4d r1 = _mm256_mul_pd(v4d_swizzle(m.ca[0], 0, 0, 0, 0), ca[0]);
        v4d r2 = _mm256_mul_pd(v4d_swizzle(m.ca[1], 0, 0, 0, 0), ca[0]);

        r1 = _mm256_fmadd_pd(v4d_swizzle(m.ca[0], 1, 1, 1, 1), ca[1], r1);
        r2 = _mm256_fmadd_pd(v4d_swizzle(m.ca[1], 1, 1, 1, 1), ca[1], r2);
        r1 = _mm256_fmadd_pd(v4d_swizzle(m.ca[0], 2, 2, 2, 2), ca[2], r1);
        r2 = _mm256_fmadd_pd(v4d_swizzle(m.ca[1], 2, 2, 2, 2), ca[2], r2);
        r1 = _mm256_fmadd_pd(v4d_swizzle(m.ca[0], 3, 3, 3, 3), ca[3], r1);
        r2 = _mm256_fmadd_pd(v4d_swizzle(m.ca[1], 3, 3, 3, 3), ca[3], r2);

        r.ca[0] = r1;
        r.ca[1] = r2;

        r1 = _mm256_mul_pd(v4d_swizzle(m.ca[2], 0, 0, 0, 0), ca[0]);
        r2 = _mm256_mul_pd(v4d_swizzle(m.ca[3], 0, 0, 0, 0), ca[0]);

        r1 = _mm256_fmadd_pd(v4d_swizzle(m.ca[2], 1, 1, 1, 1), ca[1], r1);
        r2 = _mm256_fmadd_pd(v4d_swizzle(m.ca[3], 1, 1, 1, 1), ca[1], r2);
        r1 = _mm256_fmadd_pd(v4d_swizzle(m.ca[2], 2, 2, 2, 2), ca[2], r1);
        r2 = _mm256_fmadd_pd(v4d_swizzle(m.ca[3], 2, 2, 2, 2), ca[2], r2);
        r1 = _mm256_fmadd_pd(v4d_swizzle(m.ca[2], 3, 3, 3, 3), ca[3], r1);
        r2 = _mm256_fmadd_pd(v4d_swizzle(m.ca[3], 3, 3, 3, 3), ca[3], r2);

        r.ca[2] = r1;
        r.ca[3] = r2;

        return r;
    }

    ML_INLINE double4 operator*(const double4& v) const {
        v4d r = _mm256_mul_pd(v4d_swizzle(v.ymm, 0, 0, 0, 0), ca[0]);
        r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 1, 1, 1, 1), ca[1], r);
        r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 2, 2, 2, 2), ca[2], r);
        r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 3, 3, 3, 3), ca[3], r);

        return r;
    }

    ML_INLINE double3 operator*(const double3& v) const {
        v4d r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 0, 0, 0, 0), ca[0], ca[3]);
        r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 1, 1, 1, 1), ca[1], r);
        r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 2, 2, 2, 2), ca[2], r);

        return r;
    }

    // Columns and rows

    double4& Col(uint32_t column) {
        ML_Assert(column < COORD_4D);

        return ca[column];
    }

    const double4& Col(uint32_t column) const {
        ML_Assert(column < COORD_4D);

        return ca[column];
    }

    double4& operator[](uint32_t column) {
        return Col(column);
    }

    const double4& operator[](uint32_t column) const {
        return Col(column);
    }

    ML_INLINE double4 Row(uint32_t row) const {
        ML_Assert(row < COORD_4D);

        return double4(a[row], a[COORD_4D + row], a[COORD_4D * 2 + row], a[COORD_4D * 3 + row]);
    }

    // NOTE: other

    static ML_INLINE double4x4 Identity() {
        return double4x4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    }

    ML_INLINE double GetNdcDepth(double z) const {
        double c = a22 * z + a23;
        double d = a32 * z + a33;

        return c / d;
    }

    ML_INLINE double3 GetRotationYPR() const {
        double3 r;
        r.x = atan2(-a01, a11);
        r.y = asin(a21);
        r.z = atan2(-a20, a22);

        return r;
    }

    ML_INLINE double4 GetQuaternion() const {
        double4 q;
        double t;

        if (a22 < 0.0) {
            if (a00 > a11) {
                t = 1.0 + a00 - a11 - a22;
                q = double4(t, a10 + a01, a02 + a20, a21 - a12);
            } else {
                t = 1.0 - a00 + a11 - a22;
                q = double4(a10 + a01, t, a21 + a12, a02 - a20);
            }
        } else {
            if (a00 < -a11) {
                t = 1.0 - a00 - a11 + a22;
                q = double4(a02 + a20, a21 + a12, t, a10 - a01);
            } else {
                t = 1.0 + a00 + a11 + a22;
                q = double4(a21 - a12, a02 - a20, a10 - a01, t);
            }
        }

        q *= 0.5 / sqrt(t);

        return q;
    }

    ML_INLINE double3 GetScale() const {
        double3 scale = double3(_mm256_cvtsd_f64(v4d_length(ca[0])), _mm256_cvtsd_f64(v4d_length(ca[1])), _mm256_cvtsd_f64(v4d_length(ca[2])));

        return scale;
    }

    ML_INLINE void SetTranslation(const double3& p) {
        ca[3] = v4d_setw1(p.ymm);
    }

    ML_INLINE void AddTranslation(const double3& p) {
        ca[3] = _mm256_add_pd(ca[3], v4d_setw0(p.ymm));
    }

    ML_INLINE void PreTranslation(const double3& p);

    ML_INLINE void AddScale(const double3& scale) {
        ca[0] = _mm256_mul_pd(ca[0], scale.ymm);
        ca[1] = _mm256_mul_pd(ca[1], scale.ymm);
        ca[2] = _mm256_mul_pd(ca[2], scale.ymm);
    }

    ML_INLINE void WorldToView(uint32_t uiProjFlags = 0) {
        /*
        double4x4 rot;
        rot.SetupByRotationX(c_fHalfPi);
        *this = (*this) * rot;
        InvertOrtho();
        */

        Swap(ca[1], ca[2]);

        if ((uiProjFlags & PROJ_LEFT_HANDED) == 0)
            ca[2] = v4d_negate(ca[2]);

        Transpose3x4();
    }

    ML_INLINE void ViewToWorld(uint32_t uiProjFlags = 0) {
        Transpose3x4();

        if ((uiProjFlags & PROJ_LEFT_HANDED) == 0)
            ca[2] = v4d_negate(ca[2]);

        Swap(ca[1], ca[2]);
    }

    ML_INLINE bool IsLeftHanded() const {
        double3 v1 = cross(double3(ca[0]), double3(ca[1]));

        return dot(v1, double3(ca[2])) < 0.0;
    }

    ML_INLINE void TransposeTo(double4x4& m) const {
        v4d ymm0 = v4d_Ax_Bx_Az_Bz(ca[0], ca[1]);
        v4d ymm1 = v4d_Ax_Bx_Az_Bz(ca[2], ca[3]);
        v4d ymm2 = v4d_Ay_By_Aw_Bw(ca[0], ca[1]);
        v4d ymm3 = v4d_Ay_By_Aw_Bw(ca[2], ca[3]);

        m.ca[0] = v4d_Axy_Bxy(ymm0, ymm1);
        m.ca[1] = v4d_Axy_Bxy(ymm2, ymm3);
        m.ca[2] = v4d_Azw_Bzw(ymm1, ymm0);
        m.ca[3] = v4d_Azw_Bzw(ymm3, ymm2);
    }

    ML_INLINE void Transpose() {
        v4d ymm0 = v4d_Ax_Bx_Az_Bz(ca[0], ca[1]);
        v4d ymm1 = v4d_Ax_Bx_Az_Bz(ca[2], ca[3]);
        v4d ymm2 = v4d_Ay_By_Aw_Bw(ca[0], ca[1]);
        v4d ymm3 = v4d_Ay_By_Aw_Bw(ca[2], ca[3]);

        ca[0] = v4d_Axy_Bxy(ymm0, ymm1);
        ca[1] = v4d_Axy_Bxy(ymm2, ymm3);
        ca[2] = v4d_Azw_Bzw(ymm1, ymm0);
        ca[3] = v4d_Azw_Bzw(ymm3, ymm2);
    }

    ML_INLINE void Transpose3x4() {
        v4d ymm0 = v4d_Ax_Bx_Az_Bz(ca[0], ca[1]);
        v4d ymm1 = v4d_Ax_Bx_Az_Bz(ca[2], ca[3]);
        v4d ymm2 = v4d_Ay_By_Aw_Bw(ca[0], ca[1]);
        v4d ymm3 = v4d_Ay_By_Aw_Bw(ca[2], ca[3]);

        ca[0] = v4d_Axy_Bxy(ymm0, ymm1);
        ca[1] = v4d_Axy_Bxy(ymm2, ymm3);
        ca[2] = v4d_Azw_Bzw(ymm1, ymm0);
    }

    ML_INLINE void Invert() {
        // NOTE: http://forum.devmaster.net/t/sse-mat4-inverse/16799

        v4d Fac0;
        {
            v4d Swp0a = v4d_shuffle(ca[3], ca[2], 3, 3, 3, 3);
            v4d Swp0b = v4d_shuffle(ca[3], ca[2], 2, 2, 2, 2);

            v4d Swp00 = v4d_shuffle(ca[2], ca[1], 2, 2, 2, 2);
            v4d Swp01 = v4d_swizzle(Swp0a, 0, 0, 0, 2);
            v4d Swp02 = v4d_swizzle(Swp0b, 0, 0, 0, 2);
            v4d Swp03 = v4d_shuffle(ca[2], ca[1], 3, 3, 3, 3);

            v4d Mul00 = _mm256_mul_pd(Swp00, Swp01);

            Fac0 = _mm256_fnmadd_pd(Swp02, Swp03, Mul00);
        }

        v4d Fac1;
        {
            v4d Swp0a = v4d_shuffle(ca[3], ca[2], 3, 3, 3, 3);
            v4d Swp0b = v4d_shuffle(ca[3], ca[2], 1, 1, 1, 1);

            v4d Swp00 = v4d_shuffle(ca[2], ca[1], 1, 1, 1, 1);
            v4d Swp01 = v4d_swizzle(Swp0a, 0, 0, 0, 2);
            v4d Swp02 = v4d_swizzle(Swp0b, 0, 0, 0, 2);
            v4d Swp03 = v4d_shuffle(ca[2], ca[1], 3, 3, 3, 3);

            v4d Mul00 = _mm256_mul_pd(Swp00, Swp01);

            Fac1 = _mm256_fnmadd_pd(Swp02, Swp03, Mul00);
        }

        v4d Fac2;
        {
            v4d Swp0a = v4d_shuffle(ca[3], ca[2], 2, 2, 2, 2);
            v4d Swp0b = v4d_shuffle(ca[3], ca[2], 1, 1, 1, 1);

            v4d Swp00 = v4d_shuffle(ca[2], ca[1], 1, 1, 1, 1);
            v4d Swp01 = v4d_swizzle(Swp0a, 0, 0, 0, 2);
            v4d Swp02 = v4d_swizzle(Swp0b, 0, 0, 0, 2);
            v4d Swp03 = v4d_shuffle(ca[2], ca[1], 2, 2, 2, 2);

            v4d Mul00 = _mm256_mul_pd(Swp00, Swp01);

            Fac2 = _mm256_fnmadd_pd(Swp02, Swp03, Mul00);
        }

        v4d Fac3;
        {
            v4d Swp0a = v4d_shuffle(ca[3], ca[2], 3, 3, 3, 3);
            v4d Swp0b = v4d_shuffle(ca[3], ca[2], 0, 0, 0, 0);

            v4d Swp00 = v4d_shuffle(ca[2], ca[1], 0, 0, 0, 0);
            v4d Swp01 = v4d_swizzle(Swp0a, 0, 0, 0, 2);
            v4d Swp02 = v4d_swizzle(Swp0b, 0, 0, 0, 2);
            v4d Swp03 = v4d_shuffle(ca[2], ca[1], 3, 3, 3, 3);

            v4d Mul00 = _mm256_mul_pd(Swp00, Swp01);

            Fac3 = _mm256_fnmadd_pd(Swp02, Swp03, Mul00);
        }

        v4d Fac4;
        {
            v4d Swp0a = v4d_shuffle(ca[3], ca[2], 2, 2, 2, 2);
            v4d Swp0b = v4d_shuffle(ca[3], ca[2], 0, 0, 0, 0);

            v4d Swp00 = v4d_shuffle(ca[2], ca[1], 0, 0, 0, 0);
            v4d Swp01 = v4d_swizzle(Swp0a, 0, 0, 0, 2);
            v4d Swp02 = v4d_swizzle(Swp0b, 0, 0, 0, 2);
            v4d Swp03 = v4d_shuffle(ca[2], ca[1], 2, 2, 2, 2);

            v4d Mul00 = _mm256_mul_pd(Swp00, Swp01);

            Fac4 = _mm256_fnmadd_pd(Swp02, Swp03, Mul00);
        }

        v4d Fac5;
        {
            v4d Swp0a = v4d_shuffle(ca[3], ca[2], 1, 1, 1, 1);
            v4d Swp0b = v4d_shuffle(ca[3], ca[2], 0, 0, 0, 0);

            v4d Swp00 = v4d_shuffle(ca[2], ca[1], 0, 0, 0, 0);
            v4d Swp01 = v4d_swizzle(Swp0a, 0, 0, 0, 2);
            v4d Swp02 = v4d_swizzle(Swp0b, 0, 0, 0, 2);
            v4d Swp03 = v4d_shuffle(ca[2], ca[1], 1, 1, 1, 1);

            v4d Mul00 = _mm256_mul_pd(Swp00, Swp01);

            Fac5 = _mm256_fnmadd_pd(Swp02, Swp03, Mul00);
        }

        v4d SignA = _mm256_set_pd(1.0f, -1.0f, 1.0f, -1.0f);
        v4d SignB = _mm256_set_pd(-1.0f, 1.0f, -1.0f, 1.0f);

        v4d Temp0 = v4d_shuffle(ca[1], ca[0], 0, 0, 0, 0);
        v4d Vec0 = v4d_swizzle(Temp0, 0, 2, 2, 2);

        v4d Temp1 = v4d_shuffle(ca[1], ca[0], 1, 1, 1, 1);
        v4d Vec1 = v4d_swizzle(Temp1, 0, 2, 2, 2);

        v4d Temp2 = v4d_shuffle(ca[1], ca[0], 2, 2, 2, 2);
        v4d Vec2 = v4d_swizzle(Temp2, 0, 2, 2, 2);

        v4d Temp3 = v4d_shuffle(ca[1], ca[0], 3, 3, 3, 3);
        v4d Vec3 = v4d_swizzle(Temp3, 0, 2, 2, 2);

        v4d Mul0 = _mm256_mul_pd(Vec1, Fac0);
        v4d Mul1 = _mm256_mul_pd(Vec0, Fac0);
        v4d Mul2 = _mm256_mul_pd(Vec0, Fac1);
        v4d Mul3 = _mm256_mul_pd(Vec0, Fac2);

        v4d Sub0 = _mm256_fnmadd_pd(Vec2, Fac1, Mul0);
        v4d Sub1 = _mm256_fnmadd_pd(Vec2, Fac3, Mul1);
        v4d Sub2 = _mm256_fnmadd_pd(Vec1, Fac3, Mul2);
        v4d Sub3 = _mm256_fnmadd_pd(Vec1, Fac4, Mul3);

        v4d Add0 = _mm256_fmadd_pd(Vec3, Fac2, Sub0);
        v4d Add1 = _mm256_fmadd_pd(Vec3, Fac4, Sub1);
        v4d Add2 = _mm256_fmadd_pd(Vec3, Fac5, Sub2);
        v4d Add3 = _mm256_fmadd_pd(Vec2, Fac5, Sub3);

        v4d Inv0 = _mm256_mul_pd(SignB, Add0);
        v4d Inv1 = _mm256_mul_pd(SignA, Add1);
        v4d Inv2 = _mm256_mul_pd(SignB, Add2);
        v4d Inv3 = _mm256_mul_pd(SignA, Add3);

        v4d Row0 = v4d_shuffle(Inv0, Inv1, 0, 0, 0, 0);
        v4d Row1 = v4d_shuffle(Inv2, Inv3, 0, 0, 0, 0);
        v4d Row2 = v4d_shuffle(Row0, Row1, 0, 2, 0, 2);

        v4d Det0 = v4d_dot44(ca[0], Row2);
        v4d Rcp0 = v4d_rcp(Det0);

        ca[0] = _mm256_mul_pd(Inv0, Rcp0);
        ca[1] = _mm256_mul_pd(Inv1, Rcp0);
        ca[2] = _mm256_mul_pd(Inv2, Rcp0);
        ca[3] = _mm256_mul_pd(Inv3, Rcp0);
    }

    ML_INLINE void InvertOrtho();

    // NOTE: special sets

    ML_INLINE void SetupByQuaternion(const double4& q) {
        // NOTE: assuming the quaternion is normalized
        double x2 = q.x + q.x;
        double y2 = q.y + q.y;
        double z2 = q.z + q.z;
        double xx2 = q.x * x2;
        double xy2 = q.x * y2;
        double xz2 = q.x * z2;
        double yy2 = q.y * y2;
        double yz2 = q.y * z2;
        double zz2 = q.z * z2;
        double wx2 = q.w * x2;
        double wy2 = q.w * y2;
        double wz2 = q.w * z2;

        ca[0] = double4(1.0f - (yy2 + zz2), xy2 + wz2, xz2 - wy2, 0.0).ymm;
        ca[1] = double4(xy2 - wz2, 1.0f - (xx2 + zz2), yz2 + wx2, 0.0).ymm;
        ca[2] = double4(xz2 + wy2, yz2 - wx2, 1.0f - (xx2 + yy2), 0.0).ymm;
        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByRotationX(double angleX) {
        double ct = cos(angleX);
        double st = sin(angleX);

        ca[0] = double4(1.0, 0.0, 0.0, 0.0);
        ca[1] = double4(0.0, ct, st, 0.0);
        ca[2] = double4(0.0, -st, ct, 0.0);
        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByRotationY(double angleY) {
        double ct = cos(angleY);
        double st = sin(angleY);

        ca[0] = double4(ct, 0.0, -st, 0.0);
        ca[1] = double4(0.0, 1.0, 0.0, 0.0);
        ca[2] = double4(st, 0.0, ct, 0.0);
        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByRotationZ(double angleZ) {
        double ct = cos(angleZ);
        double st = sin(angleZ);

        ca[0] = double4(ct, st, 0.0, 0.0);
        ca[1] = double4(-st, ct, 0.0, 0.0);
        ca[2] = double4(0.0, 0.0, 1.0, 0.0);
        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByRotationYPR(double fYaw, double fPitch, double fRoll) {
        // NOTE: "yaw-pitch-roll" rotation
        //       yaw - around Z (object "down-up" axis)
        //       pitch - around X (object "left-right" axis)
        //       roll - around Y (object "backward-forward" axis)

        /*
        double4x4 rot;
        rot.SetupByRotationY(fRoll);
        *this = rot;
        rot.SetupByRotationX(fPitch);
        *this = rot * (*this);
        rot.SetupByRotationZ(fYaw);
        *this = rot * (*this);
        */

        double4 angles(fYaw, fPitch, fRoll, 0.0);

        double4 c;
        double4 s = _mm256_sincos_pd(&c.ymm, angles.ymm);

        a00 = c.x * c.z - s.x * s.y * s.z;
        a10 = s.x * c.z + c.x * s.y * s.z;
        a20 = -c.y * s.z;
        a30 = 0.0;

        a01 = -s.x * c.y;
        a11 = c.x * c.y;
        a21 = s.y;
        a31 = 0.0;

        a02 = c.x * s.z + c.z * s.x * s.y;
        a12 = s.x * s.z - c.x * s.y * c.z;
        a22 = c.y * c.z;
        a32 = 0.0;

        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByRotation(double theta, const double3& v) {
        double ct = cos(theta);
        double st = sin(theta);

        SetupByRotation(st, ct, v);
    }

    ML_INLINE void SetupByRotation(double st, double ct, const double3& v) {
        double xx = v.x * v.x;
        double yy = v.y * v.y;
        double zz = v.z * v.z;
        double xy = v.x * v.y;
        double xz = v.x * v.z;
        double yz = v.y * v.z;
        double ctxy = ct * xy;
        double ctxz = ct * xz;
        double ctyz = ct * yz;
        double sty = st * v.y;
        double stx = st * v.x;
        double stz = st * v.z;

        a00 = xx + ct * (1.0 - xx);
        a01 = xy - ctxy - stz;
        a02 = xz - ctxz + sty;

        a10 = xy - ctxy + stz;
        a11 = yy + ct * (1.0 - yy);
        a12 = yz - ctyz - stx;

        a20 = xz - ctxz - sty;
        a21 = yz - ctyz + stx;
        a22 = zz + ct * (1.0 - zz);

        a30 = 0.0;
        a31 = 0.0;
        a32 = 0.0;

        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByRotation(const double3& z, const double3& d) {
        /*
        // NOTE: same as

        double3 axis = cross(z, d);
        double angle = Acos( dot(z, d) );

        SetupByRotation(angle, axis);
        */

        double3 w = cross(z, d);
        double c = dot(z, d);
        double k = (1.0 - c) / (1.0 - c * c);

        double hxy = w.x * w.y * k;
        double hxz = w.x * w.z * k;
        double hyz = w.y * w.z * k;

        a00 = c + w.x * w.x * k;
        a01 = hxy - w.z;
        a02 = hxz + w.y;

        a10 = hxy + w.z;
        a11 = c + w.y * w.y * k;
        a12 = hyz - w.x;

        a20 = hxz - w.y;
        a21 = hyz + w.x;
        a22 = c + w.z * w.z * k;

        a30 = 0.0;
        a31 = 0.0;
        a32 = 0.0;

        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByTranslation(const double3& p) {
        ca[0] = double4(1.0, 0.0, 0.0, 0.0);
        ca[1] = double4(0.0, 1.0, 0.0, 0.0);
        ca[2] = double4(0.0, 0.0, 1.0, 0.0);
        ca[3] = v4d_setw1(p);
    }

    ML_INLINE void SetupByScale(const double3& scale) {
        ca[0] = double4(scale.x, 0.0, 0.0, 0.0);
        ca[1] = double4(0.0, scale.y, 0.0, 0.0);
        ca[2] = double4(0.0, 0.0, scale.z, 0.0);
        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByLookAt(const double3& vForward) {
        double3 y = normalize(vForward);
        double3 z = GetPerpendicularVector(y);
        double3 x = cross(y, z);

        ca[0] = v4d_setw0(x);
        ca[1] = v4d_setw0(y);
        ca[2] = v4d_setw0(z);
        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByLookAt(const double3& vForward, const double3& vRight) {
        double3 y = normalize(vForward);
        double3 z = normalize(cross(vRight, y));
        double3 x = cross(y, z);

        ca[0] = v4d_setw0(x);
        ca[1] = v4d_setw0(y);
        ca[2] = v4d_setw0(z);
        ca[3] = c_v4d_0001;
    }

    ML_INLINE void SetupByOrthoProjection(double left, double right, double bottom, double top, double zNear, double zFar, uint32_t uiProjFlags = 0) {
        ML_Assert(left < right);
        ML_Assert(bottom < top);

        double rWidth = 1.0 / (right - left);
        double rHeight = 1.0 / (top - bottom);
        double rDepth = 1.0 / (zFar - zNear);

        a00 = 2.0 * rWidth;
        a01 = 0.0;
        a02 = 0.0;
        a03 = -(right + left) * rWidth;

        a10 = 0.0;
        a11 = 2.0 * rHeight;
        a12 = 0.0;
        a13 = -(top + bottom) * rHeight;

        a20 = 0.0;
        a21 = 0.0;
        a22 = -2.0 * rDepth;
        a23 = -(zFar + zNear) * rDepth;

        a30 = 0.0;
        a31 = 0.0;
        a32 = 0.0;
        a33 = 1.0;

        bool bReverseZ = (uiProjFlags & PROJ_REVERSED_Z) != 0;

        a22 = ML_ModifyProjZ(bReverseZ, a22, a32);
        a23 = ML_ModifyProjZ(bReverseZ, a23, a33);

        if (uiProjFlags & PROJ_LEFT_HANDED)
            ca[2] = v4d_negate(ca[2]);
    }

    ML_INLINE void SetupByFrustum(double left, double right, double bottom, double top, double zNear, double zFar, uint32_t uiProjFlags = 0) {
        ML_Assert(left < right);
        ML_Assert(bottom < top);

        double rWidth = 1.0 / (right - left);
        double rHeight = 1.0 / (top - bottom);
        double rDepth = 1.0 / (zNear - zFar);

        a00 = 2.0 * zNear * rWidth;
        a01 = 0.0;
        a02 = (right + left) * rWidth;
        a03 = 0.0;

        a10 = 0.0;
        a11 = 2.0 * zNear * rHeight;
        a12 = (top + bottom) * rHeight;
        a13 = 0.0;

        a20 = 0.0;
        a21 = 0.0;
        a22 = (zFar + zNear) * rDepth;
        a23 = 2.0 * zFar * zNear * rDepth;

        a30 = 0.0;
        a31 = 0.0;
        a32 = -1.0;
        a33 = 0.0;

        bool bReverseZ = (uiProjFlags & PROJ_REVERSED_Z) != 0;

        a22 = ML_ModifyProjZ(bReverseZ, a22, a32);
        a23 = ML_ModifyProjZ(bReverseZ, a23, a33);

        if (uiProjFlags & PROJ_LEFT_HANDED)
            ca[2] = v4d_negate(ca[2]);
    }

    ML_INLINE void SetupByFrustumInf(double left, double right, double bottom, double top, double zNear, uint32_t uiProjFlags = 0) {
        ML_Assert(left < right);
        ML_Assert(bottom < top);

        double rWidth = 1.0 / (right - left);
        double rHeight = 1.0 / (top - bottom);

        a00 = 2.0 * zNear * rWidth;
        a01 = 0.0;
        a02 = (right + left) * rWidth;
        a03 = 0.0;

        a10 = 0.0;
        a11 = 2.0 * zNear * rHeight;
        a12 = (top + bottom) * rHeight;
        a13 = 0.0;

        a20 = 0.0;
        a21 = 0.0;
        a22 = -1.0;
        a23 = -2.0 * zNear;

        a30 = 0.0;
        a31 = 0.0;
        a32 = -1.0;
        a33 = 0.0;

        bool bReverseZ = (uiProjFlags & PROJ_REVERSED_Z) != 0;

        a22 = ML_ModifyProjZ(bReverseZ, a22, a32);
        a23 = ML_ModifyProjZ(bReverseZ, a23, a33);

        if (uiProjFlags & PROJ_LEFT_HANDED)
            ca[2] = v4d_negate(ca[2]);
    }

    ML_INLINE void SetupByHalfFovy(double halfFovy, double aspect, double zNear, double zFar, uint32_t uiProjFlags = 0) {
        double ymax = zNear * tan(halfFovy);
        double xmax = ymax * aspect;

        SetupByFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar, uiProjFlags);
    }

    ML_INLINE void SetupByHalfFovyInf(double halfFovy, double aspect, double zNear, uint32_t uiProjFlags = 0) {
        double ymax = zNear * tan(halfFovy);
        double xmax = ymax * aspect;

        SetupByFrustumInf(-xmax, xmax, -ymax, ymax, zNear, uiProjFlags);
    }

    ML_INLINE void SetupByHalfFovx(double halfFovx, double aspect, double zNear, double zFar, uint32_t uiProjFlags = 0) {
        double xmax = zNear * tan(halfFovx);
        double ymax = xmax / aspect;

        SetupByFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar, uiProjFlags);
    }

    ML_INLINE void SetupByHalfFovxInf(double halfFovx, double aspect, double zNear, uint32_t uiProjFlags = 0) {
        double xmax = zNear * tan(halfFovx);
        double ymax = xmax / aspect;

        SetupByFrustumInf(-xmax, xmax, -ymax, ymax, zNear, uiProjFlags);
    }

    ML_INLINE void SetupByAngles(double angleMinx, double angleMaxx, double angleMiny, double angleMaxy, double zNear, double zFar, uint32_t uiProjFlags = 0) {
        double xmin = tan(angleMinx) * zNear;
        double xmax = tan(angleMaxx) * zNear;
        double ymin = tan(angleMiny) * zNear;
        double ymax = tan(angleMaxy) * zNear;

        SetupByFrustum(xmin, xmax, ymin, ymax, zNear, zFar, uiProjFlags);
    }

    ML_INLINE void SetupByAnglesInf(double angleMinx, double angleMaxx, double angleMiny, double angleMaxy, double zNear, uint32_t uiProjFlags = 0) {
        double xmin = tan(angleMinx) * zNear;
        double xmax = tan(angleMaxx) * zNear;
        double ymin = tan(angleMiny) * zNear;
        double ymax = tan(angleMaxy) * zNear;

        SetupByFrustumInf(xmin, xmax, ymin, ymax, zNear, uiProjFlags);
    }

    ML_INLINE void SubsampleProjection(double dx, double dy, uint32_t viewportWidth, uint32_t viewportHeight) {
        // NOTE: dx/dy in range [-1; 1]

        a02 += dx / double(viewportWidth);
        a12 += dy / double(viewportHeight);
    }

    ML_INLINE bool IsProjectionValid() const {
        // Do not check a20 and a21 to allow off-centered projections
        // Do not check a22 to allow reverse infinite projections

        return ((a00 != 0.0 && a10 == 0.0 && a20 == 0.0 && a30 == 0.0) && (a01 == 0.0 && a11 != 0.0 && a21 == 0.0 && a31 == 0.0) && (a32 == 1.0 || a32 == -1.0) && (a03 == 0.0 && a13 == 0.0 && a23 != 0.0 && a33 == 0.0));
    }
};

ML_INLINE double4 mul(const double4x4& m, const double4& v) {
    return m * v;
}

ML_INLINE double4x4 transpose(const double4x4& m) {
    double4x4 res;
    m.TransposeTo(res);

    return res;
}

// non-HLSL

ML_INLINE double3 Rotate(const double4x4& m, const double3& v) {
    v4d r = _mm256_mul_pd(v4d_swizzle(v.ymm, 0, 0, 0, 0), m.ca[0]);
    r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 1, 1, 1, 1), m.ca[1], r);
    r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 2, 2, 2, 2), m.ca[2], r);
    r = v4d_setw0(r);

    return r;
}

ML_INLINE double3 RotateAbs(const double4x4& m, const double3& v) {
    v4d col0_abs = v4d_abs(m.ca[0]);
    v4d col1_abs = v4d_abs(m.ca[1]);
    v4d col2_abs = v4d_abs(m.ca[2]);

    v4d r = _mm256_mul_pd(v4d_swizzle(v.ymm, 0, 0, 0, 0), col0_abs);
    r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 1, 1, 1, 1), col1_abs, r);
    r = _mm256_fmadd_pd(v4d_swizzle(v.ymm, 2, 2, 2, 2), col2_abs, r);

    return r;
}

ML_INLINE double3 Project(const double3& v, const double4x4& m) {
    double4 clip = (m * v).ymm;
    clip /= clip.w;

    return clip.ymm;
}

ML_INLINE void double4x4::PreTranslation(const double3& p) {
    v4d r = Rotate(*this, p.ymm).ymm;
    ca[3] = _mm256_add_pd(ca[3], r);
}

ML_INLINE void double4x4::InvertOrtho() {
    Transpose3x4();

    ca[3] = Rotate(*this, double3(ca[3])).ymm;
    ca[3] = v4d_negate(ca[3]);

    ca[0] = v4d_setw0(ca[0]);
    ca[1] = v4d_setw0(ca[1]);
    ca[2] = v4d_setw0(ca[2]);
    ca[3] = v4d_setw1(ca[3]);
}

//======================================================================================================================
// cBoxd
//======================================================================================================================

struct cBoxd {
public:
    double3 vMin;
    double3 vMax;

public:
    ML_INLINE cBoxd() {
        Clear();
    }

    ML_INLINE cBoxd(const double3& v) {
        vMin = v;
        vMax = v;
    }

    ML_INLINE cBoxd(const double3& minv, const double3& maxv) {
        vMin = minv;
        vMax = maxv;
    }

    ML_INLINE void Clear() {
        vMin = double3(c_v4d_Inf);
        vMax = double3(c_v4d_InfMinus);
    }

    ML_INLINE bool IsValid() const {
        v4d r = _mm256_cmplt_pd(vMin.ymm, vMax.ymm);

        return v4d_test3_all(r);
    }

    ML_INLINE double3 GetCenter() const {
        return (vMin + vMax) * 0.5;
    }

    ML_INLINE double GetRadius() const {
        return length(vMax - vMin) * 0.5;
    }

    ML_INLINE void Scale(double fScale) {
        fScale *= 0.5;

        double k1 = 0.5 + fScale;
        double k2 = 0.5 - fScale;

        double3 a = vMin * k1 + vMax * k2;
        double3 b = vMax * k1 + vMin * k2;

        vMin = a;
        vMax = b;
    }

    ML_INLINE void Enlarge(const double3& vBorder) {
        vMin -= vBorder;
        vMax += vBorder;
    }

    ML_INLINE void Add(const double3& v) {
        vMin = _mm256_min_pd(vMin.ymm, v.ymm);
        vMax = _mm256_max_pd(vMax.ymm, v.ymm);
    }

    ML_INLINE void Add(const cBoxd& b) {
        vMin = _mm256_min_pd(vMin.ymm, b.vMin.ymm);
        vMax = _mm256_max_pd(vMax.ymm, b.vMax.ymm);
    }

    ML_INLINE double DistanceSquared(const double3& from) const {
        v4d p = v4d_clamp(from.ymm, vMin.ymm, vMax.ymm);
        p = _mm256_sub_pd(p, from.ymm);
        p = v4d_dot33(p, p);

        return _mm256_cvtsd_f64(p);
    }

    ML_INLINE double Distance(const double3& from) const {
        v4d p = v4d_clamp(from.ymm, vMin.ymm, vMax.ymm);
        p = _mm256_sub_pd(p, from.ymm);
        p = v4d_length(p);

        return _mm256_cvtsd_f64(p);
    }

    ML_INLINE bool IsIntersectWith(const cBoxd& b) const {
        v4d r = _mm256_cmplt_pd(vMax.ymm, b.vMin.ymm);
        r = _mm256_or_pd(r, _mm256_cmpgt_pd(vMin.ymm, b.vMax.ymm));

        return v4d_test3_none(r);
    }

    // NOTE: intersection state 'b' vs 'this'

    ML_INLINE eClip GetIntersectionState(const cBoxd& b) const {
        if (!IsIntersectWith(b))
            return CLIP_OUT;

        v4d r = _mm256_cmplt_pd(vMin.ymm, b.vMin.ymm);
        r = _mm256_and_pd(r, _mm256_cmpgt_pd(vMax.ymm, b.vMax.ymm));

        return v4d_test3_all(r) ? CLIP_IN : CLIP_PARTIAL;
    }

    ML_INLINE bool IsContain(const double3& p) const {
        v4d r = _mm256_cmplt_pd(p.ymm, vMin.ymm);
        r = _mm256_or_pd(r, _mm256_cmpgt_pd(p.ymm, vMax.ymm));

        return v4d_test3_none(r);
    }

    ML_INLINE bool IsContainSphere(const double3& center, double radius) const {
        v4d r = _mm256_set1_pd(radius);
        v4d t = _mm256_sub_pd(vMin.ymm, r);
        t = _mm256_cmplt_pd(center.ymm, t);

        if (v4d_test3_any(t))
            return false;

        t = _mm256_add_pd(vMax.ymm, r);
        t = _mm256_cmpgt_pd(center.ymm, t);

        if (v4d_test3_any(t))
            return false;

        return true;
    }

    ML_INLINE uint32_t GetIntersectionBits(const cBoxd& b) const {
        v4d r = _mm256_cmpge_pd(b.vMin.ymm, vMin.ymm);
        uint32_t bits = (_mm256_movemask_pd(r) & ML_Mask(1, 1, 1, 0));

        r = _mm256_cmple_pd(b.vMax.ymm, vMax.ymm);
        bits |= (_mm256_movemask_pd(r) & ML_Mask(1, 1, 1, 0)) << 3;

        return bits;
    }

    ML_INLINE uint32_t IsContain(const double3& p, uint32_t bits) const {
        v4d r = _mm256_cmpge_pd(p.ymm, vMin.ymm);
        bits |= (_mm256_movemask_pd(r) & ML_Mask(1, 1, 1, 0));

        r = _mm256_cmple_pd(p.ymm, vMax.ymm);
        bits |= (_mm256_movemask_pd(r) & ML_Mask(1, 1, 1, 0)) << 3;

        return bits;
    }

    ML_INLINE bool IsIntersectWith(const double3& vRayPos, const double3& vRayDir, double* out_fTmin, double* out_fTmax) const {
        // NOTE: http://tavianator.com/2011/05/fast-branchless-raybounding-box-intersections/

        // IMPORTANT: store '1 / ray_dir' and filter INFs out!

        v4d t1 = _mm256_div_pd(_mm256_sub_pd(vMin.ymm, vRayPos.ymm), vRayDir.ymm);
        v4d t2 = _mm256_div_pd(_mm256_sub_pd(vMax.ymm, vRayPos.ymm), vRayDir.ymm);

        v4d vmin = _mm256_min_pd(t1, t2);
        v4d vmax = _mm256_max_pd(t1, t2);

        // NOTE: hmax.xxx
        v4d tmin = _mm256_max_pd(vmin, v4d_swizzle(vmin, ML_Y, ML_Z, ML_X, 0));
        tmin = _mm256_max_pd(tmin, v4d_swizzle(vmin, ML_Z, ML_X, ML_Y, 0));

        // NOTE: hmin.xxx
        v4d tmax = _mm256_min_pd(vmax, v4d_swizzle(vmax, ML_Y, ML_Z, ML_X, 0));
        tmax = _mm256_min_pd(tmax, v4d_swizzle(vmax, ML_Z, ML_X, ML_Y, 0));

        v4d_store_x(out_fTmin, tmin);
        v4d_store_x(out_fTmax, tmax);

        v4d cmp = _mm256_cmpge_pd(tmax, tmin);

        return (_mm256_movemask_pd(cmp) & ML_Mask(1, 0, 0, 0)) == ML_Mask(1, 0, 0, 0);
    }
};

ML_INLINE void TransformAabb(const double4x4& mTransform, const cBoxd& src, cBoxd& dst) {
    double3 center = (src.vMin + src.vMax) * 0.5;
    double3 extends = src.vMax - center;

    center = mTransform * center;
    extends = RotateAbs(mTransform, extends);

    dst.vMin = center - extends;
    dst.vMax = center + extends;
}
