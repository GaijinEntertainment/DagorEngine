//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// vec4d - 4D double-precision vector. Include via dag_vecMath.h (needs the platform
// intrinsics and vec4f already set up). Representation is chosen in dag_vecMathDecl.h:
// one __m256d on AVX, otherwise two 128-bit halves (.xy low pair, .zw high pair).
//
// The math is written on 128-bit / 64x2 halves shared by both x86 representations; only
// pack/unpack differs.
//
// Precision matches scalar DPoint3 exactly: the reductions below add lanes left to right,
// so vd_dot*/vd_hadd* are bit-identical to the equivalent scalar expression under
// -ffp-contract=off. Replacing scalar double math with vec4d changes speed, not results.
// That speed depends mostly on having AVX (one register instead of two): in branchy
// per-body physics code it is ~1.35x scalar on SSE4.1, ~1.5x on AVX, ~1.7x on AVX2+FMA.
//
// There are no lane-wise 3-component forms (no vd_add3/vd_div3/...). For add/sub/mul/neg
// the packed op yields .w for free, so such a form would emit the same instructions. For
// div/sqrt the .w lane could be skipped with a scalar op on the high half, but that pays
// off nowhere we ship: every narrow-divider target (Jaguar, Intel E-cores, Zen2 consoles)
// has AVX and so takes the single-register path, where there is no separate lane to skip,
// and on NEON the compiler widens the scalar op back to packed and adds moves to preserve
// .w. 3-component forms exist only where the math itself differs - vd_hadd3, vd_dot3,
// vd_cross3, vd_length3.

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable: 4714) //function marked as __forceinline not inlined
#endif

#if _TARGET_SIMD_SSE
// ------------------------------------------------------------------------------------------------
// x86 (SSE2 two __m128d / AVX one __m256d)
// ------------------------------------------------------------------------------------------------
VECTORCALL VECMATH_FINLINE __m128d vd_lo(vec4d a) {
#if VECMATH_VEC4D_256
  return _mm256_castpd256_pd128(a);
#else
  return a.xy;
#endif
}
VECTORCALL VECMATH_FINLINE __m128d vd_hi(vec4d a) {
#if VECMATH_VEC4D_256
  return _mm256_extractf128_pd(a, 1);
#else
  return a.zw;
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_from_halves(__m128d lo, __m128d hi) {
#if VECMATH_VEC4D_256
  return _mm256_insertf128_pd(_mm256_castpd128_pd256(lo), hi, 1);
#else
  vec4d r; r.xy = lo; r.zw = hi; return r;
#endif
}

VECTORCALL VECMATH_FINLINE vec4d vd_zero() {
#if VECMATH_VEC4D_256
  return _mm256_setzero_pd();
#else
  return vd_from_halves(_mm_setzero_pd(), _mm_setzero_pd());
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_splats(double a) {
#if VECMATH_VEC4D_256
  return _mm256_set1_pd(a);
#else
  return vd_from_halves(_mm_set1_pd(a), _mm_set1_pd(a));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_make_vec4d(double x, double y, double z, double w) {
  return vd_from_halves(_mm_set_pd(y, x), _mm_set_pd(w, z));
}

VECTORCALL VECMATH_FINLINE double vd_extract_x(vec4d a) { return _mm_cvtsd_f64(vd_lo(a)); }
VECTORCALL VECMATH_FINLINE double vd_extract_y(vec4d a) { __m128d l = vd_lo(a); return _mm_cvtsd_f64(_mm_unpackhi_pd(l, l)); }
VECTORCALL VECMATH_FINLINE double vd_extract_z(vec4d a) { return _mm_cvtsd_f64(vd_hi(a)); }
VECTORCALL VECMATH_FINLINE double vd_extract_w(vec4d a) { __m128d h = vd_hi(a); return _mm_cvtsd_f64(_mm_unpackhi_pd(h, h)); }

#if VECMATH_VEC4D_256
// blend keeps this one instruction (plus the broadcast); going through the halves would
// cost an extractf128 + insertf128 pair
VECTORCALL VECMATH_FINLINE vec4d vd_insert_x(vec4d a, double x) { return _mm256_blend_pd(a, _mm256_set1_pd(x), 0x1); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_y(vec4d a, double y) { return _mm256_blend_pd(a, _mm256_set1_pd(y), 0x2); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_z(vec4d a, double z) { return _mm256_blend_pd(a, _mm256_set1_pd(z), 0x4); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_w(vec4d a, double w) { return _mm256_blend_pd(a, _mm256_set1_pd(w), 0x8); }
#else
VECTORCALL VECMATH_FINLINE vec4d vd_insert_x(vec4d a, double x) { return vd_from_halves(_mm_move_sd(a.xy, _mm_set_sd(x)), a.zw); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_y(vec4d a, double y) { return vd_from_halves(_mm_unpacklo_pd(a.xy, _mm_set_sd(y)), a.zw); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_z(vec4d a, double z) { return vd_from_halves(a.xy, _mm_move_sd(a.zw, _mm_set_sd(z))); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_w(vec4d a, double w) { return vd_from_halves(a.xy, _mm_unpacklo_pd(a.zw, _mm_set_sd(w))); }
#endif

// Deliberately no 256-bit branch: the compiler folds the two half moves into a single
// vmovup/vmovap ymm on AVX, so this is already optimal there. It does mean 16-byte alignment
// is enough for the aligned forms on every build, and that only the non-AVX build faults on
// a violation - see the note in dag_vecMath.h.
NO_ASAN_INLINE vec4d vd_ld(const double *m)  { return vd_from_halves(_mm_load_pd(m),  _mm_load_pd(m + 2)); }
NO_ASAN_INLINE vec4d vd_ldu(const double *m) { return vd_from_halves(_mm_loadu_pd(m), _mm_loadu_pd(m + 2)); }
VECTORCALL VECMATH_FINLINE void vd_st(double *m, vec4d a)  { _mm_store_pd(m, vd_lo(a));  _mm_store_pd(m + 2, vd_hi(a)); }
VECTORCALL VECMATH_FINLINE void vd_stu(double *m, vec4d a) { _mm_storeu_pd(m, vd_lo(a)); _mm_storeu_pd(m + 2, vd_hi(a)); }

// DPoint3 layout: 3 packed doubles. _safe reads exactly 3 (.w = 0); store writes exactly 3.
NO_ASAN_INLINE vec4d vd_ldu_p3_safe(const double *m) { return vd_from_halves(_mm_loadu_pd(m), _mm_load_sd(m + 2)); }
VECTORCALL VECMATH_FINLINE void vd_stu_p3(double *p3, vec4d v) { _mm_storeu_pd(p3, vd_lo(v)); _mm_store_sd(p3 + 2, vd_hi(v)); }

VECTORCALL VECMATH_FINLINE vec4d vd_cvt_from_vec4f(vec4f a) {
#if VECMATH_VEC4D_256
  return _mm256_cvtps_pd(a);
#else
  return vd_from_halves(_mm_cvtps_pd(a), _mm_cvtps_pd(_mm_movehl_ps(a, a)));
#endif
}
VECTORCALL VECMATH_FINLINE vec4f vd_cvt_to_vec4f(vec4d a) {
#if VECMATH_VEC4D_256
  return _mm256_cvtpd_ps(a);
#else
  return _mm_movelh_ps(_mm_cvtpd_ps(vd_lo(a)), _mm_cvtpd_ps(vd_hi(a)));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_cvt_from_vec4i(vec4i a) {
#if VECMATH_VEC4D_256
  return _mm256_cvtepi32_pd(a);
#else
  return vd_from_halves(_mm_cvtepi32_pd(a), _mm_cvtepi32_pd(_mm_unpackhi_epi64(a, a)));
#endif
}
// truncates toward zero, like v_cvt_vec4i; out-of-range input is platform-specific
VECTORCALL VECMATH_FINLINE vec4i vd_cvt_to_vec4i(vec4d a) {
#if VECMATH_VEC4D_256
  return _mm256_cvttpd_epi32(a);
#else
  return _mm_unpacklo_epi64(_mm_cvttpd_epi32(vd_lo(a)), _mm_cvttpd_epi32(vd_hi(a)));
#endif
}

VECTORCALL VECMATH_FINLINE vec4d vd_add(vec4d a, vec4d b) {
#if VECMATH_VEC4D_256
  return _mm256_add_pd(a, b);
#else
  return vd_from_halves(_mm_add_pd(a.xy, b.xy), _mm_add_pd(a.zw, b.zw));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_sub(vec4d a, vec4d b) {
#if VECMATH_VEC4D_256
  return _mm256_sub_pd(a, b);
#else
  return vd_from_halves(_mm_sub_pd(a.xy, b.xy), _mm_sub_pd(a.zw, b.zw));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_mul(vec4d a, vec4d b) {
#if VECMATH_VEC4D_256
  return _mm256_mul_pd(a, b);
#else
  return vd_from_halves(_mm_mul_pd(a.xy, b.xy), _mm_mul_pd(a.zw, b.zw));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_div(vec4d a, vec4d b) {
#if VECMATH_VEC4D_256
  return _mm256_div_pd(a, b);
#else
  return vd_from_halves(_mm_div_pd(a.xy, b.xy), _mm_div_pd(a.zw, b.zw));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_neg(vec4d a) {
  __m128d m = _mm_set1_pd(-0.0);
  return vd_from_halves(_mm_xor_pd(vd_lo(a), m), _mm_xor_pd(vd_hi(a), m));
}
VECTORCALL VECMATH_FINLINE vec4d vd_min(vec4d a, vec4d b) {
#if VECMATH_VEC4D_256
  return _mm256_min_pd(a, b);
#else
  return vd_from_halves(_mm_min_pd(a.xy, b.xy), _mm_min_pd(a.zw, b.zw));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_max(vec4d a, vec4d b) {
#if VECMATH_VEC4D_256
  return _mm256_max_pd(a, b);
#else
  return vd_from_halves(_mm_max_pd(a.xy, b.xy), _mm_max_pd(a.zw, b.zw));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_sqrt(vec4d a) {
#if VECMATH_VEC4D_256
  return _mm256_sqrt_pd(a);
#else
  return vd_from_halves(_mm_sqrt_pd(a.xy), _mm_sqrt_pd(a.zw));
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_sqrt_x(vec4d a) { __m128d lo = vd_lo(a); return vd_from_halves(_mm_sqrt_sd(lo, lo), vd_hi(a)); }

// Lane order is left to right ((x+y)+z)+w, the same association a scalar x+y+z+w has, so
// vd_dot* reproduce scalar DPoint3 results bit-for-bit (given -ffp-contract=off, which the
// physics libs already build with). A pairwise tree would save one add of latency but would
// not match, which is what matters when replacing existing double math.
VECTORCALL VECMATH_FINLINE vec4d vd_hadd4_x(vec4d a) {
  __m128d lo = vd_lo(a), hi = vd_hi(a);
  __m128d s = _mm_add_sd(lo, _mm_unpackhi_pd(lo, lo)); // x+y
  s = _mm_add_sd(s, hi);                               // +z
  s = _mm_add_sd(s, _mm_unpackhi_pd(hi, hi));          // +w
  return vd_from_halves(s, s);
}
VECTORCALL VECMATH_FINLINE vec4d vd_hadd4(vec4d a) {
  __m128d x = vd_lo(vd_hadd4_x(a)), s = _mm_unpacklo_pd(x, x);
  return vd_from_halves(s, s);
}
VECTORCALL VECMATH_FINLINE vec4d vd_hadd3_x(vec4d a) {
  __m128d lo = vd_lo(a);
  __m128d s = _mm_add_sd(lo, _mm_unpackhi_pd(lo, lo)); // x+y
  s = _mm_add_sd(s, vd_hi(a));                         // +z
  return vd_from_halves(s, s);
}
VECTORCALL VECMATH_FINLINE vec4d vd_hadd3(vec4d a) {
  __m128d x = vd_lo(vd_hadd3_x(a)), s = _mm_unpacklo_pd(x, x);
  return vd_from_halves(s, s);
}

VECTORCALL VECMATH_FINLINE vec4d vd_dot4(vec4d a, vec4d b) { return vd_hadd4(vd_mul(a, b)); }
VECTORCALL VECMATH_FINLINE vec4d vd_dot4_x(vec4d a, vec4d b) { return vd_hadd4_x(vd_mul(a, b)); }
VECTORCALL VECMATH_FINLINE vec4d vd_dot3(vec4d a, vec4d b) { return vd_hadd3(vd_mul(a, b)); }
VECTORCALL VECMATH_FINLINE vec4d vd_dot3_x(vec4d a, vec4d b) { return vd_hadd3_x(vd_mul(a, b)); }

// r.x = ay*bz - az*by, r.y = az*bx - ax*bz, r.z = ax*by - ay*bx; .w unspecified
VECTORCALL VECMATH_FINLINE vec4d vd_cross3(vec4d a, vec4d b) {
  __m128d a_lo = vd_lo(a), a_hi = vd_hi(a), b_lo = vd_lo(b), b_hi = vd_hi(b);
  __m128d r_lo = _mm_sub_pd(_mm_mul_pd(_mm_shuffle_pd(a_lo, a_hi, 1), _mm_shuffle_pd(b_hi, b_lo, 0)),
                            _mm_mul_pd(_mm_shuffle_pd(a_hi, a_lo, 0), _mm_shuffle_pd(b_lo, b_hi, 1)));
  __m128d r_hi = _mm_sub_pd(_mm_mul_pd(a_lo, _mm_shuffle_pd(b_lo, b_lo, 1)),
                            _mm_mul_pd(_mm_shuffle_pd(a_lo, a_lo, 1), b_lo));
  return vd_from_halves(r_lo, r_hi);
}

#elif _TARGET_SIMD_NEON
// ------------------------------------------------------------------------------------------------
// aarch64 NEON (two float64x2_t halves: .xy and .zw)
// ------------------------------------------------------------------------------------------------
VECTORCALL VECMATH_FINLINE float64x2_t vd_lo(vec4d a) { return a.xy; }
VECTORCALL VECMATH_FINLINE float64x2_t vd_hi(vec4d a) { return a.zw; }
VECTORCALL VECMATH_FINLINE vec4d vd_from_halves(float64x2_t lo, float64x2_t hi) { vec4d r; r.xy = lo; r.zw = hi; return r; }

VECTORCALL VECMATH_FINLINE vec4d vd_zero() { return vd_from_halves(vdupq_n_f64(0.0), vdupq_n_f64(0.0)); }
VECTORCALL VECMATH_FINLINE vec4d vd_splats(double a) { return vd_from_halves(vdupq_n_f64(a), vdupq_n_f64(a)); }
VECTORCALL VECMATH_FINLINE vec4d vd_make_vec4d(double x, double y, double z, double w) {
  return vd_from_halves(vsetq_lane_f64(y, vdupq_n_f64(x), 1), vsetq_lane_f64(w, vdupq_n_f64(z), 1));
}

VECTORCALL VECMATH_FINLINE double vd_extract_x(vec4d a) { return vgetq_lane_f64(a.xy, 0); }
VECTORCALL VECMATH_FINLINE double vd_extract_y(vec4d a) { return vgetq_lane_f64(a.xy, 1); }
VECTORCALL VECMATH_FINLINE double vd_extract_z(vec4d a) { return vgetq_lane_f64(a.zw, 0); }
VECTORCALL VECMATH_FINLINE double vd_extract_w(vec4d a) { return vgetq_lane_f64(a.zw, 1); }

VECTORCALL VECMATH_FINLINE vec4d vd_insert_x(vec4d a, double x) { return vd_from_halves(vsetq_lane_f64(x, a.xy, 0), a.zw); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_y(vec4d a, double y) { return vd_from_halves(vsetq_lane_f64(y, a.xy, 1), a.zw); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_z(vec4d a, double z) { return vd_from_halves(a.xy, vsetq_lane_f64(z, a.zw, 0)); }
VECTORCALL VECMATH_FINLINE vec4d vd_insert_w(vec4d a, double w) { return vd_from_halves(a.xy, vsetq_lane_f64(w, a.zw, 1)); }

VECTORCALL VECMATH_FINLINE vec4d vd_ld(const double *m)  { return vd_from_halves(vld1q_f64(m), vld1q_f64(m + 2)); }
VECTORCALL VECMATH_FINLINE vec4d vd_ldu(const double *m) { return vd_from_halves(vld1q_f64(m), vld1q_f64(m + 2)); }
VECTORCALL VECMATH_FINLINE void vd_st(double *m, vec4d a)  { vst1q_f64(m, a.xy); vst1q_f64(m + 2, a.zw); }
VECTORCALL VECMATH_FINLINE void vd_stu(double *m, vec4d a) { vst1q_f64(m, a.xy); vst1q_f64(m + 2, a.zw); }

// DPoint3 layout: 3 packed doubles. _safe reads exactly 3 (.w = 0); store writes exactly 3.
NO_ASAN_INLINE vec4d vd_ldu_p3_safe(const double *m) {
  return vd_from_halves(vld1q_f64(m), vld1q_lane_f64(m + 2, vdupq_n_f64(0.0), 0));
}
VECTORCALL VECMATH_FINLINE void vd_stu_p3(double *p3, vec4d v) { vst1q_f64(p3, v.xy); vst1q_lane_f64(p3 + 2, v.zw, 0); }

VECTORCALL VECMATH_FINLINE vec4d vd_cvt_from_vec4f(vec4f a) {
  return vd_from_halves(vcvt_f64_f32(vget_low_f32(a)), vcvt_f64_f32(vget_high_f32(a)));
}
VECTORCALL VECMATH_FINLINE vec4f vd_cvt_to_vec4f(vec4d a) {
  return vcombine_f32(vcvt_f32_f64(a.xy), vcvt_f32_f64(a.zw));
}
// widen s32 to s64 first: going through f32 would lose precision for large ints
VECTORCALL VECMATH_FINLINE vec4d vd_cvt_from_vec4i(vec4i a) {
  return vd_from_halves(vcvtq_f64_s64(vmovl_s32(vget_low_s32(a))), vcvtq_f64_s64(vmovl_s32(vget_high_s32(a))));
}
// truncates toward zero, like v_cvt_vec4i; out-of-range input is platform-specific
VECTORCALL VECMATH_FINLINE vec4i vd_cvt_to_vec4i(vec4d a) {
  return vcombine_s32(vmovn_s64(vcvtq_s64_f64(a.xy)), vmovn_s64(vcvtq_s64_f64(a.zw)));
}

VECTORCALL VECMATH_FINLINE vec4d vd_add(vec4d a, vec4d b) { return vd_from_halves(vaddq_f64(a.xy, b.xy), vaddq_f64(a.zw, b.zw)); }
VECTORCALL VECMATH_FINLINE vec4d vd_sub(vec4d a, vec4d b) { return vd_from_halves(vsubq_f64(a.xy, b.xy), vsubq_f64(a.zw, b.zw)); }
VECTORCALL VECMATH_FINLINE vec4d vd_mul(vec4d a, vec4d b) { return vd_from_halves(vmulq_f64(a.xy, b.xy), vmulq_f64(a.zw, b.zw)); }
VECTORCALL VECMATH_FINLINE vec4d vd_div(vec4d a, vec4d b) { return vd_from_halves(vdivq_f64(a.xy, b.xy), vdivq_f64(a.zw, b.zw)); }
VECTORCALL VECMATH_FINLINE vec4d vd_neg(vec4d a) { return vd_from_halves(vnegq_f64(a.xy), vnegq_f64(a.zw)); }
// compare + select instead of fmin/fmax: matches SSE minpd/maxpd exactly (a < b ? a : b,
// b wins on NaN and equal incl. +/-0), same reason as the float v_min/v_max
VECTORCALL VECMATH_FINLINE vec4d vd_min(vec4d a, vec4d b) {
  return vd_from_halves(vbslq_f64(vcltq_f64(a.xy, b.xy), a.xy, b.xy), vbslq_f64(vcltq_f64(a.zw, b.zw), a.zw, b.zw));
}
VECTORCALL VECMATH_FINLINE vec4d vd_max(vec4d a, vec4d b) {
  return vd_from_halves(vbslq_f64(vcgtq_f64(a.xy, b.xy), a.xy, b.xy), vbslq_f64(vcgtq_f64(a.zw, b.zw), a.zw, b.zw));
}
VECTORCALL VECMATH_FINLINE vec4d vd_sqrt(vec4d a) { return vd_from_halves(vsqrtq_f64(a.xy), vsqrtq_f64(a.zw)); }
VECTORCALL VECMATH_FINLINE vec4d vd_sqrt_x(vec4d a) { return vd_from_halves(vsqrtq_f64(a.xy), a.zw); }

// Left to right ((x+y)+z)+w, matching scalar association; see the x86 note above. faddp
// would fold the first pair in one instruction but pairs as (x+y),(z+w), which does not match.
VECTORCALL VECMATH_FINLINE vec4d vd_hadd4_x(vec4d a) {
  float64x1_t s = vadd_f64(vget_low_f64(a.xy), vget_high_f64(a.xy)); // x+y
  s = vadd_f64(s, vget_low_f64(a.zw));                               // +z
  s = vadd_f64(s, vget_high_f64(a.zw));                              // +w
  float64x2_t r = vcombine_f64(s, s);
  return vd_from_halves(r, r);
}
VECTORCALL VECMATH_FINLINE vec4d vd_hadd4(vec4d a) { return vd_hadd4_x(a); }
VECTORCALL VECMATH_FINLINE vec4d vd_hadd3_x(vec4d a) {
  float64x1_t s = vadd_f64(vget_low_f64(a.xy), vget_high_f64(a.xy)); // x+y
  s = vadd_f64(s, vget_low_f64(a.zw));                               // +z
  float64x2_t r = vcombine_f64(s, s);
  return vd_from_halves(r, r);
}
VECTORCALL VECMATH_FINLINE vec4d vd_hadd3(vec4d a) { return vd_hadd3_x(a); }

VECTORCALL VECMATH_FINLINE vec4d vd_dot4(vec4d a, vec4d b) { return vd_hadd4(vd_mul(a, b)); }
VECTORCALL VECMATH_FINLINE vec4d vd_dot4_x(vec4d a, vec4d b) { return vd_hadd4_x(vd_mul(a, b)); }
VECTORCALL VECMATH_FINLINE vec4d vd_dot3(vec4d a, vec4d b) { return vd_hadd3(vd_mul(a, b)); }
VECTORCALL VECMATH_FINLINE vec4d vd_dot3_x(vec4d a, vec4d b) { return vd_hadd3_x(vd_mul(a, b)); }

// r.x = ay*bz - az*by, r.y = az*bx - ax*bz, r.z = ax*by - ay*bx; .w unspecified
VECTORCALL VECMATH_FINLINE vec4d vd_cross3(vec4d a, vec4d b) {
  float64x2_t a_lo = a.xy, a_hi = a.zw, b_lo = b.xy, b_hi = b.zw;
  float64x2_t r_lo = vsubq_f64(vmulq_f64(vextq_f64(a_lo, a_hi, 1), vtrn1q_f64(b_hi, b_lo)),
                               vmulq_f64(vtrn1q_f64(a_hi, a_lo), vextq_f64(b_lo, b_hi, 1)));
  float64x2_t r_hi = vsubq_f64(vmulq_f64(a_lo, vextq_f64(b_lo, b_lo, 1)),
                               vmulq_f64(vextq_f64(a_lo, a_lo, 1), b_lo));
  return vd_from_halves(r_lo, r_hi);
}

#else
 !error! unsupported target
#endif

// ------------------------------------------------------------------------------------------------
// composed, platform-independent
// ------------------------------------------------------------------------------------------------

// Fast double[3] load by one 4x64 bit load: reads 8 bytes past the 3 doubles, so the .w lane
// holds whatever follows. Safe on the stack, but a heap array of packed DPoint3 whose last
// element ends exactly at a page end crashes on the final load - use vd_ldu_p3_safe there.
// Same trade-off and the same sanitizer routing as v_ldu_p3 (see dag_vecMath_common.h).
NO_ASAN_INLINE vec4d vd_ldu_p3(const double *m)
{
#if defined(DAGOR_TSAN_ENABLED) || defined(DAGOR_ASAN_ENABLED)
  return vd_ldu_p3_safe(m);
#else
  return vd_ldu(m);
#endif
}
VECTORCALL VECMATH_FINLINE vec4d vd_clamp(vec4d t, vec4d min_val, vec4d max_val) { return vd_max(vd_min(t, max_val), min_val); }

VECTORCALL VECMATH_FINLINE vec4d vd_length4_sq(vec4d a) { return vd_dot4(a, a); }
VECTORCALL VECMATH_FINLINE vec4d vd_length3_sq(vec4d a) { return vd_dot3(a, a); }
VECTORCALL VECMATH_FINLINE vec4d vd_length4(vec4d a)   { return vd_sqrt(vd_dot4(a, a)); }
VECTORCALL VECMATH_FINLINE vec4d vd_length4_x(vec4d a) { return vd_sqrt_x(vd_dot4_x(a, a)); }
VECTORCALL VECMATH_FINLINE vec4d vd_length3(vec4d a)   { return vd_sqrt(vd_dot3(a, a)); }
VECTORCALL VECMATH_FINLINE vec4d vd_length3_x(vec4d a) { return vd_sqrt_x(vd_dot3_x(a, a)); }

#ifdef _MSC_VER
  #pragma warning(pop)
#endif
