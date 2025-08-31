#pragma once

//======================================================================================================================
// Scalar math
//======================================================================================================================

ML_INLINE int32_t Snap(const int32_t& x, const int32_t& step) {
    ML_Assert(step > 0);

    return step * ((x + step - 1) / step);
}

ML_INLINE uint32_t Snap(const uint32_t& x, const uint32_t& step) {
    ML_Assert(step > 0);

    return step * ((x + step - 1) / step);
}

ML_INLINE float Pi(float mul) {
    return mul * acosf(-1.0f);
}

ML_INLINE double Pi(double mul) {
    return mul * acos(-1.0);
}

ML_INLINE float degrees(float x) {
    return x * (180.0f / Pi(1.0f));
}

ML_INLINE double degrees(double x) {
    return x * (180.0 / Pi(1.0));
}

ML_INLINE float radians(float x) {
    return x * (Pi(1.0f) / 180.0f);
}

ML_INLINE double radians(double x) {
    return x * (Pi(1.0) / 180.0);
}

ML_INLINE float sign(float x) {
    return x == 0.0f ? 0.0f : (x < 0.0f ? -1.0f : 1.0f);
}

ML_INLINE double sign(double x) {
    return x == 0.0 ? 0.0 : (x < 0.0 ? -1.0 : 1.0);
}

ML_INLINE int32_t sign(int32_t x) {
    return x == 0 ? 0 : (x < 0 ? -1 : 1);
}

ML_INLINE float frac(float x) {
    return x - floor(x);
}

ML_INLINE double frac(double x) {
    return x - floor(x);
}

template <class T>
ML_INLINE T min(T x, T y) {
    return x < y ? x : y;
}

template <class T>
ML_INLINE T max(T x, T y) {
    return x > y ? x : y;
}

template <class T>
ML_INLINE T clamp(T x, T a, T b) {
    return x < a ? a : (x > b ? b : x);
}

ML_INLINE float saturate(float x) {
    return clamp(x, 0.0f, 1.0f);
}

ML_INLINE double saturate(double x) {
    return clamp(x, 0.0, 1.0);
}

ML_INLINE float lerp(float a, float b, float x) {
    return a + (b - a) * x;
}

ML_INLINE double lerp(double a, double b, double x) {
    return a + (b - a) * x;
}

ML_INLINE float linearstep(float a, float b, float x) {
    return saturate((x - a) / (b - a));
}

ML_INLINE double linearstep(double a, double b, double x) {
    return saturate((x - a) / (b - a));
}

ML_INLINE float smoothstep(float a, float b, float x) {
    float t = linearstep(a, b, x);

    return t * t * (3.0f - 2.0f * t);
}

ML_INLINE double smoothstep(double a, double b, double x) {
    double t = linearstep(a, b, x);

    return t * t * (3.0 - 2.0 * t);
}

ML_INLINE float step(float edge, float x) {
    return x < edge ? 0.0f : 1.0f;
}

ML_INLINE double step(double edge, double x) {
    return x < edge ? 0.0 : 1.0;
}

ML_INLINE float rsqrt(float x) {
    ML_Assert(x >= 0.0f);

    return 1.0f / ::sqrtf(x);
}

ML_INLINE double rsqrt(double x) {
    ML_Assert(x >= 0.0);

    return 1.0 / sqrt(x);
}

ML_INLINE float rcp(float x) {
    return 1.0f / x;
}

ML_INLINE double rcp(double x) {
    return 1.0 / x;
}

#ifdef ML_NAMESPACE

#    define ML_STD_ARG1(f, type) \
        ML_INLINE type f(type x) { \
            return ::f(x); \
        }

#    define ML_STD_ARG2(f, type) \
        ML_INLINE type f(type x, type y) { \
            return ::f(x, y); \
        }

ML_STD_ARG1(abs, float)
ML_STD_ARG1(abs, double)
ML_STD_ARG1(floor, float)
ML_STD_ARG1(floor, double)
ML_STD_ARG1(round, float)
ML_STD_ARG1(round, double)
ML_STD_ARG1(ceil, float)
ML_STD_ARG1(ceil, double)
ML_STD_ARG1(sin, float)
ML_STD_ARG1(sin, double)
ML_STD_ARG1(cos, float)
ML_STD_ARG1(cos, double)
ML_STD_ARG1(tan, float)
ML_STD_ARG1(tan, double)
ML_STD_ARG1(asin, float)
ML_STD_ARG1(asin, double)
ML_STD_ARG1(acos, float)
ML_STD_ARG1(acos, double)
ML_STD_ARG1(atan, float)
ML_STD_ARG1(atan, double)
ML_STD_ARG1(sqrt, float)
ML_STD_ARG1(sqrt, double)
ML_STD_ARG1(log, float)
ML_STD_ARG1(log, double)
ML_STD_ARG1(log2, float)
ML_STD_ARG1(log2, double)
ML_STD_ARG1(exp, float)
ML_STD_ARG1(exp, double)
ML_STD_ARG1(exp2, float)
ML_STD_ARG1(exp2, double)

ML_STD_ARG2(fmod, float)
ML_STD_ARG2(fmod, double)
ML_STD_ARG2(atan2, float)
ML_STD_ARG2(atan2, double)
ML_STD_ARG2(pow, float)
ML_STD_ARG2(pow, double)

#undef ML_STD_ARG1
#undef ML_STD_ARG2

#endif

//======================================================================================================================
// SVML emulation (part 1)
//======================================================================================================================

#if (!ML_SVML_AVAILABLE)

template <typename T>
ML_INLINE v4i emu_mm_div_epx(const v4i& x, const v4i& y) {
    const T* px = (T*)&x;
    const T* py = (T*)&y;

    v4i r;
    T* pr = (T*)&r;

    constexpr size_t N = sizeof(x) / sizeof(T);
    for (size_t i = 0; i < N; i++)
        pr[i] = px[i] / py[i];

    return r;
}

#    undef _mm_div_epi32
#    define _mm_div_epi32(a, b) emu_mm_div_epx<int32_t>(a, b)

#    undef _mm_div_epu32
#    define _mm_div_epu32(a, b) emu_mm_div_epx<uint32_t>(a, b)

#endif

//======================================================================================================================
// v4i
//======================================================================================================================

#define v4i_set(x, y, z, w) _mm_setr_epi32(x, y, z, w)
#define v4i_get(v, i) _mm_cvtsi128_si32(_mm_srli_si128(v, 4 * i))

#define _mm_not_epi32(a) _mm_andnot_si128(a, _mm_set1_epi32(0xFFFFFFFF))

#define _mm_cmple_epi32(a, b) _mm_not_epi32(_mm_cmpgt_epi32(a, b))
#define _mm_cmpge_epi32(a, b) _mm_not_epi32(_mm_cmplt_epi32(a, b))
#define _mm_cmpneq_epi32(a, b) _mm_not_epi32(_mm_cmpeq_epi32(a, b))

#define _mm_movemask_epi32(a) _mm_movemask_ps(_mm_castsi128_ps(a))

#define v4i_swizzle(v, x, y, z, w) _mm_castps_si128(_mm_permute_ps(_mm_castsi128_ps(v), _MM_SHUFFLE(w, z, y, x)))

ML_INLINE v4i v4i_mod(const v4i& x, const v4i& y) {
    v4i d = _mm_div_epi32(x, y);

    return _mm_sub_epi32(x, _mm_mullo_epi32(y, d));
}

ML_INLINE v4i v4u_mod(const v4i& x, const v4i& y) {
    v4i d = _mm_div_epu32(x, y);

    return _mm_sub_epi32(x, _mm_mullo_epi32(y, d));
}

ML_INLINE v4i v4i_select(const v4i& x, const v4i& y, const v4i& mask) {
    return _mm_or_si128(_mm_and_si128(mask, x), _mm_andnot_si128(mask, y));
}

//======================================================================================================================
// v4f
//======================================================================================================================

const v4f c_v4f_Inf = _mm_set1_ps(-logf(0.0f));
const v4f c_v4f_InfMinus = _mm_set1_ps(logf(0.0f));
const v4f c_v4f_0001 = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
const v4f c_v4f_1111 = _mm_set1_ps(1.0f);
const v4f c_v4f_Sign = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
const v4f c_v4f_FFF0 = _mm_castsi128_ps(_mm_setr_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000));

#define v4f_mask_dp(xi, yi, zi, wi, xo, yo, zo, wo) (xo | (yo << 1) | (zo << 2) | (wo << 3) | (xi << 4) | (yi << 5) | (zi << 6) | (wi << 7))
#define v4f_mask_dp4 v4f_mask_dp(1, 1, 1, 1, 1, 1, 1, 1)
#define v4f_mask_dp3 v4f_mask_dp(1, 1, 1, 0, 1, 1, 1, 1)

#define v4f_set(x, y, z, w) _mm_setr_ps(x, y, z, w)
#define v4f_get(v, i) _mm_cvtss_f32(_mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(v), 4 * i)))

#define v4f_setw1(x) _mm_or_ps(_mm_and_ps(x, c_v4f_FFF0), c_v4f_0001)
#define v4f_setw0(x) _mm_and_ps(x, c_v4f_FFF0)

#define v4f_test3_all(v) ((_mm_movemask_ps(v) & ML_Mask(1, 1, 1, 0)) == ML_Mask(1, 1, 1, 0))
#define v4f_test3_none(v) ((_mm_movemask_ps(v) & ML_Mask(1, 1, 1, 0)) == 0)
#define v4f_test3_any(v) ((_mm_movemask_ps(v) & ML_Mask(1, 1, 1, 0)) != 0)

#define v4f_test4_all(v) (_mm_movemask_ps(v) == ML_Mask(1, 1, 1, 1))
#define v4f_test4_none(v) (_mm_movemask_ps(v) == 0)
#define v4f_test4_any(v) (_mm_movemask_ps(v) != 0)

#define v4f_isnegative1_all(v) ((_mm_movemask_ps(v) & ML_Mask(1, 0, 0, 0)) == ML_Mask(1, 0, 0, 0))
#define v4f_isnegative3_all(v) ((_mm_movemask_ps(v) & ML_Mask(1, 1, 1, 0)) == ML_Mask(1, 1, 1, 0))
#define v4f_isnegative4_all(v) (_mm_movemask_ps(v) == ML_Mask(1, 1, 1, 1))

#define v4f_ispositive1_all(v) ((_mm_movemask_ps(v) & ML_Mask(1, 0, 0, 0)) == 0)
#define v4f_ispositive3_all(v) ((_mm_movemask_ps(v) & ML_Mask(1, 1, 1, 0)) == 0)
#define v4f_ispositive4_all(v) (_mm_movemask_ps(v) == 0)

#define v4f_swizzle(v, x, y, z, w) _mm_permute_ps(v, _MM_SHUFFLE(w, z, y, x))
#define v4f_shuffle(v0, v1, i0, j0, i1, j1) _mm_shuffle_ps(v0, v1, _MM_SHUFFLE(j1, i1, j0, i0))

#define v4f_Azw_Bzw(a, b) _mm_movehl_ps(a, b)
#define v4f_Axy_Bxy(a, b) _mm_movelh_ps(a, b)
#define v4f_Ax_Byzw(a, b) _mm_move_ss(b, a)
#define v4f_Az_Bz_Aw_Bw(a, b) _mm_unpackhi_ps(a, b)
#define v4f_Ax_Bx_Ay_By(a, b) _mm_unpacklo_ps(a, b)

#define v4f_store_x(ptr, x) _mm_store_ss(ptr, x)

#define v4f_negate(v) _mm_xor_ps(v, c_v4f_Sign)
#define v4f_abs(v) _mm_andnot_ps(c_v4f_Sign, v)

#define v4f_greater0_all(a) v4f_test4_all(_mm_cmpgt_ps(a, _mm_setzero_ps()))
#define v4f_gequal0_all(a) v4f_test4_all(_mm_cmpge_ps(a, _mm_setzero_ps()))
#define v4f_not0_all(a) v4f_test4_all(_mm_cmpneq_ps(a, _mm_setzero_ps()))

#define v4f_dot33(a, b) _mm_dp_ps(a, b, v4f_mask_dp3)
#define v4f_dot44(a, b) _mm_dp_ps(a, b, v4f_mask_dp4)
#define v4f_dot43(a, b) _mm_dp_ps(a, v4f_setw1(b), v4f_mask_dp4)

#define v4f_round(x) _mm_round_ps(x, _MM_FROUND_TO_NEAREST_INT | ML_ROUNDING_EXEPTIONS_MASK)
#define v4f_floor(x) _mm_round_ps(x, _MM_FROUND_FLOOR | ML_ROUNDING_EXEPTIONS_MASK)
#define v4f_ceil(x) _mm_round_ps(x, _MM_FROUND_CEIL | ML_ROUNDING_EXEPTIONS_MASK)

#define _v4f_vselecti(mask, x, y) _mm_blendv_ps(y, x, _mm_castsi128_ps(mask))
#define _v4f_vselect(mask, x, y) _mm_blendv_ps(y, x, mask)
#define _v4f_iselect(mask, x, y) _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(y), _mm_castsi128_ps(x), mask))
#define _v4f_mulsign(x, y) _mm_xor_ps(x, _mm_and_ps(y, c_v4f_Sign))
#define _v4f_negatei(x) _mm_sub_epi32(_mm_setzero_si128(), x)
#define _v4f_is_inf(x) _mm_cmpeq_ps(v4f_abs(x), c_v4f_Inf)
#define _v4f_is_pinf(x) _mm_cmpeq_ps(x, c_v4f_Inf)
#define _v4f_is_ninf(x) _mm_cmpeq_ps(x, c_v4f_InfMinus)
#define _v4f_is_nan(x) _mm_cmpneq_ps(x, x)
#define _v4f_is_inf2(x, y) _mm_and_ps(_v4f_is_inf(x), _mm_or_ps(_mm_and_ps(x, c_v4f_Sign), y))

#if ML_CHECK_W_IS_ZERO

ML_INLINE bool v4f_is_w_zero(const v4f& x) {
    v4f t = _mm_cmpeq_ps(x, _mm_setzero_ps());

    return (_mm_movemask_ps(t) & ML_Mask(0, 0, 0, 1)) == ML_Mask(0, 0, 0, 1);
}

#else
#    define v4f_is_w_zero(x) (true)
#endif

#if (ML_INTRINSIC_LEVEL >= ML_INTRINSIC_AVX1)
ML_INLINE v4i v4f_to_h4(const v4f& x) {
    return _mm_cvtps_ph(x, _MM_FROUND_TO_NEAREST_INT);
}
#endif

ML_INLINE v4f v4f_sign(const v4f& x) {
    // NOTE: 1 for +0, -1 for -0

    v4f v = _mm_and_ps(x, c_v4f_Sign);

    return _mm_or_ps(v, c_v4f_1111);
}

ML_INLINE v4f v4f_frac(const v4f& x) {
    v4f flr0 = v4f_floor(x);
    v4f sub0 = _mm_sub_ps(x, flr0);

    return sub0;
}

ML_INLINE v4f v4f_clamp(const v4f& x, const v4f& vmin, const v4f& vmax) {
    v4f min0 = _mm_min_ps(x, vmax);

    return _mm_max_ps(min0, vmin);
}

ML_INLINE v4f v4f_saturate(const v4f& x) {
    v4f min0 = _mm_min_ps(x, c_v4f_1111);

    return _mm_max_ps(min0, _mm_setzero_ps());
}

ML_INLINE v4f v4f_step(const v4f& edge, const v4f& x) {
    v4f cmp = _mm_cmpge_ps(x, edge);

    return _mm_and_ps(c_v4f_1111, cmp);
}

ML_INLINE v4f v4f_linearstep(const v4f& edge0, const v4f& edge1, const v4f& x) {
    v4f sub0 = _mm_sub_ps(x, edge0);
    v4f sub1 = _mm_sub_ps(edge1, edge0);
    v4f div0 = _mm_div_ps(sub0, sub1);

    return v4f_saturate(div0);
}

ML_INLINE v4f v4f_length(const v4f& x) {
    v4f r = v4f_dot33(x, x);

    return _mm_sqrt_ps(r);
}

ML_INLINE v4f v4f_cross(const v4f& x, const v4f& y) {
    v4f a = v4f_swizzle(x, 1, 2, 0, 3);
    v4f b = v4f_swizzle(y, 2, 0, 1, 3);
    v4f c = v4f_swizzle(x, 2, 0, 1, 3);
    v4f d = v4f_swizzle(y, 1, 2, 0, 3);

    c = _mm_mul_ps(c, d);

    return _mm_fmsub_ps(a, b, c);
}

ML_INLINE v4f v4f_rsqrt(const v4f& r) {
#if ML_NEWTONRAPHSON_APROXIMATION

    v4f c = _mm_rsqrt_ps(r);
    v4f a = _mm_mul_ps(c, _mm_set1_ps(0.5f));
    v4f t = _mm_mul_ps(r, c);
    v4f b = _mm_fnmadd_ps(t, c, _mm_set1_ps(3.0f));

    return _mm_mul_ps(a, b);

#else

    return _mm_rsqrt_ps(r);

#endif
}

ML_INLINE v4f v4f_rcp(const v4f& r) {
#if ML_NEWTONRAPHSON_APROXIMATION

    v4f c = _mm_rcp_ps(r);
    v4f a = _mm_mul_ps(c, r);
    v4f b = _mm_add_ps(c, c);

    return _mm_fnmadd_ps(a, c, b);

#else

    return _mm_rcp_ps(r);

#endif
}

ML_INLINE v4f v4f_mod(const v4f& x, const v4f& y) {
    v4f d = _mm_div_ps(x, y);
    v4f f = v4f_floor(d);
    v4f c = v4f_ceil(d);
    f = _mm_blendv_ps(c, f, _mm_cmpgt_ps(d, _mm_setzero_ps()));

    return _mm_fnmadd_ps(y, f, x);
}

ML_INLINE v4f v4f_mix(const v4f& a, const v4f& b, const v4f& x) {
    v4f sub0 = _mm_sub_ps(b, a);

    return _mm_fmadd_ps(sub0, x, a);
}

ML_INLINE v4f v4f_smoothstep(const v4f& edge0, const v4f& edge1, const v4f& x) {
    v4f b = v4f_linearstep(edge0, edge1, x);
    v4f c = _mm_fnmadd_ps(_mm_set1_ps(2.0f), b, _mm_set1_ps(3.0f));
    v4f t = _mm_mul_ps(b, b);

    return _mm_mul_ps(t, c);
}

ML_INLINE v4f v4f_normalize(const v4f& x) {
    v4f r = v4f_dot33(x, x);
    r = v4f_rsqrt(r);

    return _mm_mul_ps(x, r);
}

//======================================================================================================================
// v4d
//======================================================================================================================

const v4d c_v4d_Inf = _mm256_set1_pd(-log(0.0));
const v4d c_v4d_InfMinus = _mm256_set1_pd(log(0.0));
const v4d c_v4d_0001 = _mm256_setr_pd(0.0, 0.0, 0.0, 1.0);
const v4d c_v4d_1111 = _mm256_set1_pd(1.0);
const v4d c_v4d_Sign = _mm256_castsi256_pd(_mm256_set_epi32(0x80000000, 0x00000000, 0x80000000, 0x00000000, 0x80000000, 0x00000000, 0x80000000, 0x00000000));
const v4d c_v4d_FFF0 = _mm256_castsi256_pd(_mm256_setr_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000));

#define v4d_set(x, y, z, w) _mm256_setr_pd(x, y, z, w)
#define v4d_get(v, i) _mm256_cvtsd_f64(_mm256_permute4x64_pd(v, _MM_SHUFFLE(0, 0, 0, i)))

#define v4d_setw1(x) _mm256_or_pd(_mm256_and_pd(x, c_v4d_FFF0), c_v4d_0001)
#define v4d_setw0(x) _mm256_and_pd(x, c_v4d_FFF0)

#define v4d_test3_all(v) ((_mm256_movemask_pd(v) & ML_Mask(1, 1, 1, 0)) == ML_Mask(1, 1, 1, 0))
#define v4d_test3_none(v) ((_mm256_movemask_pd(v) & ML_Mask(1, 1, 1, 0)) == 0)
#define v4d_test3_any(v) ((_mm256_movemask_pd(v) & ML_Mask(1, 1, 1, 0)) != 0)

#define v4d_test4_all(v) (_mm256_movemask_pd(v) == ML_Mask(1, 1, 1, 1))
#define v4d_test4_none(v) (_mm256_movemask_pd(v) == 0)
#define v4d_test4_any(v) (_mm256_movemask_pd(v) != 0)

#define v4d_isnegative1_all(v) ((_mm256_movemask_pd(v) & ML_Mask(1, 0, 0, 0)) == ML_Mask(1, 0, 0, 0))
#define v4d_isnegative3_all(v) ((_mm256_movemask_pd(v) & ML_Mask(1, 1, 1, 0)) == ML_Mask(1, 1, 1, 0))
#define v4d_isnegative4_all(v) (_mm256_movemask_pd(v) == ML_Mask(1, 1, 1, 1))

#define v4d_ispositive1_all(v) ((_mm256_movemask_pd(v) & ML_Mask(1, 0, 0, 0)) == 0)
#define v4d_ispositive3_all(v) ((_mm256_movemask_pd(v) & ML_Mask(1, 1, 1, 0)) == 0)
#define v4d_ispositive4_all(v) (_mm256_movemask_pd(v) == 0)

#define v4d_swizzle(v, x, y, z, w) _mm256_permute4x64_pd(v, _MM_SHUFFLE(w, z, y, x))
#define v4d_shuffle(v0, v1, i0, j0, i1, j1) _mm256_blend_pd(_mm256_permute4x64_pd(v0, _MM_SHUFFLE(j1, i1, j0, i0)), _mm256_permute4x64_pd(v1, _MM_SHUFFLE(j1, i1, j0, i0)), 0xC)

#define v4d_Azw_Bzw(a, b) _mm256_permute2f128_pd(a, b, (1 << 4) | 3)
#define v4d_Axy_Bxy(a, b) _mm256_permute2f128_pd(a, b, (2 << 4) | 0)
#define v4d_Ay_By_Aw_Bw(a, b) _mm256_unpackhi_pd(a, b)
#define v4d_Ax_Bx_Az_Bz(a, b) _mm256_unpacklo_pd(a, b)

#define v4d_store_x(ptr, x) _mm_storel_pd(ptr, _mm256_extractf128_pd(x, 0))

#define v4d_negate(v) _mm256_xor_pd(v, c_v4d_Sign)
#define v4d_abs(v) _mm256_andnot_pd(c_v4d_Sign, v)

#define _mm256_cmplt_pd(a, b) _mm256_cmp_pd(a, b, _CMP_LT_OQ)
#define _mm256_cmple_pd(a, b) _mm256_cmp_pd(a, b, _CMP_LE_OQ)
#define _mm256_cmpeq_pd(a, b) _mm256_cmp_pd(a, b, _CMP_EQ_UQ)
#define _mm256_cmpge_pd(a, b) _mm256_cmp_pd(a, b, _CMP_GE_OQ)
#define _mm256_cmpgt_pd(a, b) _mm256_cmp_pd(a, b, _CMP_GT_OQ)
#define _mm256_cmpneq_pd(a, b) _mm256_cmp_pd(a, b, _CMP_NEQ_UQ)

#define v4d_greater0_all(a) v4d_test4_all(_mm256_cmpgt_pd(a, _mm256_setzero_pd()))
#define v4d_gequal0_all(a) v4d_test4_all(_mm256_cmpge_pd(a, _mm256_setzero_pd()))
#define v4d_not0_all(a) v4d_test4_all(_mm256_cmpneq_pd(a, _mm256_setzero_pd()))

#define v4d_round(x) _mm256_round_pd(x, _MM_FROUND_TO_NEAREST_INT | ML_ROUNDING_EXEPTIONS_MASK)
#define v4d_floor(x) _mm256_round_pd(x, _MM_FROUND_FLOOR | ML_ROUNDING_EXEPTIONS_MASK)
#define v4d_ceil(x) _mm256_round_pd(x, _MM_FROUND_CEIL | ML_ROUNDING_EXEPTIONS_MASK)

#define _v4d_mulsign(x, y) _mm256_xor_pd(x, _mm256_and_pd(y, c_v4d_Sign))
#define _v4d_is_inf(x) _mm256_cmpeq_pd(v4d_abs(x), c_v4d_Inf)
#define _v4d_is_pinf(x) _mm256_cmpeq_pd(x, c_v4d_Inf)
#define _v4d_is_ninf(x) _mm256_cmpeq_pd(x, c_v4d_InfMinus)
#define _v4d_is_nan(x) _mm256_cmpneq_pd(x, x)
#define _v4d_is_inf2(x, y) _mm256_and_pd(_v4d_is_inf(x), _mm256_or_pd(_mm256_and_pd(x, c_v4d_Sign), y))

#if ML_CHECK_W_IS_ZERO

ML_INLINE bool v4d_is_w_zero(const v4d& x) {
    v4d t = _mm256_cmpeq_pd(x, _mm256_setzero_pd());

    return (_mm256_movemask_pd(t) & ML_Mask(0, 0, 0, 1)) == ML_Mask(0, 0, 0, 1);
}

#else
#    define v4d_is_w_zero(x) (true)
#endif

ML_INLINE v4d v4d_dot33(const v4d& a, const v4d& b) {
    v4d r = _mm256_mul_pd(a, b);
    r = v4d_setw0(r);
    r = _mm256_hadd_pd(r, _mm256_permute2f128_pd(r, r, (0 << 4) | 3));
    r = _mm256_hadd_pd(r, r);

    return r;
}

ML_INLINE v4d v4d_dot44(const v4d& a, const v4d& b) {
    v4d r = _mm256_mul_pd(a, b);
    r = _mm256_hadd_pd(r, _mm256_permute2f128_pd(r, r, (0 << 4) | 3));
    r = _mm256_hadd_pd(r, r);

    return r;
}

ML_INLINE v4d v4d_dot43(const v4d& a, const v4d& b) {
    v4d r = v4d_setw1(b);
    r = _mm256_mul_pd(a, r);
    r = _mm256_hadd_pd(r, _mm256_permute2f128_pd(r, r, (0 << 4) | 3));
    r = _mm256_hadd_pd(r, r);

    return r;
}

ML_INLINE v4d v4d_sign(const v4d& x) {
    // NOTE: 1 for +0, -1 for -0

    v4d v = _mm256_and_pd(x, c_v4d_Sign);

    return _mm256_or_pd(v, c_v4d_1111);
}

ML_INLINE v4d v4d_frac(const v4d& x) {
    v4d flr0 = v4d_floor(x);
    v4d sub0 = _mm256_sub_pd(x, flr0);

    return sub0;
}

ML_INLINE v4d v4d_clamp(const v4d& x, const v4d& vmin, const v4d& vmax) {
    v4d min0 = _mm256_min_pd(x, vmax);

    return _mm256_max_pd(min0, vmin);
}

ML_INLINE v4d v4d_saturate(const v4d& x) {
    v4d min0 = _mm256_min_pd(x, c_v4d_1111);

    return _mm256_max_pd(min0, _mm256_setzero_pd());
}

ML_INLINE v4d v4d_step(const v4d& edge, const v4d& x) {
    v4d cmp = _mm256_cmpge_pd(x, edge);

    return _mm256_and_pd(c_v4d_1111, cmp);
}

ML_INLINE v4d v4d_linearstep(const v4d& edge0, const v4d& edge1, const v4d& x) {
    v4d sub0 = _mm256_sub_pd(x, edge0);
    v4d sub1 = _mm256_sub_pd(edge1, edge0);
    v4d div0 = _mm256_div_pd(sub0, sub1);

    return v4d_saturate(div0);
}

ML_INLINE v4d v4d_length(const v4d& x) {
    v4d r = v4d_dot33(x, x);

    return _mm256_sqrt_pd(r);
}

ML_INLINE v4d v4d_cross(const v4d& x, const v4d& y) {
    v4d a = v4d_swizzle(x, 1, 2, 0, 3);
    v4d b = v4d_swizzle(y, 2, 0, 1, 3);
    v4d c = v4d_swizzle(x, 2, 0, 1, 3);
    v4d d = v4d_swizzle(y, 1, 2, 0, 3);

    c = _mm256_mul_pd(c, d);

    return _mm256_fmsub_pd(a, b, c);
}

ML_INLINE v4d v4d_rsqrt(const v4d& r) {
    v4d c = _mm256_div_pd(c_v4d_1111, _mm256_sqrt_pd(r));

#if ML_NEWTONRAPHSON_APROXIMATION

    v4d a = _mm256_mul_pd(c, _mm256_set1_pd(0.5));
    v4d t = _mm256_mul_pd(r, c);
    v4d b = _mm256_fnmadd_pd(t, c, _mm256_set1_pd(3.0));

    return _mm256_mul_pd(a, b);

#else

    return c;

#endif
}

ML_INLINE v4d v4d_rcp(const v4d& r) {
    v4d c = _mm256_div_pd(c_v4d_1111, r);

#if ML_NEWTONRAPHSON_APROXIMATION

    v4d a = _mm256_mul_pd(c, r);
    v4d b = _mm256_add_pd(c, c);

    return _mm256_fnmadd_pd(a, c, b);

#else

    return c;

#endif
}

ML_INLINE v4d v4d_mod(const v4d& x, const v4d& y) {
    v4d d = _mm256_div_pd(x, y);
    v4d f = v4d_floor(d);
    v4d c = v4d_ceil(d);
    f = _mm256_blendv_pd(c, f, _mm256_cmpgt_pd(d, _mm256_setzero_pd()));

    return _mm256_fnmadd_pd(y, f, x);
}

ML_INLINE v4d v4d_mix(const v4d& a, const v4d& b, const v4d& x) {
    v4d sub0 = _mm256_sub_pd(b, a);

    return _mm256_fmadd_pd(sub0, x, a);
}

ML_INLINE v4d v4d_smoothstep(const v4d& edge0, const v4d& edge1, const v4d& x) {
    v4d b = v4d_linearstep(edge0, edge1, x);
    v4d c = _mm256_fnmadd_pd(_mm256_set1_pd(2.0), b, _mm256_set1_pd(3.0));
    v4d t = _mm256_mul_pd(b, b);

    return _mm256_mul_pd(t, c);
}

ML_INLINE v4d v4d_normalize(const v4d& x) {
    v4d r = v4d_dot33(x, x);
    r = v4d_rsqrt(r);

    return _mm256_mul_pd(x, r);
}

//======================================================================================================================
// SVML emulation (part 2)
//======================================================================================================================

// Based on Sleef 2.80
// https://bitbucket.org/eschnett/vecmathlib/src
// http://shibatch.sourceforge.net/

#if (!ML_SVML_AVAILABLE)

constexpr float Cf_PI4_A = 0.78515625f;
constexpr float Cf_PI4_B = 0.00024187564849853515625f;
constexpr float Cf_PI4_C = 3.7747668102383613586e-08f;
constexpr float Cf_PI4_D = 1.2816720341285448015e-12f;
constexpr float c_f[] = {0.31830988618379067154f, 0.00282363896258175373077393f, -0.0159569028764963150024414f, 0.0425049886107444763183594f, -0.0748900920152664184570312f,
    0.106347933411598205566406f, -0.142027363181114196777344f, 0.199926957488059997558594f, -0.333331018686294555664062f, 1.57079632679489661923f, 5.421010862427522E-20f,
    1.8446744073709552E19f, -Cf_PI4_A * 4.0f, -Cf_PI4_B * 4.0f, -Cf_PI4_C * 4.0f, -Cf_PI4_D * 4.0f, 2.6083159809786593541503e-06f, -0.0001981069071916863322258f,
    0.00833307858556509017944336f, -0.166666597127914428710938f, -Cf_PI4_A * 2.0f, -Cf_PI4_B * 2.0f, -Cf_PI4_C * 2.0f, -Cf_PI4_D * 2.0f, 0.63661977236758134308f,
    -0.000195169282960705459117889f, 0.00833215750753879547119141f, -0.166666537523269653320312f, -2.71811842367242206819355e-07f, 2.47990446951007470488548e-05f,
    -0.00138888787478208541870117f, 0.0416666641831398010253906f, -0.5f, 1.0f, 0.00927245803177356719970703f, 0.00331984995864331722259521f, 0.0242998078465461730957031f,
    0.0534495301544666290283203f, 0.133383005857467651367188f, 0.333331853151321411132812f, 0.78539816339744830962f, -1.0f, 0.5f, 3.14159265358979323846f, 0.7071f,
    0.2371599674224853515625f, 0.285279005765914916992188f, 0.400005519390106201171875f, 0.666666567325592041015625f, 2.0f, 0.693147180559945286226764f,
    1.442695040888963407359924681001892137426645954152985934135449406931f, -0.693145751953125f, -1.428606765330187045e-06f, 0.00136324646882712841033936f,
    0.00836596917361021041870117f, 0.0416710823774337768554688f, 0.166665524244308471679688f, 0.499999850988388061523438f};

ML_INLINE v4f _v4f_is_inf_or_zero(const v4f& x) {
    v4f t = v4f_abs(x);

    return _mm_or_ps(_mm_cmpeq_ps(t, _mm_setzero_ps()), _mm_cmpeq_ps(t, c_v4f_Inf));
}

ML_INLINE v4f _v4f_atan2(const v4f& y, const v4f& x) {
    v4i q = _v4f_iselect(_mm_cmplt_ps(x, _mm_setzero_ps()), _mm_set1_epi32(-2), _mm_setzero_si128());
    v4f r = v4f_abs(x);

    v4f mask = _mm_cmplt_ps(r, y);
    q = _v4f_iselect(mask, _mm_add_epi32(q, _mm_set1_epi32(1)), q);
    v4f s = _v4f_vselect(mask, v4f_negate(r), y);
    v4f t = _mm_max_ps(r, y);

    s = _mm_div_ps(s, t);
    t = _mm_mul_ps(s, s);

    v4f u = _mm_broadcast_ss(c_f + 1);
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 2));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 3));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 4));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 5));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 6));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 7));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 8));

    t = _mm_fmadd_ps(s, _mm_mul_ps(t, u), s);
    t = _mm_fmadd_ps(_mm_cvtepi32_ps(q), _mm_broadcast_ss(c_f + 9), t);

    return t;
}

ML_INLINE v4i _v4f_logbp1(const v4f& d) {
    v4f m = _mm_cmplt_ps(d, _mm_broadcast_ss(c_f + 10));
    v4f r = _v4f_vselect(m, _mm_mul_ps(_mm_broadcast_ss(c_f + 11), d), d);
    v4i q = _mm_and_si128(_mm_srli_epi32(_mm_castps_si128(r), 23), _mm_set1_epi32(0xff));
    q = _mm_sub_epi32(q, _v4f_iselect(m, _mm_set1_epi32(64 + 0x7e), _mm_set1_epi32(0x7e)));

    return q;
}

ML_INLINE v4f _v4f_ldexp(const v4f& x, const v4i& q) {
    v4i m = _mm_srai_epi32(q, 31);
    m = _mm_slli_epi32(_mm_sub_epi32(_mm_srai_epi32(_mm_add_epi32(m, q), 6), m), 4);
    v4i t = _mm_sub_epi32(q, _mm_slli_epi32(m, 2));
    m = _mm_add_epi32(m, _mm_set1_epi32(0x7f));
    m = _mm_and_si128(_mm_cmpgt_epi32(m, _mm_setzero_si128()), m);
    v4i n = _mm_cmpgt_epi32(m, _mm_set1_epi32(0xff));
    m = _mm_or_si128(_mm_andnot_si128(n, m), _mm_and_si128(n, _mm_set1_epi32(0xff)));
    v4f u = _mm_castsi128_ps(_mm_slli_epi32(m, 23));
    v4f r = _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(_mm_mul_ps(x, u), u), u), u);
    u = _mm_castsi128_ps(_mm_slli_epi32(_mm_add_epi32(t, _mm_set1_epi32(0x7f)), 23));

    return _mm_mul_ps(r, u);
}

ML_INLINE v4f emu_mm_sin_ps(const v4f& x) {
    v4i q = _mm_cvtps_epi32(_mm_mul_ps(x, _mm_broadcast_ss(c_f + 0)));
    v4f u = _mm_cvtepi32_ps(q);

    v4f r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 12), x);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 13), r);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 14), r);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 15), r);

    v4f s = _mm_mul_ps(r, r);

    r = _mm_castsi128_ps(_mm_xor_si128(_mm_and_si128(_mm_cmpeq_epi32(_mm_and_si128(q, _mm_set1_epi32(1)), _mm_set1_epi32(1)), _mm_castps_si128(c_v4f_Sign)), _mm_castps_si128(r)));

    u = _mm_broadcast_ss(c_f + 16);
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 17));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 18));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 19));
    u = _mm_fmadd_ps(s, _mm_mul_ps(u, r), r);

    u = _mm_or_ps(_v4f_is_inf(r), u);

    return u;
}

ML_INLINE v4f emu_mm_cos_ps(const v4f& x) {
    v4i q = _mm_cvtps_epi32(_mm_sub_ps(_mm_mul_ps(x, _mm_broadcast_ss(c_f + 0)), _mm_broadcast_ss(c_f + 42)));
    q = _mm_add_epi32(_mm_add_epi32(q, q), _mm_set1_epi32(1));

    v4f u = _mm_cvtepi32_ps(q);

    v4f r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 20), x);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 21), r);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 22), r);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 23), r);

    v4f s = _mm_mul_ps(r, r);

    r = _mm_castsi128_ps(
        _mm_xor_si128(_mm_and_si128(_mm_cmpeq_epi32(_mm_and_si128(q, _mm_set1_epi32(2)), _mm_setzero_si128()), _mm_castps_si128(c_v4f_Sign)), _mm_castps_si128(r)));

    u = _mm_broadcast_ss(c_f + 16);
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 17));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 18));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 19));
    u = _mm_fmadd_ps(s, _mm_mul_ps(u, r), r);

    u = _mm_or_ps(_v4f_is_inf(r), u);

    return u;
}

ML_INLINE v4f emu_mm_sincos_ps(v4f* pCos, const v4f& d) {
    v4i q = _mm_cvtps_epi32(_mm_mul_ps(d, _mm_broadcast_ss(c_f + 24)));

    v4f s = d;

    v4f u = _mm_cvtepi32_ps(q);
    s = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 20), s);
    s = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 21), s);
    s = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 22), s);
    s = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 23), s);

    v4f t = s;

    s = _mm_mul_ps(s, s);

    u = _mm_broadcast_ss(c_f + 25);
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 26));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 27));
    u = _mm_mul_ps(_mm_mul_ps(u, s), t);

    v4f rx = _mm_add_ps(t, u);

    u = _mm_broadcast_ss(c_f + 28);
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 29));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 30));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 31));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 32));

    v4f ry = _mm_fmadd_ps(s, u, _mm_broadcast_ss(c_f + 33));

    v4f m = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(q, _mm_set1_epi32(1)), _mm_set1_epi32(0)));
    v4f rrx = _v4f_vselect(m, rx, ry);
    v4f rry = _v4f_vselect(m, ry, rx);

    m = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(q, _mm_set1_epi32(2)), _mm_set1_epi32(2)));
    rrx = _mm_xor_ps(_mm_and_ps(m, c_v4f_Sign), rrx);

    m = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(_mm_add_epi32(q, _mm_set1_epi32(1)), _mm_set1_epi32(2)), _mm_set1_epi32(2)));
    rry = _mm_xor_ps(_mm_and_ps(m, c_v4f_Sign), rry);

    m = _v4f_is_inf(d);

    *pCos = _mm_or_ps(m, rry);

    return _mm_or_ps(m, rrx);
}

ML_INLINE v4f emu_mm_tan_ps(const v4f& x) {
    v4i q = _mm_cvtps_epi32(_mm_mul_ps(x, _mm_broadcast_ss(c_f + 24)));
    v4f r = x;

    v4f u = _mm_cvtepi32_ps(q);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 20), r);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 21), r);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 22), r);
    r = _mm_fmadd_ps(u, _mm_broadcast_ss(c_f + 23), r);

    v4f s = _mm_mul_ps(r, r);

    v4i m = _mm_cmpeq_epi32(_mm_and_si128(q, _mm_set1_epi32(1)), _mm_set1_epi32(1));
    r = _mm_castsi128_ps(_mm_xor_si128(_mm_and_si128(m, _mm_castps_si128(c_v4f_Sign)), _mm_castps_si128(r)));

    u = _mm_broadcast_ss(c_f + 34);
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 35));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 36));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 37));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 38));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 39));

    u = _mm_fmadd_ps(s, _mm_mul_ps(u, r), r);
    u = _v4f_vselecti(m, v4f_rcp(u), u);

    u = _mm_or_ps(_v4f_is_inf(r), u);

    return u;
}

ML_INLINE v4f emu_mm_atan_ps(const v4f& d) {
    v4i q = _v4f_iselect(_mm_cmplt_ps(d, _mm_setzero_ps()), _mm_set1_epi32(2), _mm_setzero_si128());
    v4f s = v4f_abs(d);

    v4f mask = _mm_cmplt_ps(_mm_broadcast_ss(c_f + 33), s);
    q = _v4f_iselect(mask, _mm_add_epi32(q, _mm_set1_epi32(1)), q);
    s = _v4f_vselect(mask, v4f_rcp(s), s);

    v4f t = _mm_mul_ps(s, s);

    v4f u = _mm_broadcast_ss(c_f + 1);
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 2));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 3));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 4));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 5));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 6));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 7));
    u = _mm_fmadd_ps(u, t, _mm_broadcast_ss(c_f + 8));

    t = _mm_fmadd_ps(s, _mm_mul_ps(t, u), s);
    t = _v4f_vselecti(_mm_cmpeq_epi32(_mm_and_si128(q, _mm_set1_epi32(1)), _mm_set1_epi32(1)), _mm_sub_ps(_mm_broadcast_ss(c_f + 9), t), t);

    t = _mm_castsi128_ps(_mm_xor_si128(_mm_and_si128(_mm_cmpeq_epi32(_mm_and_si128(q, _mm_set1_epi32(2)), _mm_set1_epi32(2)), _mm_castps_si128(c_v4f_Sign)), _mm_castps_si128(t)));

    return t;
}

ML_INLINE v4f emu_mm_atan2_ps(const v4f& y, const v4f& x) {
    v4f r = _v4f_atan2(v4f_abs(y), x);

    r = _v4f_mulsign(r, x);
    r = _v4f_vselect(_v4f_is_inf_or_zero(x), _mm_sub_ps(_mm_broadcast_ss(c_f + 9), _v4f_is_inf2(x, _v4f_mulsign(_mm_broadcast_ss(c_f + 9), x))), r);
    r = _v4f_vselect(_v4f_is_inf(y), _mm_sub_ps(_mm_broadcast_ss(c_f + 9), _v4f_is_inf2(x, _v4f_mulsign(_mm_broadcast_ss(c_f + 40), x))), r);
    r = _v4f_vselect(_mm_cmpeq_ps(y, _mm_setzero_ps()), _mm_xor_ps(_mm_cmpeq_ps(v4f_sign(x), _mm_broadcast_ss(c_f + 41)), _mm_broadcast_ss(c_f + 43)), r);

    r = _mm_or_ps(_mm_or_ps(_v4f_is_nan(x), _v4f_is_nan(y)), _v4f_mulsign(r, y));

    return r;
}

ML_INLINE v4f emu_mm_asin_ps(const v4f& d) {
    v4f x = _mm_add_ps(_mm_broadcast_ss(c_f + 33), d);
    v4f y = _mm_sub_ps(_mm_broadcast_ss(c_f + 33), d);
    x = _mm_mul_ps(x, y);
    x = _mm_sqrt_ps(x);
    x = _mm_or_ps(_v4f_is_nan(x), _v4f_atan2(v4f_abs(d), x));

    return _v4f_mulsign(x, d);
}

ML_INLINE v4f emu_mm_acos_ps(const v4f& d) {
    v4f x = _mm_add_ps(_mm_broadcast_ss(c_f + 33), d);
    v4f y = _mm_sub_ps(_mm_broadcast_ss(c_f + 33), d);
    x = _mm_mul_ps(x, y);
    x = _mm_sqrt_ps(x);
    x = _v4f_mulsign(_v4f_atan2(x, v4f_abs(d)), d);
    y = _mm_and_ps(_mm_cmplt_ps(d, _mm_setzero_ps()), _mm_broadcast_ss(c_f + 43));
    x = _mm_add_ps(x, y);

    return x;
}

ML_INLINE v4f emu_mm_log_ps(const v4f& d) {
    v4f x = _mm_mul_ps(d, _mm_broadcast_ss(c_f + 44));
    v4i e = _v4f_logbp1(x);
    v4f m = _v4f_ldexp(d, _v4f_negatei(e));
    v4f r = x;

    x = _mm_div_ps(_mm_add_ps(_mm_broadcast_ss(c_f + 41), m), _mm_add_ps(_mm_broadcast_ss(c_f + 33), m));
    v4f x2 = _mm_mul_ps(x, x);

    v4f t = _mm_broadcast_ss(c_f + 45);
    t = _mm_fmadd_ps(t, x2, _mm_broadcast_ss(c_f + 46));
    t = _mm_fmadd_ps(t, x2, _mm_broadcast_ss(c_f + 47));
    t = _mm_fmadd_ps(t, x2, _mm_broadcast_ss(c_f + 48));
    t = _mm_fmadd_ps(t, x2, _mm_broadcast_ss(c_f + 49));

    x = _mm_fmadd_ps(x, t, _mm_mul_ps(_mm_broadcast_ss(c_f + 50), _mm_cvtepi32_ps(e)));
    x = _v4f_vselect(_v4f_is_pinf(r), c_v4f_Inf, x);

    x = _mm_or_ps(_mm_cmpgt_ps(_mm_setzero_ps(), r), x);
    x = _v4f_vselect(_mm_cmpeq_ps(r, _mm_setzero_ps()), c_v4f_InfMinus, x);

    return x;
}

ML_INLINE v4f emu_mm_exp_ps(const v4f& d) {
    v4i q = _mm_cvtps_epi32(_mm_mul_ps(d, _mm_broadcast_ss(c_f + 51)));

    v4f s = _mm_fmadd_ps(_mm_cvtepi32_ps(q), _mm_broadcast_ss(c_f + 52), d);
    s = _mm_fmadd_ps(_mm_cvtepi32_ps(q), _mm_broadcast_ss(c_f + 53), s);

    v4f u = _mm_broadcast_ss(c_f + 54);
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 55));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 56));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 57));
    u = _mm_fmadd_ps(u, s, _mm_broadcast_ss(c_f + 58));

    u = _mm_add_ps(_mm_broadcast_ss(c_f + 33), _mm_fmadd_ps(_mm_mul_ps(s, s), u, s));
    u = _v4f_ldexp(u, q);

    u = _mm_andnot_ps(_v4f_is_ninf(d), u);

    return u;
}

#    undef _mm_sin_ps
#    define _mm_sin_ps emu_mm_sin_ps

#    undef _mm_cos_ps
#    define _mm_cos_ps emu_mm_cos_ps

#    undef _mm_sincos_ps
#    define _mm_sincos_ps emu_mm_sincos_ps

#    undef _mm_tan_ps
#    define _mm_tan_ps emu_mm_tan_ps

#    undef _mm_atan_ps
#    define _mm_atan_ps emu_mm_atan_ps

#    undef _mm_atan2_ps
#    define _mm_atan2_ps emu_mm_atan2_ps

#    undef _mm_asin_ps
#    define _mm_asin_ps emu_mm_asin_ps

#    undef _mm_acos_ps
#    define _mm_acos_ps emu_mm_acos_ps

#    undef _mm_log_ps
#    define _mm_log_ps emu_mm_log_ps

#    undef _mm_exp_ps
#    define _mm_exp_ps emu_mm_exp_ps

#    undef _mm_pow_ps
#    define _mm_pow_ps(x, y) _mm_exp_ps(_mm_mul_ps(_mm_log_ps(x), y))

#    undef _mm_exp2_ps
#    define _mm_exp2_ps(x) _mm_exp_ps(_mm_mul_ps(_mm_set1_ps(log(2.0f)), x))

#    undef _mm_log2_ps
#    define _mm_log2_ps(x) _mm_mul_ps(_mm_log_ps(x), _mm_set1_ps(1.0f / log(2.0f)))

#endif

#if (ML_INTRINSIC_LEVEL < ML_INTRINSIC_AVX1 || !ML_SVML_AVAILABLE)

constexpr double Cd_PI4_A = 0.78539816290140151978;
constexpr double Cd_PI4_B = 4.9604678871439933374e-10;
constexpr double Cd_PI4_C = 1.1258708853173288931e-18;
constexpr double Cd_PI4_D = 1.7607799325916000908e-27;
constexpr double c_d[] = {
    1.0,
    -1.88796008463073496563746e-05,
    0.000209850076645816976906797,
    -0.00110611831486672482563471,
    0.00370026744188713119232403,
    -0.00889896195887655491740809,
    0.016599329773529201970117,
    -0.0254517624932312641616861,
    0.0337852580001353069993897,
    -0.0407629191276836500001934,
    0.0466667150077840625632675,
    -0.0523674852303482457616113,
    0.0587666392926673580854313,
    -0.0666573579361080525984562,
    0.0769219538311769618355029,
    -0.090908995008245008229153,
    0.111111105648261418443745,
    -0.14285714266771329383765,
    0.199999999996591265594148,
    -0.333333333333311110369124,
    1.57079632679489661923,
    4.9090934652977266E-91,
    2.037035976334486E90,
    300 + 0x3fe,
    0x3fe,
    0.31830988618379067154,
    -Cd_PI4_A * 4.0,
    -Cd_PI4_B * 4.0,
    -Cd_PI4_C * 4.0,
    -Cd_PI4_D * 4.0,
    -7.97255955009037868891952e-18,
    2.81009972710863200091251e-15,
    -7.64712219118158833288484e-13,
    1.60590430605664501629054e-10,
    -2.50521083763502045810755e-08,
    2.75573192239198747630416e-06,
    -0.000198412698412696162806809,
    0.00833333333333332974823815,
    -0.166666666666666657414808,
    -0.5,
    -Cd_PI4_A * 2.0,
    -Cd_PI4_B * 2.0,
    -Cd_PI4_C * 2.0,
    -Cd_PI4_D * 2.0,
    0.63661977236758134308,
    1.58938307283228937328511e-10,
    -2.50506943502539773349318e-08,
    2.75573131776846360512547e-06,
    -0.000198412698278911770864914,
    0.0083333333333191845961746,
    -0.166666666666666130709393,
    -1.13615350239097429531523e-11,
    2.08757471207040055479366e-09,
    -2.75573144028847567498567e-07,
    2.48015872890001867311915e-05,
    -0.00138888888888714019282329,
    0.0416666666666665519592062,
    0.78539816339744830962,
    -1.0,
    3.14159265358979323846,
    0.7071,
    0.148197055177935105296783,
    0.153108178020442575739679,
    0.181837339521549679055568,
    0.22222194152736701733275,
    0.285714288030134544449368,
    0.399999999989941956712869,
    0.666666666666685503450651,
    2.0,
    0.693147180559945286226764,
    1.442695040888963407359924681001892137426645954152985934135449406931,
    -0.69314718055966295651160180568695068359375,
    -0.28235290563031577122588448175013436025525412068e-12,
    2.08860621107283687536341e-09,
    2.51112930892876518610661e-08,
    2.75573911234900471893338e-07,
    2.75572362911928827629423e-06,
    2.4801587159235472998791e-05,
    0.000198412698960509205564975,
    0.00138888888889774492207962,
    0.00833333333331652721664984,
    0.0416666666666665047591422,
    0.166666666666666851703837,
    0.5,
    1.01419718511083373224408e-05,
    -2.59519791585924697698614e-05,
    5.23388081915899855325186e-05,
    -3.05033014433946488225616e-05,
    7.14707504084242744267497e-05,
    8.09674518280159187045078e-05,
    0.000244884931879331847054404,
    0.000588505168743587154904506,
    0.00145612788922812427978848,
    0.00359208743836906619142924,
    0.00886323944362401618113356,
    0.0218694882853846389592078,
    0.0539682539781298417636002,
    0.133333333333125941821962,
    0.333333333333334980164153,
};

ML_INLINE v4d _v4d_is_inf_or_zero(const v4d& x) {
    v4d t = v4d_abs(x);

    return _mm256_or_pd(_mm256_cmpeq_pd(t, _mm256_setzero_pd()), _mm256_cmpeq_pd(t, c_v4d_Inf));
}

#    define _v4d_vselect(mask, x, y) _mm256_blendv_pd(y, x, mask)

ML_INLINE v4i _v4d_selecti(const v4d& d0, const v4d& d1, const v4i& x, const v4i& y) {
    __m128i mask = _mm256_cvtpd_epi32(_mm256_and_pd(_mm256_cmplt_pd(d0, d1), _mm256_broadcast_sd(c_d + 0)));
    mask = _mm_cmpeq_epi32(mask, _mm_set1_epi32(1));

    return _v4f_iselect(_mm_castsi128_ps(mask), x, y);
}

ML_INLINE v4d _v4d_cmp_4i(const v4i& x, const v4i& y) {
    v4i t = _mm_cmpeq_epi32(x, y);

    return _mm256_castsi256_pd(_mm256_cvtepi32_epi64(t));
}

ML_INLINE v4d _v4d_atan2(const v4d& y, const v4d& x) {
    v4i q = _v4d_selecti(x, _mm256_setzero_pd(), _mm_set1_epi32(-2), _mm_setzero_si128());
    v4d r = v4d_abs(x);

    q = _v4d_selecti(r, y, _mm_add_epi32(q, _mm_set1_epi32(1)), q);
    v4d p = _mm256_cmplt_pd(r, y);
    v4d s = _v4d_vselect(p, v4d_negate(r), y);
    v4d t = _mm256_max_pd(r, y);

    s = _mm256_div_pd(s, t);
    t = _mm256_mul_pd(s, s);

    v4d u = _mm256_broadcast_sd(c_d + 1);
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 2));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 3));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 4));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 5));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 6));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 7));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 8));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 9));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 10));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 11));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 12));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 13));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 14));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 15));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 16));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 17));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 18));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 19));

    t = _mm256_fmadd_pd(s, _mm256_mul_pd(t, u), s);
    t = _mm256_fmadd_pd(_mm256_cvtepi32_pd(q), _mm256_broadcast_sd(c_d + 20), t);

    return t;
}

ML_INLINE v4i _v4d_logbp1(const v4d& d) {
    v4d m = _mm256_cmplt_pd(d, _mm256_broadcast_sd(c_d + 21));
    v4d t = _v4d_vselect(m, _mm256_mul_pd(_mm256_broadcast_sd(c_d + 22), d), d);
    v4i c = _mm256_cvtpd_epi32(_v4d_vselect(m, _mm256_broadcast_sd(c_d + 23), _mm256_broadcast_sd(c_d + 24)));
    v4i q = _mm_castpd_si128(_mm256_castpd256_pd128(t));
    q = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(q), _mm_setzero_ps(), _MM_SHUFFLE(0, 0, 3, 1)));
    v4i r = _mm_castpd_si128(_mm256_extractf128_pd(t, 1));
    r = _mm_castps_si128(_mm_shuffle_ps(_mm_setzero_ps(), _mm_castsi128_ps(r), _MM_SHUFFLE(3, 1, 0, 0)));
    q = _mm_or_si128(q, r);
    q = _mm_srli_epi32(q, 20);
    q = _mm_sub_epi32(q, c);

    return q;
}

ML_INLINE v4d _v4d_pow2i(const v4i& q) {
    v4i t = _mm_add_epi32(_mm_set1_epi32(0x3ff), q);
    t = _mm_slli_epi32(t, 20);
    v4i r = _mm_shuffle_epi32(t, _MM_SHUFFLE(1, 0, 0, 0));
    v4d y = _mm256_castpd128_pd256(_mm_castsi128_pd(r));
    r = _mm_shuffle_epi32(t, _MM_SHUFFLE(3, 2, 2, 2));
    y = _mm256_insertf128_pd(y, _mm_castsi128_pd(r), 1);
    y = _mm256_and_pd(y, _mm256_castsi256_pd(_mm256_set_epi32(0xfff00000, 0, 0xfff00000, 0, 0xfff00000, 0, 0xfff00000, 0)));

    return y;
}

ML_INLINE v4d _v4d_ldexp(const v4d& x, const v4i& q) {
    v4i m = _mm_srai_epi32(q, 31);
    m = _mm_slli_epi32(_mm_sub_epi32(_mm_srai_epi32(_mm_add_epi32(m, q), 9), m), 7);
    v4i t = _mm_sub_epi32(q, _mm_slli_epi32(m, 2));
    m = _mm_add_epi32(_mm_set1_epi32(0x3ff), m);
    m = _mm_andnot_si128(_mm_cmplt_epi32(m, _mm_setzero_si128()), m);
    v4i n = _mm_cmpgt_epi32(m, _mm_set1_epi32(0x7ff));
    m = _mm_or_si128(_mm_andnot_si128(n, m), _mm_and_si128(n, _mm_set1_epi32(0x7ff)));
    m = _mm_slli_epi32(m, 20);
    v4i r = _mm_shuffle_epi32(m, _MM_SHUFFLE(1, 0, 0, 0));
    v4d y = _mm256_castpd128_pd256(_mm_castsi128_pd(r));
    r = _mm_shuffle_epi32(m, _MM_SHUFFLE(3, 2, 2, 2));
    y = _mm256_insertf128_pd(y, _mm_castsi128_pd(r), 1);
    y = _mm256_and_pd(y, _mm256_castsi256_pd(_mm256_set_epi32(0xfff00000, 0, 0xfff00000, 0, 0xfff00000, 0, 0xfff00000, 0)));

    return _mm256_mul_pd(_mm256_mul_pd(_mm256_mul_pd(_mm256_mul_pd(_mm256_mul_pd(x, y), y), y), y), _v4d_pow2i(t));
}

ML_INLINE v4d emu_mm256_sin_pd(const v4d& d) {
    v4i q = _mm256_cvtpd_epi32(_mm256_mul_pd(d, _mm256_broadcast_sd(c_d + 25)));

    v4d u = _mm256_cvtepi32_pd(q);

    v4d r = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 26), d);
    r = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 27), r);
    r = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 28), r);
    r = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 29), r);

    v4d s = _mm256_mul_pd(r, r);

    r = _mm256_xor_pd(_mm256_and_pd(_v4d_cmp_4i(_mm_and_si128(q, _mm_set1_epi32(1)), _mm_set1_epi32(1)), c_v4d_Sign), r);

    u = _mm256_broadcast_sd(c_d + 30);
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 31));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 32));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 33));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 34));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 35));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 36));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 37));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 38));

    u = _mm256_fmadd_pd(s, _mm256_mul_pd(u, r), r);

    return u;
}

ML_INLINE v4d emu_mm256_cos_pd(const v4d& d) {
    v4i q = _mm256_cvtpd_epi32(_mm256_fmadd_pd(d, _mm256_broadcast_sd(c_d + 25), _mm256_broadcast_sd(c_d + 39)));
    q = _mm_add_epi32(_mm_add_epi32(q, q), _mm_set1_epi32(1));

    v4d u = _mm256_cvtepi32_pd(q);

    v4d r = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 40), d);
    r = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 41), r);
    r = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 42), r);
    r = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 43), r);

    v4d s = _mm256_mul_pd(r, r);

    r = _mm256_xor_pd(_mm256_and_pd(_v4d_cmp_4i(_mm_and_si128(q, _mm_set1_epi32(2)), _mm_setzero_si128()), c_v4d_Sign), r);

    u = _mm256_broadcast_sd(c_d + 30);
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 31));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 32));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 33));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 34));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 35));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 36));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 37));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 38));

    u = _mm256_fmadd_pd(s, _mm256_mul_pd(u, r), r);

    return u;
}

ML_INLINE v4d emu_mm256_sincos_pd(v4d* pCos, const v4d& d) {
    v4i q = _mm256_cvtpd_epi32(_mm256_mul_pd(d, _mm256_broadcast_sd(c_d + 44)));
    v4d s = d;

    v4d u = _mm256_cvtepi32_pd(q);
    s = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 40), s);
    s = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 41), s);
    s = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 42), s);
    s = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 43), s);

    v4d t = s;

    s = _mm256_mul_pd(s, s);

    u = _mm256_broadcast_sd(c_d + 45);
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 46));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 47));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 48));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 49));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 50));
    u = _mm256_mul_pd(_mm256_mul_pd(u, s), t);

    v4d rx = _mm256_add_pd(t, u);

    u = _mm256_broadcast_sd(c_d + 51);
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 52));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 53));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 54));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 55));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 56));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 39));

    v4d ry = _mm256_fmadd_pd(s, u, _mm256_broadcast_sd(c_d + 0));

    v4d m = _v4d_cmp_4i(_mm_and_si128(q, _mm_set1_epi32(1)), _mm_setzero_si128());
    v4d rrx = _v4d_vselect(m, rx, ry);
    v4d rry = _v4d_vselect(m, ry, rx);

    m = _v4d_cmp_4i(_mm_and_si128(q, _mm_set1_epi32(2)), _mm_set1_epi32(2));
    rrx = _mm256_xor_pd(_mm256_and_pd(m, c_v4d_Sign), rrx);

    m = _v4d_cmp_4i(_mm_and_si128(_mm_add_epi32(q, _mm_set1_epi32(1)), _mm_set1_epi32(2)), _mm_set1_epi32(2));
    rry = _mm256_xor_pd(_mm256_and_pd(m, c_v4d_Sign), rry);

    m = _v4d_is_inf(d);
    *pCos = _mm256_or_pd(m, rry);

    return _mm256_or_pd(m, rrx);
}

ML_INLINE v4d emu_mm256_tan_pd(const v4d& d) {
    v4i q = _mm256_cvtpd_epi32(_mm256_mul_pd(d, _mm256_broadcast_sd(c_d + 44)));

    v4d u = _mm256_cvtepi32_pd(q);

    v4d x = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 40), d);
    x = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 41), x);
    x = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 42), x);
    x = _mm256_fmadd_pd(u, _mm256_broadcast_sd(c_d + 43), x);

    v4d s = _mm256_mul_pd(x, x);

    v4d m = _v4d_cmp_4i(_mm_and_si128(q, _mm_set1_epi32(1)), _mm_set1_epi32(1));
    x = _mm256_xor_pd(_mm256_and_pd(m, c_v4d_Sign), x);

    u = _mm256_broadcast_sd(c_d + 84);
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 85));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 86));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 87));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 88));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 89));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 90));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 91));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 92));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 93));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 94));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 95));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 96));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 97));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 98));

    u = _mm256_fmadd_pd(s, _mm256_mul_pd(u, x), x);
    u = _v4d_vselect(m, v4d_rcp(u), u);

    u = _mm256_or_pd(_v4d_is_inf(d), u);

    return u;
}

ML_INLINE v4d emu_mm256_atan_pd(const v4d& s) {
    v4i q = _v4d_selecti(s, _mm256_setzero_pd(), _mm_set1_epi32(2), _mm_setzero_si128());
    v4d r = v4d_abs(s);

    q = _v4d_selecti(_mm256_broadcast_sd(c_d + 0), r, _mm_add_epi32(q, _mm_set1_epi32(1)), q);
    r = _v4d_vselect(_mm256_cmplt_pd(_mm256_broadcast_sd(c_d + 0), r), v4d_rcp(r), r);

    v4d t = _mm256_mul_pd(r, r);

    v4d u = _mm256_broadcast_sd(c_d + 1);
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 2));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 3));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 4));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 5));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 6));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 7));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 8));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 9));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 10));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 11));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 12));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 13));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 14));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 15));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 16));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 17));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 18));
    u = _mm256_fmadd_pd(u, t, _mm256_broadcast_sd(c_d + 19));

    t = _mm256_fmadd_pd(r, _mm256_mul_pd(t, u), r);

    t = _v4d_vselect(_v4d_cmp_4i(_mm_and_si128(q, _mm_set1_epi32(1)), _mm_set1_epi32(1)), _mm256_sub_pd(_mm256_broadcast_sd(c_d + 20), t), t);
    t = _mm256_xor_pd(_mm256_and_pd(_v4d_cmp_4i(_mm_and_si128(q, _mm_set1_epi32(2)), _mm_set1_epi32(2)), c_v4d_Sign), t);

    return t;
}

ML_INLINE v4d emu_mm256_atan2_pd(const v4d& y, const v4d& x) {
    v4d r = _v4d_atan2(v4d_abs(y), x);

    r = _v4d_mulsign(r, x);
    r = _v4d_vselect(_mm256_or_pd(_v4d_is_inf(x), _mm256_cmpeq_pd(x, _mm256_setzero_pd())),
        _mm256_sub_pd(_mm256_broadcast_sd(c_d + 20), _v4d_is_inf2(x, _v4d_mulsign(_mm256_broadcast_sd(c_d + 20), x))), r);
    r = _v4d_vselect(_v4d_is_inf(y), _mm256_sub_pd(_mm256_broadcast_sd(c_d + 20), _v4d_is_inf2(x, _v4d_mulsign(_mm256_broadcast_sd(c_d + 57), x))), r);
    r = _v4d_vselect(_mm256_cmpeq_pd(y, _mm256_setzero_pd()), _mm256_and_pd(_mm256_cmpeq_pd(v4d_sign(x), _mm256_broadcast_sd(c_d + 58)), _mm256_broadcast_sd(c_d + 59)), r);

    r = _mm256_or_pd(_mm256_or_pd(_v4d_is_nan(x), _v4d_is_nan(y)), _v4d_mulsign(r, y));
    return r;
}

ML_INLINE v4d emu_mm256_asin_pd(const v4d& d) {
    v4d x = _mm256_add_pd(_mm256_broadcast_sd(c_d + 0), d);
    v4d y = _mm256_sub_pd(_mm256_broadcast_sd(c_d + 0), d);
    x = _mm256_mul_pd(x, y);
    x = _mm256_sqrt_pd(x);
    x = _mm256_or_pd(_v4d_is_nan(x), _v4d_atan2(v4d_abs(d), x));

    return _v4d_mulsign(x, d);
}

ML_INLINE v4d emu_mm256_acos_pd(const v4d& d) {
    v4d x = _mm256_add_pd(_mm256_broadcast_sd(c_d + 0), d);
    v4d y = _mm256_sub_pd(_mm256_broadcast_sd(c_d + 0), d);
    x = _mm256_mul_pd(x, y);
    x = _mm256_sqrt_pd(x);
    x = _v4d_mulsign(_v4d_atan2(x, v4d_abs(d)), d);
    y = _mm256_and_pd(_mm256_cmplt_pd(d, _mm256_setzero_pd()), _mm256_broadcast_sd(c_d + 59));
    x = _mm256_add_pd(x, y);

    return x;
}

ML_INLINE v4d emu_mm256_log_pd(const v4d& d) {
    v4i e = _v4d_logbp1(_mm256_mul_pd(d, _mm256_broadcast_sd(c_d + 60)));
    v4d m = _v4d_ldexp(d, _v4f_negatei(e));

    v4d x = _mm256_div_pd(_mm256_add_pd(_mm256_broadcast_sd(c_d + 58), m), _mm256_add_pd(_mm256_broadcast_sd(c_d + 0), m));
    v4d x2 = _mm256_mul_pd(x, x);

    v4d t = _mm256_broadcast_sd(c_d + 61);
    t = _mm256_fmadd_pd(t, x2, _mm256_broadcast_sd(c_d + 62));
    t = _mm256_fmadd_pd(t, x2, _mm256_broadcast_sd(c_d + 63));
    t = _mm256_fmadd_pd(t, x2, _mm256_broadcast_sd(c_d + 64));
    t = _mm256_fmadd_pd(t, x2, _mm256_broadcast_sd(c_d + 65));
    t = _mm256_fmadd_pd(t, x2, _mm256_broadcast_sd(c_d + 66));
    t = _mm256_fmadd_pd(t, x2, _mm256_broadcast_sd(c_d + 67));
    t = _mm256_fmadd_pd(t, x2, _mm256_broadcast_sd(c_d + 68));

    x = _mm256_fmadd_pd(x, t, _mm256_mul_pd(_mm256_broadcast_sd(c_d + 69), _mm256_cvtepi32_pd(e)));

    x = _v4d_vselect(_v4d_is_pinf(d), c_v4d_Inf, x);
    x = _mm256_or_pd(_mm256_cmpgt_pd(_mm256_setzero_pd(), d), x);
    x = _v4d_vselect(_mm256_cmpeq_pd(d, _mm256_setzero_pd()), c_v4d_InfMinus, x);

    return x;
}

ML_INLINE v4d emu_mm256_exp_pd(const v4d& d) {
    v4i q = _mm256_cvtpd_epi32(_mm256_mul_pd(d, _mm256_broadcast_sd(c_d + 70)));

    v4d s = _mm256_fmadd_pd(_mm256_cvtepi32_pd(q), _mm256_broadcast_sd(c_d + 71), d);
    s = _mm256_fmadd_pd(_mm256_cvtepi32_pd(q), _mm256_broadcast_sd(c_d + 72), s);

    v4d u = _mm256_broadcast_sd(c_d + 73);
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 74));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 75));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 76));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 77));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 78));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 79));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 80));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 81));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 82));
    u = _mm256_fmadd_pd(u, s, _mm256_broadcast_sd(c_d + 83));

    u = _mm256_add_pd(_mm256_broadcast_sd(c_d + 0), _mm256_fmadd_pd(_mm256_mul_pd(s, s), u, s));

    u = _v4d_ldexp(u, q);

    u = _mm256_andnot_pd(_v4d_is_ninf(d), u);

    return u;
}

#    undef _mm256_sin_pd
#    define _mm256_sin_pd emu_mm256_sin_pd

#    undef _mm256_cos_pd
#    define _mm256_cos_pd emu_mm256_cos_pd

#    undef _mm256_sincos_pd
#    define _mm256_sincos_pd emu_mm256_sincos_pd

#    undef _mm256_tan_pd
#    define _mm256_tan_pd emu_mm256_tan_pd

#    undef _mm256_atan_pd
#    define _mm256_atan_pd emu_mm256_atan_pd

#    undef _mm256_atan2_pd
#    define _mm256_atan2_pd emu_mm256_atan2_pd

#    undef _mm256_asin_pd
#    define _mm256_asin_pd emu_mm256_asin_pd

#    undef _mm256_acos_pd
#    define _mm256_acos_pd emu_mm256_acos_pd

#    undef _mm256_log_pd
#    define _mm256_log_pd emu_mm256_log_pd

#    undef _mm256_exp_pd
#    define _mm256_exp_pd emu_mm256_exp_pd

#    undef _mm256_pow_pd
#    define _mm256_pow_pd(x, y) _mm256_exp_pd(_mm256_mul_pd(_mm256_log_pd(x), y))

#    undef _mm256_exp2_pd
#    define _mm256_exp2_pd(x) _mm256_exp_pd(_mm256_mul_pd(_mm256_set1_pd(log(2.0)), x))

#    undef _mm256_log2_pd
#    define _mm256_log2_pd(x) _mm256_mul_pd(_mm256_log_pd(x), _mm256_set1_pd(1.0 / log(2.0)))

#endif
