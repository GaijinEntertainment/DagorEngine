//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if !defined(_TARGET_PC_LINUX) && !defined(_TARGET_PC_MACOSX) && !defined(_TARGET_PC_WIN)\
 && !defined(_TARGET_C1) && !defined(_TARGET_C2)  && !defined(_TARGET_XBOX) && !defined(_TARGET_PC) && !defined(_TARGET_ANDROID)
  #if __linux__ || __unix__
    #define _TARGET_PC_LINUX 1
  #elif __APPLE__
    #define _TARGET_PC_MACOSX 1
  #elif _WIN32
    #define _TARGET_PC_WIN 1
    #if _WIN64
      #define TARGET_64BIT 1
    #endif
  #endif
#endif

#include <math.h> //for fabsf, which is used once, and not wise
#if defined(_EMSCRIPTEN_VER)
#include <immintrin.h>
#elif _TARGET_PC_LINUX
#include <x86intrin.h> // MAC doesn't have it in GCC frontend, but it exist in CLANG one
#elif _TARGET_SIMD_SSE >= 4 || defined(_DAGOR_PROJECT_OPTIONAL_SSE4)
#include <smmintrin.h>
#else
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#endif
#if defined(__FMA__) || defined(__AVX__) || defined(__AVX2__)
#include <immintrin.h>
#endif
#if (defined(__FMA__) || defined(__AVX2__)) && defined(__GNUC__)
#include <fmaintrin.h>
#endif

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable: 4714) //function marked as __forceinline not inlined

  #if _MSC_VER < 1500
  VECTORCALL VECMATH_FINLINE __m128i _mm_castps_si128(__m128  v) { return *(__m128i*)&v; }
  VECTORCALL VECMATH_FINLINE __m128  _mm_castsi128_ps(__m128i v) { return *(__m128 *)&v; }
  #endif
#endif

VECTORCALL VECMATH_FINLINE vec4f v_zero() { return _mm_setzero_ps(); }
VECTORCALL VECMATH_FINLINE vec4i v_zeroi() { return _mm_setzero_si128(); }
VECTORCALL VECMATH_FINLINE vec4f v_set_all_bits() { vec4f u = _mm_undefined_ps(); return v_cmp_eq(u, u); }
VECTORCALL VECMATH_FINLINE vec4i v_set_all_bitsi() { vec4i u = _mm_undefined_si128(); return v_cmp_eqi(u, u); }
VECTORCALL VECMATH_FINLINE vec4f v_msbit() { return (const vec4f&)V_CI_SIGN_MASK; }
VECTORCALL VECMATH_FINLINE vec4f v_splats(float a) {return _mm_set1_ps(a);}//_mm_set_ps1(a) is slower...
VECTORCALL VECMATH_FINLINE vec4i v_splatsi(int a) {return _mm_set1_epi32(a);}
VECTORCALL VECMATH_FINLINE vec4f v_set_x(float a) {return _mm_set_ss(a);} // set x, zero others
VECTORCALL VECMATH_FINLINE vec4i v_seti_x(int a) {return _mm_cvtsi32_si128(a);} // set x, zero others


#if defined(DAGOR_ASAN_ENABLED) && defined(__GNUC__) && __GNUC__ >= 7
NO_ASAN_INLINE vec4f v_ld(const float *m) { return  *(__m128 *)m; }
NO_ASAN_INLINE vec4f v_ldu(const float *m) { return *(__m128_u *)m; }
NO_ASAN_INLINE vec4i v_ldi(const int *m) { return  *(__m128i *)m; }
NO_ASAN_INLINE vec4i v_ldui(const int *m) { return *(__m128i_u *)m; }
NO_ASAN_INLINE vec4f v_ldu_x(const float *m) { union { float x; vec4f vec; } mm{}; mm.x = *m; return mm.vec; } // load x, zero others
#else
NO_ASAN_INLINE vec4f v_ld(const float *m) { return _mm_load_ps(m); }
NO_ASAN_INLINE vec4f v_ldu(const float *m) { return _mm_loadu_ps(m); }
NO_ASAN_INLINE vec4i v_ldi(const int *m) { return  _mm_load_si128((const vec4i*)m); }
NO_ASAN_INLINE vec4i v_ldui(const int *m) { return _mm_loadu_si128((const vec4i*)m); }
NO_ASAN_INLINE vec4f v_ldu_x(const float *m) { return _mm_load_ss(m); } // load x, zero others
#endif

// Always safe loading of float[3], but it uses [one more register (on SSE2) and] one more memory read (slower)
#if _TARGET_SIMD_SSE >= 4
NO_ASAN_INLINE vec3f v_ldu_p3_safe(const float *m) { return _mm_insert_ps(v_ldu_half(m), v_ldu_x(m + 2), _MM_MK_INSERTPS_NDX(0, 2, 1 << 3)); }
NO_ASAN_INLINE vec4i v_ldui_p3_safe(const int *m) { return _mm_insert_epi32(v_ldui_half(m), m[2], 2); }
#else
NO_ASAN_INLINE vec3f v_ldu_p3_safe(const float *m) { return _mm_movelh_ps(v_ldu_half(m), v_ldu_x(m + 2)); }
NO_ASAN_INLINE vec4i v_ldui_p3_safe(const int *m) { return _mm_unpacklo_epi64(v_ldui_half(m), v_seti_x(m[2])); }
#endif

VECTORCALL VECMATH_FINLINE vec4i v_ldush(const signed short *m)
{
  vec4i h = _mm_loadl_epi64((__m128i const*)m);
  vec4i sx = _mm_cmplt_epi16(h, _mm_setzero_si128());
  return _mm_unpacklo_epi16(h, sx);
}
VECTORCALL VECMATH_FINLINE vec4i v_lduush(const unsigned short *m)
{
  vec4i h = _mm_loadl_epi64((__m128i const*)m);
  return _mm_unpacklo_epi16(h, _mm_setzero_si128());
}

VECTORCALL VECMATH_FINLINE vec4i v_ldui_half(const void *m) { return _mm_loadl_epi64((__m128i const*)m); }
VECTORCALL VECMATH_FINLINE vec4f v_ldu_half(const void *m) { return v_cast_vec4f(v_ldui_half(m)); }
VECMATH_FINLINE void v_prefetch(const void *m) { _mm_prefetch((const char *)m, _MM_HINT_T0); }

VECTORCALL VECMATH_FINLINE vec4i v_cvt_lo_ush_vec4i(vec4i a) { return _mm_unpacklo_epi16(a, _mm_setzero_si128()); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_hi_ush_vec4i(vec4i a) { return _mm_unpackhi_epi16(a, _mm_setzero_si128()); }
VECTORCALL VECMATH_FINLINE vec4i
  v_cvt_lo_ssh_vec4i(vec4i a) { vec4i sx = _mm_cmplt_epi16(a, _mm_setzero_si128()); return _mm_unpacklo_epi16(a, sx); }
VECTORCALL VECMATH_FINLINE vec4i
  v_cvt_hi_ssh_vec4i(vec4i a) { vec4i sx = _mm_cmplt_epi16(a, _mm_setzero_si128()); return _mm_unpackhi_epi16(a, sx); }

VECTORCALL VECMATH_FINLINE vec4i v_cvt_byte_vec4i(uint32_t a)
{
#if _TARGET_SIMD_SSE >= 4
  return _mm_cvtepu8_epi32(v_seti_x(a));
#else
  vec4i u16 = _mm_unpacklo_epi8(v_seti_x(a), _mm_setzero_si128());
  return v_cvt_lo_ush_vec4i(u16);
#endif
}

VECTORCALL VECMATH_FINLINE vec4f v_make_vec4f(float x, float y, float z, float w)
{ return _mm_setr_ps(x, y, z, w); }

VECTORCALL VECMATH_FINLINE vec4i v_make_vec4i(int x, int y, int z, int w)
{ return _mm_setr_epi32(x, y, z, w); }

VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(float x, float y, float z)
{ return _mm_setr_ps(x, y, z, z); }

VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(vec4f x, vec4f y, vec4f z)
{ return _mm_shuffle_ps(_mm_shuffle_ps(x, y, _MM_SHUFFLE(0, 0, 0, 0)), z, _MM_SHUFFLE(0, 0, 2, 0)); }

//VECTORCALL VECMATH_FINLINE vec4f v_perm_mask(){ _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), mask)); }
#define V_SHUFFLE(v, mask) _mm_shuffle_ps(v, v, mask)
#define V_SHUFFLE_REV(v, maskW, maskZ, maskY, maskX) V_SHUFFLE(v, _MM_SHUFFLE(maskW, maskZ, maskY, maskX))
#define V_SHUFFLE_FWD(v, maskX, maskY, maskZ, maskW) V_SHUFFLE(v, _MM_SHUFFLE(maskW, maskZ, maskY, maskX))

#if _TARGET_SIMD_SSE >=3
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxzz(vec4f b){ return _mm_moveldup_ps(b); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyxy(vec4f b){ return _mm_castpd_ps(_mm_movedup_pd(_mm_castps_pd(b))); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxzz(vec4f b){ return V_SHUFFLE_FWD(b, 0,0, 2,2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyxy(vec4f b){ return V_SHUFFLE_FWD(b, 0,1, 0,1); }
#endif
VECTORCALL VECMATH_FINLINE vec4f v_perm_yyww(vec4f b){ return V_SHUFFLE_FWD(b, 1,1, 3,3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzxz(vec4f b){ return V_SHUFFLE_FWD(b, 0,2,0,2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwzw(vec4f b){ return V_SHUFFLE_FWD(b, 2,3, 2,3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywyw(vec4f b){ return V_SHUFFLE_FWD(b, 1,3,1,3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzz(vec4f b){ return V_SHUFFLE_FWD(b, 0,1,2,2); }


VECTORCALL VECMATH_FINLINE vec4f v_splat_x(vec4f a)
  { return _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 0, 0));  }
VECTORCALL VECMATH_FINLINE vec4f v_splat_y(vec4f a)
  { return _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1));  }
VECTORCALL VECMATH_FINLINE vec4f v_splat_z(vec4f a)
  { return _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 2, 2, 2));  }
VECTORCALL VECMATH_FINLINE vec4f v_splat_w(vec4f a)
  { return _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3));  }

VECTORCALL VECMATH_FINLINE vec4i v_splat_xi(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(0, 0, 0, 0));  }
VECTORCALL VECMATH_FINLINE vec4i v_splat_yi(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 1, 1, 1));  }
VECTORCALL VECMATH_FINLINE vec4i v_splat_zi(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(2, 2, 2, 2));  }
VECTORCALL VECMATH_FINLINE vec4i v_splat_wi(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(3, 3, 3, 3));  }

VECTORCALL VECMATH_FINLINE void v_st(void *m, vec4f v) { _mm_store_ps((float*)m, v); }
VECTORCALL VECMATH_FINLINE void v_stu(void *m, vec4f v) { _mm_storeu_ps((float*)m, v); }
VECTORCALL VECMATH_FINLINE void v_sti(void *m, vec4i v) { _mm_store_si128((__m128i*)m, v); }
VECTORCALL VECMATH_FINLINE void v_stui(void *m, vec4i v) { _mm_storeu_si128((__m128i*)m, v); }
VECTORCALL VECMATH_FINLINE void v_stui_half(void *m, vec4i v) { _mm_storel_epi64((__m128i*)m, v); }
VECTORCALL VECMATH_FINLINE void v_stu_half(void *m, vec4f v) { _mm_storel_epi64((__m128i*)m, _mm_castps_si128(v)); }
VECTORCALL VECMATH_FINLINE void v_stu_p3(float *p3, vec3f v) { _mm_storel_pi((__m64*)p3, v); p3[2] = v_extract_z(v); } //-V1032

VECTORCALL VECMATH_FINLINE vec4f v_merge_hw(vec4f a, vec4f b) { return _mm_unpacklo_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_merge_lw(vec4f a, vec4f b) { return _mm_unpackhi_ps(a, b); }

VECTORCALL VECMATH_FINLINE int v_signmask(vec4f a) { return _mm_movemask_ps(a); }

VECTORCALL VECMATH_FINLINE bool v_test_all_bits_zeros(vec4f a)
{
#if _TARGET_SIMD_SSE >= 4
  return _mm_test_all_zeros(v_cast_vec4i(a), v_cast_vec4i(a));
#else
  vec4f notZeroMask = v_cmp_neq(a, v_zero());
  return v_signmask(notZeroMask) == 0;
#endif
}

VECTORCALL VECMATH_FINLINE bool v_test_all_bits_ones(vec4f a)
{
#if _TARGET_SIMD_SSE >= 4
  return _mm_test_all_ones(v_cast_vec4i(a));
#else
  vec4f notOnesMask = v_cmp_neq(a, v_set_all_bits());
  return v_signmask(notOnesMask) == 0;
#endif
}

VECTORCALL VECMATH_FINLINE bool v_test_any_bit_set(vec4f a)
{
  return !v_test_all_bits_zeros(a);
}

VECTORCALL VECMATH_FINLINE bool v_check_xyzw_all_true(vec4f a) { return v_signmask(a) == 0b1111; }
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_all_false(vec4f a) { return v_signmask(a) == 0; }
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_any_true(vec4f a) { return v_signmask(a) != 0; }

VECTORCALL VECMATH_FINLINE bool v_check_xyz_all_true(vec4f a) { return (v_signmask(a) & 0b111) == 0b111; }
VECTORCALL VECMATH_FINLINE bool v_check_xyz_all_false(vec4f a) { return (v_signmask(a) & 0b111) == 0; }
VECTORCALL VECMATH_FINLINE bool v_check_xyz_any_true(vec4f a) { return (v_signmask(a) & 0b111) != 0; }

VECTORCALL VECMATH_FINLINE vec4f is_neg_special(vec4f a) { return v_cast_vec4f(v_srai(v_cast_vec4i(a), 31)); }

VECTORCALL VECMATH_FINLINE vec4f v_cmp_eq(vec4f a, vec4f b) { return _mm_cmpeq_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_neq(vec4f a, vec4f b) { return _mm_cmpneq_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_cmp_eqi(vec4i a, vec4i b) { return _mm_cmpeq_epi32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_eqi(vec4f a, vec4f b)
{
  __m128i m = _mm_cmpeq_epi32(v_cast_vec4i(a), v_cast_vec4i(b));
  return v_cast_vec4f(m);
}
VECTORCALL VECMATH_FINLINE vec4f v_cmp_ge(vec4f a, vec4f b) { return _mm_cmpge_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_gt(vec4f a, vec4f b) { return _mm_cmpgt_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_cmp_lti(vec4i a, vec4i b) { return _mm_cmplt_epi32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_cmp_gti(vec4i a, vec4i b) { return _mm_cmpgt_epi32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_and(vec4f a, vec4f b) { return _mm_and_ps(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_andnot(vec4f a, vec4f b) { return _mm_andnot_ps(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_or(vec4f a, vec4f b) { return _mm_or_ps(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_xor(vec4f a, vec4f b) { return _mm_xor_ps(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_not(vec4f v) { return v_xor(v, v_set_all_bits()); }
VECTORCALL VECMATH_FINLINE vec4f v_btsel(vec4f a, vec4f b, vec4f c)
{
  return _mm_or_ps(_mm_and_ps(c, b), _mm_andnot_ps(c, a));
}

VECTORCALL VECMATH_FINLINE vec4i v_btseli(vec4i a, vec4i b, vec4i c)
{
  return _mm_or_si128(_mm_and_si128(c, b), _mm_andnot_si128(c, a));
}

VECTORCALL VECMATH_FINLINE vec4i v_cast_vec4i(vec4f a) {return _mm_castps_si128(a);}//no instruction
VECTORCALL VECMATH_FINLINE vec4f v_cast_vec4f(vec4i a) {return _mm_castsi128_ps(a);}//no instruction

VECTORCALL VECMATH_FINLINE vec4i v_cvti_vec4i(vec4f a) { return _mm_cvttps_epi32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i(vec4f a)
{
#if defined(__AVX512F_)//only works on clang/gcc
  return _mm_cvtps_epu32(v);
#elif _TARGET_64BIT //_mm_cvtss_si64 is x64 instruction
  return v_make_vec4i(uint32_t(_mm_cvtss_si64(a)),
                      uint32_t(_mm_cvtss_si64(v_splat_y(a))),
                      uint32_t(_mm_cvtss_si64(v_splat_z(a))),
                      uint32_t(_mm_cvtss_si64(v_splat_w(a))));
#else
  return v_make_vec4i(uint32_t(v_extract_x(a)),
                      uint32_t(v_extract_y(a)),
                      uint32_t(v_extract_z(a)),
                      uint32_t(v_extract_w(a)));
#endif
}

VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i_ieee(vec4f a)
{
#if defined(__AVX512F_)//only works on clang/gcc
  return _mm_cvtps_epu32(v);
#else
  return v_make_vec4i(uint32_t(v_extract_x(a)),
                      uint32_t(v_extract_y(a)),
                      uint32_t(v_extract_z(a)),
                      uint32_t(v_extract_w(a)));
#endif
}

VECTORCALL VECMATH_FINLINE vec4f v_cvti_vec4f(vec4i a) { return _mm_cvtepi32_ps(a); }
VECTORCALL VECMATH_FINLINE vec4f v_cvtu_vec4f(vec4i v)
{
#if defined(__AVX512F_)//only works on clang/gcc
  return _mm_cvtepu32_ps(v);
#else
  __m128i v2 = _mm_srli_epi32(v, 1);                 // v2 = v / 2
  __m128i v1 = _mm_and_si128(v, _mm_set1_epi32(1));  // v1 = v & 1
  __m128 v2f = _mm_cvtepi32_ps(v2);
  __m128 v1f = _mm_cvtepi32_ps(v1);
  return _mm_add_ps(_mm_add_ps(v2f, v2f), v1f);      // return 2 * v2 + v1
#endif
}

VECTORCALL VECMATH_FINLINE vec4f v_cvtu_vec4f_ieee(vec4i v)
{
#if defined(__AVX512F_)//only works on clang/gcc
  return _mm_cvtepu32_ps(v);
#else
  __m128i msk_lo    = _mm_set1_epi32(0xFFFF);
  __m128  cnst65536f= _mm_set1_ps(65536.0f);

  __m128i v_lo      = _mm_and_si128(v,msk_lo);          /* extract the 16 lowest significant bits of v                                   */
  __m128i v_hi      = _mm_srli_epi32(v,16);             /* 16 most significant bits of v                                                 */
  __m128  v_lo_flt  = _mm_cvtepi32_ps(v_lo);            /* No rounding                                                                   */
  __m128  v_hi_flt  = _mm_cvtepi32_ps(v_hi);            /* No rounding                                                                   */
          v_hi_flt  = _mm_mul_ps(cnst65536f,v_hi_flt);  /* No rounding                                                                   */
  return              _mm_add_ps(v_hi_flt,v_lo_flt);    /* Rounding may occur here, mul and add may fuse to fma for haswell and newer    */
#endif
}

VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi(vec4f a)  { return _mm_cvtps_epi32(a); }

VECTORCALL VECMATH_FINLINE vec4i sse2_cvt_floori(vec4f a)
{
  vec4i fi = _mm_cvttps_epi32(a);
  return _mm_sub_epi32(fi, v_cast_vec4i(_mm_and_ps(_mm_cmpgt_ps(_mm_cvtepi32_ps(fi), a), V_CI_1)));
}

VECTORCALL VECMATH_FINLINE vec4i sse2_cvt_ceili(vec4f a)
{
  vec4i fi = _mm_cvttps_epi32(a);
  return _mm_add_epi32(fi, v_cast_vec4i(_mm_and_ps(_mm_cmplt_ps(_mm_cvtepi32_ps(fi), a), V_CI_1)));
}

VECTORCALL VECMATH_FINLINE vec4f sse2_floor(vec4f a)
{
  vec4f fi = _mm_cvtepi32_ps(_mm_cvttps_epi32(a));
  return _mm_sub_ps(fi, _mm_and_ps(_mm_cmpgt_ps(fi, a), V_C_ONE));
}

VECTORCALL VECMATH_FINLINE vec4f sse2_ceil(vec4f a)
{
  vec4f fi = _mm_cvtepi32_ps(_mm_cvttps_epi32(a));
  return _mm_add_ps(fi, _mm_and_ps(_mm_cmplt_ps(fi, a), V_C_ONE));
}

VECTORCALL VECMATH_FINLINE vec4f sse2_round(vec4f a) { return _mm_cvtepi32_ps(_mm_cvtps_epi32(a)); }

#if _TARGET_SIMD_SSE >= 4 || defined(_DAGOR_PROJECT_OPTIONAL_SSE4) || defined(__SSE4_1__)
VECTORCALL VECMATH_FINLINE vec4f sse4_floor(vec4f a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF|_MM_FROUND_NO_EXC); }
VECTORCALL VECMATH_FINLINE vec4f sse4_ceil(vec4f a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF|_MM_FROUND_NO_EXC); }
VECTORCALL VECMATH_FINLINE vec4f sse4_round(vec4f a) { return _mm_round_ps(a, _MM_FROUND_RINT); }
VECTORCALL VECMATH_FINLINE vec4f sse4_trunc(vec4f a) { return _mm_round_ps(a, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_floori(vec4f a) { return _mm_cvttps_epi32(sse4_floor(a)); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_ceili(vec4f a)  { return _mm_cvttps_epi32(sse4_ceil(a)); }
#else // fallback to SSE2
VECTORCALL VECMATH_FINLINE vec4f sse4_floor(vec4f a) { return sse2_floor(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_ceil(vec4f a) { return sse2_ceil(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_round(vec4f a) { return sse2_round(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_trunc(vec4f a) { return v_cvti_vec4f(v_cvti_vec4i(a)); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_floori(vec4f a) { return sse2_cvt_floori(a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_ceili(vec4f a)  { return sse2_cvt_ceili(a); }
#endif
#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE vec4i v_cvt_floori(vec4f a) {return sse4_cvt_floori(a);}
VECTORCALL VECMATH_FINLINE vec4i v_cvt_ceili(vec4f a) {return sse4_cvt_ceili(a);}
VECTORCALL VECMATH_FINLINE vec4i v_cvt_trunci(vec4f a) {return v_cvti_vec4i(a);}
VECTORCALL VECMATH_FINLINE vec4f v_floor(vec4f a) { return sse4_floor(a); }
VECTORCALL VECMATH_FINLINE vec4f v_ceil(vec4f a) { return sse4_ceil(a); }
VECTORCALL VECMATH_FINLINE vec4f v_round(vec4f a) { return sse4_round(a); }
VECTORCALL VECMATH_FINLINE vec4f v_trunc(vec4f a) { return sse4_trunc(a); }
VECTORCALL VECMATH_FINLINE vec4f v_sel(vec4f a, vec4f b, vec4f c) { return _mm_blendv_ps(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4i v_seli(vec4i a, vec4i b, vec4i c)
{
  return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _mm_castsi128_ps(c)));
}
#else
VECTORCALL VECMATH_FINLINE vec4i v_cvt_floori(vec4f a) {return sse2_cvt_floori(a);}
VECTORCALL VECMATH_FINLINE vec4i v_cvt_ceili(vec4f a) {return sse2_cvt_ceili(a);}
VECTORCALL VECMATH_FINLINE vec4f v_floor(vec4f a) { return sse2_floor(a); }
VECTORCALL VECMATH_FINLINE vec4f v_ceil(vec4f a) { return sse2_ceil(a); }
VECTORCALL VECMATH_FINLINE vec4f v_round(vec4f a) { return sse2_round(a); }
VECTORCALL VECMATH_FINLINE vec4f v_trunc(vec4f a) { return v_cvti_vec4f(v_cvti_vec4i(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_sel(vec4f a, vec4f b, vec4f c)
{
  vec4f m = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(c), 31));
  return _mm_or_ps(_mm_and_ps(m, b), _mm_andnot_ps(m, a));
}
VECTORCALL VECMATH_FINLINE vec4i v_seli(vec4i a, vec4i b, vec4i c)
{
  vec4i m = _mm_srai_epi32(c, 31);
  return _mm_or_si128(_mm_and_si128(m, b), _mm_andnot_si128(m, a));
}
#endif


VECTORCALL VECMATH_FINLINE vec4f v_add(vec4f a, vec4f b) { return _mm_add_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_sub(vec4f a, vec4f b) { return _mm_sub_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_mul(vec4f a, vec4f b) { return _mm_mul_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_div(vec4f a, vec4f b) { return _mm_div_ps(a, b); }
#if defined(__FMA__) || defined(__AVX2__)
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c) { return _mm_fmadd_ps(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4f v_madd_x(vec4f a, vec4f b, vec4f c) { return _mm_fmadd_ps(a, b, c); } // _ps is better
VECTORCALL VECMATH_FINLINE vec4f v_msub(vec4f a, vec4f b, vec4f c) { return _mm_fmsub_ps(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4f v_msub_x(vec4f a, vec4f b, vec4f c) { return _mm_fmsub_ps(a, b, c); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c) { return _mm_add_ps(_mm_mul_ps(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_madd_x(vec4f a, vec4f b, vec4f c) { return _mm_add_ss(_mm_mul_ss(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_msub(vec4f a, vec4f b, vec4f c) { return _mm_sub_ps(_mm_mul_ps(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_msub_x(vec4f a, vec4f b, vec4f c) { return _mm_sub_ss(_mm_mul_ss(a, b), c); }
#endif
VECTORCALL VECMATH_FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c) { return _mm_sub_ps(c, _mm_mul_ps(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_add_x(vec4f a, vec4f b) { return _mm_add_ss(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_sub_x(vec4f a, vec4f b) { return _mm_sub_ss(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_mul_x(vec4f a, vec4f b) { return _mm_mul_ss(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_div_x(vec4f a, vec4f b) { return _mm_div_ss(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub_x(vec4f a, vec4f b, vec4f c) { return _mm_sub_ss(c, _mm_mul_ss(a, b)); }
VECTORCALL VECMATH_FINLINE vec4i v_addi(vec4i a, vec4i b) { return _mm_add_epi32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_subi(vec4i a, vec4i b) { return _mm_sub_epi32(a, b); }

VECTORCALL VECMATH_FINLINE vec4f v_hadd4_x(vec4f a)
{
#if _TARGET_SIMD_SSE >= 3
  vec4f s = _mm_hadd_ps(a, a);
  return _mm_hadd_ps(s, s);
#else
  vec4f s = v_add(a, v_rot_2(a));
  return v_add_x(s, v_splat_y(s));
#endif
}
VECTORCALL VECMATH_FINLINE vec4f v_hadd3_x(vec3f a)
{
#if _TARGET_SIMD_SSE >= 3
  vec4f s = _mm_hadd_ps(a, a);
  return _mm_add_ss(s, _mm_movehl_ps(a, a));
#else
  vec4f s = _mm_add_ss(a, v_splat_y(a));
  return _mm_add_ss(s, _mm_movehl_ps(a, a));
#endif
}

VECTORCALL VECMATH_FINLINE vec4i v_slli(vec4i v, int bits) {return _mm_slli_epi32(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_srli(vec4i v, int bits) {return _mm_srli_epi32(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_srai(vec4i v, int bits) {return _mm_srai_epi32(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_slli_n(vec4i v, int bits) { return _mm_sll_epi32(v, _mm_cvtsi32_si128(bits)); }
VECTORCALL VECMATH_FINLINE vec4i v_srli_n(vec4i v, int bits) { return _mm_srl_epi32(v, _mm_cvtsi32_si128(bits)); }
VECTORCALL VECMATH_FINLINE vec4i v_srai_n(vec4i v, int bits) { return _mm_sra_epi32(v, _mm_cvtsi32_si128(bits)); }
VECTORCALL VECMATH_FINLINE vec4i v_slli_n(vec4i v, vec4i bits) { return _mm_sll_epi32(v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_srli_n(vec4i v, vec4i bits) { return _mm_srl_epi32(v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_srai_n(vec4i v, vec4i bits) { return _mm_sra_epi32(v, bits); }

VECTORCALL VECMATH_FINLINE vec4i v_sll(vec4i v, int bits) {return _mm_slli_epi32(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_srl(vec4i v, int bits) {return _mm_srli_epi32(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_sra(vec4i v, int bits) {return _mm_srai_epi32(v, bits);}

VECTORCALL VECMATH_FINLINE vec4i v_ori(vec4i a, vec4i b) {return _mm_or_si128(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_andi(vec4i a, vec4i b) {return _mm_and_si128(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_andnoti(vec4i a, vec4i b) {return _mm_andnot_si128(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_xori(vec4i a, vec4i b){return _mm_xor_si128(a, b);}

VECTORCALL VECMATH_FINLINE vec4i v_packs(vec4i a, vec4i b){return _mm_packs_epi32(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_packs(vec4i a){return _mm_packs_epi32(a, a);}
VECTORCALL VECMATH_FINLINE vec4i sse2_packus(vec4i a, vec4i b)
{
  vec4i z = _mm_setzero_si128();
  a = v_andi(v_ori(v_srai(v_slli(a, 16), 16), _mm_cmpgt_epi32(v_srli(a, 16), z)), _mm_cmpgt_epi32(a, z));
  b = v_andi(v_ori(v_srai(v_slli(b, 16), 16), _mm_cmpgt_epi32(v_srli(b, 16), z)), _mm_cmpgt_epi32(b, z));
  return _mm_packs_epi32(a, b);
}
VECTORCALL VECMATH_FINLINE vec4i sse2_packus(vec4i a)
{
  vec4i z = _mm_setzero_si128();
  a = v_andi(v_ori(v_srai(v_slli(a, 16), 16), _mm_cmpgt_epi32(v_srli(a, 16), z)), _mm_cmpgt_epi32(a, z));
  return _mm_packs_epi32(a, a);
}

//unsigned mul
VECTORCALL VECMATH_FINLINE vec4i sse2_muli(vec4i a, vec4i b)
{
  __m128i tmp1 = _mm_mul_epu32(a,b); /* mul 2,0*/
  __m128i tmp2 = _mm_mul_epu32( _mm_srli_si128(a,4), _mm_srli_si128(b,4)); /* mul 3,1 */
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE (0,0,2,0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE (0,0,2,0))); /* shuffle results to [63..0] and pack */
}

#if _TARGET_SIMD_SSE >= 4 || defined(_DAGOR_PROJECT_OPTIONAL_SSE4) || defined(__SSE4_1__)
VECTORCALL VECMATH_FINLINE vec4i sse4_packus(vec4i a, vec4i b) { return _mm_packus_epi32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i sse4_packus(vec4i a) { return _mm_packus_epi32(a, a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_muli(vec4i a, vec4i b) { return _mm_mullo_epi32(a, b);}
#else // fallback to SSE2
VECTORCALL VECMATH_FINLINE vec4i sse4_packus(vec4i a, vec4i b) { return sse2_packus(a, b); }
VECTORCALL VECMATH_FINLINE vec4i sse4_packus(vec4i a) { return sse2_packus(a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_muli(vec4i a, vec4i b) { return sse2_muli(a, b);}
#endif

#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a, vec4i b) { return sse4_packus(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a) { return sse4_packus(a); }
VECTORCALL VECMATH_FINLINE vec4i v_muli(vec4i a, vec4i b) { return sse4_muli(a,b); }
#else
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a, vec4i b) { return sse2_packus(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a) { return sse2_packus(a); }
VECTORCALL VECMATH_FINLINE vec4i v_muli(vec4i a, vec4i b) { return sse2_muli(a,b); }
#endif

VECTORCALL VECMATH_FINLINE vec4i v_packus16(vec4i a, vec4i b) { return _mm_packus_epi16(a,b); }
VECTORCALL VECMATH_FINLINE vec4i v_packus16(vec4i a) { return _mm_packus_epi16(a,a); }

VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise(vec4f a) { return _mm_rcp_ps(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est(vec4f a)
{
  __m128 y0 = _mm_rcp_ps(a);
  return _mm_sub_ps(_mm_add_ps(y0, y0), _mm_mul_ps(a, _mm_mul_ps(y0, y0)));
}
VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise_x(vec4f a) { return _mm_rcp_ss(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est_x(vec4f a)
{
  __m128 y0 = _mm_rcp_ss(a);
  return _mm_sub_ss(_mm_add_ss(y0, y0), _mm_mul_ss(a, _mm_mul_ss(y0, y0)));
}

VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_unprecise(vec4f a) { return _mm_rsqrt_ps(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_unprecise_x(vec4f a) { return _mm_rsqrt_ss(a); }

VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_est(vec4f a)
{
  vec4f r = v_rsqrt_unprecise(a);
  a = v_mul(a, r);
  a = v_mul(a, r);
  a = v_add(a, v_splats(-3.0f));
  r = v_mul(r, v_splats(-0.5f));
  return v_mul(a, r);
}

VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_est_x(vec4f a) // Reciprocal square root estimate and 1 Newton-Raphson iteration.
{
  vec4f r = v_rsqrt_unprecise_x(a);
  a = v_mul_x(a, r);
  a = v_mul_x(a, r);
  a = v_add_x(a, v_set_x(-3.0f));
  r = v_mul_x(r, v_set_x(-0.5f));
  return v_mul_x(a, r);
}

VECTORCALL VECMATH_FINLINE vec4f v_rsqrt(vec4f a) { return v_div(v_sqrt(a), a); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_x(vec4f a) { return v_div_x(v_sqrt_x(a), a); }

VECTORCALL VECMATH_FINLINE vec4i sse2_mini(vec4i a, vec4i b)
{
  vec4i cond = v_cmp_gti(a, b);
  return v_ori(v_andnoti(cond, a), v_andi(cond, b));
}
VECTORCALL VECMATH_FINLINE vec4i sse2_maxi(vec4i a, vec4i b)
{
  vec4i cond = v_cmp_gti(b, a);
  return v_ori(v_andnoti(cond, a), v_andi(cond, b));
}

VECTORCALL VECMATH_FINLINE vec4i sse2_minu(vec4i a, vec4i b)
{
  vec4i cond = v_cmp_gti(v_subi(a, V_CI_SIGN_MASK), v_subi(b, V_CI_SIGN_MASK));
  return v_ori(v_andnoti(cond, a), v_andi(cond, b));
}
VECTORCALL VECMATH_FINLINE vec4i sse2_maxu(vec4i a, vec4i b)
{
  vec4i cond = v_cmp_lti(v_subi(a, V_CI_SIGN_MASK), v_subi(b, V_CI_SIGN_MASK));
  return v_ori(v_andnoti(cond, a), v_andi(cond, b));
}

VECTORCALL VECMATH_FINLINE vec4i sse2_absi (vec4i a)
{
  vec4i mask = v_cmp_lti( a, _mm_setzero_si128() ); // FFFF   where a < 0
  a    = v_xori ( a, mask );                         // Invert where a < 0
  mask = v_srli( mask, 31 );                        // 0001   where a < 0
  a = v_addi( a, mask );                             // Add 1  where a < 0
  return a;
}

#if _TARGET_SIMD_SSE >= 3 || defined(_DAGOR_PROJECT_OPTIONAL_SSE4) || defined(__SSE3__)
VECTORCALL VECMATH_FINLINE vec4i sse3_absi(vec4i a) {return _mm_abs_epi32(a);}
#else
VECTORCALL VECMATH_FINLINE vec4i sse3_absi(vec4i a) {return sse2_absi(a);}
#endif

#if _TARGET_SIMD_SSE >= 4 || defined(_DAGOR_PROJECT_OPTIONAL_SSE4) || defined(__SSE4_1__)
VECTORCALL VECMATH_FINLINE vec4i sse4_mini(vec4i a, vec4i b) {return _mm_min_epi32(a, b);}
VECTORCALL VECMATH_FINLINE vec4i sse4_maxi(vec4i a, vec4i b) {return _mm_max_epi32(a, b);}
VECTORCALL VECMATH_FINLINE vec4i sse4_maxu(vec4i a, vec4i b) { return _mm_max_epu32(a,b); }
VECTORCALL VECMATH_FINLINE vec4i sse4_minu(vec4i a, vec4i b) { return _mm_min_epu32(a,b); }
#else // fallback to SSE2
VECTORCALL VECMATH_FINLINE vec4i sse4_mini(vec4i a, vec4i b) {return sse2_mini(a, b);}
VECTORCALL VECMATH_FINLINE vec4i sse4_maxi(vec4i a, vec4i b) {return sse2_maxi(a, b);}
VECTORCALL VECMATH_FINLINE vec4i sse4_maxu(vec4i a, vec4i b) { return sse2_maxu(a,b); }
VECTORCALL VECMATH_FINLINE vec4i sse4_minu(vec4i a, vec4i b) { return sse2_minu(a,b); }
#endif

VECTORCALL VECMATH_FINLINE vec4f v_min(vec4f a, vec4f b) { return _mm_min_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_max(vec4f a, vec4f b) { return _mm_max_ps(a, b); }
#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE vec4i v_mini(vec4i a, vec4i b) {return sse4_mini(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_maxi(vec4i a, vec4i b) {return sse4_maxi(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_minu(vec4i a, vec4i b) {return sse4_minu(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_maxu(vec4i a, vec4i b) {return sse4_maxu(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_absi(vec4i a) {return sse3_absi(a);}
#else
VECTORCALL VECMATH_FINLINE vec4i v_mini(vec4i a, vec4i b) {return sse2_mini(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_maxi(vec4i a, vec4i b) {return sse2_maxi(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_minu(vec4i a, vec4i b) {return sse2_minu(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_maxu(vec4i a, vec4i b) {return sse2_maxu(a, b);}
VECTORCALL VECMATH_FINLINE vec4i v_absi(vec4i a) {return sse2_absi(a);}
#endif

VECTORCALL VECMATH_FINLINE vec4f v_neg(vec4f a) {return v_xor(a, v_cast_vec4f(v_splatsi(0x80000000)));}
VECTORCALL VECMATH_FINLINE vec4i v_negi(vec4i a){ return v_subi(v_cast_vec4i(v_zero()), a); }
VECTORCALL VECMATH_FINLINE vec4f v_abs(vec4f a)
{
  #if defined(__clang__)
    return v_max(v_neg(a), a);
  #else
    //for this code clang creates one instruction, but uses memory for it.
    //if we think it is good tradeoff, we'd better allocate this constant once
    __m128i absmask = _mm_castps_si128(a);
    absmask = _mm_srli_epi32(_mm_cmpeq_epi32(absmask, absmask), 1);
    return _mm_and_ps(_mm_castsi128_ps(absmask), a);
  #endif
}

VECTORCALL VECMATH_FINLINE vec4f v_sqrt4_fast(vec4f a) { return _mm_sqrt_ps(a); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt(vec4f a) { return _mm_sqrt_ps(a); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_fast_x(vec4f a) { return _mm_sqrt_ss(a); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_x(vec4f a) { return _mm_sqrt_ss(a); }

VECTORCALL VECMATH_FINLINE vec4f v_rot_1(vec4f a) { return V_SHUFFLE_REV(a, 0, 3, 2, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_rot_2(vec4f a) { return V_SHUFFLE_REV(a, 1, 0, 3, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_rot_3(vec4f a) { return V_SHUFFLE_REV(a, 2, 1, 0, 3); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_1(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(0, 3, 2, 1)); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_2(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_3(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(2, 1, 0, 3)); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxx(vec4f a) { return V_SHUFFLE_REV(a, 0,0,2,1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxy(vec4f a) { return V_SHUFFLE_REV(a, 1,0,2,1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxw(vec4f a) { return V_SHUFFLE_REV(a, 3,0,2,1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxyw(vec4f a) { return V_SHUFFLE_REV(a, 3,1,0,2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxyy(vec4f a) { return V_SHUFFLE_REV(a, 1,1,0,0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zzww(vec4f a) { return V_SHUFFLE_REV(a, 3,3,2,2); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_xzac(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(2,0,2,0)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywbd(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(3,1,3,1)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyab(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1,0,1,0)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwcd(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(3,2,3,2)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_bbyx(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(abcd, xyzw, _MM_SHUFFLE(0,1,1,1)); }
VECTORCALL VECMATH_FINLINE vec4f
  v_perm_xaxa(vec4f xyzw, vec4f abcd) { return v_perm_yzxw(_mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(0,0,0,0))); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yybb(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1,1,1,1)); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_xycd(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(3,2,1,0)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ayzw(vec4f xyzw, vec4f abcd) { return _mm_move_ss(xyzw, abcd); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_xzbx(vec4f xyzw, vec4f abcd)
{
  vec4f bbxx =_mm_shuffle_ps(abcd, xyzw, _MM_SHUFFLE(0,0,1,1));
  return _mm_shuffle_ps(xyzw, bbxx, _MM_SHUFFLE(2, 0, 2, 0));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzya(vec4f xyzw, vec4f abcd)
{
  vec4f yyaa =_mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(0,0,1,1));
  return _mm_shuffle_ps(xyzw, yyaa, _MM_SHUFFLE(2, 0, 2, 0));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_yxxc(vec4f xyzw, vec4f abcd)
{
  vec4f xxcc =_mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(2,2,0,0));
  return _mm_shuffle_ps(xyzw, xxcc, _MM_SHUFFLE(2, 0, 0, 1));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_yaxx(vec4f xyzw, vec4f abcd)
{
  vec4f yyaa =_mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(0,0,1,1));
  return _mm_shuffle_ps(yyaa, xyzw, _MM_SHUFFLE(0, 0, 2, 0));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxxb(vec4f xyzw, vec4f abcd)
{
  vec4f xxbb =_mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1,1,0,0));
  return _mm_shuffle_ps(xyzw, xxbb, _MM_SHUFFLE(2, 0, 0, 2));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_zayx(vec4f xyzw, vec4f abcd)
{
  vec4f zzaa =_mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(0,0,2,2));
  return _mm_shuffle_ps(zzaa, xyzw, _MM_SHUFFLE(0, 1, 2, 0));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_bzxx(vec4f xyzw, vec4f abcd)
{
  vec4f bbzz =_mm_shuffle_ps(abcd, xyzw, _MM_SHUFFLE(2,2,1,1));
  return _mm_shuffle_ps(bbzz, xyzw, _MM_SHUFFLE(0, 0, 2, 0));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_caxx(vec4f xyzw, vec4f abcd)
{
  return _mm_shuffle_ps(abcd, xyzw, _MM_SHUFFLE(0, 0, 0, 2));
}

#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE vec4f v_perm_xbzw(vec4f xyzw, vec4f abcd) { return _mm_blend_ps(xyzw, abcd, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xycw(vec4f xyzw, vec4f abcd) { return _mm_blend_ps(xyzw, abcd, 4); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzd(vec4f xyzw, vec4f abcd) { return _mm_blend_ps(xyzw, abcd, 8); }

#else
VECTORCALL VECMATH_FINLINE vec4f v_perm_xbzw(vec4f xyzw, vec4f abcd)
{
  vec4f xxbb = _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1, 1, 0, 0));
  return _mm_shuffle_ps(xxbb, xyzw, _MM_SHUFFLE(3, 2, 3, 0));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_xycw(vec4f xyzw, vec4f abcd)
{
  vec4f wwcc = _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(2, 2, 3, 3));
  return _mm_shuffle_ps(xyzw, wwcc, _MM_SHUFFLE(0, 3, 1, 0));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzd(vec4f xyzw, vec4f abcd)
{
  vec4f zzdd = _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(3, 3, 2, 2));
  return _mm_shuffle_ps(xyzw, zzdd, _MM_SHUFFLE(3, 0, 1, 0));
}
#endif

VECTORCALL VECMATH_FINLINE vec4f sse2_dot2(vec4f a, vec4f b)
{
  vec4f m = _mm_mul_ps(a, b);
  return _mm_add_ps(v_splat_x(m), v_splat_y(m));
}
VECTORCALL VECMATH_FINLINE vec4f sse2_dot2_x(vec4f a, vec4f b)
{
  vec4f m = _mm_mul_ps(a, b);
  return v_add_x(m, v_rot_1(m));
}
VECTORCALL VECMATH_FINLINE vec4f sse2_dot3(vec4f a, vec4f b)
{
  vec4f m = _mm_mul_ps(a, b);

  return _mm_add_ps(
    _mm_add_ps(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2,2,1,0)),
               V_SHUFFLE_REV(m, 1,1,0,2)),
               _mm_shuffle_ps(m, m, _MM_SHUFFLE(0,0,2,1))
  );
}
VECTORCALL VECMATH_FINLINE vec4f sse2_dot4(vec4f a, vec4f b)
{
  vec4f m = _mm_mul_ps(a, b);

  return _mm_add_ps(
    _mm_add_ps(m,
               V_SHUFFLE_REV(m, 2,1,0,3)),
    _mm_add_ps(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1,0,3,2)),
               V_SHUFFLE_REV(m, 0,3,2,1))
  );
}
VECTORCALL VECMATH_FINLINE vec4f sse2_dot3_x(vec4f a, vec4f b)
{
  vec4f m = _mm_mul_ps(a, b);

  return _mm_add_ss(_mm_add_ss(m, v_splat_y(m)), _mm_shuffle_ps(m, m, 2));
}
VECTORCALL VECMATH_FINLINE vec4f sse2_dot4_x(vec4f a, vec4f b)
{
  vec4f m = _mm_mul_ps(a, b);

  return _mm_add_ss(_mm_add_ss(m, v_splat_y(m)),
                    _mm_add_ss(_mm_shuffle_ps(m, m, 2), v_splat_w(m)));
}

VECTORCALL VECMATH_FINLINE vec4f sse2_plane_dist_x(plane3f a, vec3f b) { return v_add_x(sse2_dot3_x(a,b), v_splat_w(a)); }

#if _TARGET_SIMD_SSE >= 4 || defined(_DAGOR_PROJECT_OPTIONAL_SSE4) || defined(__SSE4_1__)
VECTORCALL VECMATH_FINLINE vec4f sse4_dot4(vec4f a, vec4f b) { return _mm_dp_ps(a,b, 0xFF); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot4_x(vec4f a, vec4f b) { return _mm_dp_ps(a,b,0xF1); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot3(vec4f a, vec4f b) { return _mm_dp_ps(a,b, 0x7F); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot3_x(vec4f a, vec4f b) { return _mm_dp_ps(a,b,0x71); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot2(vec4f a, vec4f b) { return _mm_dp_ps(a, b, 0x3F); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot2_x(vec4f a, vec4f b) { return _mm_dp_ps(a, b, 0x31); }
VECTORCALL VECMATH_FINLINE vec4f sse4_plane_dist_x(plane3f a, vec3f b) { return v_add_x(sse4_dot3_x(a,b), v_splat_w(a)); }
#else // fallback to SSE2
VECTORCALL VECMATH_FINLINE vec4f sse4_dot4(vec4f a, vec4f b) { return sse2_dot4(a, b); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot4_x(vec4f a, vec4f b) { return sse2_dot4_x(a, b); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot3(vec4f a, vec4f b) { return sse2_dot3(a, b); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot3_x(vec4f a, vec4f b) { return sse2_dot3_x(a, b); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot2(vec4f a, vec4f b) { return sse2_dot2(a, b); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot2_x(vec4f a, vec4f b) { return sse2_dot2_x(a, b); }
VECTORCALL VECMATH_FINLINE vec4f sse4_plane_dist_x(plane3f a, vec3f b) { return sse2_plane_dist_x(a,b); }
#endif

#if _TARGET_SIMD_SSE >= 3 || defined(_DAGOR_PROJECT_OPTIONAL_SSE4) || defined(__SSE3__)
VECTORCALL VECMATH_FINLINE vec4f sse3_dot4(vec4f a, vec4f b)
{
  vec4f m = _mm_mul_ps(a, b);
  m = _mm_hadd_ps(m,m);
  return _mm_hadd_ps(m,m);
}
VECTORCALL VECMATH_FINLINE vec4f sse3_plane_dist_x(plane3f a, vec3f b) { return v_add_x(sse2_dot3_x(a,b), v_splat_w(a)); }
#else
VECTORCALL VECMATH_FINLINE vec4f sse3_dot4(vec4f a, vec4f b) { return sse2_dot4(a, b); }
VECTORCALL VECMATH_FINLINE vec4f sse3_plane_dist_x(plane3f a, vec3f b) { return sse2_plane_dist_x(a,b); }
#endif


#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE vec4f v_dot4(vec4f a, vec4f b) { return sse4_dot4(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot4_x(vec4f a, vec4f b) { return sse4_dot4_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3(vec4f a, vec4f b) { return sse4_dot3(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3_x(vec4f a, vec4f b) { return sse4_dot3_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2(vec4f a, vec4f b) { return sse4_dot2(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2_x(vec4f a, vec4f b) { return sse4_dot2_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist_x(plane3f a, vec3f b) { return sse4_plane_dist_x(a,b); }
#elif _TARGET_SIMD_SSE >= 3
VECTORCALL VECMATH_FINLINE vec4f v_dot4(vec4f a, vec4f b) { return sse3_dot4(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot4_x(vec4f a, vec4f b) { return sse3_dot4(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3(vec4f a, vec4f b) { return sse2_dot3(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3_x(vec4f a, vec4f b) { return sse2_dot3_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2(vec4f a, vec4f b) { return sse2_dot2(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2_x(vec4f a, vec4f b) { return sse2_dot2_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist_x(plane3f a, vec3f b) { return sse3_plane_dist_x(a,b); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_dot4(vec4f a, vec4f b) { return sse2_dot4(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot4_x(vec4f a, vec4f b) { return sse2_dot4_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3(vec4f a, vec4f b) { return sse2_dot3(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3_x(vec4f a, vec4f b) { return sse2_dot3_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2(vec4f a, vec4f b) { return sse2_dot2(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2_x(vec4f a, vec4f b) { return sse2_dot2_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist_x(plane3f a, vec3f b) { return sse2_plane_dist_x(a,b); }
#endif

VECTORCALL VECMATH_FINLINE vec4f v_length4_sq(vec4f a) { return v_dot4(a, a); }
VECTORCALL VECMATH_FINLINE vec3f v_length3_sq(vec3f a) { return v_dot3(a, a); }
VECTORCALL VECMATH_FINLINE vec4f v_length2_sq(vec4f a) { return v_dot2(a, a); }
VECTORCALL VECMATH_FINLINE vec4f v_length4_sq_x(vec4f a) { return v_dot4_x(a, a); }
VECTORCALL VECMATH_FINLINE vec3f v_length3_sq_x(vec3f a) { return v_dot3_x(a, a); }
VECTORCALL VECMATH_FINLINE vec4f v_length2_sq_x(vec4f a) { return v_dot2_x(a, a); }

VECTORCALL VECMATH_FINLINE vec4f v_norm4(vec4f a) { return v_div(a, v_splat_x(v_sqrt_x(v_dot4_x(a,a)))); }
VECTORCALL VECMATH_FINLINE vec4f v_norm3(vec4f a) { return v_div(a, v_splat_x(v_sqrt_x(v_dot3_x(a,a)))); }
VECTORCALL VECMATH_FINLINE vec4f v_norm2(vec4f a) { return v_div(a, v_splat_x(v_sqrt_x(v_dot2_x(a,a)))); }

VECTORCALL VECMATH_FINLINE vec4f v_plane_dist(plane3f a, vec3f b)
{
  return v_splat_x(v_plane_dist_x(a, b));
}

VECTORCALL VECMATH_FINLINE void v_mat_33cu_from_mat33(float * __restrict m33, const mat33f& tm)
{
#if _TARGET_SIMD_SSE >= 4
  vec4f v0 = _mm_insert_ps(tm.col0, tm.col1, _MM_MK_INSERTPS_NDX(0, 3, 0));
  vec4f v1 = v_perm_xyab(v_rot_1(tm.col1), tm.col2);
#else // _TARGET_SIMD_SSE >= 4
  vec4f v0 = v_perm_xyzd(tm.col0, v_splat_x(tm.col1));
  vec4f v1 = v_perm_xyab(v_rot_1(tm.col1), tm.col2);
#endif // _TARGET_SIMD_SSE >= 4
  v_stu(m33 + 0, v0);
  v_stu(m33 + 4, v1);
  m33[8] = v_extract_z(tm.col2);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43ca(mat44f& tm, const float *const __restrict m43)
{
  v_mat44_make_from_43cu(tm, m43);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu(mat44f& tm, const float *const __restrict m43)
{
  vec4f v0 = v_ldu(m43 + 0);
  vec4f v1 = v_ldu(m43 + 4);
  vec4f v2 = v_ldu(m43 + 8);
#if _TARGET_SIMD_SSE >= 4
  tm.col0 = _mm_blend_ps(v0, v_zero(), 1 << 3);
  tm.col1 = _mm_insert_ps(_mm_shuffle_ps(v1, v1, _MM_SHUFFLE(2, 1, 0, 3)), v0, _MM_MK_INSERTPS_NDX(3, 0, 1 << 3));
  tm.col2 = _mm_blend_ps(_mm_shuffle_ps(v1, v2, _MM_SHUFFLE(1, 0, 3, 2)), v_zero(), 1 << 3);
  tm.col3 = v_rot_1(_mm_move_ss(v2, v_cast_vec4f(v_seti_x(0x3f800000))));
#else // _TARGET_SIMD_SSE >= 4
  vec4f v10 = _mm_shuffle_ps(v1, v0, _MM_SHUFFLE(3, 2, 1, 0));
  tm.col0 = v_and(v0, V_CI_MASK1110);
  tm.col1 = v_and(_mm_shuffle_ps(v10, v10, _MM_SHUFFLE(2, 1, 0, 3)), V_CI_MASK1110);
  tm.col2 = v_and(_mm_shuffle_ps(v1, v2, _MM_SHUFFLE(1, 0, 3, 2)), V_CI_MASK1110);
  tm.col3 = v_rot_1(_mm_move_ss(v2, v_cast_vec4f(v_seti_x(0x3f800000))));
#endif // _TARGET_SIMD_SSE >= 4
}

VECTORCALL VECMATH_FINLINE void v_mat_43ca_from_mat44(float * __restrict m43, const mat44f& tm)
{
  v_mat_43cu_from_mat44(m43, tm);
}

VECTORCALL VECMATH_FINLINE void v_mat_43cu_from_mat44(float * __restrict m43, const mat44f& tm)
{
#if _TARGET_SIMD_SSE >= 4
  vec4f v0 = _mm_insert_ps(tm.col0, tm.col1, _MM_MK_INSERTPS_NDX(0, 3, 0));
  vec4f v1 = v_perm_xyab(v_rot_1(tm.col1), tm.col2);
  vec4f v2 = _mm_insert_ps(_mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(tm.col3), 4)), tm.col2, _MM_MK_INSERTPS_NDX(2, 0, 0));
#else // _TARGET_SIMD_SSE >= 4
  vec4f v0 = v_perm_xyzd(tm.col0, v_splat_x(tm.col1));
  vec4f v1 = v_perm_xyab(v_rot_1(tm.col1), tm.col2);
  vec4f v2 = _mm_move_ss(_mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(tm.col3), 4)), v_splat_z(tm.col2));
#endif // _TARGET_SIMD_SSE >= 4
  v_stu(m43 + 0, v0);
  v_stu(m43 + 4, v1);
  v_stu(m43 + 8, v2);
}

VECTORCALL VECMATH_FINLINE void v_mat44_ident(mat44f &dest)
{
  dest.col0 = V_C_UNIT_1000;
  dest.col1 = V_C_UNIT_0100;
  dest.col2 = V_C_UNIT_0010;
  dest.col3 = V_C_UNIT_0001;
}
VECTORCALL VECMATH_FINLINE void v_mat44_ident_swapxz(mat44f &dest)
{
  dest.col0 = V_C_UNIT_0010;
  dest.col1 = V_C_UNIT_0100;
  dest.col2 = V_C_UNIT_1000;
  dest.col3 = V_C_UNIT_0001;
}
VECTORCALL VECMATH_FINLINE void v_mat33_ident(mat33f &dest)
{
  dest.col0 = V_C_UNIT_1000;
  dest.col1 = V_C_UNIT_0100;
  dest.col2 = V_C_UNIT_0010;
}
VECTORCALL VECMATH_FINLINE void v_mat33_ident_swapxz(mat33f &dest)
{
  dest.col0 = V_C_UNIT_0010;
  dest.col1 = V_C_UNIT_0100;
  dest.col2 = V_C_UNIT_1000;
}
VECTORCALL VECMATH_FINLINE void v_mat44_transpose(mat44f &dest, mat44f_cref src)
{
  __m128 tmp3, tmp2, tmp1, tmp0;

  tmp0 = _mm_shuffle_ps(src.col0, src.col1, 0x44);
  tmp2 = _mm_shuffle_ps(src.col0, src.col1, 0xEE);
  tmp1 = _mm_shuffle_ps(src.col2, src.col3, 0x44);
  tmp3 = _mm_shuffle_ps(src.col2, src.col3, 0xEE);

  dest.col0 = _mm_shuffle_ps(tmp0, tmp1, 0x88);
  dest.col1 = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
  dest.col2 = _mm_shuffle_ps(tmp2, tmp3, 0x88);
  dest.col3 = _mm_shuffle_ps(tmp2, tmp3, 0xDD);
}

VECTORCALL VECMATH_FINLINE void v_mat44_transpose(vec4f &r0, vec4f &r1, vec4f &r2, vec4f &r3)
{
   _MM_TRANSPOSE4_PS(r0, r1, r2, r3);
}

VECTORCALL VECMATH_FINLINE void v_mat43_transpose_to_mat44(mat44f &dest, mat43f_cref src)
{
  __m128 tmp3, tmp2, tmp1, tmp0;

  tmp0 = _mm_shuffle_ps(src.row0, src.row1, 0x44);
  tmp2 = _mm_shuffle_ps(src.row0, src.row1, 0xEE);
  tmp1 = _mm_shuffle_ps(src.row2, v_zero(), 0x44);
  tmp3 = _mm_shuffle_ps(src.row2, v_zero(), 0xEE);

  dest.col0 = _mm_shuffle_ps(tmp0, tmp1, 0x88);
  dest.col1 = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
  dest.col2 = _mm_shuffle_ps(tmp2, tmp3, 0x88);
  dest.col3 = _mm_shuffle_ps(tmp2, tmp3, 0xDD);
}
VECTORCALL VECMATH_FINLINE void v_mat44_transpose_to_mat43(mat43f &dest, mat44f_cref src)
{
  __m128 tmp3, tmp2, tmp1, tmp0;

  tmp0 = _mm_shuffle_ps(src.col0, src.col1, 0x44);
  tmp2 = _mm_shuffle_ps(src.col0, src.col1, 0xEE);
  tmp1 = _mm_shuffle_ps(src.col2, src.col3, 0x44);
  tmp3 = _mm_shuffle_ps(src.col2, src.col3, 0xEE);

  dest.row0 = _mm_shuffle_ps(tmp0, tmp1, 0x88);
  dest.row1 = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
  dest.row2 = _mm_shuffle_ps(tmp2, tmp3, 0x88);
}

VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec4(mat44f_cref m, vec4f v)
{
  vec4f xxxx = v_splat_x(v);
  vec4f yyyy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1));
  vec4f zzzz = v_splat_z(v);
  vec4f wwww = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3,3,3,3));

  return _mm_add_ps(
    _mm_add_ps(_mm_mul_ps(xxxx, m.col0), _mm_mul_ps(yyyy, m.col1)),
    _mm_add_ps(_mm_mul_ps(zzzz, m.col2), _mm_mul_ps(wwww, m.col3))
  );
}
VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec3v(mat44f_cref m, vec3f v)
{
  vec4f xxxx = v_splat_x(v);
  vec4f yyyy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1));
  vec4f zzzz = v_splat_z(v);

  return _mm_add_ps(_mm_add_ps(_mm_mul_ps(xxxx, m.col0), _mm_mul_ps(yyyy, m.col1)),
                    _mm_mul_ps(zzzz, m.col2));
}
VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec3p(mat44f_cref m, vec3f v)
{
  vec4f xxxx = v_splat_x(v);
  vec4f yyyy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1));
  vec4f zzzz = v_splat_z(v);

  return _mm_add_ps(
    _mm_add_ps(_mm_mul_ps(xxxx, m.col0), _mm_mul_ps(yyyy, m.col1)),
    _mm_add_ps(_mm_mul_ps(zzzz, m.col2), m.col3)
  );
}

VECTORCALL VECMATH_FINLINE vec3f v_mat33_mul_vec3(mat33f_cref m, vec3f v)
{
  vec4f xxxx = v_splat_x(v);
  vec4f yyyy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1));
  vec4f zzzz = v_splat_z(v);

  return _mm_add_ps(_mm_add_ps(_mm_mul_ps(xxxx, m.col0), _mm_mul_ps(yyyy, m.col1)),
                    _mm_mul_ps(zzzz, m.col2));
}

VECTORCALL VECMATH_FINLINE void v_mat44_inverse(mat44f &dest, mat44f_cref m)
{
  __m128 minor0, minor1, minor2, minor3;
  __m128 row0, row1, row2, row3;
  __m128 det, tmp0, tmp1, tmp2, tmp3;

  tmp0 = _mm_shuffle_ps(m.col0, m.col1, _MM_SHUFFLE(2,0,2,0));
  tmp1 = _mm_shuffle_ps(m.col2, m.col3, _MM_SHUFFLE(2,0,2,0));
  tmp2 = _mm_shuffle_ps(m.col0, m.col1, _MM_SHUFFLE(3,1,3,1));
  tmp3 = _mm_shuffle_ps(m.col2, m.col3, _MM_SHUFFLE(3,1,3,1));
  row0 = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(2,0,2,0));
  row1 = _mm_shuffle_ps(tmp3, tmp2, _MM_SHUFFLE(2,0,2,0));
  row2 = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(3,1,3,1));
  row3 = _mm_shuffle_ps(tmp3, tmp2, _MM_SHUFFLE(3,1,3,1));

  tmp1 = _mm_mul_ps(row2, row3);
  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  minor0 = _mm_mul_ps(row1, tmp1);
  minor1 = _mm_mul_ps(row0, tmp1);
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
  minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
  minor1 = V_SHUFFLE(minor1, 0x4E);

  tmp1 = _mm_mul_ps(row1, row2);
  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
  minor3 = _mm_mul_ps(row0, tmp1);
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
  minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
  minor3 = V_SHUFFLE(minor3, 0x4E);

  tmp1 = _mm_mul_ps(V_SHUFFLE(row1, 0x4E), row3);
  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  row2 = V_SHUFFLE(row2, 0x4E);
  minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
  minor2 = _mm_mul_ps(row0, tmp1);
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
  minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
  minor2 = V_SHUFFLE(minor2, 0x4E);

  tmp1 = _mm_mul_ps(row0, row1);

  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
  minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
  minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));

  tmp1 = _mm_mul_ps(row0, row3);
  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
  minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
  minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

  tmp1 = _mm_mul_ps(row0, row2);
  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
  minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
  minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

  det = _mm_mul_ps(row0, minor0);
  det = _mm_add_ps(V_SHUFFLE(det, 0x4E), det);
  det = _mm_add_ss(V_SHUFFLE(det, 0xB1), det);
  tmp1 = v_rcp_safe(det);
  det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
  det = v_splat_x(det);
  dest.col0 = _mm_mul_ps(det, minor0);
  dest.col1 = _mm_mul_ps(det, minor1);
  dest.col2 = _mm_mul_ps(det, minor2);
  dest.col3 = _mm_mul_ps(det, minor3);
}

VECTORCALL VECMATH_FINLINE void v_mat33_inverse(mat33f &dest, mat33f_cref m)
{
  vec4f tmp0, tmp1, tmp2, tmp3, tmp4, dot, invdet, inv0, inv1, inv2;

  tmp2 = v_cross3(m.col0, m.col1);
  tmp0 = v_cross3(m.col1, m.col2);
  tmp1 = v_cross3(m.col2, m.col0);
  dot = v_dot3(tmp2, m.col2);
  invdet = v_rcp_safe(dot);

  tmp3 = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(0,2,0,2));
  tmp4 = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(1,3,1,3));
  inv0 = _mm_shuffle_ps(tmp3, tmp2, _MM_SHUFFLE(3,0,3,1));
  inv1 = _mm_shuffle_ps(tmp4, tmp2, _MM_SHUFFLE(3,1,3,1));
  inv2 = _mm_shuffle_ps(tmp3, tmp2, _MM_SHUFFLE(3,2,2,0));

  dest.col0 = _mm_mul_ps(inv0, invdet);
  dest.col1 = _mm_mul_ps(inv1, invdet);
  dest.col2 = _mm_mul_ps(inv2, invdet);
}

VECTORCALL VECMATH_FINLINE vec4f v_mat44_det(mat44f_cref m)
{
  __m128 minor0;
  __m128 row0, row1, row2, row3;
  __m128 det, tmp0, tmp1, tmp2, tmp3;

  tmp0 = _mm_shuffle_ps(m.col0, m.col1, _MM_SHUFFLE(2,0,2,0));
  tmp1 = _mm_shuffle_ps(m.col2, m.col3, _MM_SHUFFLE(2,0,2,0));
  tmp2 = _mm_shuffle_ps(m.col0, m.col1, _MM_SHUFFLE(3,1,3,1));
  tmp3 = _mm_shuffle_ps(m.col2, m.col3, _MM_SHUFFLE(3,1,3,1));
  row0 = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(2,0,2,0));
  row1 = _mm_shuffle_ps(tmp3, tmp2, _MM_SHUFFLE(2,0,2,0));
  row2 = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(3,1,3,1));
  row3 = _mm_shuffle_ps(tmp3, tmp2, _MM_SHUFFLE(3,1,3,1));

  tmp1 = _mm_mul_ps(row2, row3);
  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  minor0 = _mm_mul_ps(row1, tmp1);
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);

  tmp1 = _mm_mul_ps(row1, row2);
  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));

  tmp1 = _mm_mul_ps(V_SHUFFLE(row1, 0x4E), row3);
  tmp1 = V_SHUFFLE(tmp1, 0xB1);
  row2 = V_SHUFFLE(row2, 0x4E);
  minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
  tmp1 = V_SHUFFLE(tmp1, 0x4E);
  minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));

  det = _mm_mul_ps(row0, minor0);
  det = _mm_add_ps(V_SHUFFLE(det, 0x4E), det);
  det = _mm_add_ss(V_SHUFFLE(det, 0xB1), det);
  return det;
}

#if defined(_MSC_VER) && (_MSC_VER < 1600 || (_MSC_VER < 1700 && _TARGET_64BIT)) && !defined(__clang__)
//due to compiler bug (vc2008-32 and vc2010-64)
VECTORCALL VECMATH_FINLINE float v_extract_x(vec4f v) { return v.m128_f32[0]; }
VECTORCALL VECMATH_FINLINE float v_extract_y(vec4f v) { return v.m128_f32[1]; }
VECTORCALL VECMATH_FINLINE float v_extract_z(vec4f v) { return v.m128_f32[2]; }
VECTORCALL VECMATH_FINLINE float v_extract_w(vec4f v) { return v.m128_f32[3]; }
#else
VECTORCALL VECMATH_FINLINE float v_extract_x( vec4f a ) { return _mm_cvtss_f32(a); }
VECTORCALL VECMATH_FINLINE float v_extract_y( vec4f a ) { return _mm_cvtss_f32(v_splat_y(a)); }
VECTORCALL VECMATH_FINLINE float v_extract_z( vec4f a ) { return _mm_cvtss_f32(v_splat_z(a)); }
VECTORCALL VECMATH_FINLINE float v_extract_w( vec4f a ) { return _mm_cvtss_f32(v_splat_w(a)); }
#endif

VECTORCALL VECMATH_FINLINE int v_extract_xi(vec4i v) {return _mm_cvtsi128_si32(v);}
#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE int v_extract_yi(vec4i v) {return _mm_extract_epi32(v, 1);}
VECTORCALL VECMATH_FINLINE int v_extract_zi(vec4i v) {return _mm_extract_epi32(v, 2);}
VECTORCALL VECMATH_FINLINE int v_extract_wi(vec4i v) {return _mm_extract_epi32(v, 3);}
VECTORCALL VECMATH_FINLINE int64_t v_extract_xi64(vec4i v)
{
#if defined(_MSC_VER) && !defined(__clang__) && !defined(_M_X64)
    return v.m128i_i64[0];
#else
    return _mm_extract_epi64(v, 0);
#endif
}
VECTORCALL VECMATH_FINLINE int64_t v_extract_yi64(vec4i v)
{
#if defined(_MSC_VER) && !defined(__clang__) && !defined(_M_X64)
  return v.m128i_i64[1];
#else
  return _mm_extract_epi64(v, 1);
#endif
}
#else
VECTORCALL VECMATH_FINLINE int v_extract_yi(vec4i v) {return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, _MM_SHUFFLE(1,1,1,1)));}
VECTORCALL VECMATH_FINLINE int v_extract_zi(vec4i v) {return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, _MM_SHUFFLE(2,2,2,2)));}
VECTORCALL VECMATH_FINLINE int v_extract_wi(vec4i v) {return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, _MM_SHUFFLE(3,3,3,3)));}
VECTORCALL VECMATH_FINLINE int64_t v_extract_xi64(vec4i a)
{
#if defined(_MSC_VER) && !defined(__clang__)//visual studio is not capable of produce reasonable code otherwise!
    return a.m128i_i64[0];
#else
    int64_t t; _mm_storel_epi64((__m128i*)&t, a); return t;
#endif
}
VECTORCALL VECMATH_FINLINE int64_t v_extract_yi64(vec4i a)
{
#if defined(_MSC_VER) && !defined(__clang__)
  return a.m128i_i64[1];
#else
  int64_t t; _mm_storel_epi64((__m128i*)&t, v_roti_2(a)); return t;
#endif
}
#endif

VECTORCALL VECMATH_FINLINE vec4i v_splatsi64(int64_t a) { return _mm_set1_epi64x(a); }

VECTORCALL VECMATH_FINLINE short v_extract_xi16(vec4i v) {return (short)_mm_extract_epi16(v, 0);}

VECTORCALL VECMATH_FINLINE int v_test_vec_x_eqi(vec3f v, vec3f a)
{
  vec4i eq = _mm_cmpeq_epi32(v_cast_vec4i(v),v_cast_vec4i(a));
  return v_extract_xi(eq) ? 1 : 0;
}
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eqi_0(vec3f v) { return v_extract_xi(v_cast_vec4i(v)) == 0 ? 1 : 0; }

VECTORCALL VECMATH_FINLINE int v_test_vec_x_eq(vec3f v, vec3f a) { return _mm_comieq_ss(v,a); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_gt(vec3f v, vec3f a) { return _mm_comigt_ss(v,a); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_ge(vec3f v, vec3f a) { return _mm_comige_ss(v,a); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_lt(vec3f v, vec3f a) { return _mm_comilt_ss(v,a); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_le(vec3f v, vec3f a) { return _mm_comile_ss(v,a); }

VECTORCALL VECMATH_FINLINE int v_test_vec_x_eq_0(vec3f v) { return v_test_vec_x_eq(v, v_zero()); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_gt_0(vec3f v) { return v_test_vec_x_gt(v, v_zero()); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_ge_0(vec3f v) { return v_test_vec_x_ge(v, v_zero()); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_lt_0(vec3f v) { return v_test_vec_x_lt(v, v_zero()); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_le_0(vec3f v) { return v_test_vec_x_le(v, v_zero()); }

#undef V_SHUFFLE
#undef  V_SHUFFLE_REV
#undef  V_SHUFFLE_FWD

#ifdef _MSC_VER
  #pragma warning(pop)
#endif
