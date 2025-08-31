#pragma once

typedef uint64_t v2i;
typedef __m128d v2d;

typedef __m128 v4f;
typedef __m128i v4i;

#define ML_Mask(x, y, z, w) (x | (y << 1) | (z << 2) | (w << 3))

// Based on maxim.locktyukhin and phil.j.kerly work

template <typename T>
ML_INLINE T _mm_copy(const T& x, const T& y) {
    ML_Unused(x);

    return y;
}

//======================================================================================================================
// SSE4
//======================================================================================================================

#undef _MM_FROUND_TO_NEAREST_INT
#undef _MM_FROUND_TO_NEG_INF
#undef _MM_FROUND_TO_POS_INF
#undef _MM_FROUND_TO_ZERO
#undef _MM_FROUND_CUR_DIRECTION
#undef _MM_FROUND_NO_EXC
#undef _MM_ROUND_NEAREST
#undef _MM_ROUND_DOWN
#undef _MM_ROUND_UP
#undef _MM_ROUND_TOWARD_ZERO
#undef _MM_FROUND_RAISE_EXC
#undef _MM_FROUND_NINT
#undef _MM_FROUND_FLOOR
#undef _MM_FROUND_CEIL
#undef _MM_FROUND_TRUNC
#undef _MM_FROUND_RINT
#undef _MM_FROUND_NEARBYINT
#undef _CMP_EQ_OQ
#undef _CMP_LT_OS
#undef _CMP_LE_OS
#undef _CMP_UNORD_Q
#undef _CMP_NEQ_UQ
#undef _CMP_NLT_US
#undef _CMP_NLE_US
#undef _CMP_ORD_Q
#undef _CMP_EQ_UQ
#undef _CMP_NGE_US
#undef _CMP_NGT_US
#undef _CMP_FALSE_OQ
#undef _CMP_NEQ_OQ
#undef _CMP_GE_OS
#undef _CMP_GT_OS
#undef _CMP_TRUE_UQ
#undef _CMP_EQ_OS
#undef _CMP_LT_OQ
#undef _CMP_LE_OQ
#undef _CMP_UNORD_S
#undef _CMP_NEQ_US
#undef _CMP_NLT_UQ
#undef _CMP_NLE_UQ
#undef _CMP_ORD_S
#undef _CMP_EQ_US
#undef _CMP_NGE_UQ
#undef _CMP_NGT_UQ
#undef _CMP_FALSE_OS
#undef _CMP_NEQ_OS
#undef _CMP_GE_OQ
#undef _CMP_GT_OQ
#undef _CMP_TRUE_US

#define _MM_FROUND_TO_NEAREST_INT 0x00
#define _MM_FROUND_TO_NEG_INF 0x01
#define _MM_FROUND_TO_POS_INF 0x02
#define _MM_FROUND_TO_ZERO 0x03
#define _MM_FROUND_CUR_DIRECTION 0x04
#define _MM_FROUND_NO_EXC 0x08

#define _MM_ROUND_NEAREST 0x0000
#define _MM_ROUND_DOWN 0x2000
#define _MM_ROUND_UP 0x4000
#define _MM_ROUND_TOWARD_ZERO 0x6000

#define _MM_FROUND_RAISE_EXC 0x00
#define _MM_FROUND_NINT _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_FLOOR _MM_FROUND_TO_NEG_INF | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_CEIL _MM_FROUND_TO_POS_INF | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_TRUNC _MM_FROUND_TO_ZERO | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_RINT _MM_FROUND_CUR_DIRECTION | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_NEARBYINT _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC

#define _CMP_EQ_OQ 0x00    /* Equal (ordered, non-signaling)  */
#define _CMP_LT_OS 0x01    /* Less-than (ordered, signaling)  */
#define _CMP_LE_OS 0x02    /* Less-than-or-equal (ordered, signaling)  */
#define _CMP_UNORD_Q 0x03  /* Unordered (non-signaling)  */
#define _CMP_NEQ_UQ 0x04   /* Not-equal (unordered, non-signaling)  */
#define _CMP_NLT_US 0x05   /* Not-less-than (unordered, signaling)  */
#define _CMP_NLE_US 0x06   /* Not-less-than-or-equal (unordered, signaling)  */
#define _CMP_ORD_Q 0x07    /* Ordered (non-signaling)   */
#define _CMP_EQ_UQ 0x08    /* Equal (unordered, non-signaling)  */
#define _CMP_NGE_US 0x09   /* Not-greater-than-or-equal (unordered, signaling)  */
#define _CMP_NGT_US 0x0a   /* Not-greater-than (unordered, signaling)  */
#define _CMP_FALSE_OQ 0x0b /* False (ordered, non-signaling)  */
#define _CMP_NEQ_OQ 0x0c   /* Not-equal (ordered, non-signaling)  */
#define _CMP_GE_OS 0x0d    /* Greater-than-or-equal (ordered, signaling)  */
#define _CMP_GT_OS 0x0e    /* Greater-than (ordered, signaling)  */
#define _CMP_TRUE_UQ 0x0f  /* True (unordered, non-signaling)  */
#define _CMP_EQ_OS 0x10    /* Equal (ordered, signaling)  */
#define _CMP_LT_OQ 0x11    /* Less-than (ordered, non-signaling)  */
#define _CMP_LE_OQ 0x12    /* Less-than-or-equal (ordered, non-signaling)  */
#define _CMP_UNORD_S 0x13  /* Unordered (signaling)  */
#define _CMP_NEQ_US 0x14   /* Not-equal (unordered, signaling)  */
#define _CMP_NLT_UQ 0x15   /* Not-less-than (unordered, non-signaling)  */
#define _CMP_NLE_UQ 0x16   /* Not-less-than-or-equal (unordered, non-signaling)  */
#define _CMP_ORD_S 0x17    /* Ordered (signaling)  */
#define _CMP_EQ_US 0x18    /* Equal (unordered, signaling)  */
#define _CMP_NGE_UQ 0x19   /* Not-greater-than-or-equal (unordered, non-signaling)  */
#define _CMP_NGT_UQ 0x1a   /* Not-greater-than (unordered, non-signaling)  */
#define _CMP_FALSE_OS 0x1b /* False (ordered, signaling)  */
#define _CMP_NEQ_OS 0x1c   /* Not-equal (ordered, signaling)  */
#define _CMP_GE_OQ 0x1d    /* Greater-than-or-equal (ordered, non-signaling)  */
#define _CMP_GT_OQ 0x1e    /* Greater-than (ordered, non-signaling)  */
#define _CMP_TRUE_US 0x1f  /* True (unordered, signaling)  */

#if (ML_INTRINSIC_LEVEL < ML_INTRINSIC_SSE4)

// test

ML_INLINE int emu_mm_testz_si128(const __m128i& x, const __m128i& y) {
    ML_Unused(x, y);
    ML_AssertMsg(false, "TODO: unimplemented!");

    return 0;
}

ML_INLINE int emu_mm_testc_si128(const __m128i& x, const __m128i& y) {
    ML_Unused(x, y);
    ML_AssertMsg(false, "TODO: unimplemented!");

    return 0;
}

ML_INLINE int emu_mm_testnzc_si128(const __m128i& x, const __m128i& y) {
    ML_Unused(x, y);
    ML_AssertMsg(false, "TODO: unimplemented!");

    return 0;
}

#    undef _mm_testz_si128
#    define _mm_testz_si128(x, y) emu_mm_testz_si128(x, y)

#    undef _mm_testc_si128
#    define _mm_testc_si128(x, y) emu_mm_testc_si128(x, y)

#    undef _mm_testnzc_si128
#    define _mm_testnzc_si128(x, y) emu_mm_testnzc_si128(x, y)

// round

template <int32_t imm>
ML_INLINE __m128 emu_mm_round_ps(const __m128& x) {
    ML_StaticAssertMsg(imm != imm, "Unsupported rounding mode!");

    return _mm_setzero_ps();
}

#    undef _mm_round_ps
#    define _mm_round_ps(x, imm) emu_mm_round_ps<imm>(x)

template <>
ML_INLINE __m128 emu_mm_round_ps<_MM_FROUND_TO_NEAREST_INT | ML_ROUNDING_EXEPTIONS_MASK>(const __m128& x) {
    __m128 and0 = _mm_and_ps(_mm_castsi128_ps(_mm_set1_epi32(0x80000000)), x);
    __m128 or0 = _mm_or_ps(and0, _mm_set1_ps(8388608.0f));
    __m128 add0 = _mm_add_ps(x, or0);

    return _mm_sub_ps(add0, or0);
}

template <>
ML_INLINE __m128 emu_mm_round_ps<_MM_FROUND_FLOOR | ML_ROUNDING_EXEPTIONS_MASK>(const __m128& x) {
    __m128 rnd0 = _mm_round_ps(x, _MM_FROUND_TO_NEAREST_INT | ML_ROUNDING_EXEPTIONS_MASK);
    __m128 cmp0 = _mm_cmplt_ps(x, rnd0);
    __m128 and0 = _mm_and_ps(cmp0, _mm_set1_ps(1.0f));

    return _mm_sub_ps(rnd0, and0);
}

template <>
ML_INLINE __m128 emu_mm_round_ps<_MM_FROUND_CEIL | ML_ROUNDING_EXEPTIONS_MASK>(const __m128& x) {
    __m128 rnd0 = _mm_round_ps(x, _MM_FROUND_TO_NEAREST_INT | ML_ROUNDING_EXEPTIONS_MASK);
    __m128 cmp0 = _mm_cmpgt_ps(x, rnd0);
    __m128 and0 = _mm_and_ps(cmp0, _mm_set1_ps(1.0f));

    return _mm_add_ps(rnd0, and0);
}

template <int32_t imm>
ML_INLINE __m128d emu_mm_round_pd(const __m128d& x) {
    ML_StaticAssertMsg(imm != imm, "Unsupported rounding mode!");

    return _mm_setzero_pd();
}

#    undef _mm_round_pd
#    define _mm_round_pd(x, imm) emu_mm_round_pd<imm>(x)

template <>
ML_INLINE __m128d emu_mm_round_pd<_MM_FROUND_TO_NEAREST_INT | ML_ROUNDING_EXEPTIONS_MASK>(const __m128d& x) {
    __m128d and0 = _mm_and_pd(_mm_castsi128_pd(_mm_set_epi32(0x80000000, 0x00000000, 0x80000000, 0x00000000)), x);
    __m128d or0 = _mm_or_pd(and0, _mm_set1_pd(4503599627370496.0));
    __m128d add0 = _mm_add_pd(x, or0);

    return _mm_sub_pd(add0, or0);
}

template <>
ML_INLINE __m128d emu_mm_round_pd<_MM_FROUND_FLOOR | ML_ROUNDING_EXEPTIONS_MASK>(const __m128d& x) {
    __m128d rnd0 = _mm_round_pd(x, _MM_FROUND_TO_NEAREST_INT | ML_ROUNDING_EXEPTIONS_MASK);
    __m128d cmp0 = _mm_cmplt_pd(x, rnd0);
    __m128d and0 = _mm_and_pd(cmp0, _mm_set1_pd(1.0));

    return _mm_sub_pd(rnd0, and0);
}

template <>
ML_INLINE __m128d emu_mm_round_pd<_MM_FROUND_CEIL | ML_ROUNDING_EXEPTIONS_MASK>(const __m128d& x) {
    __m128d rnd0 = _mm_round_pd(x, _MM_FROUND_TO_NEAREST_INT | ML_ROUNDING_EXEPTIONS_MASK);
    __m128d cmp0 = _mm_cmpgt_pd(x, rnd0);
    __m128d and0 = _mm_and_pd(cmp0, _mm_set1_pd(1.0));

    return _mm_add_pd(rnd0, and0);
}

// dp

template <int32_t imm>
ML_INLINE __m128 emu_mm_dp_ps(const __m128& x, const __m128& y) {
    ML_StaticAssertMsg(imm != imm, "Unsupported dp mode!");

    return _mm_setzero_ps();
}

#    undef _mm_dp_ps
#    define _mm_dp_ps(x, y, imm) emu_mm_dp_ps<imm>(x, y)

template <>
ML_INLINE __m128 emu_mm_dp_ps<127>(const __m128& x, const __m128& y) {
    __m128 r = _mm_mul_ps(x, y);
    r = _mm_and_ps(r, _mm_castsi128_ps(_mm_setr_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000)));
    r = _mm_hadd_ps(r, r);
    r = _mm_hadd_ps(r, r);

    return r;
}

template <>
ML_INLINE __m128 emu_mm_dp_ps<255>(const __m128& x, const __m128& y) {
    __m128 r = _mm_mul_ps(x, y);
    r = _mm_hadd_ps(r, r);
    r = _mm_hadd_ps(r, r);

    return r;
}

// blendv

#    undef _mm_blendv_ps
#    undef _mm_blendv_pd
#    define _mm_blendv_ps(a, b, mask) _mm_xor_ps(a, _mm_and_ps(mask, _mm_xor_ps(b, a)))
#    define _mm_blendv_pd(a, b, mask) _mm_xor_pd(a, _mm_and_pd(mask, _mm_xor_pd(b, a)))

// blend

#    undef _mm_blend_ps
#    undef _mm_blend_pd
#    define _mm_blend_ps(a, b, mask) _mm_blendv_ps(a, b, _mm_castsi128_pd(_mm_set_epi64x(((uint64_t(mask) >> 1ull) & 0x1ull) ? -1ll : 0ll, (uint64_t(mask) & 0x1ull) ? -1ll : 0ll)))
#    define _mm_blend_pd(a, b, mask) _mm_blendv_pd(a, b, _mm_castsi128_pd(_mm_set_epi64x(((uint64_t(mask) >> 1ull) & 0x1ull) ? -1ll : 0ll, (uint64_t(mask) & 0x1ull) ? -1ll : 0ll)))

// mullo

ML_INLINE __m128i emu_mm_mullo_epi32(const __m128i& a, const __m128i& b) {
    __m128i c = _mm_mul_epu32(a, b);
    __m128i d = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));
    c = _mm_shuffle_epi32(c, _MM_SHUFFLE(0, 0, 2, 0));
    d = _mm_shuffle_epi32(d, _MM_SHUFFLE(0, 0, 2, 0));

    return _mm_unpacklo_epi32(c, d);
}

#    undef _mm_mullo_epi32
#    define _mm_mullo_epi32 emu_mm_mullo_epi32

// min

template <typename T>
ML_INLINE v4i emu_mm_min_epx(const v4i& x, const v4i& y) {
    const T* px = (T*)&x;
    const T* py = (T*)&y;

    v4i r;
    T* pr = (T*)&r;

    constexpr size_t N = sizeof(x) / sizeof(T);
    for (size_t i = 0; i < N; i++)
        pr[i] = px[i] < py[i] ? px[i] : py[i];

    return r;
}

#    undef _mm_min_epi32
#    define _mm_min_epi32(a, b) emu_mm_min_epx<int32_t>(a, b)

#    undef _mm_min_epu32
#    define _mm_min_epu32(a, b) emu_mm_min_epx<uint32_t>(a, b)

// max

template <typename T>
ML_INLINE v4i emu_mm_max_epx(const v4i& x, const v4i& y) {
    const T* px = (T*)&x;
    const T* py = (T*)&y;

    v4i r;
    T* pr = (T*)&r;

    constexpr size_t N = sizeof(x) / sizeof(T);
    for (size_t i = 0; i < N; i++)
        pr[i] = px[i] > py[i] ? px[i] : py[i];

    return r;
}

#    undef _mm_max_epi32
#    define _mm_max_epi32(a, b) emu_mm_max_epx<int32_t>(a, b)

#    undef _mm_max_epu32
#    define _mm_max_epu32(a, b) emu_mm_max_epx<uint32_t>(a, b)

#endif

//======================================================================================================================
// AVX
//======================================================================================================================

#if (ML_INTRINSIC_LEVEL < ML_INTRINSIC_AVX1)

#    define M256_ALIGN(a) alignas(a)

union M256_ALIGN(32) emu__m256 {
    __m128 emu_m128[2];
    float emu_arr[8];
};

union M256_ALIGN(32) emu__m256d {
    __m128d emu_m128[2];
    double emu_arr[4];
};

union M256_ALIGN(32) emu__m256i {
    __m128i emu_m128[2];
    int32_t emu_arr[8];
};

#    define __EMU_M256_IMPL_M1(type, func) \
        ML_INLINE emu##type emu_mm256_##func(const emu##type& m256_param1) { \
            emu##type res; \
            res.emu_m128[0] = _mm_##func(m256_param1.emu_m128[0]); \
            res.emu_m128[1] = _mm_##func(m256_param1.emu_m128[1]); \
            return (res); \
        }

#    define __EMU_M256_IMPL_M1_RET(ret_type, type, func) \
        ML_INLINE emu##ret_type emu_mm256_##func(const emu##type& m256_param1) { \
            emu##ret_type res; \
            res.emu_m128[0] = _mm_##func(m256_param1.emu_m128[0]); \
            res.emu_m128[1] = _mm_##func(m256_param1.emu_m128[1]); \
            return (res); \
        }

#    define __EMU_M256_IMPL_M1_RET_NAME(ret_type, type, func, name) \
        ML_INLINE emu##ret_type emu_mm256_##name(const emu##type& m256_param1) { \
            emu##ret_type res; \
            res.emu_m128[0] = _mm_##func(m256_param1.emu_m128[0]); \
            res.emu_m128[1] = _mm_##func(m256_param1.emu_m128[1]); \
            return (res); \
        }

#    define __EMU_M256_IMPL_M1_LH(type, type_128, func) \
        ML_INLINE emu##type emu_mm256_##func(const type_128& m128_param) { \
            emu##type res; \
            res.emu_m128[0] = _mm_##func(m128_param); \
            __m128 m128_param_high = _mm_movehl_ps(*(__m128*)&m128_param, *(__m128*)&m128_param); \
            res.emu_m128[1] = _mm_##func(*(type_128*)&m128_param_high); \
            return (res); \
        }

#    define __EMU_M256_IMPL_M1_HL(type_128, type, func) \
        ML_INLINE type_128 emu_mm256_##func(const emu##type& m256_param1) { \
            type_128 res, tmp; \
            res = _mm_##func(m256_param1.emu_m128[0]); \
            tmp = _mm_##func(m256_param1.emu_m128[1]); \
            *(((int64_t*)&res) + 1) = *(int64_t*)&tmp; \
            return (res); \
        }

#    define __EMU_M256_IMPL_M1P_DUP(type, type_param, func) \
        ML_INLINE emu##type emu_mm256_##func(const type_param& param) { \
            emu##type res; \
            res.emu_m128[0] = _mm_##func(param); \
            res.emu_m128[1] = _mm_##func(param); \
            return (res); \
        }

#    define __EMU_M256_IMPL2_M1I_SHIFT(type, func, shift_for_hi) \
        ML_INLINE emu##type emu_mm256_##func(const emu##type& m256_param1, const int32_t param2) { \
            emu##type res; \
            res.emu_m128[0] = emu_mm_##func(m256_param1.emu_m128[0], param2 & ((1 << shift_for_hi) - 1)); \
            res.emu_m128[1] = emu_mm_##func(m256_param1.emu_m128[1], param2 >> shift_for_hi); \
            return (res); \
        }

#    define __EMU_M256_IMPL_M2(type, func) \
        ML_INLINE emu##type emu_mm256_##func(const emu##type& m256_param1, const emu##type& m256_param2) { \
            emu##type res; \
            res.emu_m128[0] = _mm_##func(m256_param1.emu_m128[0], m256_param2.emu_m128[0]); \
            res.emu_m128[1] = _mm_##func(m256_param1.emu_m128[1], m256_param2.emu_m128[1]); \
            return (res); \
        }

#    define __EMU_M256_IMPL2_M2T(type, type_2, func) \
        ML_INLINE emu##type emu_mm256_##func(const emu##type& m256_param1, const emu##type_2& m256_param2) { \
            emu##type res; \
            res.emu_m128[0] = emu_mm_##func(m256_param1.emu_m128[0], m256_param2.emu_m128[0]); \
            res.emu_m128[1] = emu_mm_##func(m256_param1.emu_m128[1], m256_param2.emu_m128[1]); \
            return (res); \
        }

#    define __EMU_M256_IMPL_M3(type, func) \
        ML_INLINE emu##type emu_mm256_##func(const emu##type& m256_param1, const emu##type& m256_param2, const emu##type& m256_param3) { \
            emu##type res; \
            res.emu_m128[0] = _mm_##func(m256_param1.emu_m128[0], m256_param2.emu_m128[0], m256_param3.emu_m128[0]); \
            res.emu_m128[1] = _mm_##func(m256_param1.emu_m128[1], m256_param2.emu_m128[1], m256_param3.emu_m128[1]); \
            return (res); \
        }

__EMU_M256_IMPL_M2(__m256d, add_pd);
__EMU_M256_IMPL_M2(__m256, add_ps);

__EMU_M256_IMPL_M2(__m256d, addsub_pd);
__EMU_M256_IMPL_M2(__m256, addsub_ps);

__EMU_M256_IMPL_M2(__m256d, and_pd);
__EMU_M256_IMPL_M2(__m256, and_ps);

__EMU_M256_IMPL_M2(__m256d, andnot_pd);
__EMU_M256_IMPL_M2(__m256, andnot_ps);

__EMU_M256_IMPL_M2(__m256d, div_pd);
__EMU_M256_IMPL_M2(__m256, div_ps);

__EMU_M256_IMPL_M2(__m256d, hadd_pd);
__EMU_M256_IMPL_M2(__m256, hadd_ps);

__EMU_M256_IMPL_M2(__m256d, hsub_pd);
__EMU_M256_IMPL_M2(__m256, hsub_ps);

__EMU_M256_IMPL_M2(__m256d, max_pd);
__EMU_M256_IMPL_M2(__m256, max_ps);

__EMU_M256_IMPL_M2(__m256d, min_pd);
__EMU_M256_IMPL_M2(__m256, min_ps);

__EMU_M256_IMPL_M2(__m256d, mul_pd);
__EMU_M256_IMPL_M2(__m256, mul_ps);

__EMU_M256_IMPL_M2(__m256d, or_pd);
__EMU_M256_IMPL_M2(__m256, or_ps);

__EMU_M256_IMPL_M2(__m256d, sub_pd);
__EMU_M256_IMPL_M2(__m256, sub_ps);

__EMU_M256_IMPL_M2(__m256d, xor_pd);
__EMU_M256_IMPL_M2(__m256, xor_ps);

#    define emu_mm_permute_ps(a, control) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), control))

#    define emu_mm_shuffle_ps(x, y, imm) \
        emu__m256 { \
            { _mm_shuffle_ps(x.emu_128[0], y.emu_128[0], imm), _mm_shuffle_ps(x.emu_128[1], y.emu_128[1], imm) } \
        }

#    define emu_mm256_shuffle_pd(x, y, imm) \
        emu__m256d { \
            { _mm_shuffle_pd(x.emu_m128[0], y.emu_m128[0], imm), _mm_shuffle_pd(x.emu_m128[1], y.emu_m128[1], imm) } \
        }

#    define emu_mm256_permute_ps(m256_param1, param2) \
        emu__m256d { \
            { emu_mm_permute_ps(m256_param1.emu_m128[0], param2), emu_mm_permute_ps(m256_param1.emu_m128[1], param2) } \
        }

#    define emu_mm256_blend_pd(m256_param1, m256_param2, param3) \
        emu__m256d { \
            { \
                _mm_blend_pd(m256_param1.emu_m128[0], m256_param2.emu_m128[0], param3 & ((1 << 2) - 1)), \
                    _mm_blend_pd(m256_param1.emu_m128[1], m256_param2.emu_m128[1], param3 >> 2) \
            } \
        }

#    define emu_mm256_blend_ps(m256_param1, m256_param2, param3) \
        emu__m256 { \
            { \
                _mm_blend_ps(m256_param1.emu_m128[0], m256_param2.emu_m128[0], param3 & ((1 << 4) - 1)), \
                    _mm_blend_ps(m256_param1.emu_m128[1], m256_param2.emu_m128[1], param3 >> 4) \
            } \
        }

#    define emu_mm256_dp_ps(m256_param1, m256_param2, param3) \
        emu__m256 { \
            { _mm_dp_ps(m256_param1.emu_m128[0], m256_param2.emu_m128[0], param3), _mm_dp_ps(m256_param1.emu_m128[1], m256_param2.emu_m128[1], param3) } \
        }

#    define emu_mm256_round_pd(m256_param1, param2) \
        emu__m256d { \
            { _mm_round_pd(m256_param1.emu_m128[0], param2), _mm_round_pd(m256_param1.emu_m128[1], param2) } \
        }

#    define emu_mm256_round_ps(m256_param1, param2) \
        emu__m256 { \
            { _mm_round_ps(m256_param1.emu_m128[0], param2), _mm_round_ps(m256_param1.emu_m128[1], param2) } \
        }

__EMU_M256_IMPL_M3(__m256d, blendv_pd);
__EMU_M256_IMPL_M3(__m256, blendv_ps);

const __m128d sign_bits_pd = _mm_castsi128_pd(_mm_set_epi32(1 << 31, 0, 1 << 31, 0));
const __m128 sign_bits_ps = _mm_castsi128_ps(_mm_set1_epi32(1 << 31));

#    define emu_mm_test_impl(op, sfx, vec_type) \
        ML_INLINE int32_t emu_mm_test##op##_##sfx(const vec_type& s1, const vec_type& s2) { \
            vec_type t1 = _mm_and_##sfx(s1, sign_bits_##sfx); \
            vec_type t2 = _mm_and_##sfx(s2, sign_bits_##sfx); \
            return _mm_test##op##_si128(_mm_cast##sfx##_si128(t1), _mm_cast##sfx##_si128(t2)); \
        }

emu_mm_test_impl(z, pd, __m128d);
emu_mm_test_impl(c, pd, __m128d);
emu_mm_test_impl(nzc, pd, __m128d);

emu_mm_test_impl(z, ps, __m128);
emu_mm_test_impl(c, ps, __m128);
emu_mm_test_impl(nzc, ps, __m128);

#    define emu_mm256_test_impl(prfx, op, sfx, sfx_impl, vec_type) \
        ML_INLINE int32_t emu_mm256_test##op##_##sfx(const vec_type& s1, const vec_type& s2) { \
            int32_t ret1 = prfx##_test##op##_##sfx_impl(s1.emu_m128[0], s2.emu_m128[0]); \
            int32_t ret2 = prfx##_test##op##_##sfx_impl(s1.emu_m128[1], s2.emu_m128[1]); \
            return (ret1 && ret2); \
        };

emu_mm256_test_impl(_mm, z, si256, si128, emu__m256i);
emu_mm256_test_impl(_mm, c, si256, si128, emu__m256i);
emu_mm256_test_impl(_mm, nzc, si256, si128, emu__m256i);

emu_mm256_test_impl(emu_mm, z, pd, pd, emu__m256d);
emu_mm256_test_impl(emu_mm, c, pd, pd, emu__m256d);
emu_mm256_test_impl(emu_mm, nzc, pd, pd, emu__m256d);

emu_mm256_test_impl(emu_mm, z, ps, ps, emu__m256);
emu_mm256_test_impl(emu_mm, c, ps, ps, emu__m256);
emu_mm256_test_impl(emu_mm, nzc, ps, ps, emu__m256);

template <int32_t imm>
ML_INLINE __m128 emu_mm_cmp_ps(const __m128&, const __m128&) {
    return _mm_setzero_ps();
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_EQ_OQ>(const __m128& a, const __m128& b) {
    return _mm_cmpeq_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_EQ_UQ>(const __m128& a, const __m128& b) {
    return _mm_cmpeq_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_EQ_US>(const __m128& a, const __m128& b) {
    return _mm_cmpeq_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_EQ_OS>(const __m128& a, const __m128& b) {
    return _mm_cmpeq_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_LT_OS>(const __m128& a, const __m128& b) {
    return _mm_cmplt_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_LT_OQ>(const __m128& a, const __m128& b) {
    return _mm_cmplt_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_LE_OS>(const __m128& a, const __m128& b) {
    return _mm_cmple_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_LE_OQ>(const __m128& a, const __m128& b) {
    return _mm_cmple_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_GE_OS>(const __m128& a, const __m128& b) {
    return _mm_cmpge_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_GE_OQ>(const __m128& a, const __m128& b) {
    return _mm_cmpge_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_GT_OS>(const __m128& a, const __m128& b) {
    return _mm_cmpgt_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_GT_OQ>(const __m128& a, const __m128& b) {
    return _mm_cmpgt_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NEQ_UQ>(const __m128& a, const __m128& b) {
    return _mm_cmpneq_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NEQ_OQ>(const __m128& a, const __m128& b) {
    return _mm_cmpneq_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NEQ_US>(const __m128& a, const __m128& b) {
    return _mm_cmpneq_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NEQ_OS>(const __m128& a, const __m128& b) {
    return _mm_cmpneq_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NLT_US>(const __m128& a, const __m128& b) {
    return _mm_cmpnlt_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NLT_UQ>(const __m128& a, const __m128& b) {
    return _mm_cmpnlt_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NLE_US>(const __m128& a, const __m128& b) {
    return _mm_cmpnle_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NLE_UQ>(const __m128& a, const __m128& b) {
    return _mm_cmpnle_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NGT_US>(const __m128& a, const __m128& b) {
    return _mm_cmpngt_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NGT_UQ>(const __m128& a, const __m128& b) {
    return _mm_cmpngt_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NGE_US>(const __m128& a, const __m128& b) {
    return _mm_cmpnge_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_NGE_UQ>(const __m128& a, const __m128& b) {
    return _mm_cmpnge_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_ORD_Q>(const __m128& a, const __m128& b) {
    return _mm_cmpord_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_ORD_S>(const __m128& a, const __m128& b) {
    return _mm_cmpord_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_UNORD_Q>(const __m128& a, const __m128& b) {
    return _mm_cmpunord_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_UNORD_S>(const __m128& a, const __m128& b) {
    return _mm_cmpunord_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_TRUE_UQ>(const __m128& a, const __m128& b) {
    return _mm_cmpunord_ps(a, b);
}

template <>
ML_INLINE __m128 emu_mm_cmp_ps<_CMP_TRUE_US>(const __m128&, const __m128&) {
    return _mm_castsi128_ps(_mm_setr_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));
}

template <int32_t imm>
ML_INLINE __m128d emu_mm_cmp_pd(const __m128d&, const __m128d&) {
    return _mm_setzero_pd();
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_EQ_OQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpeq_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_EQ_UQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpeq_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_EQ_US>(const __m128d& a, const __m128d& b) {
    return _mm_cmpeq_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_EQ_OS>(const __m128d& a, const __m128d& b) {
    return _mm_cmpeq_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_LT_OS>(const __m128d& a, const __m128d& b) {
    return _mm_cmplt_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_LT_OQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmplt_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_LE_OS>(const __m128d& a, const __m128d& b) {
    return _mm_cmple_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_LE_OQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmple_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_GE_OS>(const __m128d& a, const __m128d& b) {
    return _mm_cmpge_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_GE_OQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpge_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_GT_OS>(const __m128d& a, const __m128d& b) {
    return _mm_cmpgt_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_GT_OQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpgt_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NEQ_UQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpneq_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NEQ_OQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpneq_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NEQ_US>(const __m128d& a, const __m128d& b) {
    return _mm_cmpneq_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NEQ_OS>(const __m128d& a, const __m128d& b) {
    return _mm_cmpneq_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NLT_US>(const __m128d& a, const __m128d& b) {
    return _mm_cmpnlt_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NLT_UQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpnlt_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NLE_US>(const __m128d& a, const __m128d& b) {
    return _mm_cmpnle_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NLE_UQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpnle_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NGT_US>(const __m128d& a, const __m128d& b) {
    return _mm_cmpngt_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NGT_UQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpngt_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NGE_US>(const __m128d& a, const __m128d& b) {
    return _mm_cmpnge_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_NGE_UQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpnge_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_ORD_Q>(const __m128d& a, const __m128d& b) {
    return _mm_cmpord_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_ORD_S>(const __m128d& a, const __m128d& b) {
    return _mm_cmpord_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_UNORD_Q>(const __m128d& a, const __m128d& b) {
    return _mm_cmpunord_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_UNORD_S>(const __m128d& a, const __m128d& b) {
    return _mm_cmpunord_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_TRUE_UQ>(const __m128d& a, const __m128d& b) {
    return _mm_cmpunord_pd(a, b);
}

template <>
ML_INLINE __m128d emu_mm_cmp_pd<_CMP_TRUE_US>(const __m128d&, const __m128d&) {
    return _mm_castsi128_pd(_mm_setr_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));
}

template <int imm8>
ML_INLINE emu__m256 emu_mm256_cmp_ps(emu__m256 a, emu__m256 b) {
    emu__m256 result;
    result.emu_m128[0] = emu_mm_cmp_ps<imm8>(a.emu_m128[0], b.emu_m128[0]);
    result.emu_m128[1] = emu_mm_cmp_ps<imm8>(a.emu_m128[1], b.emu_m128[1]);
    return result;
}

template <int imm8>
ML_INLINE emu__m256d emu_mm256_cmp_pd(emu__m256d a, emu__m256d b) {
    emu__m256d result;
    result.emu_m128[0] = emu_mm_cmp_pd<imm8>(a.emu_m128[0], b.emu_m128[0]);
    result.emu_m128[1] = emu_mm_cmp_pd<imm8>(a.emu_m128[1], b.emu_m128[1]);
    return result;
}

#    define emu_mm256_cmp_ps(a, b, imm8) emu_mm256_cmp_ps<imm8>(a, b)
#    define emu_mm256_cmp_pd(a, b, imm8) emu_mm256_cmp_pd<imm8>(a, b)

__EMU_M256_IMPL_M1_LH(__m256d, __m128i, cvtepi32_pd);
__EMU_M256_IMPL_M1_RET(__m256, __m256i, cvtepi32_ps);
__EMU_M256_IMPL_M1_HL(__m128, __m256d, cvtpd_ps);
__EMU_M256_IMPL_M1_RET(__m256i, __m256, cvtps_epi32);
__EMU_M256_IMPL_M1_LH(__m256d, __m128, cvtps_pd);
__EMU_M256_IMPL_M1_HL(__m128i, __m256d, cvttpd_epi32);
__EMU_M256_IMPL_M1_HL(__m128i, __m256d, cvtpd_epi32);
__EMU_M256_IMPL_M1_RET(__m256i, __m256, cvttps_epi32);

ML_INLINE __m128 emu_mm256_extractf128_ps(const emu__m256& m1, const int32_t offset) {
    return m1.emu_m128[offset];
}

ML_INLINE __m128d emu_mm256_extractf128_pd(const emu__m256d& m1, const int32_t offset) {
    return m1.emu_m128[offset];
}

ML_INLINE __m128i emu_mm256_extractf128_si256(const emu__m256i& m1, const int32_t offset) {
    return m1.emu_m128[offset];
}

ML_INLINE void emu_mm256_zeroall(void) {
}

ML_INLINE void emu_mm256_zeroupper(void) {
}

ML_INLINE __m128 emu_mm_permutevar_ps(const __m128& a, __m128i control) {
    int32_t const* sel = (int32_t const*)&control;
    float const* src = (float const*)&a;
    M256_ALIGN(16) float dest[4];

    for (size_t i = 0; i < 4; i++)
        dest[i] = src[3 & sel[i]];

    return (*(__m128*)dest);
}

__EMU_M256_IMPL2_M2T(__m256, __m256i, permutevar_ps);

ML_INLINE __m128d emu_mm_permutevar_pd(const __m128d& a, const __m128i& control) {
    int64_t const* sel = (int64_t const*)&control;
    double const* src = (double const*)&a;
    M256_ALIGN(16) double dest[2];

    for (size_t i = 0; i < 2; i++)
        dest[i] = src[(2 & sel[i]) >> 1];

    return (*(__m128d*)dest);
}

__EMU_M256_IMPL2_M2T(__m256d, __m256i, permutevar_pd);

ML_INLINE __m128d emu_mm_permute_pd(const __m128d& a, int32_t control) {
    double const* src = (double const*)&a;
    M256_ALIGN(16) double dest[2];

    for (size_t i = 0; i < 2; i++)
        dest[i] = src[1 & (control >> i)];

    return (*(__m128d*)dest);
}

__EMU_M256_IMPL2_M1I_SHIFT(__m256d, permute_pd, 2);

#    define emu_mm256_permute2f128_impl(name, m128_type, m256_type) \
        ML_INLINE m256_type name(const m256_type& m1, const m256_type& m2, int32_t control) { \
            m256_type res; \
            __m128 zero = _mm_setzero_ps(); \
            const m128_type param[4] = {m1.emu_m128[0], m1.emu_m128[1], m2.emu_m128[0], m2.emu_m128[1]}; \
            res.emu_m128[0] = (control & 8) ? *(m128_type*)&zero : param[control & 0x3]; \
            control >>= 4; \
            res.emu_m128[1] = (control & 8) ? *(m128_type*)&zero : param[control & 0x3]; \
            return (res); \
        }

emu_mm256_permute2f128_impl(emu_mm256_permute2f128_ps, __m128, emu__m256);
emu_mm256_permute2f128_impl(emu_mm256_permute2f128_pd, __m128d, emu__m256d);
emu_mm256_permute2f128_impl(emu_mm256_permute2f128_si256, __m128i, emu__m256i);

#    define emu_mm_broadcast_impl(name, res_type, type) \
        ML_INLINE res_type name(type const* a) { \
            const size_t size = sizeof(res_type) / sizeof(type); \
            M256_ALIGN(32) type res[size]; \
            for (size_t i = 0; i < size; i++) \
                res[i] = *a; \
            return (*(res_type*)&res); \
        }

emu_mm_broadcast_impl(emu_mm256_broadcast_ss, emu__m256, float);
emu_mm_broadcast_impl(emu_mm_broadcast_sd, __m128, double) emu_mm_broadcast_impl(emu_mm256_broadcast_sd, emu__m256d, double);
emu_mm_broadcast_impl(emu_mm256_broadcast_ps, emu__m256, __m128) emu_mm_broadcast_impl(emu_mm256_broadcast_pd, emu__m256d, __m128d);

ML_INLINE emu__m256 emu_mm256_insertf128_ps(const emu__m256& a, const __m128& b, int32_t offset) {
    emu__m256 t = a;
    t.emu_m128[offset] = b;
    return t;
}

ML_INLINE emu__m256d emu_mm256_insertf128_pd(const emu__m256d& a, const __m128d& b, int32_t offset) {
    emu__m256d t = a;
    t.emu_m128[offset] = b;
    return t;
}

ML_INLINE emu__m256i emu_mm256_insertf128_si256(const emu__m256i& a, const __m128i& b, int32_t offset) {
    emu__m256i t = a;
    t.emu_m128[offset] = b;
    return t;
}

#    define emu_mm_load_impl(name, sfx, m256_sfx, m256_type, type_128, type) \
        ML_INLINE emu##m256_type emu_mm256_##name##_##m256_sfx(const type* a) { \
            emu##m256_type res; \
            res.emu_m128[0] = _mm_##name##_##sfx((const type_128*)a); \
            res.emu_m128[1] = _mm_##name##_##sfx((const type_128*)(1 + (const __m128*)a)); \
            return (res); \
        }

#    define emu_mm_store_impl(name, sfx, m256_sfx, m256_type, type_128, type) \
        ML_INLINE void emu_mm256_##name##_##m256_sfx(type* a, const emu##m256_type& b) { \
            _mm_##name##_##sfx((type_128*)a, b.emu_m128[0]); \
            _mm_##name##_##sfx((type_128*)(1 + (__m128*)a), b.emu_m128[1]); \
        }

emu_mm_load_impl(load, pd, pd, __m256d, double, double);
emu_mm_store_impl(store, pd, pd, __m256d, double, double);

emu_mm_load_impl(load, ps, ps, __m256, float, float);
emu_mm_store_impl(store, ps, ps, __m256, float, float);

emu_mm_load_impl(loadu, pd, pd, __m256d, double, double);
emu_mm_store_impl(storeu, pd, pd, __m256d, double, double);

emu_mm_load_impl(loadu, ps, ps, __m256, float, float);
emu_mm_store_impl(storeu, ps, ps, __m256, float, float);

emu_mm_load_impl(load, si128, si256, __m256i, __m128i, emu__m256i);
emu_mm_store_impl(store, si128, si256, __m256i, __m128i, emu__m256i);

emu_mm_load_impl(loadu, si128, si256, __m256i, __m128i, emu__m256i);
emu_mm_store_impl(storeu, si128, si256, __m256i, __m128i, emu__m256i);

#    define emu_maskload_impl(name, vec_type, mask_vec_type, type, mask_type) \
        ML_INLINE vec_type name(type const* a, const mask_vec_type& mask) { \
            const size_t size_type = sizeof(type); \
            const size_t size = sizeof(vec_type) / size_type; \
            M256_ALIGN(32) type res[size]; \
            const mask_type* p_mask = (const mask_type*)&mask; \
            mask_type sign_bit = 1; \
            sign_bit <<= (8 * size_type - 1); \
            for (size_t i = 0; i < size; i++) \
                res[i] = (sign_bit & *(p_mask + i)) ? *(a + i) : 0; \
            return (*(vec_type*)&res); \
        }

#    define emu_maskstore_impl(name, vec_type, mask_vec_type, type, mask_type) \
        ML_INLINE void name(type* a, const mask_vec_type& mask, const vec_type& data) { \
            const size_t size_type = sizeof(type); \
            const size_t size = sizeof(vec_type) / sizeof(type); \
            type* p_data = (type*)&data; \
            const mask_type* p_mask = (const mask_type*)&mask; \
            mask_type sign_bit = 1; \
            sign_bit <<= (8 * size_type - 1); \
            for (size_t i = 0; i < size; i++) \
                if (*(p_mask + i) & sign_bit) \
                    *(a + i) = *(p_data + i); \
        }

emu_maskload_impl(emu_mm256_maskload_pd, emu__m256d, emu__m256i, double, int64_t);
emu_maskstore_impl(emu_mm256_maskstore_pd, emu__m256d, emu__m256i, double, int64_t);

emu_maskload_impl(emu_mm_maskload_pd, __m128d, __m128i, double, int64_t);
emu_maskstore_impl(emu_mm_maskstore_pd, __m128d, __m128i, double, int64_t);

emu_maskload_impl(emu_mm256_maskload_ps, emu__m256, emu__m256i, float, int32_t);
emu_maskstore_impl(emu_mm256_maskstore_ps, emu__m256, emu__m256i, float, int32_t);

emu_maskload_impl(emu_mm_maskload_ps, __m128, __m128i, float, int32_t);
emu_maskstore_impl(emu_mm_maskstore_ps, __m128, __m128i, float, int32_t);

__EMU_M256_IMPL_M1(__m256, movehdup_ps);
__EMU_M256_IMPL_M1(__m256, moveldup_ps);
__EMU_M256_IMPL_M1(__m256d, movedup_pd);

emu_mm_load_impl(lddqu, si128, si256, __m256i, __m128i, emu__m256i);

emu_mm_store_impl(stream, si128, si256, __m256i, __m128i, emu__m256i);
emu_mm_store_impl(stream, pd, pd, __m256d, double, double);
emu_mm_store_impl(stream, ps, ps, __m256, float, float);

__EMU_M256_IMPL_M1(__m256, rcp_ps);
__EMU_M256_IMPL_M1(__m256, rsqrt_ps);

__EMU_M256_IMPL_M1(__m256d, sqrt_pd);
__EMU_M256_IMPL_M1(__m256, sqrt_ps);

__EMU_M256_IMPL_M2(__m256d, unpackhi_pd);
__EMU_M256_IMPL_M2(__m256, unpackhi_ps);
__EMU_M256_IMPL_M2(__m256d, unpacklo_pd);
__EMU_M256_IMPL_M2(__m256, unpacklo_ps);

ML_INLINE int32_t emu_mm256_movemask_pd(const emu__m256d& a) {
    return (_mm_movemask_pd(a.emu_m128[1]) << 2) | _mm_movemask_pd(a.emu_m128[0]);
}

ML_INLINE int32_t emu_mm256_movemask_ps(const emu__m256& a) {
    return (_mm_movemask_ps(a.emu_m128[1]) << 4) | _mm_movemask_ps(a.emu_m128[0]);
}

ML_INLINE emu__m256d emu_mm256_setzero_pd(void) {
    return emu__m256d{{_mm_setzero_pd(), _mm_setzero_pd()}};
}

ML_INLINE emu__m256 emu_mm256_setzero_ps(void) {
    return emu__m256{{_mm_setzero_ps(), _mm_setzero_ps()}};
}

ML_INLINE emu__m256i emu_mm256_setzero_si256(void) {
    return emu__m256i{{_mm_setzero_si128(), _mm_setzero_si128()}};
}

ML_INLINE emu__m256d emu_mm256_set_pd(double a1, double a2, double a3, double a4) {
    return emu__m256d{{_mm_set_pd(a3, a4), _mm_set_pd(a1, a2)}};
}

ML_INLINE emu__m256 emu_mm256_set_ps(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
    return emu__m256{{_mm_set_ps(a5, a6, a7, a8), _mm_set_ps(a1, a2, a3, a4)}};
}

ML_INLINE emu__m256i emu_mm256_set_epi8(int8_t a1, int8_t a2, int8_t a3, int8_t a4, int8_t a5, int8_t a6, int8_t a7, int8_t a8, int8_t a9, int8_t a10, int8_t a11, int8_t a12,
    int8_t a13, int8_t a14, int8_t a15, int8_t a16, int8_t a17, int8_t a18, int8_t a19, int8_t a20, int8_t a21, int8_t a22, int8_t a23, int8_t a24, int8_t a25, int8_t a26,
    int8_t a27, int8_t a28, int8_t a29, int8_t a30, int8_t a31, int8_t a32) {
    return emu__m256i{{_mm_set_epi8(a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32),
        _mm_set_epi8(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)}};
}

ML_INLINE emu__m256i emu_mm256_set_epi16(int16_t a1, int16_t a2, int16_t a3, int16_t a4, int16_t a5, int16_t a6, int16_t a7, int16_t a8, int16_t a9, int16_t a10, int16_t a11,
    int16_t a12, int16_t a13, int16_t a14, int16_t a15, int16_t a16) {
    return emu__m256i{{_mm_set_epi16(a9, a10, a11, a12, a13, a14, a15, a16), _mm_set_epi16(a1, a2, a3, a4, a5, a6, a7, a8)}};
}

ML_INLINE emu__m256i emu_mm256_set_epi32(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8) {
    return emu__m256i{{_mm_set_epi32(a5, a6, a7, a8), _mm_set_epi32(a1, a2, a3, a4)}};
}

ML_INLINE __m128i emu_mm_set_epi64x(int64_t a, int64_t b) {
    return _mm_set_epi64x(a, b);
}

ML_INLINE emu__m256i emu_mm256_set_epi64x(int64_t a1, int64_t a2, int64_t a3, int64_t a4) {
    return emu__m256i{{emu_mm_set_epi64x(a3, a4), emu_mm_set_epi64x(a1, a2)}};
}

ML_INLINE emu__m256d emu_mm256_setr_pd(double a1, double a2, double a3, double a4) {
    return emu__m256d{{_mm_setr_pd(a1, a2), _mm_setr_pd(a3, a4)}};
}

ML_INLINE emu__m256 emu_mm256_setr_ps(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
    return emu__m256{{_mm_setr_ps(a1, a2, a3, a4), _mm_setr_ps(a5, a6, a7, a8)}};
}

ML_INLINE emu__m256i emu_mm256_setr_epi8(int8_t a1, int8_t a2, int8_t a3, int8_t a4, int8_t a5, int8_t a6, int8_t a7, int8_t a8, int8_t a9, int8_t a10, int8_t a11, int8_t a12,
    int8_t a13, int8_t a14, int8_t a15, int8_t a16, int8_t a17, int8_t a18, int8_t a19, int8_t a20, int8_t a21, int8_t a22, int8_t a23, int8_t a24, int8_t a25, int8_t a26,
    int8_t a27, int8_t a28, int8_t a29, int8_t a30, int8_t a31, int8_t a32) {
    return emu__m256i{{_mm_setr_epi8(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16),
        _mm_setr_epi8(a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32)}};
}

ML_INLINE emu__m256i emu_mm256_setr_epi16(int16_t a1, int16_t a2, int16_t a3, int16_t a4, int16_t a5, int16_t a6, int16_t a7, int16_t a8, int16_t a9, int16_t a10, int16_t a11,
    int16_t a12, int16_t a13, int16_t a14, int16_t a15, int16_t a16) {
    return emu__m256i{{_mm_setr_epi16(a1, a2, a3, a4, a5, a6, a7, a8), _mm_setr_epi16(a9, a10, a11, a12, a13, a14, a15, a16)}};
}

ML_INLINE emu__m256i emu_mm256_setr_epi32(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8) {
    return emu__m256i{{_mm_setr_epi32(a1, a2, a3, a4), _mm_setr_epi32(a5, a6, a7, a8)}};
}

ML_INLINE emu__m256i emu_mm256_setr_epi64x(int64_t a1, int64_t a2, int64_t a3, int64_t a4) {
    return emu__m256i{{emu_mm_set_epi64x(a2, a1), emu_mm_set_epi64x(a4, a3)}};
}

__EMU_M256_IMPL_M1P_DUP(__m256d, double, set1_pd);
__EMU_M256_IMPL_M1P_DUP(__m256, float, set1_ps);
__EMU_M256_IMPL_M1P_DUP(__m256i, int8_t, set1_epi8);
__EMU_M256_IMPL_M1P_DUP(__m256i, int16_t, set1_epi16);
__EMU_M256_IMPL_M1P_DUP(__m256i, int32_t, set1_epi32);

ML_INLINE emu__m256i emu_mm256_set1_epi64x(int64_t a) {
    int64_t res[4] = {a, a, a, a};
    return *((emu__m256i*)res);
}

/*
 * Support intrinsics to do vector type casts. These intrinsics do not introduce
 * extra moves to generated code. When cast is done from a 128 to 256-bit type
 * the low 128 bits of the 256-bit result contain source parameter value; the
 * upper 128 bits of the result are undefined
 */
__EMU_M256_IMPL_M1_RET(__m256, __m256d, castpd_ps);
__EMU_M256_IMPL_M1_RET(__m256d, __m256, castps_pd);

__EMU_M256_IMPL_M1_RET_NAME(__m256i, __m256, castps_si128, castps_si256);
__EMU_M256_IMPL_M1_RET_NAME(__m256i, __m256d, castpd_si128, castpd_si256);

__EMU_M256_IMPL_M1_RET_NAME(__m256, __m256i, castsi128_ps, castsi256_ps);
__EMU_M256_IMPL_M1_RET_NAME(__m256d, __m256i, castsi128_pd, castsi256_pd);

ML_INLINE __m128 emu_mm256_castps256_ps128(const emu__m256& a) {
    return (a.emu_m128[0]);
}

ML_INLINE __m128d emu_mm256_castpd256_pd128(const emu__m256d& a) {
    return (a.emu_m128[0]);
}

ML_INLINE __m128i emu_mm256_castsi256_si128(const emu__m256i& a) {
    return (a.emu_m128[0]);
}

ML_INLINE emu__m256 emu_mm256_castps128_ps256(const __m128& a) {
    return emu__m256{{a, _mm_setzero_ps()}};
}

ML_INLINE emu__m256d emu_mm256_castpd128_pd256(const __m128d& a) {
    return emu__m256d{{a, _mm_setzero_pd()}};
}

ML_INLINE emu__m256i emu_mm256_castsi128_si256(const __m128i& a) {
    return emu__m256i{{a, _mm_setzero_si128()}};
}

ML_INLINE double emu_mm256_cvtsd_f64(const emu__m256d& a) {
    return _mm_cvtsd_f64(a.emu_m128[0]);
}

#    undef _mm256_cvtsd_f64
#    define _mm256_cvtsd_f64 emu_mm256_cvtsd_f64

#    undef _mm256_add_pd
#    undef _mm256_add_ps
#    define _mm256_add_pd emu_mm256_add_pd
#    define _mm256_add_ps emu_mm256_add_ps

#    undef _mm256_addsub_pd
#    undef _mm256_addsub_ps
#    define _mm256_addsub_pd emu_mm256_addsub_pd
#    define _mm256_addsub_ps emu_mm256_addsub_ps

#    undef _mm256_and_pd
#    undef _mm256_and_ps
#    define _mm256_and_pd emu_mm256_and_pd
#    define _mm256_and_ps emu_mm256_and_ps

#    undef _mm256_andnot_pd
#    undef _mm256_andnot_ps
#    define _mm256_andnot_pd emu_mm256_andnot_pd
#    define _mm256_andnot_ps emu_mm256_andnot_ps

#    undef _mm256_blend_pd
#    undef _mm256_blend_ps
#    define _mm256_blend_pd emu_mm256_blend_pd
#    define _mm256_blend_ps emu_mm256_blend_ps

#    undef _mm256_blendv_pd
#    undef _mm256_blendv_ps
#    define _mm256_blendv_pd emu_mm256_blendv_pd
#    define _mm256_blendv_ps emu_mm256_blendv_ps

#    undef _mm256_div_pd
#    undef _mm256_div_ps
#    define _mm256_div_pd emu_mm256_div_pd
#    define _mm256_div_ps emu_mm256_div_ps

#    undef _mm256_dp_ps
#    define _mm256_dp_ps emu_mm256_dp_ps

#    undef _mm256_hadd_pd
#    undef _mm256_hadd_ps
#    define _mm256_hadd_pd emu_mm256_hadd_pd
#    define _mm256_hadd_ps emu_mm256_hadd_ps

#    undef _mm256_hsub_pd
#    undef _mm256_hsub_ps
#    define _mm256_hsub_pd emu_mm256_hsub_pd
#    define _mm256_hsub_ps emu_mm256_hsub_ps

#    undef _mm256_max_pd
#    undef _mm256_max_ps
#    define _mm256_max_pd emu_mm256_max_pd
#    define _mm256_max_ps emu_mm256_max_ps

#    undef _mm256_min_pd
#    undef _mm256_min_ps
#    define _mm256_min_pd emu_mm256_min_pd
#    define _mm256_min_ps emu_mm256_min_ps

#    undef _mm256_mul_pd
#    undef _mm256_mul_ps
#    define _mm256_mul_pd emu_mm256_mul_pd
#    define _mm256_mul_ps emu_mm256_mul_ps

#    undef _mm256_or_pd
#    undef _mm256_or_ps
#    define _mm256_or_pd emu_mm256_or_pd
#    define _mm256_or_ps emu_mm256_or_ps

#    undef _mm256_shuffle_pd
#    undef _mm256_shuffle_ps
#    define _mm256_shuffle_pd emu_mm256_shuffle_pd
#    define _mm256_shuffle_ps emu_mm256_shuffle_ps

#    undef _mm256_sub_pd
#    undef _mm256_sub_ps
#    define _mm256_sub_pd emu_mm256_sub_pd
#    define _mm256_sub_ps emu_mm256_sub_ps

#    undef _mm256_xor_pd
#    undef _mm256_xor_ps
#    define _mm256_xor_pd emu_mm256_xor_pd
#    define _mm256_xor_ps emu_mm256_xor_ps

#    undef _mm_cmp_pd
#    undef _mm256_cmp_pd
#    define _mm_cmp_pd(a, b, imm) emu_mm_cmp_pd<imm>(a, b)
#    define _mm256_cmp_pd emu_mm256_cmp_pd

#    undef _mm_cmp_ps
#    undef _mm256_cmp_ps
#    define _mm_cmp_ps(a, b, imm) emu_mm_cmp_ps<imm>(a, b)
#    define _mm256_cmp_ps emu_mm256_cmp_ps

#    undef _mm256_cvtepi32_pd
#    undef _mm256_cvtepi32_ps
#    define _mm256_cvtepi32_pd emu_mm256_cvtepi32_pd
#    define _mm256_cvtepi32_ps emu_mm256_cvtepi32_ps

#    undef _mm256_cvtpd_ps
#    undef _mm256_cvtps_epi32
#    undef _mm256_cvtps_pd
#    define _mm256_cvtpd_ps emu_mm256_cvtpd_ps
#    define _mm256_cvtps_epi32 emu_mm256_cvtps_epi32
#    define _mm256_cvtps_pd emu_mm256_cvtps_pd

#    undef _mm256_cvttpd_epi32
#    undef _mm256_cvtpd_epi32
#    undef _mm256_cvttps_epi32
#    define _mm256_cvttpd_epi32 emu_mm256_cvttpd_epi32
#    define _mm256_cvtpd_epi32 emu_mm256_cvtpd_epi32
#    define _mm256_cvttps_epi32 emu_mm256_cvttps_epi32

#    undef _mm256_extractf128_ps
#    undef _mm256_extractf128_pd
#    undef _mm256_extractf128_si256
#    define _mm256_extractf128_ps emu_mm256_extractf128_ps
#    define _mm256_extractf128_pd emu_mm256_extractf128_pd
#    define _mm256_extractf128_si256 emu_mm256_extractf128_si256

#    undef _mm256_zeroall
#    undef _mm256_zeroupper
#    define _mm256_zeroall emu_mm256_zeroall
#    define _mm256_zeroupper emu_mm256_zeroupper

#    undef _mm256_permutevar_ps
#    undef _mm_permutevar_ps
#    define _mm256_permutevar_ps emu_mm256_permutevar_ps
#    define _mm_permutevar_ps emu_mm_permutevar_ps

#    undef _mm256_permute_ps
#    undef _mm_permute_ps
#    define _mm256_permute_ps emu_mm256_permute_ps
#    define _mm_permute_ps emu_mm_permute_ps

#    undef _mm256_permutevar_pd
#    undef _mm_permutevar_pd
#    define _mm256_permutevar_pd emu_mm256_permutevar_pd
#    define _mm_permutevar_pd emu_mm_permutevar_pd

#    undef _mm256_permute_pd
#    undef _mm_permute_pd
#    define _mm256_permute_pd emu_mm256_permute_pd
#    define _mm_permute_pd emu_mm_permute_pd

#    undef _mm256_permute2f128_ps
#    undef _mm256_permute2f128_pd
#    undef _mm256_permute2f128_si256
#    define _mm256_permute2f128_ps emu_mm256_permute2f128_ps
#    define _mm256_permute2f128_pd emu_mm256_permute2f128_pd
#    define _mm256_permute2f128_si256 emu_mm256_permute2f128_si256

#    undef _mm256_broadcast_ss
#    undef _mm_broadcast_ss
#    define _mm256_broadcast_ss emu_mm256_broadcast_ss
#    define _mm_broadcast_ss(x) _mm_set1_ps(*(x))

#    undef _mm256_broadcast_sd
// #define _mm256_broadcast_sd(x) emu_mm256_set1_pd(*(x))
#    define _mm256_broadcast_sd emu_mm256_broadcast_sd

#    undef _mm256_broadcast_ps
#    undef _mm256_broadcast_pd
#    define _mm256_broadcast_ps emu_mm256_broadcast_ps
#    define _mm256_broadcast_pd emu_mm256_broadcast_pd

#    undef _mm256_insertf128_ps
#    undef _mm256_insertf128_pd
#    undef _mm256_insertf128_si256
#    define _mm256_insertf128_ps emu_mm256_insertf128_ps
#    define _mm256_insertf128_pd emu_mm256_insertf128_pd
#    define _mm256_insertf128_si256 emu_mm256_insertf128_si256

#    undef _mm256_load_pd
#    undef _mm256_store_pd
#    undef _mm256_load_ps
#    undef _mm256_store_ps
#    define _mm256_load_pd emu_mm256_load_pd
#    define _mm256_store_pd emu_mm256_store_pd
#    define _mm256_load_ps emu_mm256_load_ps
#    define _mm256_store_ps emu_mm256_store_ps

#    undef _mm256_loadu_pd
#    undef _mm256_storeu_pd
#    undef _mm256_loadu_ps
#    undef _mm256_storeu_ps
#    define _mm256_loadu_pd emu_mm256_loadu_pd
#    define _mm256_storeu_pd emu_mm256_storeu_pd
#    define _mm256_loadu_ps emu_mm256_loadu_ps
#    define _mm256_storeu_ps emu_mm256_storeu_ps

#    undef _mm256_load_si256
#    undef _mm256_store_si256
#    undef _mm256_loadu_si256
#    undef _mm256_storeu_si256
#    define _mm256_load_si256 emu_mm256_load_si256
#    define _mm256_store_si256 emu_mm256_store_si256
#    define _mm256_loadu_si256 emu_mm256_loadu_si256
#    define _mm256_storeu_si256 emu_mm256_storeu_si256

#    undef _mm256_maskload_pd
#    undef _mm256_maskstore_pd
#    undef _mm_maskload_pd
#    undef _mm_maskstore_pd
#    define _mm256_maskload_pd emu_mm256_maskload_pd
#    define _mm256_maskstore_pd emu_mm256_maskstore_pd
#    define _mm_maskload_pd emu_mm_maskload_pd
#    define _mm_maskstore_pd emu_mm_maskstore_pd

#    undef _mm256_maskload_ps
#    undef _mm256_maskstore_ps
#    undef _mm_maskload_ps
#    undef _mm_maskstore_ps
#    define _mm256_maskload_ps emu_mm256_maskload_ps
#    define _mm256_maskstore_ps emu_mm256_maskstore_ps
#    define _mm_maskload_ps emu_mm_maskload_ps
#    define _mm_maskstore_ps emu_mm_maskstore_ps

#    undef _mm256_movehdup_ps
#    undef _mm256_moveldup_ps
#    define _mm256_movehdup_ps emu_mm256_movehdup_ps
#    define _mm256_moveldup_ps emu_mm256_moveldup_ps

#    undef _mm256_movedup_pd
#    undef _mm256_lddqu_si256
#    define _mm256_movedup_pd emu_mm256_movedup_pd
#    define _mm256_lddqu_si256 emu_mm256_lddqu_si256

#    undef _mm256_stream_si256
#    undef _mm256_stream_pd
#    undef _mm256_stream_ps
#    define _mm256_stream_si256 emu_mm256_stream_si256
#    define _mm256_stream_pd emu_mm256_stream_pd
#    define _mm256_stream_ps emu_mm256_stream_ps

#    undef _mm256_rcp_ps
#    undef _mm256_rsqrt_ps
#    define _mm256_rcp_ps emu_mm256_rcp_ps
#    define _mm256_rsqrt_ps emu_mm256_rsqrt_ps

#    undef _mm256_sqrt_pd
#    undef _mm256_sqrt_ps
#    define _mm256_sqrt_pd emu_mm256_sqrt_pd
#    define _mm256_sqrt_ps emu_mm256_sqrt_ps

#    undef _mm256_round_pd
#    define _mm256_round_pd emu_mm256_round_pd

#    undef _mm256_round_ps
#    define _mm256_round_ps emu_mm256_round_ps

#    undef _mm256_unpackhi_pd
#    undef _mm256_unpackhi_ps
#    define _mm256_unpackhi_pd emu_mm256_unpackhi_pd
#    define _mm256_unpackhi_ps emu_mm256_unpackhi_ps

#    undef _mm256_unpacklo_pd
#    undef _mm256_unpacklo_ps
#    define _mm256_unpacklo_pd emu_mm256_unpacklo_pd
#    define _mm256_unpacklo_ps emu_mm256_unpacklo_ps

#    undef _mm256_testz_si256
#    undef _mm256_testc_si256
#    undef _mm256_testnzc_si256
#    define _mm256_testz_si256 emu_mm256_testz_si256
#    define _mm256_testc_si256 emu_mm256_testc_si256
#    define _mm256_testnzc_si256 emu_mm256_testnzc_si256

#    undef _mm256_testz_pd
#    undef _mm256_testc_pd
#    undef _mm256_testnzc_pd
#    undef _mm_testz_pd
#    undef _mm_testc_pd
#    undef _mm_testnzc_pd
#    define _mm256_testz_pd emu_mm256_testz_pd
#    define _mm256_testc_pd emu_mm256_testc_pd
#    define _mm256_testnzc_pd emu_mm256_testnzc_pd
#    define _mm_testz_pd emu_mm_testz_pd
#    define _mm_testc_pd emu_mm_testc_pd
#    define _mm_testnzc_pd emu_mm_testnzc_pd

#    undef _mm256_testz_ps
#    undef _mm256_testc_ps
#    undef _mm256_testnzc_ps
#    undef _mm_testz_ps
#    undef _mm_testc_ps
#    undef _mm_testnzc_ps
#    define _mm256_testz_ps emu_mm256_testz_ps
#    define _mm256_testc_ps emu_mm256_testc_ps
#    define _mm256_testnzc_ps emu_mm256_testnzc_ps
#    define _mm_testz_ps emu_mm_testz_ps
#    define _mm_testc_ps emu_mm_testc_ps
#    define _mm_testnzc_ps emu_mm_testnzc_ps

#    undef _mm256_movemask_pd
#    undef _mm256_movemask_ps
#    define _mm256_movemask_pd emu_mm256_movemask_pd
#    define _mm256_movemask_ps emu_mm256_movemask_ps

#    undef _mm256_setzero_pd
#    undef _mm256_setzero_ps
#    undef _mm256_setzero_si256
#    define _mm256_setzero_pd emu_mm256_setzero_pd
#    define _mm256_setzero_ps emu_mm256_setzero_ps
#    define _mm256_setzero_si256 emu_mm256_setzero_si256

#    undef _mm256_set_pd
#    undef _mm256_set_ps
#    undef _mm256_set_epi8
#    undef _mm256_set_epi16
#    undef _mm256_set_epi32
#    undef _mm256_set_epi64x
#    define _mm256_set_pd emu_mm256_set_pd
#    define _mm256_set_ps emu_mm256_set_ps
#    define _mm256_set_epi8 emu_mm256_set_epi8
#    define _mm256_set_epi16 emu_mm256_set_epi16
#    define _mm256_set_epi32 emu_mm256_set_epi32
#    define _mm256_set_epi64x emu_mm256_set_epi64x

#    undef _mm256_setr_pd
#    undef _mm256_setr_ps
#    undef _mm256_setr_epi8
#    undef _mm256_setr_epi16
#    undef _mm256_setr_epi32
#    undef _mm256_setr_epi64x
#    define _mm256_setr_pd emu_mm256_setr_pd
#    define _mm256_setr_ps emu_mm256_setr_ps
#    define _mm256_setr_epi8 emu_mm256_setr_epi8
#    define _mm256_setr_epi16 emu_mm256_setr_epi16
#    define _mm256_setr_epi32 emu_mm256_setr_epi32
#    define _mm256_setr_epi64x emu_mm256_setr_epi64x

#    undef _mm256_set1_pd
#    undef _mm256_set1_ps
#    undef _mm256_set1_epi8
#    undef _mm256_set1_epi16
#    undef _mm256_set1_epi32
#    undef _mm256_set1_epi64x
#    define _mm256_set1_pd emu_mm256_set1_pd
#    define _mm256_set1_ps emu_mm256_set1_ps
#    define _mm256_set1_epi8 emu_mm256_set1_epi8
#    define _mm256_set1_epi16 emu_mm256_set1_epi16
#    define _mm256_set1_epi32 emu_mm256_set1_epi32
#    define _mm256_set1_epi64x emu_mm256_set1_epi64x

#    undef _mm256_castpd_ps
#    undef _mm256_castps_pd
#    undef _mm256_castps_si256
#    undef _mm256_castpd_si256
#    undef _mm256_castsi256_ps
#    undef _mm256_castsi256_pd
#    undef _mm256_castps256_ps128
#    undef _mm256_castpd256_pd128
#    undef _mm256_castsi256_si128
#    undef _mm256_castps128_ps256
#    undef _mm256_castpd128_pd256
#    undef _mm256_castsi128_si256
#    define _mm256_castpd_ps emu_mm256_castpd_ps
#    define _mm256_castps_pd emu_mm256_castps_pd
#    define _mm256_castps_si256 emu_mm256_castps_si256
#    define _mm256_castpd_si256 emu_mm256_castpd_si256
#    define _mm256_castsi256_ps emu_mm256_castsi256_ps
#    define _mm256_castsi256_pd emu_mm256_castsi256_pd
#    define _mm256_castps256_ps128 emu_mm256_castps256_ps128
#    define _mm256_castpd256_pd128 emu_mm256_castpd256_pd128
#    define _mm256_castsi256_si128 emu_mm256_castsi256_si128
#    define _mm256_castps128_ps256 emu_mm256_castps128_ps256
#    define _mm256_castpd128_pd256 emu_mm256_castpd128_pd256
#    define _mm256_castsi128_si256 emu_mm256_castsi128_si256

typedef emu__m256 v8f;
typedef emu__m256i v8i;
typedef emu__m256d v4d;

#else

typedef __m256 v8f;
typedef __m256i v8i;
typedef __m256d v4d;

#endif

//======================================================================================================================
// AVX2
//======================================================================================================================

#if (ML_INTRINSIC_LEVEL < ML_INTRINSIC_AVX2)

ML_INLINE v4d emu_mm256_permute4x64_pd(const v4d& x, int32_t imm) {
    ML_Assert(imm >= 0 && imm <= 0xFF);

    v4d r;
    const double* src = (double*)&x;
    double* dst = (double*)&r;

    dst[0] = src[(imm >> 0) & 0x3];
    dst[1] = src[(imm >> 2) & 0x3];
    dst[2] = src[(imm >> 4) & 0x3];
    dst[3] = src[(imm >> 6) & 0x3];

    return r;
}

ML_INLINE __m128i emu_mm_sllv_epi32(const __m128i& x, const __m128i& y) {
    const uint32_t* px = (uint32_t*)&x;
    const uint32_t* py = (uint32_t*)&y;

    __m128i r;
    uint32_t* pr = (uint32_t*)&r;

    constexpr size_t N = sizeof(x) / sizeof(uint32_t);
    for (size_t i = 0; i < N; i++)
        pr[i] = px[i] << py[i];

    return r;
}

ML_INLINE __m128i emu_mm_srlv_epi32(const __m128i& x, const __m128i& y) {
    const uint32_t* px = (uint32_t*)&x;
    const uint32_t* py = (uint32_t*)&y;

    __m128i r;
    uint32_t* pr = (uint32_t*)&r;

    constexpr size_t N = sizeof(x) / sizeof(uint32_t);
    for (size_t i = 0; i < N; i++)
        pr[i] = px[i] >> py[i];

    return r;
}

#    undef _mm_sllv_epi32
#    define _mm_sllv_epi32(a, b) emu_mm_sllv_epi32(a, b)

#    undef _mm_srlv_epi32
#    define _mm_srlv_epi32(a, b) emu_mm_srlv_epi32(a, b)

#    undef _mm256_cvtepi32_epi64
#    undef _mm256_permute4x64_pd
#    define _mm256_cvtepi32_epi64(a) _mm256_castpd_si256(_mm256_cmp_pd(_mm256_cvtepi32_pd(_mm_and_si128(a, _mm_set1_epi32(1))), _mm256_set1_pd(1.0), _CMP_EQ_OQ))
#    define _mm256_permute4x64_pd emu_mm256_permute4x64_pd

#    undef _mm_fmadd_ps
#    undef _mm_fmsub_ps
#    undef _mm_fnmadd_ps
#    define _mm_fmadd_ps(a, b, c) _mm_add_ps(_mm_mul_ps(a, b), c)
#    define _mm_fmsub_ps(a, b, c) _mm_sub_ps(_mm_mul_ps(a, b), c)
#    define _mm_fnmadd_ps(a, b, c) _mm_sub_ps(c, _mm_mul_ps(a, b))

#    undef _mm256_fmadd_pd
#    undef _mm256_fmsub_pd
#    undef _mm256_fnmadd_pd
#    define _mm256_fmadd_pd(a, b, c) _mm256_add_pd(_mm256_mul_pd(a, b), c)
#    define _mm256_fmsub_pd(a, b, c) _mm256_sub_pd(_mm256_mul_pd(a, b), c)
#    define _mm256_fnmadd_pd(a, b, c) _mm256_sub_pd(c, _mm256_mul_pd(a, b))

#endif
