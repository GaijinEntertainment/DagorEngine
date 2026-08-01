//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

VECTORCALL VECMATH_FINLINE vec4f v_zero() { return vdupq_n_f32(0); }
VECTORCALL VECMATH_FINLINE vec4i v_zeroi() { return vdupq_n_u32(0); }
VECTORCALL VECMATH_FINLINE vec4f v_set_all_bits() { return v_cast_vec4f(vmvnq_u32(vdupq_n_u32(0))); }
VECTORCALL VECMATH_FINLINE vec4i v_set_all_bitsi() { return vmvnq_u32(vdupq_n_u32(0)); }
VECTORCALL VECMATH_FINLINE vec4f v_msbit() { return (vec4f)vdupq_n_u32(0x80000000); }
VECTORCALL VECMATH_FINLINE vec4f v_ld(const float *m) { return vld1q_f32(m); }
VECTORCALL VECMATH_FINLINE vec4f v_ldu(const float *m) { return vld1q_f32(m); }
VECTORCALL VECMATH_FINLINE void v_ld_soa2(const float *m, vec4f &x, vec4f &y)
{
  float32x4x2_t r = vld2q_f32(m);
  x = r.val[0];
  y = r.val[1];
}
VECTORCALL VECMATH_FINLINE void v_ld_soa3(const float *m, vec4f &x, vec4f &y, vec4f &z)
{
  float32x4x3_t r = vld3q_f32(m);
  x = r.val[0];
  y = r.val[1];
  z = r.val[2];
}
VECTORCALL VECMATH_FINLINE void v_ld_soa4(const float *m, vec4f &x, vec4f &y, vec4f &z, vec4f &w)
{
  float32x4x4_t r = vld4q_f32(m);
  x = r.val[0];
  y = r.val[1];
  z = r.val[2];
  w = r.val[3];
}
// vld2/3/4q have no alignment requirement
VECTORCALL VECMATH_FINLINE void v_ldu_soa2(const float *m, vec4f &x, vec4f &y) { v_ld_soa2(m, x, y); }
VECTORCALL VECMATH_FINLINE void v_ldu_soa3(const float *m, vec4f &x, vec4f &y, vec4f &z) { v_ld_soa3(m, x, y, z); }
VECTORCALL VECMATH_FINLINE void v_ldu_soa4(const float *m, vec4f &x, vec4f &y, vec4f &z, vec4f &w) { v_ld_soa4(m, x, y, z, w); }

VECTORCALL VECMATH_FINLINE void v_st_soa2(float *m, vec4f x, vec4f y)
{
  float32x4x2_t r = {{x, y}};
  vst2q_f32(m, r);
}
VECTORCALL VECMATH_FINLINE void v_st_soa3(float *m, vec4f x, vec4f y, vec4f z)
{
  float32x4x3_t r = {{x, y, z}};
  vst3q_f32(m, r);
}
VECTORCALL VECMATH_FINLINE void v_st_soa4(float *m, vec4f x, vec4f y, vec4f z, vec4f w)
{
  float32x4x4_t r = {{x, y, z, w}};
  vst4q_f32(m, r);
}
// vst2/3/4q have no alignment requirement
VECTORCALL VECMATH_FINLINE void v_stu_soa2(float *m, vec4f x, vec4f y) { v_st_soa2(m, x, y); }
VECTORCALL VECMATH_FINLINE void v_stu_soa3(float *m, vec4f x, vec4f y, vec4f z) { v_st_soa3(m, x, y, z); }
VECTORCALL VECMATH_FINLINE void v_stu_soa4(float *m, vec4f x, vec4f y, vec4f z, vec4f w) { v_st_soa4(m, x, y, z, w); }
VECTORCALL VECMATH_FINLINE vec4f v_ldu_x(const float *m) { return vsetq_lane_f32(*m, v_zero(), 0); } // load x, zero others
VECTORCALL VECMATH_FINLINE vec4i v_ldi(const int *m) { return vld1q_s32(m); }
VECTORCALL VECMATH_FINLINE vec4i v_ldui(const int *m) { return vld1q_s32(m); }
VECTORCALL VECMATH_FINLINE vec4i v_ldush(const signed short *m) { return vmovl_s16(vld1_s16(m)); }
VECTORCALL VECMATH_FINLINE vec4i v_lduush(const unsigned short *m) { return (vec4i)vmovl_u16(vld1_u16(m)); }
VECTORCALL VECMATH_FINLINE vec3f v_ldu_p3_safe(const float *m) { return vcombine_f32(vld1_f32(m), vld1_lane_f32(m + 2, vdup_n_f32(0), 0)); } // 2 loads + INS
VECTORCALL VECMATH_FINLINE vec4i v_ldui_p3_safe(const int *m) { return vcombine_s32(vld1_s32(m), vld1_lane_s32(m + 2, vdup_n_s32(0), 0)); }
VECTORCALL VECMATH_FINLINE vec4f v_splat_x(vec4f a) { return vdupq_lane_f32(vget_low_f32(a), 0); }
VECTORCALL VECMATH_FINLINE vec4f v_splat_y(vec4f a) { return vdupq_lane_f32(vget_low_f32(a), 1); }
VECTORCALL VECMATH_FINLINE vec4f v_splat_z(vec4f a) { return vdupq_lane_f32(vget_high_f32(a), 0); }
VECTORCALL VECMATH_FINLINE vec4f v_splat_w(vec4f a) { return vdupq_lane_f32(vget_high_f32(a), 1); }
VECTORCALL VECMATH_FINLINE vec4i v_splat_xi(vec4i a) { return vdupq_lane_s32(vget_low_s32(a), 0); }
VECTORCALL VECMATH_FINLINE vec4i v_splat_yi(vec4i a) { return vdupq_lane_s32(vget_low_s32(a), 1); }
VECTORCALL VECMATH_FINLINE vec4i v_splat_zi(vec4i a) { return vdupq_lane_s32(vget_high_s32(a), 0); }
VECTORCALL VECMATH_FINLINE vec4i v_splat_wi(vec4i a) { return vdupq_lane_s32(vget_high_s32(a), 1); }

VECTORCALL VECMATH_FINLINE vec4f v_splats(float a) { return vmovq_n_f32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_splatsi(int a) {return vmovq_n_s32(a);}
VECTORCALL VECMATH_FINLINE vec4i v_splatsi64(int64_t a) {return vdupq_n_s64(a);}
VECTORCALL VECMATH_FINLINE vec4f v_set_x(float a) { return vsetq_lane_f32(a, v_zero(), 0); } // set x, zero others
VECTORCALL VECMATH_FINLINE vec4i v_seti_x(int a) { return vsetq_lane_s32(a, v_zero(), 0); } // set x, zero others
VECTORCALL VECMATH_FINLINE vec4f v_make_vec4f(float x, float y, float z, float w)
{
#if defined(_MSC_VER) && !defined(__clang__)
  alignas(16) float data[4] = { x, y, z, w };
  return vld1q_f32(data);
#else
  return (vec4f){x, y, z, w};
#endif
}
VECTORCALL VECMATH_FINLINE vec4i v_make_vec4i(int x, int y, int z, int w)
{
  alignas(16) int data[4] = { x, y, z, w };
  return vld1q_s32(data);
}
VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(float x, float y, float z) { return v_make_vec4f(x, y, z, z); }
VECTORCALL VECMATH_FINLINE vec4i v_make_vec3i(int x, int y, int z) { return v_make_vec4i(x, y, z, z); }

VECTORCALL VECMATH_FINLINE void v_st(void *m, vec4f v)  { vst1q_f32((float*)m, v); }
VECTORCALL VECMATH_FINLINE void v_stu(void *m, vec4f v) { vst1q_f32((float*)m, v); }
VECTORCALL VECMATH_FINLINE void v_sti(void *m, vec4i v)  { vst1q_s32((int*)m, v); }
VECTORCALL VECMATH_FINLINE void v_stui(void *m, vec4i v) { vst1q_s32((int*)m, v); }
VECTORCALL VECMATH_FINLINE void v_stui_half(void *m, vec4i v) { vst1_s32((int*)m, vget_low_s32(v)); }
VECTORCALL VECMATH_FINLINE void v_stu_half(void *m, vec4f v) { vst1_f32((float*)m, vget_low_f32(v)); }
VECTORCALL VECMATH_FINLINE void v_stu_p3(float *p3, vec3f v) { v_stu_half(p3, v); p3[2] = v_extract_z(v); }
VECTORCALL VECMATH_FINLINE void v_stui_p3(int *p3, vec4i v) { v_stui_half(p3, v); p3[2] = v_extract_zi(v); }

VECTORCALL VECMATH_FINLINE vec4f v_merge_hw(vec4f a, vec4f b)
{
  return vzipq_f32(a, b).val[0];
}
VECTORCALL VECMATH_FINLINE vec4f v_merge_lw(vec4f a, vec4f b)
{
  return vzipq_f32(a, b).val[1];
}

VECTORCALL VECMATH_FINLINE int v_signmask(vec4f a)
{
  // arithmetic shift + and: integer vector multiply is slow on little cores and Cortex-A57
  alignas(16) static const uint32_t movemask[4] = {1, 2, 4, 8};
  uint32x4_t signs = vreinterpretq_u32_s32(vshrq_n_s32(vreinterpretq_s32_f32(a), 31));
  return vaddvq_u32(vandq_u32(signs, vld1q_u32(movemask)));
}

// canonical mask lanes are already all-ones/zero, no sign isolation needed
VECTORCALL VECMATH_FINLINE int v_truemask(vec4f a)
{
  alignas(16) static const uint32_t movemask[4] = {1, 2, 4, 8};
  return vaddvq_u32(vandq_u32(vreinterpretq_u32_f32(a), vld1q_u32(movemask)));
}
VECTORCALL VECMATH_FINLINE int v_count_true(vec4f a)
{
  // a true lane is all-ones == -1; one addv sums them, negate for the count
  return -vaddvq_s32(vreinterpretq_s32_f32(a));
}

// a lane's sign bit is set iff its bit pattern as a signed int is < 0; sminv beats the positional mask
VECTORCALL VECMATH_FINLINE bool v_is_any_neg_b(vec4f a)
{
  return vminvq_s32(vreinterpretq_s32_f32(a)) < 0;
}

VECTORCALL VECMATH_FINLINE int v_is_merge_planes_nout(vec4f m0, vec4f m1, vec4f m2, vec4f m3, vec4f m4, vec4f m5)
{
  // pairwise max == pairwise OR for canonical masks; the umaxp tree keeps the merge in vector
  // regs with one vector->GPR crossing, instead of six serialized umaxv+fmov reductions
  uint32x4_t r01 = vpmaxq_u32(vreinterpretq_u32_f32(m0), vreinterpretq_u32_f32(m1));
  uint32x4_t r23 = vpmaxq_u32(vreinterpretq_u32_f32(m2), vreinterpretq_u32_f32(m3));
  uint32x4_t r45 = vpmaxq_u32(vreinterpretq_u32_f32(m4), vreinterpretq_u32_f32(m5));
  uint32x4_t p0123 = vpmaxq_u32(r01, r23); // (p0, p1, p2, p3)
  uint32x4_t p4545 = vpmaxq_u32(r45, r45); // (p4, p5, p4, p5)
  // lanes stay canonical through umaxp/and, so the reduced min is already 0 or all-ones
  return int(vminvq_u32(vandq_u32(p0123, p4545)));
}

VECTORCALL VECMATH_FINLINE vec4f v_min_pairs(vec4f a, vec4f b) { return vpminq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_max_pairs(vec4f a, vec4f b) { return vpmaxq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_add_pairs(vec4f a, vec4f b) { return vpaddq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_addi_pairs(vec4i a, vec4i b) { return vpaddq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_mini_pairs(vec4i a, vec4i b) { return vpminq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_maxi_pairs(vec4i a, vec4i b) { return vpmaxq_s32(a, b); }

// horizontal reductions keep the test to a single vector->GPR crossing
VECTORCALL VECMATH_FINLINE bool v_test_all_bits_zeros(vec4f a)
{
  return vmaxvq_u32(vreinterpretq_u32_f32(a)) == 0u;
}

VECTORCALL VECMATH_FINLINE bool v_test_all_bits_ones(vec4f a)
{
  return vminvq_u32(vreinterpretq_u32_f32(a)) == ~0u;
}

VECTORCALL VECMATH_FINLINE bool v_test_any_bit_set(vec4f a)
{
  return !v_test_all_bits_zeros(a);
}

VECTORCALL VECMATH_FINLINE bool v_check_xyzw_all_true(vec4f a) { return v_test_all_bits_ones(a); }
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_all_false(vec4f a) { return v_test_all_bits_zeros(a); }
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_any_true(vec4f a) { return v_test_any_bit_set(a); }

VECTORCALL VECMATH_FINLINE bool v_check_xyz_all_true(vec4f a) { return v_check_xyzw_all_true(v_perm_xyzz(a)); }
VECTORCALL VECMATH_FINLINE bool v_check_xyz_all_false(vec4f a) { return v_check_xyzw_all_false(v_perm_xyzz(a)); }
VECTORCALL VECMATH_FINLINE bool v_check_xyz_any_true(vec4f a) { return v_check_xyzw_any_true(v_perm_xyzz(a)); }

VECTORCALL VECMATH_FINLINE vec4f is_neg_special(vec4f a)
{
  vec4f msbit = v_msbit();
  return v_cmp_eqi(v_and(a, msbit), msbit);
}

VECTORCALL VECMATH_FINLINE vec4f v_cmp_eq(vec4f a, vec4f b) { return (vec4f)vceqq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_neq(vec4f a, vec4f b) { return (vec4f)vmvnq_u32(vceqq_f32(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_eqi(vec4f a, vec4f b) { return (vec4f)vceqq_s32((vec4i)a, (vec4i)b); }
#if defined(__clang__) || defined(__GNUC__)
VECTORCALL VECMATH_FINLINE vec4i v_cmp_eqi(vec4i a, vec4i b) { return (vec4i)vceqq_s32(a, b); }
#endif
VECTORCALL VECMATH_FINLINE vec4f v_cmp_ge(vec4f a, vec4f b) { return (vec4f)vcgeq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_gt(vec4f a, vec4f b) { return (vec4f)vcgtq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_cmp_lti(vec4i a, vec4i b) { return (vec4i)vcltq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_cmp_gti(vec4i a, vec4i b) { return (vec4i)vcgtq_s32(a, b); }

VECTORCALL VECMATH_FINLINE vec4f v_and(vec4f a, vec4f b) { return (vec4f)vandq_s32((vec4i)a, (vec4i)b); }
VECTORCALL VECMATH_FINLINE vec4f v_andnot(vec4f a, vec4f b) { return (vec4f)vandq_s32(vmvnq_s32((vec4i)a), (vec4i)b); }
VECTORCALL VECMATH_FINLINE vec4f v_or(vec4f a, vec4f b) { return (vec4f)vorrq_s32((vec4i)a, (vec4i)b); }
VECTORCALL VECMATH_FINLINE vec4f v_xor(vec4f a, vec4f b) { return (vec4f)veorq_s32((vec4i)a, (vec4i)b); }
VECTORCALL VECMATH_FINLINE vec4f v_not(vec4f a) { return (vec4f)vmvnq_u32((vec4i)a); }
VECTORCALL VECMATH_FINLINE vec4f v_sel(vec4f a, vec4f b, vec4f c)
{
  return vbslq_f32((uint32x4_t)vshrq_n_s32(vreinterpretq_s32_f32(c), 31), b, a);
}
VECTORCALL VECMATH_FINLINE vec4i v_seli(vec4i a, vec4i b, vec4i c) { return vbslq_s32((uint32x4_t)vshrq_n_s32(c, 31), b, a); }
VECTORCALL VECMATH_FINLINE vec4f v_btsel(vec4f a, vec4f b, vec4f c) { return vbslq_f32((uint32x4_t)c, b, a); }
VECTORCALL VECMATH_FINLINE vec4i v_btseli(vec4i a, vec4i b, vec4i c) { return vbslq_s32((uint32x4_t)c, b, a); }

VECTORCALL VECMATH_FINLINE vec4i v_cvti_vec4i(vec4f a) { return vcvtq_s32_f32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i_ieee(vec4f a) { return vcvtq_u32_f32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i(vec4f a) { return v_cvtu_vec4i_ieee(a); }
VECTORCALL VECMATH_FINLINE vec4f v_cvtu_vec4f_ieee(vec4i a) { return vcvtq_f32_u32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_cvtu_vec4f(vec4i a) { return v_cvtu_vec4f_ieee(a); }
VECTORCALL VECMATH_FINLINE vec4f v_cvti_vec4f(vec4i a) { return vcvtq_f32_s32(a); }

VECTORCALL VECMATH_FINLINE vec4i v_cast_vec4i(vec4f a) { return vreinterpretq_s32_f32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_cast_vec4f(vec4i a) { return vreinterpretq_f32_s32(a); }

VECTORCALL VECMATH_FINLINE vec4f v_floor(vec4f a) { return vrndmq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_ceil(vec4f a) { return vrndpq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_trunc(vec4f a) { return vrndq_f32(a); }
// frinta/fcvtas round ties away from zero natively, one instruction; SSE has no such mode and
// pays a compare sequence for the same result (dag_vecMath_pc_sse.h)
VECTORCALL VECMATH_FINLINE vec4f v_round(vec4f a) { return vrndaq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi(vec4f a) { return vcvtaq_s32_f32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_trunci(vec4f a) { return v_cvti_vec4i(a); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_floori(vec4f a) { return vcvtq_s32_f32(vrndmq_f32(a)); }

VECTORCALL VECMATH_FINLINE vec4f sse4_floor(vec4f a) { return v_floor(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_ceil(vec4f a) { return v_ceil(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_round(vec4f a) { return v_round(a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_floori(vec4f a) { return v_cvt_floori(a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_ceili(vec4f a)  { return v_cvt_ceili(a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_trunci(vec4f a)  { return v_cvt_trunci(a); }

VECTORCALL VECMATH_FINLINE vec4i v_cvt_ceili(vec4f a) { return vcvtq_s32_f32(vrndpq_f32(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_round_ieee(vec4f a) { return vrndnq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi_ieee(vec4f a) { return vcvtnq_s32_f32(a); }

VECTORCALL VECMATH_FINLINE vec4f v_add(vec4f a, vec4f b) { return vaddq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_sub(vec4f a, vec4f b) { return vsubq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_mul(vec4f a, vec4f b) { return vmulq_f32(a, b); }
#ifndef VECMATH_NO_FMA
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c) { return vfmaq_f32(c, a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c) { return vfmsq_f32(c, a, b); }
#else
// non-fused to match x86 -mno-fma builds; needs -ffp-contract=off and -fno-unsafe-math-optimizations
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c) { return vmlaq_f32(c, a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c) { return vmlsq_f32(c, a, b); }
#endif
VECTORCALL VECMATH_FINLINE vec4f v_msub(vec4f a, vec4f b, vec4f c) { return vsubq_f32(vmulq_f32(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_add_x(vec4f a, vec4f b) { return vaddq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_sub_x(vec4f a, vec4f b) { return vsubq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_mul_x(vec4f a, vec4f b) { return vmulq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_madd_x(vec4f a, vec4f b, vec4f c) { return v_madd(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4f v_msub_x(vec4f a, vec4f b, vec4f c) { return vsubq_f32(vmulq_f32(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub_x(vec4f a, vec4f b, vec4f c) { return v_nmsub(a, b, c); }

VECTORCALL VECMATH_FINLINE vec4i v_addi(vec4i a, vec4i b) { return vaddq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_subi(vec4i a, vec4i b) { return vsubq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_muli(vec4i a, vec4i b) { return vmulq_s32(a, b); }

VECTORCALL VECMATH_FINLINE vec4i v_addi16(vec4i a, vec4i b)
{ return vreinterpretq_s32_s16(vaddq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_subi16(vec4i a, vec4i b)
{ return vreinterpretq_s32_s16(vsubq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_muli16(vec4i a, vec4i b)
{ return vreinterpretq_s32_s16(vmulq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_mulhi16(vec4i a, vec4i b)
{
  int16x8_t sa = vreinterpretq_s16_s32(a), sb = vreinterpretq_s16_s32(b);
  int32x4_t lo = vmull_s16(vget_low_s16(sa), vget_low_s16(sb));
  int32x4_t hi = vmull_s16(vget_high_s16(sa), vget_high_s16(sb));
  return vreinterpretq_s32_s16(vcombine_s16(vshrn_n_s32(lo, 16), vshrn_n_s32(hi, 16)));
}
VECTORCALL VECMATH_FINLINE vec4i v_madd_i16(vec4i a, vec4i b)
{
  int16x8_t sa = vreinterpretq_s16_s32(a), sb = vreinterpretq_s16_s32(b);
  int32x4_t lo = vmull_s16(vget_low_s16(sa), vget_low_s16(sb));
  int32x4_t hi = vmull_s16(vget_high_s16(sa), vget_high_s16(sb));
  return vpaddq_s32(lo, hi);
}
VECTORCALL VECMATH_FINLINE vec4i v_splatsi16(int v)
{ return vreinterpretq_s32_s16(vdupq_n_s16((int16_t)v)); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i8(vec4i a, vec4i b)
{ return vreinterpretq_s32_u8(vzip1q_u8(vreinterpretq_u8_s32(a), vreinterpretq_u8_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i8(vec4i a, vec4i b)
{ return vreinterpretq_s32_u8(vzip2q_u8(vreinterpretq_u8_s32(a), vreinterpretq_u8_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i16(vec4i a, vec4i b)
{ return vreinterpretq_s32_u16(vzip1q_u16(vreinterpretq_u16_s32(a), vreinterpretq_u16_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i16(vec4i a, vec4i b)
{ return vreinterpretq_s32_u16(vzip2q_u16(vreinterpretq_u16_s32(a), vreinterpretq_u16_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i32(vec4i a, vec4i b)
{ return vreinterpretq_s32_u32(vzip1q_u32(vreinterpretq_u32_s32(a), vreinterpretq_u32_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i32(vec4i a, vec4i b)
{ return vreinterpretq_s32_u32(vzip2q_u32(vreinterpretq_u32_s32(a), vreinterpretq_u32_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i64(vec4i a, vec4i b)
{ return vreinterpretq_s32_u64(vzip1q_u64(vreinterpretq_u64_s32(a), vreinterpretq_u64_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i64(vec4i a, vec4i b)
{ return vreinterpretq_s32_u64(vzip2q_u64(vreinterpretq_u64_s32(a), vreinterpretq_u64_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_perm_i8(vec4i t, vec4i k)
{
  // match pshufb exactly: index bits 4..6 are ignored, bit 7 zeroes the lane
  // (vqtbl1q returns 0 for any out-of-range index)
  uint8x16_t ki = vandq_u8(vreinterpretq_u8_s32(k), vdupq_n_u8(0x8F));
  return vreinterpretq_s32_u8(vqtbl1q_u8(vreinterpretq_u8_s32(t), ki));
}
VECTORCALL VECMATH_FINLINE vec4i v_cmp_eqi8(vec4i a, vec4i b)
{ return vreinterpretq_s32_u8(vceqq_u8(vreinterpretq_u8_s32(a), vreinterpretq_u8_s32(b))); }

VECTORCALL VECMATH_FINLINE vec4f v_hadd4_x(vec4f a)
{
  // dup, not v_set_x: rebuilding a vector around the scalar reduction result costs
  // an extra zero + insert, while dup is the cheapest way back to a vec4f
  return vdupq_n_f32(vaddvq_f32(a));
}
VECTORCALL VECMATH_FINLINE vec4f v_hadd3_x(vec4f a)
{
  vec4f s = v_add_x(a, v_splat_y(a));
  return v_add_x(s, v_splat_z(a));
}

// horizontal min/max via dedicated fminv/fmaxv instructions; the *3 variants first
// duplicate .z into the unspecified .w lane so the full 4-lane reduction is valid
VECTORCALL VECMATH_FINLINE vec4f v_hmin(vec4f a) { return vdupq_n_f32(vminvq_f32(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_hmax(vec4f a) { return vdupq_n_f32(vmaxvq_f32(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_hmin3(vec3f a) { return v_hmin(vcopyq_laneq_f32(a, 3, a, 2)); }
VECTORCALL VECMATH_FINLINE vec4f v_hmax3(vec3f a) { return v_hmax(vcopyq_laneq_f32(a, 3, a, 2)); }
VECTORCALL VECMATH_FINLINE vec4i v_hmini(vec4i a) { return vdupq_n_s32(vminvq_s32(a)); }
VECTORCALL VECMATH_FINLINE vec4i v_hmaxi(vec4i a) { return vdupq_n_s32(vmaxvq_s32(a)); }
VECTORCALL VECMATH_FINLINE vec4i v_hmini3(vec4i a) { return v_hmini(vcopyq_laneq_s32(a, 3, a, 2)); }
VECTORCALL VECMATH_FINLINE vec4i v_hmaxi3(vec4i a) { return v_hmaxi(vcopyq_laneq_s32(a, 3, a, 2)); }

VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise(vec4f a) { return vrecpeq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_iter(vec4f a, vec4f est)
{
  return vmulq_f32(est, vrecpsq_f32(est, a));
}
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est(vec4f a)
{
  return v_rcp_iter(a, v_rcp_iter(a, vrecpeq_f32(a)));
}
VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise_x(vec4f a) { return vrecpeq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est_x(vec4f a) { return v_rcp_est(a); }
VECTORCALL VECMATH_FINLINE vec4f v_div(vec4f a, vec4f b) { return vdivq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_div_x(vec4f a, vec4f b) { float32x2_t r = vdiv_f32(vget_low_f32(a), vget_low_f32(b)); return vcombine_f32(r, r); }
// compare + select instead of fmin/fmax: matches SSE minps/maxps exactly (a < b ? a : b,
// b wins on NaN and equal incl. +/-0) so default results stay cross-platform identical
VECTORCALL VECMATH_FINLINE vec4f v_min(vec4f a, vec4f b) { return vbslq_f32(vcltq_f32(a, b), a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_max(vec4f a, vec4f b) { return vbslq_f32(vcgtq_f32(a, b), a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_neg(vec4f a) { return vnegq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_negi(vec4i a){ return vnegq_s32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_abs(vec4f a) { return vabsq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4i v_absi(vec4i a) { return vabsq_s32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_abs_diff(vec4f a, vec4f b) { return vabdq_f32(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_abs_ge(vec4f a, vec4f b) { return vreinterpretq_f32_u32(vcageq_f32(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_abs_gt(vec4f a, vec4f b) { return vreinterpretq_f32_u32(vcagtq_f32(a, b)); }
VECTORCALL VECMATH_FINLINE vec4i v_maxi(vec4i a, vec4i b) { return vmaxq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_mini(vec4i a, vec4i b) { return vminq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_maxu(vec4i a, vec4i b) { return vmaxq_u32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_minu(vec4i a, vec4i b) { return vminq_u32(a, b); }

VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_unprecise(vec4f a) { return vrsqrteq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_unprecise_x(vec4f a)
{
  float32x2_t e = vrsqrte_f32(vget_low_f32(a));
  return vcombine_f32(e, e);
}

// Precision ~equal to rsqrt+1 NR round on sse
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_est(vec4f a)
{
  vec4f e = vrsqrteq_f32(a);
  e = vmulq_f32(e, vrsqrtsq_f32(vmulq_f32(e, e), a));
  return vmulq_f32(e, vrsqrtsq_f32(vmulq_f32(e, e), a));
}

VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_est_x(vec4f a)
{
  float32x2_t lo = vget_low_f32(a);
  float32x2_t e = vrsqrte_f32(lo);
  e = vmul_f32(e, vrsqrts_f32(vmul_f32(e, e), lo));
  e = vmul_f32(e, vrsqrts_f32(vmul_f32(e, e), lo));
  return vcombine_f32(e, e);
}

VECTORCALL VECMATH_FINLINE vec4f v_sqrt(vec4f a) { return vsqrtq_f32(a); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_x(vec4f _a)
{
  float32x2_t r = vsqrt_f32(vget_low_f32(_a));
  return vcombine_f32(r, r);
}

VECTORCALL VECMATH_FINLINE vec4f v_rot_1(vec4f a) { return vextq_f32(a, a, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_rot_2(vec4f a) { return vextq_f32(a, a, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_rot_3(vec4f a) { return vextq_f32(a, a, 3); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_1(vec4i a) { return vextq_u32(a, a, 1); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_2(vec4i a) { return vextq_u32(a, a, 2); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_3(vec4i a) { return vextq_u32(a, a, 3); }

// clang/gcc lower every shuffle below to optimal ext/zip/trn/uzp/rev (no tbl); grouped by
// instruction count of the clang aarch64 lowering
#if defined(__clang__) || defined(__GNUC__)
// 1 op: native zip/uzp/trn/ext/rev64/dup or a single lane insert
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxyy(vec4f v) { return __builtin_shufflevector(v, v, 0, 0, 1, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxzz(vec4f v) { return __builtin_shufflevector(v, v, 0, 0, 2, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyxy(vec4f v) { return __builtin_shufflevector(v, v, 0, 1, 0, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzz(vec4f v) { return __builtin_shufflevector(v, v, 0, 1, 2, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzxz(vec4f v) { return __builtin_shufflevector(v, v, 0, 2, 0, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywyw(vec4f v) { return __builtin_shufflevector(v, v, 1, 3, 1, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yxwz(vec4f v) { return __builtin_shufflevector(v, v, 1, 0, 3, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yyww(vec4f v) { return __builtin_shufflevector(v, v, 1, 1, 3, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwzw(vec4f v) { return __builtin_shufflevector(v, v, 2, 3, 2, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zzww(vec4f v) { return __builtin_shufflevector(v, v, 2, 2, 3, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xycd(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 1, 6, 7); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwcd(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 2, 3, 6, 7); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyab(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 1, 4, 5); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ayzw(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 4, 1, 2, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xbzw(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 5, 2, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xycw(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 1, 6, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzd(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 1, 2, 7); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzac(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 2, 4, 6); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywbd(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 3, 5, 7); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xazc(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 4, 2, 6); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ybwd(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 5, 3, 7); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzwa(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 2, 3, 4); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwab(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 2, 3, 4, 5); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wabc(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 3, 4, 5, 6); }
// 2 ops
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxy(vec4f v) { return __builtin_shufflevector(v, v, 1, 2, 0, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxyw(vec4f a) { return __builtin_shufflevector(a, a, 2, 0, 1, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxzx(vec4f v) { return __builtin_shufflevector(v, v, 2, 0, 2, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wwyy(vec4f v) { return __builtin_shufflevector(v, v, 3, 3, 1, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xaxa(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 4, 0, 4); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yybb(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 1, 5, 5); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxab(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 0, 4, 5); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzab(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 2, 4, 5); }
// 3 ops: prefer a 1-2 op pattern above when the layout is free to choose
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxw(vec4f v) { return __builtin_shufflevector(v, v, 1, 2, 0, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxx(vec4f v) { return __builtin_shufflevector(v, v, 1, 2, 0, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_bbyx(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 5, 5, 1, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_bzxx(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 5, 2, 0, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_caxx(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 6, 4, 0, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzbx(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 2, 5, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzya(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 2, 1, 4); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yaxx(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 4, 0, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yxxc(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 0, 0, 6); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxxb(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 2, 0, 0, 5); }
// 4 ops
VECTORCALL VECMATH_FINLINE vec4f v_perm_zayx(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 2, 4, 1, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(vec4f x, vec4f y, vec4f z)
{
  return __builtin_shufflevector(__builtin_shufflevector(x, y, 0, 0, 4, 4), z, 0, 2, 4, 4);
}
#else
// hand-written for other compilers (MSVC ARM64); same order and grouping as above
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxyy(vec4f v) { return vzip1q_f32(v, v); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxzz(vec4f v) { return vtrn1q_f32(v, v); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyxy(vec4f v) { return vdupq_laneq_f64(v, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzz(vec4f v) { return vcopyq_laneq_f32(v, 3, v, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzxz(vec4f v) { return vuzp1q_f32(v, v); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywyw(vec4f v) { return vuzp2q_f32(v, v); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yxwz(vec4f v) { return vrev64q_f32(v); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yyww(vec4f v) { return vtrn2q_f32(v, v); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwzw(vec4f v) { return vdupq_laneq_f64(v, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zzww(vec4f v) { return vzip2q_f32(v, v); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xycd(vec4f xyzw, vec4f abcd)
{
  return vcombine_f32(vget_low_f32(xyzw), vget_high_f32(abcd));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwcd(vec4f xyzw, vec4f abcd)
{
  return vcombine_f32(vget_high_f32(xyzw), vget_high_f32(abcd));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyab(vec4f xyzw, vec4f abcd) { return vextq_f32(vdupq_laneq_f64(xyzw, 0), abcd, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ayzw(vec4f xyzw, vec4f abcd) { return vcopyq_laneq_f32(xyzw, 0, abcd, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xbzw(vec4f xyzw, vec4f abcd) { return vcopyq_laneq_f32(xyzw, 1, abcd, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xycw(vec4f xyzw, vec4f abcd) { return vcopyq_laneq_f32(xyzw, 2, abcd, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzd(vec4f xyzw, vec4f abcd) { return vcopyq_laneq_f32(xyzw, 3, abcd, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzac(vec4f xyzw, vec4f abcd) { return vuzp1q_f32(xyzw, abcd); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywbd(vec4f xyzw, vec4f abcd) { return vuzp2q_f32(xyzw, abcd); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xazc(vec4f xyzw, vec4f abcd) { return vtrn1q_f32(xyzw, abcd); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_ybwd(vec4f xyzw, vec4f abcd) { return vtrn2q_f32(xyzw, abcd); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzwa(vec4f xyzw, vec4f abcd) { return vextq_f32(xyzw, abcd, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwab(vec4f xyzw, vec4f abcd) { return vextq_f32(xyzw, abcd, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wabc(vec4f xyzw, vec4f abcd) { return vextq_f32(xyzw, abcd, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxy(vec4f v) { return vextq_f32(vextq_f32(v, v, 3), v, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxyw(vec4f a)
{
  return vuzpq_f32(vextq_f32(a, a, 1), a).val[1];
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxzx(vec4f v) { return v_rot_1(vuzp1q_f32(v, v)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wwyy(vec4f v) { return v_rot_2(vtrn2q_f32(v, v)); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xaxa(vec4f xyzw, vec4f abcd)
{
  float32x2_t xy = vget_low_f32(xyzw);
  float32x2_t XY = vget_low_f32(abcd);
  float32x2x2_t xX_yY = vtrn_f32(xy, XY);
  return vcombine_f32(xX_yY.val[0], xX_yY.val[0]);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_yybb(vec4f xyzw, vec4f abcd)
{
  float32x2_t yy = vdup_lane_f32(vget_low_f32(xyzw), 1);
  float32x2_t bb = vdup_lane_f32(vget_low_f32(abcd), 1);
  return vcombine_f32(yy, bb);
}
// (xyzw.x, xyzw.x, abcd.x, abcd.y): low half = duplicate-low of xyzw, high half = unchanged low of abcd.
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxab(vec4f xyzw, vec4f abcd)
{
  return vcombine_f32(vdup_lane_f32(vget_low_f32(xyzw), 0), vget_low_f32(abcd));
}
// (xyzw.y, xyzw.z, abcd.x, abcd.y): low half via vext (slide 1 across xyzw's halves) = (y, z); high half = abcd's low.
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzab(vec4f xyzw, vec4f abcd)
{
  return vcombine_f32(vext_f32(vget_low_f32(xyzw), vget_high_f32(xyzw), 1), vget_low_f32(abcd));
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxw(vec4f v) { return vzip2q_f32(vzip1q_f32(v, vextq_f32(v, v, 3)), v); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxx(vec4f v) { return vcopyq_laneq_f32(vextq_f32(v, v, 1), 2, v, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_bbyx(vec4f xyzw, vec4f abcd)
{
  float32x2_t bb = vdup_lane_f32(vget_low_f32(abcd), 1);
  float32x2_t yx = vrev64_f32(vget_low_f32(xyzw));
  return vcombine_f32(bb, yx);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_bzxx(vec4f xyzw, vec4f abcd)
{
  float32x2_t bz = vext_f32(vget_low_f32(abcd), vget_high_f32(xyzw), 1);
  float32x2_t xx = vdup_lane_f32(vget_low_f32(xyzw), 0);
  return vcombine_f32(bz, xx);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_caxx(vec4f xyzw, vec4f abcd)
{
  float32x2_t dc = vrev64_f32(vget_high_f32(abcd));
  float32x2_t xx = vdup_lane_f32(vget_low_f32(xyzw), 0);
  float32x2_t ca = vext_f32(dc, vget_low_f32(abcd), 1);
  return vcombine_f32(ca, xx);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzbx(vec4f xyzw, vec4f abcd)
{
  float32x2_t xy = vget_low_f32(xyzw);
  float32x2_t xz = vzip_f32(xy, vget_high_f32(xyzw)).val[0];
  float32x2_t bx = vext_f32(vget_low_f32(abcd), xy, 1);
  return vcombine_f32(xz, bx);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzya(vec4f xyzw, vec4f abcd)
{
  float32x2_t xy = vget_low_f32(xyzw);
  float32x2_t yx = vrev64_f32(xy);
  float32x2x2_t xz_yw = vzip_f32(xy, vget_high_f32(xyzw));
  float32x2x2_t ya_xb = vzip_f32(yx, vget_low_f32(abcd));
  return vcombine_f32(xz_yw.val[0], ya_xb.val[0]);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_yaxx(vec4f xyzw, vec4f abcd)
{
  float32x2_t xy = vget_low_f32(xyzw);
  float32x2_t ya = vext_f32(xy, vget_low_f32(abcd), 1);
  float32x2_t xx = vdup_lane_f32(xy, 0);
  return vcombine_f32(ya, xx);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_yxxc(vec4f xyzw, vec4f abcd)
{
  float32x2_t xy = vget_low_f32(xyzw);
  float32x2_t cd = vget_high_f32(abcd);
  return vcombine_f32(vrev64_f32(xy), vzip_f32(xy, cd).val[0]);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxxb(vec4f xyzw, vec4f abcd)
{
  float32x2_t xy = vget_low_f32(xyzw);
  float32x2_t zw = vget_high_f32(xyzw);
  float32x2_t ba = vrev64_f32(vget_low_f32(abcd));
  float32x2x2_t zx_wy = vzip_f32(zw, xy);
  float32x2x2_t xb_ya = vzip_f32(xy, ba);
  return vcombine_f32(zx_wy.val[0], xb_ya.val[0]);
}
VECTORCALL VECMATH_FINLINE vec4f v_perm_zayx(vec4f xyzw, vec4f abcd)
{
  float32x2x2_t za_wb = vtrn_f32(vget_high_f32(xyzw), vget_low_f32(abcd));
  float32x2_t za = za_wb.val[0];
  float32x2_t yx = vrev64_f32(vget_low_f32(xyzw));
  return vcombine_f32(za, yx);
}
VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(vec4f x, vec4f y, vec4f z) { return vzip1q_f32(vzip1q_f32(x, z), vzip1q_f32(y, z)); }
#endif

// integer single-source perms: NEON permutes are typeless, the float forms compile identically
VECTORCALL VECMATH_FINLINE vec4i v_permi_xzxz(vec4i xyzw) { return v_cast_vec4i(v_perm_xzxz(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_ywyw(vec4i xyzw) { return v_cast_vec4i(v_perm_ywyw(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xyxy(vec4i xyzw) { return v_cast_vec4i(v_perm_xyxy(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_zwzw(vec4i xyzw) { return v_cast_vec4i(v_perm_zwzw(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xxyy(vec4i xyzw) { return v_cast_vec4i(v_perm_xxyy(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_zzww(vec4i xyzw) { return v_cast_vec4i(v_perm_zzww(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xxzz(vec4i xyzw) { return v_cast_vec4i(v_perm_xxzz(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_yyww(vec4i xyzw) { return v_cast_vec4i(v_perm_yyww(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_wwyy(vec4i xyzw) { return v_cast_vec4i(v_perm_wwyy(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_yzxw(vec4i xyzw) { return v_cast_vec4i(v_perm_yzxw(v_cast_vec4f(xyzw))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_yzxy(vec4i xyzw) { return v_cast_vec4i(v_perm_yzxy(v_cast_vec4f(xyzw))); }

#if defined(__clang__) || defined(__GNUC__)
VECTORCALL VECMATH_FINLINE vec3f v_mat43_extract_pos(mat43f_cref mat)
{
  vec4f xyjj = __builtin_shufflevector(mat.row0, mat.row1, 3, 7, 0, 0);
  vec4f xyzj = __builtin_shufflevector(xyjj, mat.row2, 0, 1, 7, 0);
  return xyzj;
}
#else
VECTORCALL VECMATH_FINLINE vec3f v_mat43_extract_pos(mat43f_cref mat)
{
  vec4f jxjz = vcopyq_laneq_f32(mat.row2, 1, mat.row0, 3);
  vec4f yxjz = vcopyq_laneq_f32(jxjz, 0, mat.row1, 3);
  vec4f xyzj = vrev64q_f32(yxjz);
  return xyzj;
}
#endif

VECTORCALL VECMATH_FINLINE vec4f v_dot2(vec4f a, vec4f b)
{
  float32x2_t m = vmul_f32(vget_low_f32(a), vget_low_f32(b));
  return vdupq_lane_f32(vadd_f32(m, vrev64_f32(m)), 0);
}

VECTORCALL VECMATH_FINLINE vec4f v_dot2_x(vec4f a, vec4f b)
{
  float32x2_t m = vmul_f32(vget_low_f32(a), vget_low_f32(b));
  return vdupq_lane_f32(vadd_f32(m, vrev64_f32(m)), 0);
}

VECTORCALL VECMATH_FINLINE vec4f v_dot3_x(vec4f a, vec4f b)
{
  float32x4_t mul = vmulq_f32(a, b);
  float32x2_t lo = vget_low_f32(mul);
  float32x2_t hi = vget_high_f32(mul);
  float32x2_t xy = vpadd_f32(lo, lo);
  float32x2_t r = vadd_f32(xy, hi);
  return vcombine_f32(r, r);
}

VECTORCALL VECMATH_FINLINE vec4f v_dot3(vec4f a, vec4f b)
{
  float32x4_t mul = vmulq_f32(a, b);
  float32x2_t lo = vget_low_f32(mul);
  float32x2_t hi = vget_high_f32(mul);
  float32x2_t xy = vpadd_f32(lo, lo);
  float32x2_t r = vadd_f32(xy, hi);
  return vdupq_lane_f32(r, 0);
}

VECTORCALL VECMATH_FINLINE vec4f v_dot4_x(vec4f a, vec4f b) { return vdupq_n_f32(vaddvq_f32(vmulq_f32(a, b))); }
VECTORCALL VECMATH_FINLINE vec4f v_dot4(vec4f a, vec4f b) { return vdupq_n_f32(vaddvq_f32(vmulq_f32(a, b))); }

// v_length*_sq and v_norm2/3/4 live in dag_vecMath_common.h (portable form).

VECTORCALL VECMATH_FINLINE vec4f v_plane_dist_x(plane3f a, vec3f b) { return v_add_x(v_dot3_x(a,b), v_rot_3(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist(plane3f a, vec3f b) { return v_splat_x(v_plane_dist_x(a,b)); }

VECTORCALL VECMATH_FINLINE void v_mat_33cu_from_mat33(float * __restrict m33, const mat33f& tm)
{
  vec4f v0 = v_perm_xyzd(tm.col0, v_splat_x(tm.col1));
  vec4f v1 = v_perm_xyab(v_rot_1(tm.col1), tm.col2);
  v_stu(m33 + 0, v0);
  v_stu(m33 + 4, v1);
  m33[8] = v_extract_z(tm.col2);
}

VECTORCALL VECMATH_FINLINE void v_mat_43ca_from_mat44(float * __restrict m43, const mat44f &tm)
{
  v_mat_43cu_from_mat44(m43, tm);
}

VECTORCALL VECMATH_FINLINE void v_mat_43cu_from_mat44(float * __restrict m43, const mat44f &tm)
{
#if defined(__clang__) || defined(__GNUC__)
  vec4f v0 = __builtin_shufflevector(tm.col0, tm.col1, 0, 1, 2, 4);
  vec4f v1 = __builtin_shufflevector(tm.col1, tm.col2, 1, 2, 4, 5);
  vec4f v2 = __builtin_shufflevector(tm.col2, tm.col3, 2, 4, 5, 6);
#else
  vec4f v0 = v_perm_xyzd(tm.col0, v_splat_x(tm.col1));
  vec4f v1 = v_perm_xyab(v_rot_1(tm.col1), tm.col2);
  vec4f v2 = v_perm_ayzw(v_rot_3(tm.col3), v_splat_z(tm.col2));
#endif

  v_stu(m43 + 0, v0);
  v_stu(m43 + 4, v1);
  v_stu(m43 + 8, v2);
}

// mat44f from unaligned TMatrix
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu_unsafe(mat44f &tmV, const float *const __restrict m43)
{
  vec4f v0 = v_ldu(m43 + 0);
  vec4f v1 = v_ldu(m43 + 4);
  vec4f v2 = v_ldu(m43 + 8);

  tmV.col0 = v0;
  tmV.col1 = vextq_f32(v0, v1, 3);
  tmV.col2 = vextq_f32(v1, v2, 2);
  tmV.col3 = vextq_f32(v2, v2, 1);
}

// clang fuses the affine fixup into the construction, per instruction the same code as a
// hand-interleaved version
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu(mat44f &tmV, const float *const __restrict m43)
{
  v_mat44_make_from_43cu_unsafe(tmV, m43);
  v_mat44_make_affine(tmV);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43ca(mat44f &tmV, const float *const __restrict m43)
{
  v_mat44_make_from_43cu(tmV, m43);
}

// the single ld3 gives the translation in .w for free, no cheaper form exists
VECTORCALL VECMATH_FINLINE void v_mat43_make_from_43cu_unsafe(mat43f &tmV, const float *const __restrict m43)
{
  v_ldu_soa3(m43, tmV.row0, tmV.row1, tmV.row2);
}

VECTORCALL VECMATH_FINLINE void v_mat44_ident(mat44f &dest)
{
  dest.col3 = V_C_UNIT_0001;
  dest.col2 = v_rot_1(dest.col3);
  dest.col1 = v_rot_1(dest.col2);
  dest.col0 = v_rot_1(dest.col1);
}
VECTORCALL VECMATH_FINLINE void v_mat44_ident_swapxz(mat44f &dest)
{
  dest.col3 = V_C_UNIT_0001;
  dest.col0 = v_rot_1(dest.col3);
  dest.col1 = v_rot_1(dest.col0);
  dest.col2 = v_rot_1(dest.col1);
}
VECTORCALL VECMATH_FINLINE void v_mat33_ident(mat33f &dest)
{
  dest.col2 = V_C_UNIT_0010;
  dest.col1 = v_rot_1(dest.col2);
  dest.col0 = v_rot_1(dest.col1);
}
VECTORCALL VECMATH_FINLINE void v_mat33_ident_swapxz(mat33f &dest)
{
  dest.col0 = V_C_UNIT_0010;
  dest.col1 = v_rot_1(dest.col0);
  dest.col2 = v_rot_1(dest.col1);
}

// v_mat44_transpose*, v_mat43_transpose_to_mat44, v_mat44_transpose_to_mat43 and
// v_mat44/33_mul_vec* live in dag_vecMath_common.h (portable form, identical to the
// NEON code that used to be here).

// v_mat33_inverse and v_mat44_det live in dag_vecMath_common.h (portable form).

VECTORCALL VECMATH_FINLINE short v_extract_xi16(vec4i v) { return vgetq_lane_s16(vreinterpretq_s16_s32(v), 0); }

VECTORCALL VECMATH_FINLINE float v_extract_x(vec4f v) { return vgetq_lane_f32(v, 0); }
VECTORCALL VECMATH_FINLINE float v_extract_y(vec4f v) { return vgetq_lane_f32(v, 1); }
VECTORCALL VECMATH_FINLINE float v_extract_z(vec4f v) { return vgetq_lane_f32(v, 2); }
VECTORCALL VECMATH_FINLINE float v_extract_w(vec4f v) { return vgetq_lane_f32(v, 3); }

VECTORCALL VECMATH_FINLINE int v_extract_xi(vec4i v) {return vgetq_lane_s32(v, 0);}
VECTORCALL VECMATH_FINLINE int v_extract_yi(vec4i v) {return vgetq_lane_s32(v, 1);}
VECTORCALL VECMATH_FINLINE int v_extract_zi(vec4i v) {return vgetq_lane_s32(v, 2);}
VECTORCALL VECMATH_FINLINE int v_extract_wi(vec4i v) {return vgetq_lane_s32(v, 3);}

VECTORCALL VECMATH_FINLINE int64_t v_extract_xi64(vec4i v) {return vgetq_lane_s64(vreinterpretq_s64_s32(v), 0);}
VECTORCALL VECMATH_FINLINE int64_t v_extract_yi64(vec4i v) { return vgetq_lane_s64(vreinterpretq_s64_s32(v), 1); }

#define V_TEST_VEC_X_BIT0(v) (vget_lane_u32(v, 0) & 1)

VECTORCALL VECMATH_FINLINE int v_test_vec_x_eqi(vec3f v, vec3f a)
{
  return V_TEST_VEC_X_BIT0(vceq_s32(vget_low_f32(v), vget_low_f32(a)));
}
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eqi_0(vec3f v) { return v_extract_xi(v_cast_vec4i(v)) == 0 ? 1 : 0; }

VECTORCALL VECMATH_FINLINE int v_test_vec_x_eq(vec3f v, vec3f a) { return V_TEST_VEC_X_BIT0(vceq_f32(vget_low_f32(v), vget_low_f32(a))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_gt(vec3f v, vec3f a) { return V_TEST_VEC_X_BIT0(vcgt_f32(vget_low_f32(v), vget_low_f32(a))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_ge(vec3f v, vec3f a) { return V_TEST_VEC_X_BIT0(vcge_f32(vget_low_f32(v), vget_low_f32(a))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_lt(vec3f v, vec3f a) { return V_TEST_VEC_X_BIT0(vclt_f32(vget_low_f32(v), vget_low_f32(a))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_le(vec3f v, vec3f a) { return V_TEST_VEC_X_BIT0(vcle_f32(vget_low_f32(v), vget_low_f32(a))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eq_0(vec3f v) { return V_TEST_VEC_X_BIT0(vceq_f32(vget_low_f32(v), vmov_n_f32(0))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_gt_0(vec3f v) { return V_TEST_VEC_X_BIT0(vcgt_f32(vget_low_f32(v), vmov_n_f32(0))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_ge_0(vec3f v) { return V_TEST_VEC_X_BIT0(vcge_f32(vget_low_f32(v), vmov_n_f32(0))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_lt_0(vec3f v) { return V_TEST_VEC_X_BIT0(vclt_f32(vget_low_f32(v), vmov_n_f32(0))); }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_le_0(vec3f v) { return V_TEST_VEC_X_BIT0(vcle_f32(vget_low_f32(v), vmov_n_f32(0))); }

#undef V_TEST_VEC_X_BIT0

VECTORCALL VECMATH_FINLINE vec4i v_ldui_half(const void *m)
{
  return vcombine_s32(vld1_s32((int32_t const*)m), vcreate_s32(0));
}
VECTORCALL VECMATH_FINLINE vec4f v_ldu_half(const void *m)
{
  return vreinterpretq_f32_s32(v_ldui_half(m));
}
#if defined(_MSC_VER) && !defined(__clang__)
VECMATH_FINLINE void v_prefetch(const void *) {}
#else
VECMATH_FINLINE void v_prefetch(const void *m) { __builtin_prefetch(m); }
#endif

VECTORCALL VECMATH_FINLINE vec4i v_cvt_lo_ush_vec4i(vec4i a) { return vreinterpretq_s64_s16(vzip1q_s16(vreinterpretq_s16_s64(a), vdupq_n_s16(0))); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_hi_ush_vec4i(vec4i a) { return vreinterpretq_s64_s16(vzip2q_s16(vreinterpretq_s16_s64(a), vdupq_n_s16(0))); }

VECTORCALL VECMATH_FINLINE vec4i v_cvt_lo_ssh_vec4i(vec4i a) { return vmovl_s16(vget_low_s16(vreinterpretq_s16_s32(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_hi_ssh_vec4i(vec4i a)
{
  return vreinterpretq_s64_s16(vzip2q_s16(vreinterpretq_s16_s64(a), vcltq_s16(vreinterpretq_s16_s64(a), vdupq_n_s16(0))));
}

VECMATH_FINLINE vec4i v_cvt_byte_vec4i(uint32_t a)
{
  uint8x16_t u8x16 = vreinterpretq_u8_s64(v_seti_x(a)); /* xxxx xxxx xxxx DCBA */
  uint16x8_t u16x8 = vmovl_u8(vget_low_u8(u8x16));      /* 0x0x 0x0x 0D0C 0B0A */
  uint32x4_t u32x4 = vmovl_u16(vget_low_u16(u16x8));    /* 000D 000C 000B 000A */
  return vreinterpretq_s64_u32(u32x4);
}

#if __APPLE__ || defined(__clang__) || defined(_MSC_VER) // fix error 'bits is not imm'
#define v_slli(v, bits) (int32x4_t)vshlq_n_u32((uint32x4_t)(v), bits)
#define v_srli(v, bits) (int32x4_t)vshrq_n_u32((uint32x4_t)(v), bits)
#define v_srai(v, bits) vshrq_n_s32(v, bits)
#define v_slli_64(v, bits) (int32x4_t)vshlq_n_u64((uint64x2_t)(v), bits)
#define v_srli_64(v, bits) (int32x4_t)vshrq_n_u64((uint64x2_t)(v), bits)

#else
VECTORCALL VECMATH_FINLINE vec4i v_slli(vec4i v, int bits) { return (int32x4_t)vshlq_n_u32((uint32x4_t)v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_srli(vec4i v, int bits) { return (int32x4_t)vshrq_n_u32((uint32x4_t)v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_srai(vec4i v, int bits) { return vshrq_n_s32(v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_slli_64(vec4i v, int bits) { return (int32x4_t)vshlq_n_u64((uint64x2_t)v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_srli_64(vec4i v, int bits) { return (int32x4_t)vshrq_n_u64((uint64x2_t)v, bits); }
#endif
// bits must be in [0, 31]; sshl/ushl still zero-fill (or sign-fill for sra) up to 127, but
// negative or wider counts alias through the low byte and differ from x86
VECTORCALL VECMATH_FINLINE vec4i v_slli_n(vec4i v, int bits)
{
  return vshlq_s32(v, vdupq_n_s32(bits));
}
VECTORCALL VECMATH_FINLINE vec4i v_srli_n(vec4i v, int bits)
{
  return (int32x4_t)vshlq_u32((uint32x4_t)v, vdupq_n_s32(-bits));
}
VECTORCALL VECMATH_FINLINE vec4i v_srai_n(vec4i v, int bits)
{
  return vshlq_s32(v, vdupq_n_s32(-bits));
}
VECTORCALL VECMATH_FINLINE vec4i v_slli_n(vec4i v, vec4i bits)
{
#if defined(_MSC_VER) && !defined(__clang__)
  int64_t c = vget_low_s64(bits).n64_i64[0];
#else
  int64_t c = (int64_t)vget_low_s64(bits);
#endif
  return vshlq_s32(v, vdupq_n_s32(c));
}
VECTORCALL VECMATH_FINLINE vec4i v_srli_n(vec4i v, vec4i bits)
{
#if defined(_MSC_VER) && !defined(__clang__)
  int64_t c = vget_low_s64(bits).n64_i64[0];
#else
  int64_t c = (int64_t)vget_low_s64(bits);
#endif
  return (int32x4_t)vshlq_u32((uint32x4_t)v, vdupq_n_s32(-c));
}
VECTORCALL VECMATH_FINLINE vec4i v_srai_n(vec4i v, vec4i bits)
{
#if defined(_MSC_VER) && !defined(__clang__)
  int64_t c = vget_low_s64(bits).n64_i64[0];
#else
  int64_t c = (int64_t)vget_low_s64(bits);
#endif
  return vshlq_s32(v, vdupq_n_s32(-c));
}

VECTORCALL VECMATH_FINLINE vec4i v_sll(vec4i v, int bits) { return (int32x4_t)vshlq_u32((uint32x4_t)v, (int32x4_t)v_splatsi(bits)); }
VECTORCALL VECMATH_FINLINE vec4i v_srl(vec4i v, int bits) { return (int32x4_t)vshlq_u32((uint32x4_t)v, (int32x4_t)v_splatsi(-bits)); }
VECTORCALL VECMATH_FINLINE vec4i v_sra(vec4i v, int bits) { return vshlq_s32(v, (int32x4_t)v_splatsi(-bits)); }

VECTORCALL VECMATH_FINLINE vec4i v_ori(vec4i a, vec4i b) { return vorrq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_andi(vec4i a, vec4i b) { return vandq_s32(a, b); }
VECTORCALL VECMATH_FINLINE vec4i v_andnoti(vec4i a, vec4i b) { return vandq_s32(vmvnq_s32(a), b); }
VECTORCALL VECMATH_FINLINE vec4i v_xori(vec4i a, vec4i b) { return veorq_s32(a, b); }

VECTORCALL VECMATH_FINLINE vec4i v_packs(vec4i a, vec4i b) { return vreinterpretq_s32_s16(vcombine_s16(vqmovn_s32(a), vqmovn_s32(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_packs(vec4i a) { int16x4_t w = vqmovn_s32(a); return vreinterpretq_s32_s16(vcombine_s16(w, w)); }

VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a, vec4i b)
{
  return vreinterpretq_s32_u16(vcombine_u16(vqmovun_s32(a), vqmovun_s32(b)));
}
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a)
{
  uint16x4_t w = vqmovun_s32(a);
  return vreinterpretq_s32_u16(vcombine_u16(w, w));
}
VECTORCALL VECMATH_FINLINE vec4i v_packus16(vec4i a, vec4i b)
{
  return vreinterpretq_s32_u8(
    vcombine_u8(vqmovun_s16(vreinterpretq_s16_s32(a)),
      vqmovun_s16(vreinterpretq_s16_s32(b))));
}
VECTORCALL VECMATH_FINLINE vec4i v_packus16(vec4i a)
{
  uint8x8_t t = vqmovun_s16(vreinterpretq_s16_s32(a));
  return vreinterpretq_s32_u8(vcombine_u8(t,t));
}

#define _TARGET_HAS_FC16 1

VECTORCALL VECMATH_FINLINE vec4f v_fc16_half_to_float_lo(vec4i v)
{
  int16x4_t h16 = vreinterpret_s16_s32(vget_low_s32(v));
  return vcvt_f32_f16(vreinterpret_f16_s16(h16));
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_pack_half4_lo(float16x4_t h)
{
  return vreinterpretq_s32_u16(vcombine_u16(vreinterpret_u16_f16(h), vdup_n_u16(0)));
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_pack_u32_half4_lo(vec4i h)
{
  uint16x4_t h16 = vmovn_u32(vreinterpretq_u32_s32(h));
  return vreinterpretq_s32_u16(vcombine_u16(h16, vdup_n_u16(0)));
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_rtne_lo(vec4f v)
{
  float16x4_t h = vcvt_f16_f32(v);
  return v_fc16_pack_half4_lo(h);
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_down_lo(vec4f v)
{
  float16x4_t h = vcvt_f16_f32(v);
  vec4i c = vreinterpretq_s32_u32(vmovl_u16(vreinterpret_u16_f16(h)));
  vec4f f = vcvt_f32_f16(h);
  vec4f decMask = v_cmp_gt(f, v);
  vec4i delta = v_ori(v_cast_vec4i(v_cmp_lt(v, v_zero())), v_splatsi(1));
  return v_fc16_pack_u32_half4_lo(v_seli(c, v_subi(c, delta), v_cast_vec4i(decMask)));
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_up_lo(vec4f v)
{
  float16x4_t h = vcvt_f16_f32(v);
  vec4i c = vreinterpretq_s32_u32(vmovl_u16(vreinterpret_u16_f16(h)));
  vec4f f = vcvt_f32_f16(h);
  vec4f incMask = v_cmp_lt(f, v);
  vec4i delta = v_ori(v_cast_vec4i(v_cmp_lt(v, v_zero())), v_splatsi(1));
  return v_fc16_pack_u32_half4_lo(v_seli(c, v_addi(c, delta), v_cast_vec4i(incMask)));
}

VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_trunc_lo(vec4f v)
{
  float16x4_t h = vcvt_f16_f32(v);
  vec4i c = vreinterpretq_s32_u32(vmovl_u16(vreinterpret_u16_f16(h)));
  vec4f f = vcvt_f32_f16(h);
  vec4f isPos = v_cmp_gt(v, v_zero());
  vec4f isNeg = v_cmp_lt(v, v_zero());
  vec4f needFix = v_or(v_and(v_cmp_gt(f, v), isPos),
                       v_and(v_cmp_lt(f, v), isNeg));
  return v_fc16_pack_u32_half4_lo(v_seli(c, v_subi(c, v_splatsi(1)), v_cast_vec4i(needFix)));
}
