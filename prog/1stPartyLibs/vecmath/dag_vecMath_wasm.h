//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// WebAssembly SIMD128 backend: implements the same primitive contract as dag_vecMath_pc_sse.h /
// dag_vecMath_neon.h on the fixed-width wasm SIMD proposal (clang -msimd128). Every two-source
// lane permutation is one i8x16.shuffle, so the v_perm_* family is written as plain
// __builtin_shufflevector. Semantics follow the SSE backend where the ISA allows it at no cost:
// v_min/v_max are pmin/pmax with swapped operands (a < b ? a : b, the second operand wins on NaN
// and on signed-zero ties), v_sel/v_seli and the v_check_*/v_signmask family read only the lane
// sign bit, shift counts past the lane width zero-fill (sign-fill for v_srai). Float->int
// conversions saturate and map NaN to 0 (i32x4.trunc_sat), the NEON contract - SSE's INT_MIN
// answer has no single-instruction form here. There are no estimate instructions: the _est and
// _unprecise reciprocal/rsqrt forms are the exact division. v_madd/v_nmsub fuse under
// -mrelaxed-simd: the product is not rounded before the add.

#include <wasm_simd128.h>
#include <stdint.h>

#define VECMATH_WASM_V(a) ((v128_t)(a))
#define VECMATH_WASM_F(a) ((vec4f)(a))
#define VECMATH_WASM_I(a) ((vec4i)(a))
#define VECMATH_WASM_D(a) ((vecmath_f64x2)(a))
#if defined(__wasm_relaxed_simd__) && !defined(VECMATH_NO_FMA)
  #define VECMATH_WASM_FMA 1
#else
  #define VECMATH_WASM_FMA 0
#endif

VECTORCALL VECMATH_FINLINE vec4f v_zero() { return VECMATH_WASM_F(wasm_f32x4_const_splat(0.f)); }
VECTORCALL VECMATH_FINLINE vec4i v_zeroi() { return VECMATH_WASM_I(wasm_i32x4_const_splat(0)); }
VECTORCALL VECMATH_FINLINE vec4f v_set_all_bits() { return VECMATH_WASM_F(wasm_i32x4_const_splat(-1)); }
VECTORCALL VECMATH_FINLINE vec4i v_set_all_bitsi() { return VECMATH_WASM_I(wasm_i32x4_const_splat(-1)); }
VECTORCALL VECMATH_FINLINE vec4f v_msbit() { return VECMATH_WASM_F(wasm_i32x4_const_splat((int32_t)0x80000000)); }
VECTORCALL VECMATH_FINLINE vec4f v_ld(const float *m) { return VECMATH_WASM_F(wasm_v128_load(m)); }
VECTORCALL VECMATH_FINLINE vec4f v_ldu(const float *m) { return VECMATH_WASM_F(wasm_v128_load(m)); }
VECTORCALL VECMATH_FINLINE void v_ld_soa2(const float *m, vec4f &x, vec4f &y)
{
  vec4f a = v_ld(m), b = v_ld(m + 4);
  x = __builtin_shufflevector(a, b, 0, 2, 4, 6);
  y = __builtin_shufflevector(a, b, 1, 3, 5, 7);
}
VECTORCALL VECMATH_FINLINE void v_ld_soa3(const float *m, vec4f &x, vec4f &y, vec4f &z)
{
  vec4f x0y0z0x1 = v_ld(m), y1z1x2y2 = v_ld(m + 4), z2x3y3z3 = v_ld(m + 8);
  vec4f x2y2x3y3 = __builtin_shufflevector(y1z1x2y2, z2x3y3z3, 2, 3, 5, 6);
  vec4f y0z0y1z1 = __builtin_shufflevector(x0y0z0x1, y1z1x2y2, 1, 2, 4, 5);
  x = __builtin_shufflevector(x0y0z0x1, x2y2x3y3, 0, 3, 4, 6);
  y = __builtin_shufflevector(y0z0y1z1, x2y2x3y3, 0, 2, 5, 7);
  z = __builtin_shufflevector(y0z0y1z1, z2x3y3z3, 1, 3, 4, 7);
}
VECTORCALL VECMATH_FINLINE void v_ld_soa4(const float *m, vec4f &x, vec4f &y, vec4f &z, vec4f &w)
{
  vec4f a = v_ld(m), b = v_ld(m + 4), c = v_ld(m + 8), d = v_ld(m + 12);
  vec4f t0 = __builtin_shufflevector(a, b, 0, 4, 1, 5), t1 = __builtin_shufflevector(a, b, 2, 6, 3, 7);
  vec4f t2 = __builtin_shufflevector(c, d, 0, 4, 1, 5), t3 = __builtin_shufflevector(c, d, 2, 6, 3, 7);
  x = __builtin_shufflevector(t0, t2, 0, 1, 4, 5);
  y = __builtin_shufflevector(t0, t2, 2, 3, 6, 7);
  z = __builtin_shufflevector(t1, t3, 0, 1, 4, 5);
  w = __builtin_shufflevector(t1, t3, 2, 3, 6, 7);
}
// v128.load has no alignment requirement
VECTORCALL VECMATH_FINLINE void v_ldu_soa2(const float *m, vec4f &x, vec4f &y) { v_ld_soa2(m, x, y); }
VECTORCALL VECMATH_FINLINE void v_ldu_soa3(const float *m, vec4f &x, vec4f &y, vec4f &z) { v_ld_soa3(m, x, y, z); }
VECTORCALL VECMATH_FINLINE void v_ldu_soa4(const float *m, vec4f &x, vec4f &y, vec4f &z, vec4f &w) { v_ld_soa4(m, x, y, z, w); }

VECTORCALL VECMATH_FINLINE void v_interleave3(vec4f x, vec4f y, vec4f z, vec4f &e0, vec4f &e1, vec4f &e2)
{
  vec4f x0y0x1x1 = __builtin_shufflevector(x, y, 0, 4, 1, 1);
  vec4f y1z1y2y2 = __builtin_shufflevector(y, z, 1, 5, 2, 2);
  vec4f z2x3z3z3 = __builtin_shufflevector(z, x, 2, 7, 3, 3);
  e0 = __builtin_shufflevector(x0y0x1x1, z, 0, 1, 4, 2); // x0 y0 z0 x1
  e1 = __builtin_shufflevector(y1z1y2y2, x, 0, 1, 6, 2); // y1 z1 x2 y2
  e2 = __builtin_shufflevector(z2x3z3z3, y, 0, 1, 7, 2); // z2 x3 y3 z3
}
VECTORCALL VECMATH_FINLINE void v_interleave4(vec4f x, vec4f y, vec4f z, vec4f w, vec4f &e0, vec4f &e1, vec4f &e2, vec4f &e3)
{
  vec4f t0 = __builtin_shufflevector(x, y, 0, 4, 1, 5), t1 = __builtin_shufflevector(x, y, 2, 6, 3, 7);
  vec4f t2 = __builtin_shufflevector(z, w, 0, 4, 1, 5), t3 = __builtin_shufflevector(z, w, 2, 6, 3, 7);
  e0 = __builtin_shufflevector(t0, t2, 0, 1, 4, 5);
  e1 = __builtin_shufflevector(t0, t2, 2, 3, 6, 7);
  e2 = __builtin_shufflevector(t1, t3, 0, 1, 4, 5);
  e3 = __builtin_shufflevector(t1, t3, 2, 3, 6, 7);
}
VECTORCALL VECMATH_FINLINE void v_st_soa2(float *m, vec4f x, vec4f y)
{
  wasm_v128_store(m, VECMATH_WASM_V(__builtin_shufflevector(x, y, 0, 4, 1, 5)));
  wasm_v128_store(m + 4, VECMATH_WASM_V(__builtin_shufflevector(x, y, 2, 6, 3, 7)));
}
VECTORCALL VECMATH_FINLINE void v_st_soa3(float *m, vec4f x, vec4f y, vec4f z)
{
  vec4f e0, e1, e2;
  v_interleave3(x, y, z, e0, e1, e2);
  wasm_v128_store(m, VECMATH_WASM_V(e0));
  wasm_v128_store(m + 4, VECMATH_WASM_V(e1));
  wasm_v128_store(m + 8, VECMATH_WASM_V(e2));
}
VECTORCALL VECMATH_FINLINE void v_st_soa4(float *m, vec4f x, vec4f y, vec4f z, vec4f w)
{
  vec4f e0, e1, e2, e3;
  v_interleave4(x, y, z, w, e0, e1, e2, e3);
  wasm_v128_store(m, VECMATH_WASM_V(e0));
  wasm_v128_store(m + 4, VECMATH_WASM_V(e1));
  wasm_v128_store(m + 8, VECMATH_WASM_V(e2));
  wasm_v128_store(m + 12, VECMATH_WASM_V(e3));
}
// v128.store has no alignment requirement
VECTORCALL VECMATH_FINLINE void v_stu_soa2(float *m, vec4f x, vec4f y) { v_st_soa2(m, x, y); }
VECTORCALL VECMATH_FINLINE void v_stu_soa3(float *m, vec4f x, vec4f y, vec4f z) { v_st_soa3(m, x, y, z); }
VECTORCALL VECMATH_FINLINE void v_stu_soa4(float *m, vec4f x, vec4f y, vec4f z, vec4f w) { v_st_soa4(m, x, y, z, w); }
VECTORCALL VECMATH_FINLINE vec4f v_ldu_x(const float *m) { return VECMATH_WASM_F(wasm_v128_load32_zero(m)); } // load x, zero others
VECTORCALL VECMATH_FINLINE vec4i v_ldi(const int *m) { return VECMATH_WASM_I(wasm_v128_load(m)); }
VECTORCALL VECMATH_FINLINE vec4i v_ldui(const int *m) { return VECMATH_WASM_I(wasm_v128_load(m)); }
VECTORCALL VECMATH_FINLINE vec4i v_ldush(const signed short *m) { return VECMATH_WASM_I(wasm_i32x4_load16x4(m)); }
VECTORCALL VECMATH_FINLINE vec4i v_lduush(const unsigned short *m) { return VECMATH_WASM_I(wasm_u32x4_load16x4(m)); }
VECTORCALL VECMATH_FINLINE vec3f v_ldu_p3_safe(const float *m) { return VECMATH_WASM_F(wasm_v128_load32_lane(m + 2, wasm_v128_load64_zero(m), 2)); }
VECTORCALL VECMATH_FINLINE vec4i v_ldui_p3_safe(const int *m) { return VECMATH_WASM_I(wasm_v128_load32_lane(m + 2, wasm_v128_load64_zero(m), 2)); }
VECTORCALL VECMATH_FINLINE vec4f v_splat_x(vec4f a) { return __builtin_shufflevector(a, a, 0, 0, 0, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_splat_y(vec4f a) { return __builtin_shufflevector(a, a, 1, 1, 1, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_splat_z(vec4f a) { return __builtin_shufflevector(a, a, 2, 2, 2, 2); }
VECTORCALL VECMATH_FINLINE vec4f v_splat_w(vec4f a) { return __builtin_shufflevector(a, a, 3, 3, 3, 3); }
VECTORCALL VECMATH_FINLINE vec4i v_splat_xi(vec4i a) { return __builtin_shufflevector(a, a, 0, 0, 0, 0); }
VECTORCALL VECMATH_FINLINE vec4i v_splat_yi(vec4i a) { return __builtin_shufflevector(a, a, 1, 1, 1, 1); }
VECTORCALL VECMATH_FINLINE vec4i v_splat_zi(vec4i a) { return __builtin_shufflevector(a, a, 2, 2, 2, 2); }
VECTORCALL VECMATH_FINLINE vec4i v_splat_wi(vec4i a) { return __builtin_shufflevector(a, a, 3, 3, 3, 3); }

VECTORCALL VECMATH_FINLINE vec4f v_splats(float a) { return VECMATH_WASM_F(wasm_f32x4_splat(a)); }
VECTORCALL VECMATH_FINLINE vec4i v_splatsi(int a) { return VECMATH_WASM_I(wasm_i32x4_splat(a)); }
VECTORCALL VECMATH_FINLINE vec4i v_splatsi64(int64_t a) { return VECMATH_WASM_I(wasm_i64x2_splat(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_set_x(float a) { return VECMATH_WASM_F(wasm_f32x4_make(a, 0.f, 0.f, 0.f)); } // set x, zero others
VECTORCALL VECMATH_FINLINE vec4i v_seti_x(int a) { return VECMATH_WASM_I(wasm_i32x4_make(a, 0, 0, 0)); } // set x, zero others
VECTORCALL VECMATH_FINLINE vec4f v_make_vec4f(float x, float y, float z, float w) { return VECMATH_WASM_F(wasm_f32x4_make(x, y, z, w)); }
VECTORCALL VECMATH_FINLINE vec4i v_make_vec4i(int x, int y, int z, int w) { return VECMATH_WASM_I(wasm_i32x4_make(x, y, z, w)); }
VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(float x, float y, float z) { return v_make_vec4f(x, y, z, z); }
VECTORCALL VECMATH_FINLINE vec4i v_make_vec3i(int x, int y, int z) { return v_make_vec4i(x, y, z, z); }

VECTORCALL VECMATH_FINLINE void v_st(void *m, vec4f v)  { wasm_v128_store(m, VECMATH_WASM_V(v)); }
VECTORCALL VECMATH_FINLINE void v_stu(void *m, vec4f v) { wasm_v128_store(m, VECMATH_WASM_V(v)); }
VECTORCALL VECMATH_FINLINE void v_sti(void *m, vec4i v)  { wasm_v128_store(m, VECMATH_WASM_V(v)); }
VECTORCALL VECMATH_FINLINE void v_stui(void *m, vec4i v) { wasm_v128_store(m, VECMATH_WASM_V(v)); }
VECTORCALL VECMATH_FINLINE void v_stui_half(void *m, vec4i v) { wasm_v128_store64_lane(m, VECMATH_WASM_V(v), 0); }
VECTORCALL VECMATH_FINLINE void v_stu_half(void *m, vec4f v) { wasm_v128_store64_lane(m, VECMATH_WASM_V(v), 0); }
VECTORCALL VECMATH_FINLINE void v_stu_p3(float *p3, vec3f v) { v_stu_half(p3, v); wasm_v128_store32_lane(p3 + 2, VECMATH_WASM_V(v), 2); }
VECTORCALL VECMATH_FINLINE void v_stui_p3(int *p3, vec4i v) { v_stui_half(p3, v); wasm_v128_store32_lane(p3 + 2, VECMATH_WASM_V(v), 2); }

VECTORCALL VECMATH_FINLINE vec4f v_merge_hw(vec4f a, vec4f b) { return __builtin_shufflevector(a, b, 0, 4, 1, 5); }
VECTORCALL VECMATH_FINLINE vec4f v_merge_lw(vec4f a, vec4f b) { return __builtin_shufflevector(a, b, 2, 6, 3, 7); }

// i32x4.bitmask gathers the lane sign bits, the movmskps of this ISA
VECTORCALL VECMATH_FINLINE int v_signmask(vec4f a) { return (int)wasm_i32x4_bitmask(VECMATH_WASM_V(a)); }
VECTORCALL VECMATH_FINLINE int v_truemask(vec4f a) { return (int)wasm_i32x4_bitmask(VECMATH_WASM_V(a)); }
VECTORCALL VECMATH_FINLINE int v_count_true(vec4f a) { return __builtin_popcount(wasm_i32x4_bitmask(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE bool v_is_any_neg_b(vec4f a) { return wasm_i32x4_bitmask(VECMATH_WASM_V(a)) != 0; }

VECTORCALL VECMATH_FINLINE int v_is_merge_planes_nout(vec4f m0, vec4f m1, vec4f m2, vec4f m3, vec4f m4, vec4f m5)
{
  // unsigned(-x) has bit 31 set iff x != 0, so the & chain needs no setcc per plane
  unsigned nout = unsigned(-v_signmask(m0)) & unsigned(-v_signmask(m1)) & unsigned(-v_signmask(m2))
                & unsigned(-v_signmask(m3)) & unsigned(-v_signmask(m4)) & unsigned(-v_signmask(m5));
  return int(nout) >> 31; // arithmetic shift broadcasts bit 31: 0 or -1
}

VECTORCALL VECMATH_FINLINE vec4f v_min(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_pmin(VECMATH_WASM_V(b), VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_max(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_pmax(VECMATH_WASM_V(b), VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_maxi(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_max(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_mini(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_min(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_maxu(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_u32x4_max(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_minu(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_u32x4_min(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_add(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_add(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_sub(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_sub(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_mul(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_mul(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_addi(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_add(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_subi(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_sub(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_muli(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_mul(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }

// pair ops fold (x,y),(z,w) of a into .xy and of b into .zw, the SSE haddps lane order
VECTORCALL VECMATH_FINLINE vec4f v_min_pairs(vec4f a, vec4f b) { return v_min(v_perm_xzac(a, b), v_perm_ywbd(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_max_pairs(vec4f a, vec4f b) { return v_max(v_perm_xzac(a, b), v_perm_ywbd(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_add_pairs(vec4f a, vec4f b) { return v_add(v_perm_xzac(a, b), v_perm_ywbd(a, b)); }
VECTORCALL VECMATH_FINLINE vec4i v_addi_pairs(vec4i a, vec4i b) { return v_addi(__builtin_shufflevector(a, b, 0, 2, 4, 6), __builtin_shufflevector(a, b, 1, 3, 5, 7)); }
VECTORCALL VECMATH_FINLINE vec4i v_mini_pairs(vec4i a, vec4i b) { return v_mini(__builtin_shufflevector(a, b, 0, 2, 4, 6), __builtin_shufflevector(a, b, 1, 3, 5, 7)); }
VECTORCALL VECMATH_FINLINE vec4i v_maxi_pairs(vec4i a, vec4i b) { return v_maxi(__builtin_shufflevector(a, b, 0, 2, 4, 6), __builtin_shufflevector(a, b, 1, 3, 5, 7)); }

VECTORCALL VECMATH_FINLINE bool v_test_all_bits_zeros(vec4f a) { return !wasm_v128_any_true(VECMATH_WASM_V(a)); }
VECTORCALL VECMATH_FINLINE bool v_test_all_bits_ones(vec4f a) { return !wasm_v128_any_true(wasm_v128_not(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE bool v_test_any_bit_set(vec4f a) { return wasm_v128_any_true(VECMATH_WASM_V(a)); }

VECTORCALL VECMATH_FINLINE bool v_check_xyzw_all_true(vec4f a) { return v_signmask(a) == 0b1111; }
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_all_false(vec4f a) { return v_signmask(a) == 0; }
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_any_true(vec4f a) { return v_signmask(a) != 0; }
VECTORCALL VECMATH_FINLINE bool v_check_xyz_all_true(vec4f a) { return (v_signmask(a) & 0b111) == 0b111; }
VECTORCALL VECMATH_FINLINE bool v_check_xyz_all_false(vec4f a) { return (v_signmask(a) & 0b111) == 0; }
VECTORCALL VECMATH_FINLINE bool v_check_xyz_any_true(vec4f a) { return (v_signmask(a) & 0b111) != 0; }

VECTORCALL VECMATH_FINLINE vec4f v_cmp_eq(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_eq(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_neq(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_ne(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_eqi(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_i32x4_eq(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_cmp_eqi(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_eq(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_ge(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_ge(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_gt(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_gt(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_cmp_lti(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_lt(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_cmp_gti(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_gt(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }

VECTORCALL VECMATH_FINLINE vec4f is_neg_special(vec4f a) { return v_cast_vec4f(v_srai(v_cast_vec4i(a), 31)); }

VECTORCALL VECMATH_FINLINE vec4f v_and(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_v128_and(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_andnot(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_v128_andnot(VECMATH_WASM_V(b), VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_or(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_v128_or(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_xor(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_v128_xor(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_not(vec4f a) { return VECMATH_WASM_F(wasm_v128_not(VECMATH_WASM_V(a))); }
// v128.bitselect selects per bit; the arithmetic shift widens the sign bit into the lane first
VECTORCALL VECMATH_FINLINE vec4f v_sel(vec4f a, vec4f b, vec4f c)
{
  return VECMATH_WASM_F(wasm_v128_bitselect(VECMATH_WASM_V(b), VECMATH_WASM_V(a), wasm_i32x4_shr(VECMATH_WASM_V(c), 31)));
}
VECTORCALL VECMATH_FINLINE vec4i v_seli(vec4i a, vec4i b, vec4i c)
{
  return VECMATH_WASM_I(wasm_v128_bitselect(VECMATH_WASM_V(b), VECMATH_WASM_V(a), wasm_i32x4_shr(VECMATH_WASM_V(c), 31)));
}
VECTORCALL VECMATH_FINLINE vec4f v_btsel(vec4f a, vec4f b, vec4f c) { return VECMATH_WASM_F(wasm_v128_bitselect(VECMATH_WASM_V(b), VECMATH_WASM_V(a), VECMATH_WASM_V(c))); }
VECTORCALL VECMATH_FINLINE vec4i v_btseli(vec4i a, vec4i b, vec4i c) { return VECMATH_WASM_I(wasm_v128_bitselect(VECMATH_WASM_V(b), VECMATH_WASM_V(a), VECMATH_WASM_V(c))); }

VECTORCALL VECMATH_FINLINE vec4i v_cvti_vec4i(vec4f a) { return VECMATH_WASM_I(wasm_i32x4_trunc_sat_f32x4(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i_ieee(vec4f a) { return VECMATH_WASM_I(wasm_u32x4_trunc_sat_f32x4(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i(vec4f a) { return v_cvtu_vec4i_ieee(a); }
VECTORCALL VECMATH_FINLINE vec4f v_cvtu_vec4f_ieee(vec4i a) { return VECMATH_WASM_F(wasm_f32x4_convert_u32x4(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_cvtu_vec4f(vec4i a) { return v_cvtu_vec4f_ieee(a); }
VECTORCALL VECMATH_FINLINE vec4f v_cvti_vec4f(vec4i a) { return VECMATH_WASM_F(wasm_f32x4_convert_i32x4(VECMATH_WASM_V(a))); }

VECTORCALL VECMATH_FINLINE vec4i v_cast_vec4i(vec4f a) { return VECMATH_WASM_I(a); }
VECTORCALL VECMATH_FINLINE vec4f v_cast_vec4f(vec4i a) { return VECMATH_WASM_F(a); }

VECTORCALL VECMATH_FINLINE vec4f v_floor(vec4f a) { return VECMATH_WASM_F(wasm_f32x4_floor(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_ceil(vec4f a) { return VECMATH_WASM_F(wasm_f32x4_ceil(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_trunc(vec4f a) { return VECMATH_WASM_F(wasm_f32x4_trunc(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_round_ieee(vec4f a) { return VECMATH_WASM_F(wasm_f32x4_nearest(VECMATH_WASM_V(a))); }
// f32x4.nearest is ties-to-even; ties away from zero is decided on the truncated remainder, the
// SSE form: trunc and a - trunc(a) are both exact, unlike biasing a by a signed half first
VECTORCALL VECMATH_FINLINE vec4f v_round(vec4f a)
{
  vec4f t = v_trunc(a);
  vec4f sign = v_and(a, v_cast_vec4f(V_CI_SIGN_MASK));
  vec4f absFrac = v_xor(v_sub(a, t), sign); // truncation keeps the remainder on a's side of zero
  vec4f away = v_cmp_ge(absFrac, V_C_HALF);
  return v_add(t, v_or(v_and(away, V_C_ONE), sign));
}
VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi(vec4f a) { return v_cvti_vec4i(v_round(a)); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi_ieee(vec4f a) { return v_cvti_vec4i(v_round_ieee(a)); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_trunci(vec4f a) { return v_cvti_vec4i(a); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_floori(vec4f a) { return v_cvti_vec4i(v_floor(a)); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_ceili(vec4f a) { return v_cvti_vec4i(v_ceil(a)); }

VECTORCALL VECMATH_FINLINE vec4f sse4_floor(vec4f a) { return v_floor(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_ceil(vec4f a) { return v_ceil(a); }
VECTORCALL VECMATH_FINLINE vec4f sse4_round(vec4f a) { return v_round(a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_floori(vec4f a) { return v_cvt_floori(a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_ceili(vec4f a)  { return v_cvt_ceili(a); }
VECTORCALL VECMATH_FINLINE vec4i sse4_cvt_trunci(vec4f a)  { return v_cvt_trunci(a); }

#if VECMATH_WASM_FMA
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c) { return VECMATH_WASM_F(wasm_f32x4_relaxed_madd(VECMATH_WASM_V(a), VECMATH_WASM_V(b), VECMATH_WASM_V(c))); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c) { return VECMATH_WASM_F(wasm_f32x4_relaxed_nmadd(VECMATH_WASM_V(a), VECMATH_WASM_V(b), VECMATH_WASM_V(c))); }
#else
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c) { return v_add(v_mul(a, b), c); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c) { return v_sub(c, v_mul(a, b)); }
#endif
VECTORCALL VECMATH_FINLINE vec4f v_msub(vec4f a, vec4f b, vec4f c) { return v_sub(v_mul(a, b), c); }
// there is no scalar-lane form: the _x variants are the packed ops, .yzw hold the packed result
VECTORCALL VECMATH_FINLINE vec4f v_add_x(vec4f a, vec4f b) { return v_add(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_sub_x(vec4f a, vec4f b) { return v_sub(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_mul_x(vec4f a, vec4f b) { return v_mul(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_madd_x(vec4f a, vec4f b, vec4f c) { return v_madd(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4f v_msub_x(vec4f a, vec4f b, vec4f c) { return v_msub(a, b, c); }
VECTORCALL VECMATH_FINLINE vec4f v_nmsub_x(vec4f a, vec4f b, vec4f c) { return v_nmsub(a, b, c); }

VECTORCALL VECMATH_FINLINE vec4i v_addi16(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i16x8_add(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_subi16(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i16x8_sub(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_muli16(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i16x8_mul(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_mulhi16(vec4i a, vec4i b)
{
  v128_t lo = wasm_i32x4_extmul_low_i16x8(VECMATH_WASM_V(a), VECMATH_WASM_V(b));
  v128_t hi = wasm_i32x4_extmul_high_i16x8(VECMATH_WASM_V(a), VECMATH_WASM_V(b));
  return VECMATH_WASM_I(wasm_i16x8_narrow_i32x4(wasm_i32x4_shr(lo, 16), wasm_i32x4_shr(hi, 16)));
}
// i32x4.dot_i16x8 is pmaddwd: pairwise products summed into the 32-bit lane, wrapping
VECTORCALL VECMATH_FINLINE vec4i v_madd_i16(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i32x4_dot_i16x8(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_splatsi16(int v) { return VECMATH_WASM_I(wasm_i16x8_splat((int16_t)v)); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i8(vec4i a, vec4i b)
{ return VECMATH_WASM_I(wasm_i8x16_shuffle(VECMATH_WASM_V(a), VECMATH_WASM_V(b), 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23)); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i8(vec4i a, vec4i b)
{ return VECMATH_WASM_I(wasm_i8x16_shuffle(VECMATH_WASM_V(a), VECMATH_WASM_V(b), 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31)); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i16(vec4i a, vec4i b)
{ return VECMATH_WASM_I(wasm_i16x8_shuffle(VECMATH_WASM_V(a), VECMATH_WASM_V(b), 0, 8, 1, 9, 2, 10, 3, 11)); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i16(vec4i a, vec4i b)
{ return VECMATH_WASM_I(wasm_i16x8_shuffle(VECMATH_WASM_V(a), VECMATH_WASM_V(b), 4, 12, 5, 13, 6, 14, 7, 15)); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i32(vec4i a, vec4i b) { return __builtin_shufflevector(a, b, 0, 4, 1, 5); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i32(vec4i a, vec4i b) { return __builtin_shufflevector(a, b, 2, 6, 3, 7); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i64(vec4i a, vec4i b) { return __builtin_shufflevector(a, b, 0, 1, 4, 5); }
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i64(vec4i a, vec4i b) { return __builtin_shufflevector(a, b, 2, 3, 6, 7); }
VECTORCALL VECMATH_FINLINE vec4i v_perm_i8(vec4i t, vec4i k)
{
  // match pshufb exactly: index bits 4..6 are ignored, bit 7 zeroes the lane
  // (i8x16.swizzle returns 0 for any index >= 16)
  v128_t ki = wasm_v128_and(VECMATH_WASM_V(k), wasm_u8x16_const_splat(0x8F));
  return VECMATH_WASM_I(wasm_i8x16_swizzle(VECMATH_WASM_V(t), ki));
}
VECTORCALL VECMATH_FINLINE vec4i v_cmp_eqi8(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i8x16_eq(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }

VECTORCALL VECMATH_FINLINE vec4f v_hadd4_x(vec4f a)
{
  vec4f s = v_add(a, __builtin_shufflevector(a, a, 2, 3, 0, 1)); // x+z, y+w, z+x, w+y
  return v_add(s, __builtin_shufflevector(s, s, 1, 0, 3, 2));    // all lanes
}
VECTORCALL VECMATH_FINLINE vec4f v_hadd3_x(vec4f a)
{
  vec4f s = v_add(a, v_splat_y(a));
  return v_add(s, v_splat_z(a));
}

VECTORCALL VECMATH_FINLINE vec4f v_rot_1(vec4f a) { return __builtin_shufflevector(a, a, 1, 2, 3, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_rot_2(vec4f a) { return __builtin_shufflevector(a, a, 2, 3, 0, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_rot_3(vec4f a) { return __builtin_shufflevector(a, a, 3, 0, 1, 2); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_1(vec4i a) { return __builtin_shufflevector(a, a, 1, 2, 3, 0); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_2(vec4i a) { return __builtin_shufflevector(a, a, 2, 3, 0, 1); }
VECTORCALL VECMATH_FINLINE vec4i v_roti_3(vec4i a) { return __builtin_shufflevector(a, a, 3, 0, 1, 2); }

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
VECTORCALL VECMATH_FINLINE vec4f v_hmin3(vec3f a) { return v_min(v_splat_x(a), v_min(v_splat_y(a), v_splat_z(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_hmax3(vec3f a) { return v_max(v_splat_x(a), v_max(v_splat_y(a), v_splat_z(a))); }
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

VECTORCALL VECMATH_FINLINE vec4f v_div(vec4f a, vec4f b) { return VECMATH_WASM_F(wasm_f32x4_div(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4f v_div_x(vec4f a, vec4f b) { return v_div(a, b); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt(vec4f a) { return VECMATH_WASM_F(wasm_f32x4_sqrt(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_x(vec4f a) { return v_sqrt(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise(vec4f a) { return v_div(V_C_ONE, a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est(vec4f a) { return v_rcp_unprecise(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise_x(vec4f a) { return v_rcp_unprecise(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est_x(vec4f a) { return v_rcp_unprecise(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_unprecise(vec4f a) { return v_rcp_unprecise(v_sqrt(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_unprecise_x(vec4f a) { return v_rsqrt_unprecise(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_est(vec4f a) { return v_rsqrt_unprecise(a); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_est_x(vec4f a) { return v_rsqrt_unprecise(a); }

VECTORCALL VECMATH_FINLINE vec4f v_neg(vec4f a) { return VECMATH_WASM_F(wasm_f32x4_neg(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_negi(vec4i a) { return VECMATH_WASM_I(wasm_i32x4_neg(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_abs(vec4f a) { return VECMATH_WASM_F(wasm_f32x4_abs(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_absi(vec4i a) { return VECMATH_WASM_I(wasm_i32x4_abs(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4f v_abs_diff(vec4f a, vec4f b) { return v_abs(v_sub(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_abs_ge(vec4f a, vec4f b) { return v_cmp_ge(v_abs(a), v_abs(b)); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_abs_gt(vec4f a, vec4f b) { return v_cmp_gt(v_abs(a), v_abs(b)); }

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
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxy(vec4f v) { return __builtin_shufflevector(v, v, 1, 2, 0, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxyw(vec4f a) { return __builtin_shufflevector(a, a, 2, 0, 1, 3); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxzx(vec4f v) { return __builtin_shufflevector(v, v, 2, 0, 2, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wwyy(vec4f v) { return __builtin_shufflevector(v, v, 3, 3, 1, 1); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xaxa(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 4, 0, 4); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yybb(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 1, 5, 5); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxab(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 0, 0, 4, 5); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzab(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 1, 2, 4, 5); }
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
VECTORCALL VECMATH_FINLINE vec4f v_perm_zayx(vec4f xyzw, vec4f abcd) { return __builtin_shufflevector(xyzw, abcd, 2, 4, 1, 0); }
VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(vec4f x, vec4f y, vec4f z)
{
  return __builtin_shufflevector(__builtin_shufflevector(x, y, 0, 0, 4, 4), z, 0, 2, 4, 4);
}

// integer single-source perms: the shuffle is typeless, the float forms compile identically
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

VECTORCALL VECMATH_FINLINE vec3f v_mat43_extract_pos(mat43f_cref mat)
{
  vec4f xyjj = __builtin_shufflevector(mat.row0, mat.row1, 3, 7, 0, 0);
  return __builtin_shufflevector(xyjj, mat.row2, 0, 1, 7, 0);
}

VECTORCALL VECMATH_FINLINE vec4f v_dot2_x(vec4f a, vec4f b)
{
  vec4f m = v_mul(a, b);
  return v_add(m, v_splat_y(m));
}
VECTORCALL VECMATH_FINLINE vec4f v_dot2(vec4f a, vec4f b) { return v_splat_x(v_dot2_x(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3_x(vec4f a, vec4f b) { return v_hadd3_x(v_mul(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_dot3(vec4f a, vec4f b) { return v_splat_x(v_dot3_x(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_dot4_x(vec4f a, vec4f b) { return v_hadd4_x(v_mul(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_dot4(vec4f a, vec4f b) { return v_dot4_x(a, b); }

VECTORCALL VECMATH_FINLINE vec3f v_cross3(vec3f a, vec3f b)
{
  // (a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
  vec3f ayzx = v_perm_yzxy(a);
  vec3f byzx = v_perm_yzxy(b);
  return v_perm_yzxy(v_sub(v_mul(a, byzx), v_mul(ayzx, b)));
}

// v_length*_sq and v_norm2/3/4 live in dag_vecMath_common.h (portable form).

VECTORCALL VECMATH_FINLINE vec4f v_plane_dist_x(plane3f a, vec3f b) { return v_add_x(v_dot3_x(a, b), v_rot_3(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist(plane3f a, vec3f b) { return v_splat_x(v_plane_dist_x(a, b)); }

VECTORCALL VECMATH_FINLINE void v_mat_33cu_from_mat33(float * __restrict m33, const mat33f& tm)
{
  vec4f v0 = __builtin_shufflevector(tm.col0, tm.col1, 0, 1, 2, 4);
  vec4f v1 = __builtin_shufflevector(tm.col1, tm.col2, 1, 2, 4, 5);
  v_stu(m33 + 0, v0);
  v_stu(m33 + 4, v1);
  m33[8] = v_extract_z(tm.col2);
}

VECTORCALL VECMATH_FINLINE void v_mat_43cu_from_mat44(float * __restrict m43, const mat44f &tm)
{
  vec4f v0 = __builtin_shufflevector(tm.col0, tm.col1, 0, 1, 2, 4);
  vec4f v1 = __builtin_shufflevector(tm.col1, tm.col2, 1, 2, 4, 5);
  vec4f v2 = __builtin_shufflevector(tm.col2, tm.col3, 2, 4, 5, 6);
  v_stu(m43 + 0, v0);
  v_stu(m43 + 4, v1);
  v_stu(m43 + 8, v2);
}

VECTORCALL VECMATH_FINLINE void v_mat_43ca_from_mat44(float * __restrict m43, const mat44f &tm)
{
  v_mat_43cu_from_mat44(m43, tm);
}

// mat44f from unaligned TMatrix
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu_unsafe(mat44f &tmV, const float *const __restrict m43)
{
  vec4f v0 = v_ldu(m43 + 0);
  vec4f v1 = v_ldu(m43 + 4);
  vec4f v2 = v_ldu(m43 + 8);

  tmV.col0 = v0;
  tmV.col1 = __builtin_shufflevector(v0, v1, 3, 4, 5, 6);
  tmV.col2 = __builtin_shufflevector(v1, v2, 2, 3, 4, 5);
  tmV.col3 = __builtin_shufflevector(v2, v2, 1, 2, 3, 0);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu(mat44f &tmV, const float *const __restrict m43)
{
  v_mat44_make_from_43cu_unsafe(tmV, m43);
  v_mat44_make_affine(tmV);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43ca(mat44f &tmV, const float *const __restrict m43)
{
  v_mat44_make_from_43cu(tmV, m43);
}

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

// v_mat44_transpose*, v_mat43_transpose_to_mat44, v_mat44_transpose_to_mat43,
// v_mat44/33_mul_vec*, v_mat33_inverse and v_mat44_det live in dag_vecMath_common.h.

VECTORCALL VECMATH_FINLINE short v_extract_xi16(vec4i v) { return (short)wasm_i16x8_extract_lane(VECMATH_WASM_V(v), 0); }

VECTORCALL VECMATH_FINLINE float v_extract_x(vec4f v) { return wasm_f32x4_extract_lane(VECMATH_WASM_V(v), 0); }
VECTORCALL VECMATH_FINLINE float v_extract_y(vec4f v) { return wasm_f32x4_extract_lane(VECMATH_WASM_V(v), 1); }
VECTORCALL VECMATH_FINLINE float v_extract_z(vec4f v) { return wasm_f32x4_extract_lane(VECMATH_WASM_V(v), 2); }
VECTORCALL VECMATH_FINLINE float v_extract_w(vec4f v) { return wasm_f32x4_extract_lane(VECMATH_WASM_V(v), 3); }

VECTORCALL VECMATH_FINLINE int v_extract_xi(vec4i v) { return wasm_i32x4_extract_lane(VECMATH_WASM_V(v), 0); }
VECTORCALL VECMATH_FINLINE int v_extract_yi(vec4i v) { return wasm_i32x4_extract_lane(VECMATH_WASM_V(v), 1); }
VECTORCALL VECMATH_FINLINE int v_extract_zi(vec4i v) { return wasm_i32x4_extract_lane(VECMATH_WASM_V(v), 2); }
VECTORCALL VECMATH_FINLINE int v_extract_wi(vec4i v) { return wasm_i32x4_extract_lane(VECMATH_WASM_V(v), 3); }

VECTORCALL VECMATH_FINLINE int64_t v_extract_xi64(vec4i v) { return wasm_i64x2_extract_lane(VECMATH_WASM_V(v), 0); }
VECTORCALL VECMATH_FINLINE int64_t v_extract_yi64(vec4i v) { return wasm_i64x2_extract_lane(VECMATH_WASM_V(v), 1); }

VECTORCALL VECMATH_FINLINE int v_test_vec_x_eqi(vec3f v, vec3f a) { return v_extract_xi(v_cast_vec4i(v)) == v_extract_xi(v_cast_vec4i(a)) ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eqi_0(vec3f v) { return v_extract_xi(v_cast_vec4i(v)) == 0 ? 1 : 0; }

VECTORCALL VECMATH_FINLINE int v_test_vec_x_eq(vec3f v, vec3f a) { return v_extract_x(v) == v_extract_x(a) ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_gt(vec3f v, vec3f a) { return v_extract_x(v) > v_extract_x(a) ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_ge(vec3f v, vec3f a) { return v_extract_x(v) >= v_extract_x(a) ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_lt(vec3f v, vec3f a) { return v_extract_x(v) < v_extract_x(a) ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_le(vec3f v, vec3f a) { return v_extract_x(v) <= v_extract_x(a) ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eq_0(vec3f v) { return v_extract_x(v) == 0.f ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_gt_0(vec3f v) { return v_extract_x(v) > 0.f ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_ge_0(vec3f v) { return v_extract_x(v) >= 0.f ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_lt_0(vec3f v) { return v_extract_x(v) < 0.f ? 1 : 0; }
VECTORCALL VECMATH_FINLINE int v_test_vec_x_le_0(vec3f v) { return v_extract_x(v) <= 0.f ? 1 : 0; }

VECTORCALL VECMATH_FINLINE vec4i v_ldui_half(const void *m) { return VECMATH_WASM_I(wasm_v128_load64_zero(m)); }
VECTORCALL VECMATH_FINLINE vec4f v_ldu_half(const void *m) { return VECMATH_WASM_F(wasm_v128_load64_zero(m)); }
VECMATH_FINLINE void v_prefetch(const void *m) { __builtin_prefetch(m); }

VECTORCALL VECMATH_FINLINE vec4i v_cvt_lo_ush_vec4i(vec4i a) { return VECMATH_WASM_I(wasm_u32x4_extend_low_u16x8(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_hi_ush_vec4i(vec4i a) { return VECMATH_WASM_I(wasm_u32x4_extend_high_u16x8(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_lo_ssh_vec4i(vec4i a) { return VECMATH_WASM_I(wasm_i32x4_extend_low_i16x8(VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_cvt_hi_ssh_vec4i(vec4i a) { return VECMATH_WASM_I(wasm_i32x4_extend_high_i16x8(VECMATH_WASM_V(a))); }

VECMATH_FINLINE vec4i v_cvt_byte_vec4i(uint32_t a)
{
  v128_t u8x16 = wasm_i32x4_make((int)a, 0, 0, 0);            /* xxxx xxxx xxxx DCBA */
  v128_t u16x8 = wasm_u16x8_extend_low_u8x16(u8x16);           /* 0x0x 0x0x 0D0C 0B0A */
  return VECMATH_WASM_I(wasm_u32x4_extend_low_u16x8(u16x8));   /* 000D 000C 000B 000A */
}

// i32x4.shl/shr take the count modulo the lane width; the guards keep the x86 answer for counts
// past it (zero, or the sign for sra) and fold away on an immediate count
VECTORCALL VECMATH_FINLINE vec4i v_slli(vec4i v, int bits)
{ return (unsigned)bits < 32u ? VECMATH_WASM_I(wasm_i32x4_shl(VECMATH_WASM_V(v), (uint32_t)bits)) : v_zeroi(); }
VECTORCALL VECMATH_FINLINE vec4i v_srli(vec4i v, int bits)
{ return (unsigned)bits < 32u ? VECMATH_WASM_I(wasm_u32x4_shr(VECMATH_WASM_V(v), (uint32_t)bits)) : v_zeroi(); }
VECTORCALL VECMATH_FINLINE vec4i v_srai(vec4i v, int bits)
{ return VECMATH_WASM_I(wasm_i32x4_shr(VECMATH_WASM_V(v), (unsigned)bits < 32u ? (uint32_t)bits : 31u)); }
VECTORCALL VECMATH_FINLINE vec4i v_slli_64(vec4i v, int bits)
{ return (unsigned)bits < 64u ? VECMATH_WASM_I(wasm_i64x2_shl(VECMATH_WASM_V(v), (uint32_t)bits)) : v_zeroi(); }
VECTORCALL VECMATH_FINLINE vec4i v_srli_64(vec4i v, int bits)
{ return (unsigned)bits < 64u ? VECMATH_WASM_I(wasm_u64x2_shr(VECMATH_WASM_V(v), (uint32_t)bits)) : v_zeroi(); }
VECTORCALL VECMATH_FINLINE vec4i v_slli_n(vec4i v, int bits) { return v_slli(v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_srli_n(vec4i v, int bits) { return v_srli(v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_srai_n(vec4i v, int bits) { return v_srai(v, bits); }
// the count rides in the low 64 bits of the vector, the psll/psrl/psra register form
VECTORCALL VECMATH_FINLINE vec4i v_slli_n(vec4i v, vec4i bits) { return v_slli(v, (int)v_extract_xi64(bits)); }
VECTORCALL VECMATH_FINLINE vec4i v_srli_n(vec4i v, vec4i bits) { return v_srli(v, (int)v_extract_xi64(bits)); }
VECTORCALL VECMATH_FINLINE vec4i v_srai_n(vec4i v, vec4i bits) { return v_srai(v, (int)v_extract_xi64(bits)); }
VECTORCALL VECMATH_FINLINE vec4i v_sll(vec4i v, int bits) { return v_slli(v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_srl(vec4i v, int bits) { return v_srli(v, bits); }
VECTORCALL VECMATH_FINLINE vec4i v_sra(vec4i v, int bits) { return v_srai(v, bits); }

VECTORCALL VECMATH_FINLINE vec4i v_ori(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_v128_or(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_andi(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_v128_and(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_andnoti(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_v128_andnot(VECMATH_WASM_V(b), VECMATH_WASM_V(a))); }
VECTORCALL VECMATH_FINLINE vec4i v_xori(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_v128_xor(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }

VECTORCALL VECMATH_FINLINE vec4i v_packs(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_i16x8_narrow_i32x4(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_packs(vec4i a) { return v_packs(a, a); }
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_u16x8_narrow_i32x4(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a) { return v_packus(a, a); }
VECTORCALL VECMATH_FINLINE vec4i v_packus16(vec4i a, vec4i b) { return VECMATH_WASM_I(wasm_u8x16_narrow_i16x8(VECMATH_WASM_V(a), VECMATH_WASM_V(b))); }
VECTORCALL VECMATH_FINLINE vec4i v_packus16(vec4i a) { return v_packus16(a, a); }

// no half-precision conversion in the fixed-width proposal: the software half<->float path in
// dag_vecMath_common.h applies (_TARGET_HAS_FC16 stays undefined)
