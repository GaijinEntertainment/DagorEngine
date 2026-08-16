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

#include <immintrin.h>

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable: 4714) //function marked as __forceinline not inlined

  extern "C" unsigned char _BitScanForward(unsigned long *_Index, unsigned long _Mask);
  #pragma intrinsic(_BitScanForward)

  #if _MSC_VER < 1500
  VECTORCALL VECMATH_FINLINE __m128i _mm_castps_si128(__m128  v) { return *(__m128i*)&v; }
  VECTORCALL VECMATH_FINLINE __m128  _mm_castsi128_ps(__m128i v) { return *(__m128 *)&v; }
  #endif
#endif

VECTORCALL VECMATH_FINLINE vec4f v_zero() { return _mm_setzero_ps(); }
VECTORCALL VECMATH_FINLINE vec4i v_zeroi() { return _mm_setzero_si128(); }
VECTORCALL VECMATH_FINLINE vec4f v_set_all_bits() { return v_cast_vec4f(v_set_all_bitsi()); }
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

VECTORCALL VECMATH_FINLINE void v_ld_soa2(const float *m, vec4f &x, vec4f &y)
{
  vec4f a = v_ld(m), b = v_ld(m + 4);
  x = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
  y = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
}

VECTORCALL VECMATH_FINLINE void v_ld_soa3(const float *m, vec4f &x, vec4f &y, vec4f &z)
{
  vec4f x0y0z0x1 = v_ld(m), y1z1x2y2 = v_ld(m + 4), z2x3y3z3 = v_ld(m + 8);
  vec4f x2y2x3y3 = _mm_shuffle_ps(y1z1x2y2, z2x3y3z3, _MM_SHUFFLE(2, 1, 3, 2));
  vec4f y0z0y1z1 = _mm_shuffle_ps(x0y0z0x1, y1z1x2y2, _MM_SHUFFLE(1, 0, 2, 1));
  x = _mm_shuffle_ps(x0y0z0x1, x2y2x3y3, _MM_SHUFFLE(2, 0, 3, 0));
  y = _mm_shuffle_ps(y0z0y1z1, x2y2x3y3, _MM_SHUFFLE(3, 1, 2, 0));
  z = _mm_shuffle_ps(y0z0y1z1, z2x3y3z3, _MM_SHUFFLE(3, 0, 3, 1));
}

VECTORCALL VECMATH_FINLINE void v_ld_soa4(const float *m, vec4f &x, vec4f &y, vec4f &z, vec4f &w)
{
  vec4f a = v_ld(m), b = v_ld(m + 4), c = v_ld(m + 8), d = v_ld(m + 12);
  vec4f t0 = _mm_unpacklo_ps(a, b), t1 = _mm_unpackhi_ps(a, b);
  vec4f t2 = _mm_unpacklo_ps(c, d), t3 = _mm_unpackhi_ps(c, d);
  x = _mm_movelh_ps(t0, t2);
  y = _mm_movehl_ps(t2, t0);
  z = _mm_movelh_ps(t1, t3);
  w = _mm_movehl_ps(t3, t1);
}

VECTORCALL VECMATH_FINLINE void v_ldu_soa2(const float *m, vec4f &x, vec4f &y)
{
  vec4f a = v_ldu(m), b = v_ldu(m + 4);
  x = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
  y = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
}

VECTORCALL VECMATH_FINLINE void v_ldu_soa3(const float *m, vec4f &x, vec4f &y, vec4f &z)
{
  vec4f x0y0z0x1 = v_ldu(m), y1z1x2y2 = v_ldu(m + 4), z2x3y3z3 = v_ldu(m + 8);
  vec4f x2y2x3y3 = _mm_shuffle_ps(y1z1x2y2, z2x3y3z3, _MM_SHUFFLE(2, 1, 3, 2));
  vec4f y0z0y1z1 = _mm_shuffle_ps(x0y0z0x1, y1z1x2y2, _MM_SHUFFLE(1, 0, 2, 1));
  x = _mm_shuffle_ps(x0y0z0x1, x2y2x3y3, _MM_SHUFFLE(2, 0, 3, 0));
  y = _mm_shuffle_ps(y0z0y1z1, x2y2x3y3, _MM_SHUFFLE(3, 1, 2, 0));
  z = _mm_shuffle_ps(y0z0y1z1, z2x3y3z3, _MM_SHUFFLE(3, 0, 3, 1));
}

VECTORCALL VECMATH_FINLINE void v_ldu_soa4(const float *m, vec4f &x, vec4f &y, vec4f &z, vec4f &w)
{
  vec4f a = v_ldu(m), b = v_ldu(m + 4), c = v_ldu(m + 8), d = v_ldu(m + 12);
  vec4f t0 = _mm_unpacklo_ps(a, b), t1 = _mm_unpackhi_ps(a, b);
  vec4f t2 = _mm_unpacklo_ps(c, d), t3 = _mm_unpackhi_ps(c, d);
  x = _mm_movelh_ps(t0, t2);
  y = _mm_movehl_ps(t2, t0);
  z = _mm_movelh_ps(t1, t3);
  w = _mm_movehl_ps(t3, t1);
}

// SoA lanes to 3 packed-float3 registers, the inverse of the v_ldu_soa3 deinterleave
VECTORCALL VECMATH_FINLINE void v_interleave3(vec4f x, vec4f y, vec4f z, vec4f &e0, vec4f &e1, vec4f &e2)
{
  vec4f xy01 = _mm_unpacklo_ps(x, y);
  vec4f zx01 = _mm_shuffle_ps(z, x, _MM_SHUFFLE(1, 1, 0, 0));
  e0 = _mm_shuffle_ps(xy01, zx01, _MM_SHUFFLE(2, 0, 1, 0)); // x0 y0 z0 x1
  vec4f yz1 = _mm_shuffle_ps(y, z, _MM_SHUFFLE(1, 1, 1, 1));
  vec4f xy2 = _mm_shuffle_ps(x, y, _MM_SHUFFLE(2, 2, 2, 2));
  e1 = _mm_shuffle_ps(yz1, xy2, _MM_SHUFFLE(2, 0, 2, 0)); // y1 z1 x2 y2
  vec4f zx23 = _mm_shuffle_ps(z, x, _MM_SHUFFLE(3, 3, 2, 2));
  vec4f yz23 = _mm_unpackhi_ps(y, z);
  e2 = _mm_shuffle_ps(zx23, yz23, _MM_SHUFFLE(3, 2, 2, 0)); // z2 x3 y3 z3
}

// SoA lanes to 4 packed-float4 registers (a transpose), the inverse of the v_ldu_soa4 deinterleave
VECTORCALL VECMATH_FINLINE void v_interleave4(vec4f x, vec4f y, vec4f z, vec4f w, vec4f &e0, vec4f &e1, vec4f &e2, vec4f &e3)
{
  vec4f t0 = _mm_unpacklo_ps(x, y), t1 = _mm_unpackhi_ps(x, y);
  vec4f t2 = _mm_unpacklo_ps(z, w), t3 = _mm_unpackhi_ps(z, w);
  e0 = _mm_movelh_ps(t0, t2);
  e1 = _mm_movehl_ps(t2, t0);
  e2 = _mm_movelh_ps(t1, t3);
  e3 = _mm_movehl_ps(t3, t1);
}

VECTORCALL VECMATH_FINLINE void v_st_soa2(float *m, vec4f x, vec4f y)
{
  v_st(m, _mm_unpacklo_ps(x, y));
  v_st(m + 4, _mm_unpackhi_ps(x, y));
}
VECTORCALL VECMATH_FINLINE void v_stu_soa2(float *m, vec4f x, vec4f y)
{
  v_stu(m, _mm_unpacklo_ps(x, y));
  v_stu(m + 4, _mm_unpackhi_ps(x, y));
}
VECTORCALL VECMATH_FINLINE void v_st_soa3(float *m, vec4f x, vec4f y, vec4f z)
{
  vec4f e0, e1, e2;
  v_interleave3(x, y, z, e0, e1, e2);
  v_st(m, e0);
  v_st(m + 4, e1);
  v_st(m + 8, e2);
}
VECTORCALL VECMATH_FINLINE void v_stu_soa3(float *m, vec4f x, vec4f y, vec4f z)
{
  vec4f e0, e1, e2;
  v_interleave3(x, y, z, e0, e1, e2);
  v_stu(m, e0);
  v_stu(m + 4, e1);
  v_stu(m + 8, e2);
}
VECTORCALL VECMATH_FINLINE void v_st_soa4(float *m, vec4f x, vec4f y, vec4f z, vec4f w)
{
  vec4f e0, e1, e2, e3;
  v_interleave4(x, y, z, w, e0, e1, e2, e3);
  v_st(m, e0);
  v_st(m + 4, e1);
  v_st(m + 8, e2);
  v_st(m + 12, e3);
}
VECTORCALL VECMATH_FINLINE void v_stu_soa4(float *m, vec4f x, vec4f y, vec4f z, vec4f w)
{
  vec4f e0, e1, e2, e3;
  v_interleave4(x, y, z, w, e0, e1, e2, e3);
  v_stu(m, e0);
  v_stu(m + 4, e1);
  v_stu(m + 8, e2);
  v_stu(m + 12, e3);
}

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

VECTORCALL VECMATH_FINLINE vec4i v_make_vec3i(int x, int y, int z)
{
#if _TARGET_SIMD_SSE >= 4
  int64_t xy = int64_t(uint32_t(x)) | (int64_t(y) << 32);
  return _mm_insert_epi32(_mm_set_epi64x(0, xy), z, 2);
#else
  return _mm_set_epi32(z, z, y, x);
#endif
}

VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(vec4f x, vec4f y, vec4f z)
{ return _mm_shuffle_ps(_mm_shuffle_ps(x, y, _MM_SHUFFLE(0, 0, 0, 0)), z, _MM_SHUFFLE(0, 0, 2, 0)); }

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
VECTORCALL VECMATH_FINLINE vec4f v_perm_wwyy(vec4f b){ return V_SHUFFLE_FWD(b, 3,3, 1,1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yyww(vec4f b){ return V_SHUFFLE_FWD(b, 1,1, 3,3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzxz(vec4f b){ return V_SHUFFLE_FWD(b, 0,2,0,2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxzx(vec4f b){ return V_SHUFFLE_FWD(b, 2,0,2,0); }
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
VECTORCALL VECMATH_FINLINE void v_stui_p3(int *p3, vec4i v) { _mm_storel_epi64((__m128i*)p3, v); p3[2] = v_extract_zi(v); } //-V1032

VECTORCALL VECMATH_FINLINE vec4f v_merge_hw(vec4f a, vec4f b) { return _mm_unpacklo_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_merge_lw(vec4f a, vec4f b) { return _mm_unpackhi_ps(a, b); }

VECTORCALL VECMATH_FINLINE int v_signmask(vec4f a) { return _mm_movemask_ps(a); }
VECTORCALL VECMATH_FINLINE int v_truemask(vec4f a) { return _mm_movemask_ps(a); }
VECTORCALL VECMATH_FINLINE int v_count_true(vec4f a)
{
  // a true lane is all-ones == -1; sum the four lanes and negate
  __m128i v = _mm_castps_si128(a);
  v = _mm_add_epi32(v, _mm_shuffle_epi32(v, _MM_SHUFFLE(1, 0, 3, 2)));
  v = _mm_add_epi32(v, _mm_shuffle_epi32(v, _MM_SHUFFLE(2, 3, 0, 1)));
  return -_mm_cvtsi128_si32(v);
}
VECTORCALL VECMATH_FINLINE bool v_is_any_neg_b(vec4f a) { return _mm_movemask_ps(a) != 0; }
VECTORCALL VECMATH_FINLINE int v_is_merge_planes_nout(vec4f m0, vec4f m1, vec4f m2, vec4f m3, vec4f m4, vec4f m5)
{
  // per-plane movemask merged on scalar ports, effectively free next to the vector work
  // (a vector merge tree measured ~20% slower); unsigned(-x) has bit 31 set iff x != 0,
  // so the & chain needs no setcc per plane
  unsigned nout = unsigned(-_mm_movemask_ps(m0)) & unsigned(-_mm_movemask_ps(m1)) & unsigned(-_mm_movemask_ps(m2))
                & unsigned(-_mm_movemask_ps(m3)) & unsigned(-_mm_movemask_ps(m4)) & unsigned(-_mm_movemask_ps(m5));
  return int(nout) >> 31; // arithmetic shift broadcasts bit 31: 0 or -1
}

VECTORCALL VECMATH_FINLINE vec4f v_min_pairs(vec4f a, vec4f b)
{
  return _mm_min_ps(_mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0)), _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1)));
}
VECTORCALL VECMATH_FINLINE vec4f v_max_pairs(vec4f a, vec4f b)
{
  return _mm_max_ps(_mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0)), _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1)));
}
// haddps decodes to the same two shuffles plus the add on every core we target, and its lane
// order is the pairwise one, so the explicit form only schedules better
VECTORCALL VECMATH_FINLINE vec4f v_add_pairs(vec4f a, vec4f b)
{
  return _mm_add_ps(_mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0)), _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1)));
}

VECTORCALL VECMATH_FINLINE bool v_test_all_bits_zeros(vec4f a)
{
#if _TARGET_SIMD_SSE >= 4
  return _mm_test_all_zeros(v_cast_vec4i(a), v_cast_vec4i(a));
#else
  vec4f zeroMask = v_cmp_eqi(a, v_zero());
  return v_signmask(zeroMask) == 0b1111;
#endif
}

VECTORCALL VECMATH_FINLINE bool v_test_all_bits_ones(vec4f a)
{
#if _TARGET_SIMD_SSE >= 4
  return _mm_test_all_ones(v_cast_vec4i(a));
#else
  vec4f onesMask = v_cmp_eqi(a, v_set_all_bits());
  return v_signmask(onesMask) == 0b1111;
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
#if defined(__AVX512F__)//only works on clang/gcc
  return _mm_cvtps_epu32(a);
#elif _TARGET_64BIT //_mm_cvttss_si64 is x64 instruction. cvtt, not cvt: this truncates like the cast it stands for
  return v_make_vec4i(uint32_t(_mm_cvttss_si64(a)),
                      uint32_t(_mm_cvttss_si64(v_splat_y(a))),
                      uint32_t(_mm_cvttss_si64(v_splat_z(a))),
                      uint32_t(_mm_cvttss_si64(v_splat_w(a))));
#else
  return v_make_vec4i(uint32_t(v_extract_x(a)),
                      uint32_t(v_extract_y(a)),
                      uint32_t(v_extract_z(a)),
                      uint32_t(v_extract_w(a)));
#endif
}

VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i_ieee(vec4f a)
{
#if defined(__AVX512F__)//only works on clang/gcc
  return _mm_cvtps_epu32(a);
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
#if defined(__AVX512F__)//only works on clang/gcc
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
#if defined(__AVX512F__)//only works on clang/gcc
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

VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi_ieee(vec4f a) { return _mm_cvtps_epi32(a); }

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

VECTORCALL VECMATH_FINLINE vec4f sse2_round_ieee(vec4f a) { return _mm_cvtepi32_ps(_mm_cvtps_epi32(a)); }

#if _TARGET_SIMD_SSE >= 4 || defined(_DAGOR_PROJECT_OPTIONAL_SSE4) || defined(__SSE4_1__)
VECTORCALL VECMATH_FINLINE vec4f sse4_floor(vec4f a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF|_MM_FROUND_NO_EXC); }
VECTORCALL VECMATH_FINLINE vec4f sse4_ceil(vec4f a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF|_MM_FROUND_NO_EXC); }
VECTORCALL VECMATH_FINLINE vec4f sse4_round(vec4f a) { return v_round(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_round_ieee(vec4f a) { return _mm_round_ps(a, _MM_FROUND_RINT); }
VECTORCALL VECMATH_FINLINE vec4f sse4_trunc(vec4f a) { return _mm_round_ps(a, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_floori(vec4f a) { return _mm_cvttps_epi32(sse4_floor(a)); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_ceili(vec4f a)  { return _mm_cvttps_epi32(sse4_ceil(a)); }
#else // fallback to SSE2
VECTORCALL VECMATH_FINLINE vec4f sse4_floor(vec4f a) { return sse2_floor(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_ceil(vec4f a) { return sse2_ceil(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_round(vec4f a) { return v_round(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_round_ieee(vec4f a) { return sse2_round_ieee(a); }
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
VECTORCALL VECMATH_FINLINE vec4f v_round_ieee(vec4f a) { return sse4_round_ieee(a); }
VECTORCALL VECMATH_FINLINE vec4f v_trunc(vec4f a) { return sse4_trunc(a); }
VECTORCALL VECMATH_FINLINE vec4f v_sel(vec4f a, vec4f b, vec4f c) { return _mm_blendv_ps(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4i v_seli(vec4i a, vec4i b, vec4i c)
{
  return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _mm_castsi128_ps(c)));
}
#else
VECTORCALL VECMATH_FINLINE vec4i v_cvt_floori(vec4f a) {return sse2_cvt_floori(a);}
VECTORCALL VECMATH_FINLINE vec4i v_cvt_ceili(vec4f a) {return sse2_cvt_ceili(a);}
VECTORCALL VECMATH_FINLINE vec4i v_cvt_trunci(vec4f a) { return v_cvti_vec4i(a); }
VECTORCALL VECMATH_FINLINE vec4f v_floor(vec4f a) { return sse2_floor(a); }
VECTORCALL VECMATH_FINLINE vec4f v_ceil(vec4f a) { return sse2_ceil(a); }
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

// roundps has no ties-away-from-zero mode, so decide on the truncated remainder: trunc and the
// a - trunc(a) that follows are both exact, unlike biasing a by a signed half first (that add
// can carry into the next integer, e.g. 0.49999997 -> 1). NEON has the mode natively
VECTORCALL VECMATH_FINLINE vec4f v_round(vec4f a)
{
  vec4f t = v_trunc(a);
  vec4f sign = v_and(a, v_cast_vec4f(V_CI_SIGN_MASK));
  vec4f absFrac = v_xor(v_sub(a, t), sign); // truncation keeps the remainder on a's side of zero
  vec4f away = v_cmp_ge(absFrac, V_C_HALF);
  return v_add(t, v_or(v_and(away, V_C_ONE), sign));
}

VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi(vec4f a) { return v_cvt_trunci(v_round(a)); }


VECTORCALL VECMATH_FINLINE vec4f v_add(vec4f a, vec4f b) { return _mm_add_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_sub(vec4f a, vec4f b) { return _mm_sub_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_mul(vec4f a, vec4f b) { return _mm_mul_ps(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_div(vec4f a, vec4f b) { return _mm_div_ps(a, b); }
#if !defined(VECMATH_NO_FMA) && (defined(__FMA__) || (defined(__AVX2__) && defined(_MSC_VER) && !defined(__clang__)))
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c) { return _mm_fmadd_ps(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4f v_madd_x(vec4f a, vec4f b, vec4f c) { return _mm_fmadd_ps(a, b, c); } // _ps is better
VECTORCALL VECMATH_FINLINE vec4f v_msub(vec4f a, vec4f b, vec4f c) { return _mm_fmsub_ps(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4f v_msub_x(vec4f a, vec4f b, vec4f c) { return _mm_fmsub_ps(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c) { return _mm_fnmadd_ps(a, b, c); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c) { return _mm_add_ps(_mm_mul_ps(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_madd_x(vec4f a, vec4f b, vec4f c) { return _mm_add_ss(_mm_mul_ss(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_msub(vec4f a, vec4f b, vec4f c) { return _mm_sub_ps(_mm_mul_ps(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_msub_x(vec4f a, vec4f b, vec4f c) { return _mm_sub_ss(_mm_mul_ss(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c) { return _mm_sub_ps(c, _mm_mul_ps(a, b)); }
#endif
VECTORCALL VECMATH_FINLINE vec4f v_add_x(vec4f a, vec4f b) { return _mm_add_ss(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_sub_x(vec4f a, vec4f b) { return _mm_sub_ss(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_mul_x(vec4f a, vec4f b) { return _mm_mul_ss(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_div_x(vec4f a, vec4f b) { return _mm_div_ss(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub_x(vec4f a, vec4f b, vec4f c) { return _mm_sub_ss(c, _mm_mul_ss(a, b)); }
VECTORCALL VECMATH_FINLINE vec4i v_addi(vec4i a, vec4i b) { return _mm_add_epi32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_subi(vec4i a, vec4i b) { return _mm_sub_epi32(a, b); }

VECTORCALL VECMATH_FINLINE vec4i v_addi16(vec4i a, vec4i b) { return _mm_add_epi16(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_subi16(vec4i a, vec4i b) { return _mm_sub_epi16(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_muli16(vec4i a, vec4i b) { return _mm_mullo_epi16(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_mulhi16(vec4i a, vec4i b) { return _mm_mulhi_epi16(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_madd_i16(vec4i a, vec4i b) { return _mm_madd_epi16(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_splatsi16(int v) { return _mm_set1_epi16((int16_t)v); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i8(vec4i a, vec4i b) { return _mm_unpacklo_epi8(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i8(vec4i a, vec4i b) { return _mm_unpackhi_epi8(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i16(vec4i a, vec4i b) { return _mm_unpacklo_epi16(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i16(vec4i a, vec4i b) { return _mm_unpackhi_epi16(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i32(vec4i a, vec4i b) { return _mm_unpacklo_epi32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i32(vec4i a, vec4i b) { return _mm_unpackhi_epi32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i64(vec4i a, vec4i b) { return _mm_unpacklo_epi64(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i64(vec4i a, vec4i b) { return _mm_unpackhi_epi64(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_perm_i8(vec4i t, vec4i k)
{
#if _TARGET_SIMD_SSE >= 3
  return _mm_shuffle_epi8(t, k);
#else
  alignas(16) uint8_t tb[16], kb[16], r[16];
  _mm_store_si128((__m128i *)tb, t);
  _mm_store_si128((__m128i *)kb, k);
  for (int i = 0; i < 16; ++i)
    r[i] = (kb[i] & 0x80) ? 0 : tb[kb[i] & 15];
  return _mm_load_si128((const __m128i *)r);
#endif
}
VECTORCALL VECMATH_FINLINE vec4i v_cmp_eqi8(vec4i a, vec4i b) { return _mm_cmpeq_epi8(a, b); }

VECTORCALL VECMATH_FINLINE vec4f v_hadd4_x(vec4f a)
{
#if _TARGET_SIMD_SSE >= 4
  __m128 shuf = _mm_movehdup_ps(a);
  __m128 sums = _mm_add_ps(a, shuf);
  shuf = _mm_movehl_ps(shuf, sums);
  return _mm_add_ss(sums, shuf);
#else
  vec4f s = v_add(a, v_rot_2(a));
  return v_add_x(s, v_splat_y(s));
#endif
}
VECTORCALL VECMATH_FINLINE vec4f v_hadd3_x(vec3f a)
{
#if _TARGET_SIMD_SSE >= 4
  __m128 shuf = _mm_movehdup_ps(a);
  __m128 sums = _mm_add_ss(a, shuf);
  return _mm_add_ss(sums, _mm_movehl_ps(a, a));
#else
  vec4f s = _mm_add_ss(a, v_splat_y(a));
  return _mm_add_ss(s, _mm_movehl_ps(a, a));
#endif
}

VECTORCALL VECMATH_FINLINE vec4i v_slli(vec4i v, int bits) {return _mm_slli_epi32(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_srli(vec4i v, int bits) {return _mm_srli_epi32(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_srai(vec4i v, int bits) {return _mm_srai_epi32(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_slli_64(vec4i v, int bits) {return _mm_slli_epi64(v, bits);}
VECTORCALL VECMATH_FINLINE vec4i v_srli_64(vec4i v, int bits) {return _mm_srli_epi64(v, bits);}
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
// Newton step kept as y*(2 - a*y), the form NEON's vrecps computes: the algebraically equal
// 2y - a*y*y evaluates y*y, which leaves float range long before 1/a does - it overflows below
// |a| ~ 5e-20 (giving a sign flipped inf) and underflows above |a| ~ 1e19 (giving exactly 2x)
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est(vec4f a)
{
  __m128 y0 = _mm_rcp_ps(a);
  return _mm_mul_ps(y0, _mm_sub_ps(V_C_TWO, _mm_mul_ps(a, y0)));
}
VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise_x(vec4f a) { return _mm_rcp_ss(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est_x(vec4f a)
{
  __m128 y0 = _mm_rcp_ss(a);
  return _mm_mul_ss(y0, _mm_sub_ss(V_C_TWO, _mm_mul_ss(a, y0)));
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

// SSE has no pairwise integer op: deinterleave the even and odd lanes of both operands (one
// shufps each - there is no two-source integer shuffle, the float-domain cast is free) and
// apply the elementwise op
VECTORCALL VECMATH_FINLINE vec4i v_addi_pairs(vec4i a, vec4i b)
{
  vec4f af = v_cast_vec4f(a), bf = v_cast_vec4f(b);
  return v_addi(v_cast_vec4i(v_perm_xzac(af, bf)), v_cast_vec4i(v_perm_ywbd(af, bf)));
}
VECTORCALL VECMATH_FINLINE vec4i v_mini_pairs(vec4i a, vec4i b)
{
  vec4f af = v_cast_vec4f(a), bf = v_cast_vec4f(b);
  return v_mini(v_cast_vec4i(v_perm_xzac(af, bf)), v_cast_vec4i(v_perm_ywbd(af, bf)));
}
VECTORCALL VECMATH_FINLINE vec4i v_maxi_pairs(vec4i a, vec4i b)
{
  vec4f af = v_cast_vec4f(a), bf = v_cast_vec4f(b);
  return v_maxi(v_cast_vec4i(v_perm_xzac(af, bf)), v_cast_vec4i(v_perm_ywbd(af, bf)));
}

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

VECTORCALL VECMATH_FINLINE vec4f v_abs_diff(vec4f a, vec4f b) { return v_abs(_mm_sub_ps(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_abs_ge(vec4f a, vec4f b) { return _mm_cmpge_ps(v_abs(a), v_abs(b)); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_abs_gt(vec4f a, vec4f b) { return _mm_cmpgt_ps(v_abs(a), v_abs(b)); }

VECTORCALL VECMATH_FINLINE vec4f v_sqrt(vec4f a)   { return _mm_sqrt_ps(a); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_x(vec4f a) { return _mm_sqrt_ss(a); }

VECTORCALL VECMATH_FINLINE vec4f v_rot_1(vec4f a) { return V_SHUFFLE_REV(a, 0, 3, 2, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_rot_2(vec4f a) { return V_SHUFFLE_REV(a, 1, 0, 3, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_rot_3(vec4f a) { return V_SHUFFLE_REV(a, 2, 1, 0, 3); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_1(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(0, 3, 2, 1)); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_2(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_3(vec4i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(2, 1, 0, 3)); }

// horizontal min/max: 2 shuffles + 2 min/max is the log2 floor for a broadcast
// reduction on SSE; NEON implements these with dedicated fminv/fmaxv instructions
VECTORCALL VECMATH_FINLINE vec4f v_hmin(vec4f a)
{
  a = v_min(a, v_rot_1(a));
  return v_min(a, v_rot_2(a));
}
VECTORCALL VECMATH_FINLINE vec4f v_hmax(vec4f a)
{
  a = v_max(a, v_rot_1(a));
  return v_max(a, v_rot_2(a));
}
VECTORCALL VECMATH_FINLINE vec4f v_hmin3(vec3f a)
{
  return v_min(v_splat_x(a), v_min(v_splat_y(a), v_splat_z(a)));
}
VECTORCALL VECMATH_FINLINE vec4f v_hmax3(vec3f a)
{
  return v_max(v_splat_x(a), v_max(v_splat_y(a), v_splat_z(a)));
}
VECTORCALL VECMATH_FINLINE vec4i v_hmini(vec4i a)
{
  a = v_mini(a, v_roti_1(a));
  return v_mini(a, v_roti_2(a));
}
VECTORCALL VECMATH_FINLINE vec4i v_hmaxi(vec4i a)
{
  a = v_maxi(a, v_roti_1(a));
  return v_maxi(a, v_roti_2(a));
}
VECTORCALL VECMATH_FINLINE vec4i v_hmini3(vec4i a) { return v_mini(v_splat_xi(a), v_mini(v_splat_yi(a), v_splat_zi(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_hmaxi3(vec4i a) { return v_maxi(v_splat_xi(a), v_maxi(v_splat_yi(a), v_splat_zi(a))); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxx(vec4f a) { return V_SHUFFLE_REV(a, 0,0,2,1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxy(vec4f a) { return V_SHUFFLE_REV(a, 1,0,2,1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxw(vec4f a) { return V_SHUFFLE_REV(a, 3,0,2,1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxyw(vec4f a) { return V_SHUFFLE_REV(a, 3,1,0,2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yxwz(vec4f a) { return V_SHUFFLE_REV(a, 2,3,0,1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxyy(vec4f a) { return V_SHUFFLE_REV(a, 1,1,0,0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zzww(vec4f a) { return V_SHUFFLE_REV(a, 3,3,2,2); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_xzac(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(2,0,2,0)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywbd(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(3,1,3,1)); }
#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE vec4f v_perm_xazc(vec4f xyzw, vec4f abcd) { return _mm_blend_ps(xyzw, _mm_moveldup_ps(abcd), 0xA); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ybwd(vec4f xyzw, vec4f abcd) { return _mm_blend_ps(_mm_movehdup_ps(xyzw), abcd, 0xA); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_perm_xazc(vec4f xyzw, vec4f abcd)
{ return _mm_movelh_ps(_mm_unpacklo_ps(xyzw, abcd), _mm_unpackhi_ps(xyzw, abcd)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ybwd(vec4f xyzw, vec4f abcd)
{ return _mm_movehl_ps(_mm_unpackhi_ps(xyzw, abcd), _mm_unpacklo_ps(xyzw, abcd)); }
#endif
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwab(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1,0,3,2)); }
#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzwa(vec4f xyzw, vec4f abcd)
{ return _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(abcd), _mm_castps_si128(xyzw), 4)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wabc(vec4f xyzw, vec4f abcd)
{ return _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(abcd), _mm_castps_si128(xyzw), 12)); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzwa(vec4f xyzw, vec4f abcd)
{ return _mm_shuffle_ps(xyzw, _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(0,0,3,3)), _MM_SHUFFLE(2,0,2,1)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wabc(vec4f xyzw, vec4f abcd)
{ return _mm_shuffle_ps(_mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(0,0,3,3)), abcd, _MM_SHUFFLE(2,1,2,0)); }
#endif
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyab(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1,0,1,0)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwcd(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(3,2,3,2)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_bbyx(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(abcd, xyzw, _MM_SHUFFLE(0,1,1,1)); }
VECTORCALL VECMATH_FINLINE vec4f
  v_perm_xaxa(vec4f xyzw, vec4f abcd) { return v_perm_yzxw(_mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(0,0,0,0))); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yybb(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1,1,1,1)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxab(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1,0,0,0)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzab(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(1,0,2,1)); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_xycd(vec4f xyzw, vec4f abcd) { return _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(3,2,1,0)); }
#if _TARGET_SIMD_SSE >= 4
// blend runs on 3 ports where movss is a port 5 shuffle
VECTORCALL VECMATH_FINLINE vec4f v_perm_ayzw(vec4f xyzw, vec4f abcd) { return _mm_blend_ps(xyzw, abcd, 1); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_perm_ayzw(vec4f xyzw, vec4f abcd) { return _mm_move_ss(xyzw, abcd); }
#endif

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

VECTORCALL VECMATH_FINLINE vec3f v_mat43_extract_pos(mat43f_cref mat)
{
  vec4f xjyj = _mm_shuffle_ps(mat.row0, mat.row1, _MM_SHUFFLE(0, 3, 0, 3));
  vec4f xyzj = _mm_shuffle_ps(xjyj, mat.row2, _MM_SHUFFLE(0, 3, 2, 0));
  return xyzj;
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

// integer single-source perms: pshufd is non-destructive, folds loads and stays in the int domain
VECTORCALL VECMATH_FINLINE vec4i v_permi_xzxz(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(2, 0, 2, 0)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_ywyw(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(3, 1, 3, 1)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xyxy(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(1, 0, 1, 0)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_zwzw(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(3, 2, 3, 2)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xxyy(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(1, 1, 0, 0)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_zzww(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(3, 3, 2, 2)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xxzz(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(2, 2, 0, 0)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_yyww(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(3, 3, 1, 1)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_wwyy(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(1, 1, 3, 3)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_yzxw(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(3, 0, 2, 1)); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_yzxy(vec4i xyzw) { return _mm_shuffle_epi32(xyzw, _MM_SHUFFLE(1, 0, 2, 1)); }

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
VECTORCALL VECMATH_FINLINE vec4f sse4_dot4_x(vec4f a, vec4f b)
{
  // dpps is slower, especially on AMD
  __m128 mul = _mm_mul_ps(a, b);
  __m128 shuf = _mm_movehdup_ps(mul);
  __m128 sums = _mm_add_ps(mul, shuf);
  shuf = _mm_movehl_ps(shuf, sums);
  return _mm_add_ss(sums, shuf);
}
VECTORCALL VECMATH_FINLINE vec4f sse4_dot4(vec4f a, vec4f b) { return v_splat_x(sse4_dot4_x(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot3_x(vec4f a, vec4f b)
{
  // dpps is slower, especially on AMD
  __m128 mul = _mm_mul_ps(a, b);
  __m128 shuf = _mm_movehl_ps(mul, mul);
  __m128 sums = _mm_add_ss(mul, shuf);
  shuf = _mm_movehdup_ps(mul);
  return _mm_add_ss(sums, shuf);
}
VECTORCALL VECMATH_FINLINE vec4f sse4_dot3(vec4f a, vec4f b) { return v_splat_x(sse4_dot3_x(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f sse4_dot2_x(vec4f a, vec4f b)
{
  __m128 mul = _mm_mul_ps(a, b);
  return _mm_add_ss(mul, _mm_movehdup_ps(mul));
}
VECTORCALL VECMATH_FINLINE vec4f sse4_dot2(vec4f a, vec4f b) { return v_splat_x(sse4_dot2_x(a, b)); }
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



#if _TARGET_SIMD_SSE >= 4
VECTORCALL VECMATH_FINLINE vec4f v_dot4(vec4f a, vec4f b) { return sse4_dot4(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot4_x(vec4f a, vec4f b) { return sse4_dot4_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3(vec4f a, vec4f b) { return sse4_dot3(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3_x(vec4f a, vec4f b) { return sse4_dot3_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2(vec4f a, vec4f b) { return sse4_dot2(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2_x(vec4f a, vec4f b) { return sse4_dot2_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist_x(plane3f a, vec3f b) { return sse4_plane_dist_x(a,b); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_dot4(vec4f a, vec4f b) { return sse2_dot4(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot4_x(vec4f a, vec4f b) { return sse2_dot4_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3(vec4f a, vec4f b) { return sse2_dot3(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3_x(vec4f a, vec4f b) { return sse2_dot3_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2(vec4f a, vec4f b) { return sse2_dot2(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_dot2_x(vec4f a, vec4f b) { return sse2_dot2_x(a,b); }
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist_x(plane3f a, vec3f b) { return sse2_plane_dist_x(a,b); }
#endif

// both products must stay rounded so a x a is exactly 0; unprotected, the compiler contracts
// one mul into the sub and returns the other's rounding error instead
VECTORCALL VECMATH_FINLINE vec3f v_cross3(vec3f a, vec3f b)
{
  // (a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
#if defined(__FMA__) || (defined(__AVX2__) && defined(_MSC_VER) && !defined(__clang__))
  // subtract through hsubps: FMA has no horizontal form, so no compiler can contract the
  // muls into it and both products stay rounded by construction
  vec4f u = v_mul(V_SHUFFLE_FWD(a, 1, 2, 2, 0), V_SHUFFLE_FWD(b, 2, 1, 0, 2)); // ay*bz az*by az*bx ax*bz
  vec4f v = v_mul(V_SHUFFLE_FWD(a, 0, 1, 0, 1), V_SHUFFLE_FWD(b, 1, 0, 1, 0)); // ax*by ay*bx ax*by ay*bx
  return _mm_hsub_ps(u, v);
#else
  // without FMA in the target the mul+sub pair cannot be contracted
  vec3f yzxw = v_perm_yzxw(a);
  vec3f bcad = v_perm_yzxw(b);
  return v_perm_yzxy(v_sub(v_mul(a, bcad), v_mul(yzxw, b)));
#endif
}

// v_length*_sq and v_norm2/3/4 live in dag_vecMath_common.h (portable form).

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

VECTORCALL VECMATH_FINLINE void v_mat_43ca_from_mat44(float * __restrict m43, const mat44f& tm)
{
  v_mat_43cu_from_mat44(m43, tm);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu_unsafe(mat44f &tmV, const float *const __restrict m43)
{
  vec4f v0 = v_ldu(m43 + 0);
  vec4f v1 = v_ldu(m43 + 4);
  vec4f v2 = v_ldu(m43 + 8);
  tmV.col0 = v0;
  tmV.col1 = v_perm_wxyz(v_perm_xycd(v1, v0));
  tmV.col2 = v_perm_zwxy(v_perm_xycd(v2, v1));
  tmV.col3 = v_rot_1(v2);
}

// hand-interleaved w cleanup: MSVC does not fuse _unsafe + v_mat44_make_affine back into this form
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu(mat44f& tm, const float *const __restrict m43)
{
  vec4f v0 = v_ldu(m43 + 0);
  vec4f v1 = v_ldu(m43 + 4);
  vec4f v2 = v_ldu(m43 + 8);
#if _TARGET_SIMD_SSE >= 4
  tm.col0 = _mm_blend_ps(v0, v_zero(), 1 << 3);
  tm.col1 = _mm_insert_ps(_mm_shuffle_ps(v1, v1, _MM_SHUFFLE(2, 1, 0, 3)), v0, _MM_MK_INSERTPS_NDX(3, 0, 1 << 3));
  tm.col2 = _mm_blend_ps(_mm_shuffle_ps(v1, v2, _MM_SHUFFLE(1, 0, 3, 2)), v_zero(), 1 << 3);
  tm.col3 = v_rot_1(_mm_blend_ps(v2, V_C_UNIT_1000, 1)); // pool constant instead of a GPR->vector crossing
#else // _TARGET_SIMD_SSE >= 4
  vec4f v10 = _mm_shuffle_ps(v1, v0, _MM_SHUFFLE(3, 2, 1, 0));
  tm.col0 = v_and(v0, V_CI_MASK1110);
  tm.col1 = v_and(_mm_shuffle_ps(v10, v10, _MM_SHUFFLE(2, 1, 0, 3)), V_CI_MASK1110);
  tm.col2 = v_and(_mm_shuffle_ps(v1, v2, _MM_SHUFFLE(1, 0, 3, 2)), V_CI_MASK1110);
  tm.col3 = v_rot_1(_mm_move_ss(v2, V_C_UNIT_1000)); // pool constant instead of a GPR->vector crossing
#endif // _TARGET_SIMD_SSE >= 4
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43ca(mat44f& tm, const float *const __restrict m43)
{
  v_mat44_make_from_43cu(tm, m43);
}

VECTORCALL VECMATH_FINLINE void v_mat43_make_from_43cu_unsafe(mat43f &tmV, const float *const __restrict m43)
{
  // rows' .w lanes carry junk instead of the translation, which needs fewer shuffles
  vec4f l0 = v_ldu(m43), l1 = v_ldu(m43 + 4), l2 = v_ldu(m43 + 8);
  tmV.row0 = _mm_shuffle_ps(l0, l1, _MM_SHUFFLE(2, 2, 3, 0));
  vec4f t = _mm_shuffle_ps(l0, l1, _MM_SHUFFLE(3, 0, 1, 1));
  tmV.row1 = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 3, 2, 0));
  vec4f u = _mm_shuffle_ps(l0, l1, _MM_SHUFFLE(1, 1, 2, 2));
  tmV.row2 = _mm_shuffle_ps(u, l2, _MM_SHUFFLE(0, 0, 2, 0));
}

VECTORCALL VECMATH_FINLINE void v_mat_43cu_from_mat44(float * __restrict m43, const mat44f& tm)
{
#if defined(__clang__)
  vec4f v0 = __builtin_shufflevector(tm.col0, tm.col1, 0, 1, 2, 4);
  vec4f v1 = __builtin_shufflevector(tm.col1, tm.col2, 1, 2, 4, 5);
  vec4f v2 = __builtin_shufflevector(tm.col2, tm.col3, 2, 4, 5, 6);
#elif _TARGET_SIMD_SSE >= 4
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
// v_mat44_transpose*, v_mat43_transpose_to_mat44, v_mat44_transpose_to_mat43 and
// v_mat44/33_mul_vec* live in dag_vecMath_common.h: the portable form maps to
// optimal shuffles on both SSE and NEON, so a hw-specific version is not needed.

// v_mat33_inverse lives in dag_vecMath_common.h (portable form).

// v_mat44_det lives in dag_vecMath_common.h (portable form).

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
#else
VECTORCALL VECMATH_FINLINE int v_extract_yi(vec4i v) {return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, _MM_SHUFFLE(1,1,1,1)));}
VECTORCALL VECMATH_FINLINE int v_extract_zi(vec4i v) {return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, _MM_SHUFFLE(2,2,2,2)));}
VECTORCALL VECMATH_FINLINE int v_extract_wi(vec4i v) {return _mm_cvtsi128_si32(_mm_shuffle_epi32(v, _MM_SHUFFLE(3,3,3,3)));}
#endif
VECTORCALL VECMATH_FINLINE int64_t v_extract_xi64(vec4i v)
{
#if _TARGET_SIMD_SSE >= 4 && (defined(__x86_64__) || defined(_M_X64))
    return _mm_extract_epi64(v, 0);
#elif defined(_MSC_VER) && !defined(__clang__)
    return v.m128i_i64[0];
#else
    union { __m128i m; int64_t t; } u;
    _mm_storel_epi64(&u.m, v);
    return u.t;
#endif
}
VECTORCALL VECMATH_FINLINE int64_t v_extract_yi64(vec4i v)
{
#if _TARGET_SIMD_SSE >= 4 && (defined(__x86_64__) || defined(_M_X64))
    return _mm_extract_epi64(v, 1);
#elif defined(_MSC_VER) && !defined(__clang__)
    return v.m128i_i64[1];
#else
    union { __m128i m; int64_t t; } u;
    _mm_storel_epi64(&u.m, v_roti_2(v));
    return u.t;
#endif
}

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

#if (!defined(_TARGET_HAS_FC16) && (_TARGET_PC_WIN || defined(__F16C__) || defined(__AVX2__))) || _TARGET_HAS_FC16

// on _TARGET_PC_WIN v_fc16_* are optionally possible

#if (defined(__F16C__) || defined(__AVX2__)) && !defined(_TARGET_HAS_FC16)
#define _TARGET_HAS_FC16 1 // target should use v_fc16* intrinsics instead of emulation
#endif

VECTORCALL VECMATH_FINLINE vec4f v_fc16_half_to_float_lo(vec4i v)
{
  return _mm_cvtph_ps(v);
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_rtne_lo(vec4f v)
{
  return _mm_cvtps_ph(v, _MM_FROUND_TO_NEAREST_INT);
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_down_lo(vec4f v)
{
  return _mm_cvtps_ph(v, _MM_FROUND_TO_NEG_INF);
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_up_lo(vec4f v)
{
  return _mm_cvtps_ph(v, _MM_FROUND_TO_POS_INF);
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_trunc_lo(vec4f v)
{
  return _mm_cvtps_ph(v, _MM_FROUND_TO_ZERO);
}
#endif

#undef V_SHUFFLE
#undef  V_SHUFFLE_REV
#undef  V_SHUFFLE_FWD

#ifdef _MSC_VER
  #pragma warning(pop)
#endif
