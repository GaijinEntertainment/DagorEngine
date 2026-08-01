//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_vecMathDecl.h"
#include <stdint.h>
//
// basic arithmetic
//

//! 0
VECTORCALL VECMATH_FINLINE vec4f v_zero();
VECTORCALL VECMATH_FINLINE vec4i v_zeroi();
//! ~0
VECTORCALL VECMATH_FINLINE vec4f v_set_all_bits();
VECTORCALL VECMATH_FINLINE vec4i v_set_all_bitsi();
//! 0x80000000
VECTORCALL VECMATH_FINLINE vec4f v_msbit();
//! load vector from 16-byte aligned memory
NO_ASAN_INLINE vec4f v_ld(const float *m);
NO_ASAN_INLINE vec4i v_ldi(const int *m);
//! load vector from unaligned memory
NO_ASAN_INLINE vec4f v_ldu(const float *m);
NO_ASAN_INLINE vec4i v_ldui(const int *m);
//! load one 32 bit element, zero others
NO_ASAN_INLINE vec4f v_ldu_x(const float *m);
//! load 3x32 bit elements fast by one 4x32 bit load, but safe for thread sanitizer
NO_ASAN_INLINE vec3f v_ldu_p3(const float *m);
NO_ASAN_INLINE vec4i v_ldui_p3(const int *m);
//! load 3x32 bit elements safe, use it only when v_ldu_p3 can cause memory access crashes
NO_ASAN_INLINE vec3f v_ldu_p3_safe(const float *m);
NO_ASAN_INLINE vec4i v_ldui_p3_safe(const int *m);
//! load unaligned memory and unpacks vector from 4 signed short ints
VECTORCALL VECMATH_FINLINE vec4i v_ldush(const signed short *m);
//! load unaligned memory and unpacks vector from 4 unsigned short ints
VECTORCALL VECMATH_FINLINE vec4i v_lduush(const unsigned short *m);

//! load 64 bit from unaligned memory into .xy (.zw=0)
VECTORCALL VECMATH_FINLINE vec4i v_ldui_half(const void *m);
VECTORCALL VECMATH_FINLINE vec4f v_ldu_half(const void *m);

//! load 8 floats (4 packed float2) from 16-byte aligned memory into SoA x/y;
//! a single ld2 on NEON (e.g. an array of xy points or complex numbers)
VECTORCALL VECMATH_FINLINE void v_ld_soa2(const float *m, vec4f &x, vec4f &y);
//! load 12 floats (4 packed float3) from 16-byte aligned memory into SoA x/y/z;
//! a single ld3 on NEON (e.g. a Point3 array to SoA)
VECTORCALL VECMATH_FINLINE void v_ld_soa3(const float *m, vec4f &x, vec4f &y, vec4f &z);
//! load 16 floats (4 float4) from 16-byte aligned memory into SoA x/y/z/w; a single ld4 on NEON
VECTORCALL VECMATH_FINLINE void v_ld_soa4(const float *m, vec4f &x, vec4f &y, vec4f &z, vec4f &w);
//! unaligned variants of the deinterleaving loads
VECTORCALL VECMATH_FINLINE void v_ldu_soa2(const float *m, vec4f &x, vec4f &y);
VECTORCALL VECMATH_FINLINE void v_ldu_soa3(const float *m, vec4f &x, vec4f &y, vec4f &z);
VECTORCALL VECMATH_FINLINE void v_ldu_soa4(const float *m, vec4f &x, vec4f &y, vec4f &z, vec4f &w);

//! fetch the cache line that contains address m
VECMATH_FINLINE void v_prefetch(const void *m);

//! .xyzw = a.x
VECTORCALL VECMATH_FINLINE vec4f v_splat_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_splat_xi(vec4i a);
//! .xyzw = a.y
VECTORCALL VECMATH_FINLINE vec4f v_splat_y(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_splat_yi(vec4i a);
//! .xyzw = a.z
VECTORCALL VECMATH_FINLINE vec4f v_splat_z(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_splat_zi(vec4i a);
//! .xyzw = a.w
VECTORCALL VECMATH_FINLINE vec4f v_splat_w(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_splat_wi(vec4i a);

//! .xyzw = a
VECTORCALL VECMATH_FINLINE vec4f v_splats(float a);
VECTORCALL VECMATH_FINLINE vec4i v_splatsi(int a);
//! .xyzw = {a64 a64}
VECTORCALL VECMATH_FINLINE vec4i v_splatsi64(int64_t a);

//! .xyzw = {a 0 0 0}
VECTORCALL VECMATH_FINLINE vec4f v_set_x(float a);
VECTORCALL VECMATH_FINLINE vec4i v_seti_x(int a);

//! .xyzw = {x y z w}
VECTORCALL VECMATH_FINLINE vec4f v_make_vec4f(float x, float y, float z, float w);
VECTORCALL VECMATH_FINLINE vec4i v_make_vec4i(int x, int y, int z, int w);

//! .xyz = {x y z}
VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(float x, float y, float z);
VECTORCALL VECMATH_FINLINE vec4i v_make_vec3i(int x, int y, int z);
//! .xyz = {x y z}, where .x component of each source vector used
VECTORCALL VECMATH_FINLINE vec4f v_make_vec3f(vec4f x, vec4f y, vec4f z);

//! unpack 4 low bits from bitmask to 4x32 bits mask, true=0xFFFFFFFF, false=0
VECTORCALL VECMATH_FINLINE vec4f v_make_vec4f_mask(uint8_t bitmask);
//! create mask in all 4 elements from single bool, true=0xFFFFFFFF, false=0
VECTORCALL VECMATH_FINLINE vec4f v_bool_to_mask(bool bool_mask);
//! set highest bit in all 4 elements if param is true
VECTORCALL VECMATH_FINLINE vec4f v_bool_to_msbit(bool param);

//! insert one float to vector
VECTORCALL VECMATH_FINLINE vec4f v_insert_x(vec4f a, float x);
VECTORCALL VECMATH_FINLINE vec4f v_insert_y(vec4f a, float y);
VECTORCALL VECMATH_FINLINE vec4f v_insert_z(vec4f a, float z);
VECTORCALL VECMATH_FINLINE vec4f v_insert_w(vec4f a, float w);

//! add float value to one of components of vector
VECTORCALL VECMATH_FINLINE vec4f v_add_x(vec4f a, float x);
VECTORCALL VECMATH_FINLINE vec4f v_add_y(vec4f a, float y);
VECTORCALL VECMATH_FINLINE vec4f v_add_z(vec4f a, float z);
VECTORCALL VECMATH_FINLINE vec4f v_add_w(vec4f a, float w);

//! multiply one component of vector with float value
VECTORCALL VECMATH_FINLINE vec4f v_mul_x(vec4f a, float x);
VECTORCALL VECMATH_FINLINE vec4f v_mul_y(vec4f a, float y);
VECTORCALL VECMATH_FINLINE vec4f v_mul_z(vec4f a, float z);
VECTORCALL VECMATH_FINLINE vec4f v_mul_w(vec4f a, float w);

//! store vector to  16-byte aligned memory
VECTORCALL VECMATH_FINLINE void v_st(void *m, vec4f v);
//! store vector to unaligned memory
VECTORCALL VECMATH_FINLINE void v_stu(void *m, vec4f v);
//! store low 64 bits of vector (.xy) to unaligned memory
VECTORCALL VECMATH_FINLINE void v_stu_half(void *m, vec4f v);

//! interleaving stores, the inverse of v_ld_soa2/3/4: write SoA lanes back as packed
//! float2/float3/float4 elements (single st2/st3/st4 on NEON); 16-byte aligned memory
VECTORCALL VECMATH_FINLINE void v_st_soa2(float *m, vec4f x, vec4f y);
VECTORCALL VECMATH_FINLINE void v_st_soa3(float *m, vec4f x, vec4f y, vec4f z);
VECTORCALL VECMATH_FINLINE void v_st_soa4(float *m, vec4f x, vec4f y, vec4f z, vec4f w);
//! unaligned variants
VECTORCALL VECMATH_FINLINE void v_stu_soa2(float *m, vec4f x, vec4f y);
VECTORCALL VECMATH_FINLINE void v_stu_soa3(float *m, vec4f x, vec4f y, vec4f z);
VECTORCALL VECMATH_FINLINE void v_stu_soa4(float *m, vec4f x, vec4f y, vec4f z, vec4f w);

//! store vector to  16-byte aligned memory
VECTORCALL VECMATH_FINLINE void v_sti(void *m, vec4i v);
//! store vector to unaligned memory
VECTORCALL VECMATH_FINLINE void v_stui(void *m, vec4i v);
//! store low 64 bits of vector (.xy) to unaligned memory
VECTORCALL VECMATH_FINLINE void v_stui_half(void *m, vec4i v);
//! store 3-d vector (.xyz) to unaligned memory
VECTORCALL VECMATH_FINLINE void v_stu_p3(float *p3, vec3f v);
VECTORCALL VECMATH_FINLINE void v_stui_p3(int *p3, vec3f v);

//! merge high words: .xyzw = {a.x, b.x, a.y, b.y}
VECTORCALL VECMATH_FINLINE vec4f v_merge_hw(vec4f a, vec4f b);
//! merge low words:  .xyzw = {a.z, b.z, a.w, b.w}
VECTORCALL VECMATH_FINLINE vec4f v_merge_lw(vec4f a, vec4f b);

//! return signbit mask for each compnent - 1|2|4|8. ith bit is ith float signbit
VECTORCALL VECMATH_FINLINE int v_signmask(vec4f a);
//! same result as v_signmask, but input must be a canonical per-lane mask (v_cmp_* result);
//! cheaper on NEON, identical on SSE
VECTORCALL VECMATH_FINLINE int v_truemask(vec4f a);
//! number of true (all-ones) lanes in a canonical mask, 0..4; a horizontal sum, so it avoids
//! the mask-build plus scalar popcount of __popcount(v_truemask(m))
VECTORCALL VECMATH_FINLINE int v_count_true(vec4f a);
//! number of true (all-ones) lanes among x,y,z of a canonical mask, 0..3; the w lane is ignored
VECTORCALL VECMATH_FINLINE int v_count_true_xyz(vec4f a);
//! true if any of the 4 lanes has its sign bit set (raw floats, not just masks)
VECTORCALL VECMATH_FINLINE bool v_is_any_neg_b(vec4f a);

//! bitwise checks for whole register
VECTORCALL VECMATH_FINLINE bool v_test_all_bits_zeros(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_test_all_bits_ones(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_test_any_bit_set(vec4f a);

//! component-wise check that ALL selected components set to true (0xFFFFFFFF), not to false (0). Result for values between is UB!
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_all_true(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_check_xyz_all_true(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_check_xz_all_true(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_check_xy_all_true(vec4f a);

//! component-wise check that ANY of selected components set to true (0xFFFFFFFF), not to false (0). Result for values between is UB!
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_any_true(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_check_xyz_any_true(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_check_xy_any_true(vec4f a);

//! component-wise check that ALL selected components set to false (0), not to true (0xFFFFFFFF). Result for values between is UB!
VECTORCALL VECMATH_FINLINE bool v_check_xyzw_all_false(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_check_xyz_all_false(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_check_xy_all_false(vec4f a);

//! component-wise comparison: for C={xyzw}  .C = a.C==b.C ? 0xFFFFFFFF : 0
VECTORCALL VECMATH_FINLINE vec4f v_cmp_eq(vec4f a, vec4f b);
//! component-wise comparison: for C={xyzw}  .C = a.C!=b.C ? 0xFFFFFFFF : 0
VECTORCALL VECMATH_FINLINE vec4f v_cmp_neq(vec4f a, vec4f b);
//! component-wise integer comparison: for C={xyzw}  .C = a.C==b.C ? 0xFFFFFFFF : 0
VECTORCALL VECMATH_FINLINE vec4f v_cmp_eqi(vec4f a, vec4f b);
VECTORCALL VECMATH_FINLINE vec4i v_cmp_eqi(vec4i a, vec4i b);
//! component-wise comparison: for C={xyzw}  .C = a.C>=b.C ? 0xFFFFFFFF : 0
VECTORCALL VECMATH_FINLINE vec4f v_cmp_ge(vec4f a, vec4f b);
//! component-wise comparison: for C={xyzw}  .C = a.C>b.C ? 0xFFFFFFFF : 0
VECTORCALL VECMATH_FINLINE vec4f v_cmp_gt(vec4f a, vec4f b);
//! component-wise comparison of magnitudes: .C = |a.C|>=|b.C| ? 0xFFFFFFFF : 0; single facge on NEON
VECTORCALL VECMATH_FINLINE vec4f v_cmp_abs_ge(vec4f a, vec4f b);
//! component-wise comparison of magnitudes: .C = |a.C|>|b.C| ? 0xFFFFFFFF : 0; single facgt on NEON
VECTORCALL VECMATH_FINLINE vec4f v_cmp_abs_gt(vec4f a, vec4f b);
//! component-wise comparison: for C={xyzw}  .C = a.C<=b.C ? 0xFFFFFFFF : 0
VECTORCALL VECMATH_FINLINE vec4f v_cmp_le(vec4f a, vec4f b);
//! component-wise comparison: for C={xyzw}  .C = a.C<b.C ? 0xFFFFFFFF : 0
VECTORCALL VECMATH_FINLINE vec4f v_cmp_lt(vec4f a, vec4f b);

VECTORCALL VECMATH_FINLINE vec4i v_cmp_gti(vec4i a, vec4i b);
VECTORCALL VECMATH_FINLINE vec4i v_cmp_lti(vec4i a, vec4i b);

//! a & b
VECTORCALL VECMATH_FINLINE vec4f v_and(vec4f a, vec4f b);
//! ~a & b
VECTORCALL VECMATH_FINLINE vec4f v_andnot(vec4f a, vec4f b);
VECTORCALL VECMATH_FINLINE vec4i v_andnoti(vec4i a, vec4i b);
//! a | b
VECTORCALL VECMATH_FINLINE vec4f v_or(vec4f a, vec4f b);
//! a ^ b
VECTORCALL VECMATH_FINLINE vec4f v_xor(vec4f a, vec4f b);
//! ~a
VECTORCALL VECMATH_FINLINE vec4f v_not(vec4f a);
//! component-wise select: for C={xyzw}  .C = c.C>=0 ? a.C : b.C
VECTORCALL VECMATH_FINLINE vec4f v_sel(vec4f a, vec4f b, vec4f c);
VECTORCALL VECMATH_FINLINE vec4i v_seli(vec4i a, vec4i b, vec4i c);
//! branchless ternary operator with boolean condition .xyzw = c ? b : a
VECTORCALL VECMATH_FINLINE vec4f v_sel_b(vec4f a, vec4f b, bool c);
VECTORCALL VECMATH_FINLINE vec4i v_seli_b(vec4i a, vec4i b, bool c);
//! bit-wise select: for N={0..127}  .bitN = (c.bitN == 0) ? a.bitN : b.bitN
VECTORCALL VECMATH_FINLINE vec4f v_btsel(vec4f a, vec4f b, vec4f c);
VECTORCALL VECMATH_FINLINE vec4i v_btseli(vec4i a, vec4i b, vec4i c);

//! convert to integer using cast
VECTORCALL VECMATH_FINLINE vec4i v_cast_vec4i(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_cast_vec4f(vec4i a);

//! convert to integer using round-to-zero mode
VECTORCALL VECMATH_FINLINE vec4i v_cvti_vec4i(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i(vec4f a);//works correctly on all correct values (positive). on negative - implementation defined
VECTORCALL VECMATH_FINLINE vec4i v_cvtu_vec4i_ieee(vec4f a);//works same as scalar operations on all values
VECTORCALL VECMATH_FINLINE vec4i v_cvt_vec4i(vec4f a){return v_cvti_vec4i(a);}
//! convert to float
VECTORCALL VECMATH_FINLINE vec4f v_cvti_vec4f(vec4i a);
VECTORCALL VECMATH_FINLINE vec4f v_cvtu_vec4f(vec4i a);//works correctly on all accurately representable values (< 1<<24). on others - implementation defined
VECTORCALL VECMATH_FINLINE vec4f v_cvtu_vec4f_ieee(vec4i a);//works same as scalar operations on all values
VECTORCALL VECMATH_FINLINE vec4f v_cvt_vec4f(vec4i a) {return v_cvti_vec4f(a);}

//! unpacks 4 low/high unsigned shorts (in low 64 bits of vector, .xy) to 4 ints
VECTORCALL VECMATH_FINLINE vec4i v_cvt_lo_ush_vec4i(vec4i a);
VECTORCALL VECMATH_FINLINE vec4i v_cvt_hi_ush_vec4i(vec4i a);
//! unpacks 4 low/high signed shorts (in low 64 bits of vector, .xy) to 4 ints
VECTORCALL VECMATH_FINLINE vec4i v_cvt_lo_ssh_vec4i(vec4i a);
VECTORCALL VECMATH_FINLINE vec4i v_cvt_hi_ssh_vec4i(vec4i a);
//! unpacks 4 unsigned bytes to 4 unsigned ints in .xyzw
VECTORCALL VECMATH_FINLINE vec4i v_cvt_byte_vec4i(uint32_t a);

//! converts float vector to 4 halfs and stores(unaligned)
VECTORCALL VECMATH_FINLINE void v_float_to_half(uint16_t* __restrict m, const vec4f v);
//! unpacks 4 halfs into float vector. handles denorms, does not nesessarily (platform dependent) handles infs and nans
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float(vec4i m);
//! unpacks 4 halfs into float vector. handles denorms, infs and nans
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials(vec4i m);
//! unpacks 4 halfs from lo 64 bits of vec4i (.xy)into float vector. handles denorms, does not nesessarily handles infs and nans
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_lo(vec4i m);
//! unpacks 4 halfs from lo 64 bits of vec4i (.xy)into float vector. handles denorms, infs and nans
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials_lo(vec4i m);
//! reads(unaligned) and unpacks 4 halfs into float vector
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float(const uint16_t* __restrict m);
//! reads(unaligned) and unpacks 4 halfs into float vector
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials(const uint16_t* __restrict m);

//! converts float vector to 4 halfs (lower 16 bits of vec4i), truncates to zero, doesn't nesessarily handle specials
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_trunc(vec4f v);
//! converts float vector to 4 halfs (lower 16 bits of vec4i), truncates to zero, doesn't nesessarily handle specials
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half(vec4f v);
//handles infs/nans, doesn't round correctly
//it may (incorrectly) translates some of sNaNs into infinity, depending on platform, so be careful!
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_specials(vec4f f);

// round-to-nearest-even, doesn't nesessarily handle NANs
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_rtne(vec4f f);
//! does not nesessarily check for NANs, result halves are always bigger than source
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_up(vec4f a);
//! does not nesessarily check for NANs, result halves are always smaller than source
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_down(vec4f a);

//! converts float vector to 4 halfs (lower 64 bits of vec4i, to .xy), truncates to zero, doesn't nesessarily handle specials
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_trunc_lo(vec4f v);
//handles infs/nans, truncates
//it may (incorrectly) translates some of sNaNs into infinity, so be careful!
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_specials_lo(vec4f f);
// round-to-nearest-even, doesn't handle NANs
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_rtne_lo(vec4f f);
//! not check for NANs, result halves are always bigger than source
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_up_lo(vec4f a);
//! not check for NANs, result halves are always smaller than source
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_down_lo(vec4f a);

//! pack float vector to 4 bytes with saturation
VECMATH_FINLINE uint32_t v_float_to_byte ( vec4f x );

//! unpacks 4 bytes to float vector
VECMATH_FINLINE vec4f v_byte_to_float ( uint32_t x );

//! round to biggest integer (result remains fp)
VECTORCALL VECMATH_FINLINE vec4f v_ceil(vec4f a);

//! round to smallest integer (result remains fp)
VECTORCALL VECMATH_FINLINE vec4f v_floor(vec4f a);

//! round to biggest integer (result remains int)
VECTORCALL VECMATH_FINLINE vec4i v_cvt_ceili(vec4f a);

//! round to smallest integer (result remains int)
VECTORCALL VECMATH_FINLINE vec4i v_cvt_floori(vec4f a);

//! round to nearest integer like roundf (result remains int)
VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi(vec4f a);

//! round to nearest even integer (result remains int)
VECTORCALL VECMATH_FINLINE vec4i v_cvt_roundi_ieee(vec4f a);

//! round to zero (result remains int)
VECTORCALL VECMATH_FINLINE vec4i v_cvt_trunci(vec4f a);

//! round to nearest integer like roundf (result remains fp)
VECTORCALL VECMATH_FINLINE vec4f v_round(vec4f a);

//! round to nearest even integer (result remains fp)
VECTORCALL VECMATH_FINLINE vec4f v_round_ieee(vec4f a);

//! round to zero (result remains fp)
VECTORCALL VECMATH_FINLINE vec4f v_trunc(vec4f a);

//! (a + b)
VECTORCALL VECMATH_FINLINE vec4f v_add(vec4f a, vec4f b);
//! (a - b)
VECTORCALL VECMATH_FINLINE vec4f v_sub(vec4f a, vec4f b);
//! (a * b)
VECTORCALL VECMATH_FINLINE vec4f v_mul(vec4f a, vec4f b);
//! (a / b)
VECTORCALL VECMATH_FINLINE vec4f v_div(vec4f a, vec4f b);
//! (a / b), fast/estimate version
VECTORCALL VECMATH_FINLINE vec4f v_div_est(vec4f a, vec4f b);
//! (a / b) if |b| > VERY_SMALL_NUMBER, else def value (component-wise)
VECTORCALL VECMATH_FINLINE vec4f v_safediv(vec4f a, vec4f b, vec4f def = v_zero());
//! (a % b) (float)
VECTORCALL VECMATH_FINLINE vec4f v_mod(vec4f a, vec4f b);
//! (a * b + c)
VECTORCALL VECMATH_FINLINE vec4f v_madd(vec4f a, vec4f b, vec4f c);
//! (a * b - c)
VECTORCALL VECMATH_FINLINE vec4f v_msub(vec4f a, vec4f b, vec4f c);
//! -(a * b - c) == c - a*b
VECTORCALL VECMATH_FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c);
//! .x = (a.x + b.x)
VECTORCALL VECMATH_FINLINE vec4f v_add_x(vec4f a, vec4f b);
//! .x = (a.x - b.x)
VECTORCALL VECMATH_FINLINE vec4f v_sub_x(vec4f a, vec4f b);
//! .x = (a.x * b.x)
VECTORCALL VECMATH_FINLINE vec4f v_mul_x(vec4f a, vec4f b);
//! .x = (a.x / b.x)
VECTORCALL VECMATH_FINLINE vec4f v_div_x(vec4f a, vec4f b);
//! .x = (a.x * b.x + c.x)
VECTORCALL VECMATH_FINLINE vec4f v_madd_x(vec4f a, vec4f b, vec4f c);
//! .x = (a.x * b.x - c.x)
VECTORCALL VECMATH_FINLINE vec4f v_msub_x(vec4f a, vec4f b, vec4f c);
//! .x = (c.x - a.x * b.x)
VECTORCALL VECMATH_FINLINE vec4f v_nmsub_x(vec4f a, vec4f b, vec4f c);
//! Get middle point between a and b
VECTORCALL VECMATH_FINLINE vec4f v_midp(vec4f a, vec4f b);
//! 1/a, very unprecise, fastest available on platform
VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rcp_unprecise_x(vec4f a);
//! 1/a, fast estimate
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rcp_est_x(vec4f a);
//! 1/a, precise and recommended to use
VECTORCALL VECMATH_FINLINE vec4f v_rcp(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rcp_x(vec4f a);
//! 1/a if |a|>VERY_SMALL_NUMBER, else def value (component-wise)
VECTORCALL VECMATH_FINLINE vec4f v_rcp_safe(vec4f a, vec4f def = v_zero());
//! (a * a)
VECTORCALL VECMATH_FINLINE vec4f v_sqr(vec4f a);
//! .x = a.x*a.x
VECTORCALL VECMATH_FINLINE vec4f v_sqr_x(vec4f a);
//! min(a,b): per component a < b ? a : b (b wins on NaN), identical on all platforms
VECTORCALL VECMATH_FINLINE vec4f v_min(vec4f a, vec4f b);
VECTORCALL VECMATH_FINLINE vec4i v_mini(vec4i a, vec4i b);
VECTORCALL VECMATH_FINLINE vec4i v_minu(vec4i a, vec4i b);
//! max(a,b): per component a > b ? a : b (b wins on NaN), identical on all platforms
VECTORCALL VECMATH_FINLINE vec4f v_max(vec4f a, vec4f b);
VECTORCALL VECMATH_FINLINE vec4i v_maxi(vec4i a, vec4i b);
VECTORCALL VECMATH_FINLINE vec4i v_maxu(vec4i a, vec4i b);
//! -a
VECTORCALL VECMATH_FINLINE vec4f v_neg(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_negi(vec4i a);
//! fabs(a)
VECTORCALL VECMATH_FINLINE vec4f v_abs(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_absi(vec4i a);
//! fabs(a - b); single fabd on NEON
VECTORCALL VECMATH_FINLINE vec4f v_abs_diff(vec4f a, vec4f b);

//! clamp values in [min; max] range component-wise
VECTORCALL VECMATH_FINLINE vec4f v_clamp(vec4f t, vec4f min_val, vec4f max_val);
VECTORCALL VECMATH_FINLINE vec4i v_clampi(vec4i t, vec4i min_val, vec4i max_val);

//! special floats compare for relative equality
VECTORCALL VECMATH_FINLINE vec4f v_cmp_relative_equal(vec4f a, vec4f b, vec4f max_diff = v_splats(1e-5f), vec4f max_rel_diff = v_splats(1.192092896e-07f));
VECTORCALL VECMATH_FINLINE bool v_is_relative_equal_vec3f(vec4f a, vec4f b);
VECTORCALL VECMATH_FINLINE bool v_is_relative_equal_vec4f(vec4f a, vec4f b);

//! check if /a can produce NaN's or inf
VECTORCALL VECMATH_FINLINE vec4f v_is_unsafe_positive_divisor(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_is_unsafe_divisor(vec4f a);

//! LERP a to b using parameter tttt
VECTORCALL VECMATH_FINLINE vec4f v_lerp_vec4f(vec4f tttt, vec4f a, vec4f b);

//! Clamps to [0, 1]
VECTORCALL VECMATH_FINLINE vec4f v_saturate(vec4f a);

//! (a + b)
VECTORCALL VECMATH_FINLINE vec4i v_addi(vec4i a, vec4i b);
//! (a - b)
VECTORCALL VECMATH_FINLINE vec4i v_subi(vec4i a, vec4i b);
//! (a * b), signed integers. result is only correctly defined if both operands and result is within [-1<<31, 1<<31) range
VECTORCALL VECMATH_FINLINE vec4i v_muli(vec4i a, vec4i b);

//! 8x16-bit packed integer addition
VECTORCALL VECMATH_FINLINE vec4i v_addi16(vec4i a, vec4i b);
//! 8x16-bit packed integer subtraction
VECTORCALL VECMATH_FINLINE vec4i v_subi16(vec4i a, vec4i b);
//! 8x16-bit packed signed multiply, returns low 16 bits of each product
VECTORCALL VECMATH_FINLINE vec4i v_muli16(vec4i a, vec4i b);
//! 8x16-bit packed signed multiply, returns high 16 bits of each product
VECTORCALL VECMATH_FINLINE vec4i v_mulhi16(vec4i a, vec4i b);
//! 8x16-bit multiply pairs and pairwise add: {a0*b0+a1*b1, a2*b2+a3*b3, ...} -> 4x int32
VECTORCALL VECMATH_FINLINE vec4i v_madd_i16(vec4i a, vec4i b);
//! broadcast int16 value to all 8 16-bit lanes within vec4i
VECTORCALL VECMATH_FINLINE vec4i v_splatsi16(int v);

//! shift left (unsigned integer, 32-bit lanes). bits is immediate value
VECTORCALL VECMATH_FINLINE vec4i v_slli(vec4i v, int bits);
//! shift right (unsigned integer, 32-bit lanes). bits is immediate value
VECTORCALL VECMATH_FINLINE vec4i v_srli(vec4i v, int bits);
//! shift right (signed integer, 32-bit lanes). bits is immediate value
VECTORCALL VECMATH_FINLINE vec4i v_srai(vec4i v, int bits);
//! shift left (unsigned integer, 64-bit lanes). bits is immediate value
VECTORCALL VECMATH_FINLINE vec4i v_slli_64(vec4i v, int bits);
//! shift right (unsigned integer, 64-bit lanes). bits is immediate value
VECTORCALL VECMATH_FINLINE vec4i v_srli_64(vec4i v, int bits);
//! shift left (unsigned integer). bits is variative value, must be in [0, 31]
VECTORCALL VECMATH_FINLINE vec4i v_slli_n(vec4i v, int bits);
//! shift right (unsigned integer). bits is variative value, must be in [0, 31]
VECTORCALL VECMATH_FINLINE vec4i v_srli_n(vec4i v, int bits);
//! shift right (signed integer). bits is variative value, must be in [0, 31]
VECTORCALL VECMATH_FINLINE vec4i v_srai_n(vec4i v, int bits);
//! shift left (unsigned integer). bits is variative 64-bit value, must be in [0, 31]
VECTORCALL VECMATH_FINLINE vec4i v_slli_n(vec4i v, vec4i bits);
//! shift left (unsigned integer). bits is variative 64-bit value, must be in [0, 31]
VECTORCALL VECMATH_FINLINE vec4i v_srli_n(vec4i v, vec4i bits);
//! shift right (signed integer). bits is variative 64-bit value, must be in [0, 31]
VECTORCALL VECMATH_FINLINE vec4i v_srai_n(vec4i v, vec4i bits);

//! shift left (unsigned integer)
VECTORCALL VECMATH_FINLINE vec4i v_sll(vec4i v, int bits);
//! shift right (unsigned integer)
VECTORCALL VECMATH_FINLINE vec4i v_srl(vec4i v, int bits);
//! shift right (signed integer)
VECTORCALL VECMATH_FINLINE vec4i v_sra(vec4i v, int bits);

//! a | b (bitwise, integer)
VECTORCALL VECMATH_FINLINE vec4i v_ori(vec4i a, vec4i b);
//! a & b (bitwise, integer)
VECTORCALL VECMATH_FINLINE vec4i v_andi(vec4i a, vec4i b);
//! a ^ b (bitwise, integer)
VECTORCALL VECMATH_FINLINE vec4i v_xori(vec4i a, vec4i b);

//! .xyzw = x & y & z & w
VECTORCALL VECMATH_FINLINE vec4f v_hand(vec4f a);
//! .xyzw = x | y | z | w
VECTORCALL VECMATH_FINLINE vec4f v_hor(vec4f a);
//! -1 (all bits set) if each of the six canonical masks (v_cmp_* results) has at least one
//! true lane, 0 otherwise; branchless with no per-plane early-out: the frustum-cull callers
//! run after coarse (tile/kd) pre-culling, so most boxes reaching it pass all six planes
VECTORCALL VECMATH_FINLINE int v_is_merge_planes_nout(vec4f m0, vec4f m1, vec4f m2, vec4f m3, vec4f m4, vec4f m5);
//! .xyzw = x & y & z
VECTORCALL VECMATH_FINLINE vec4f v_hand3(vec3f a);
//! .xyzw = x | y | z
VECTORCALL VECMATH_FINLINE vec4f v_hor3(vec3f a);
//! .x = x + y + z + w
VECTORCALL VECMATH_FINLINE vec4f v_hadd4_x(vec4f a);
//! .x = x + y + z
VECTORCALL VECMATH_FINLINE vec4f v_hadd3_x(vec3f a);
//! return min of .xyzw; NaN lanes give platform-dependent results
VECTORCALL VECMATH_FINLINE vec4f v_hmin(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_hmini(vec4i a);
//! return max of .xyzw; NaN lanes give platform-dependent results
VECTORCALL VECMATH_FINLINE vec4f v_hmax(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_hmaxi(vec4i a);
//! return min of .xyz
VECTORCALL VECMATH_FINLINE vec4f v_hmin3(vec3f a);
VECTORCALL VECMATH_FINLINE vec4i v_hmini3(vec4i a);
//! return max of .xyz
VECTORCALL VECMATH_FINLINE vec4f v_hmax3(vec3f a);
VECTORCALL VECMATH_FINLINE vec4i v_hmaxi3(vec4i a);
//! pairwise min: (min(a.x,a.y), min(a.z,a.w), min(b.x,b.y), min(b.z,b.w)); single fminp on NEON.
//! NaN lanes give platform-dependent results (finite inputs are identical everywhere)
VECTORCALL VECMATH_FINLINE vec4f v_min_pairs(vec4f a, vec4f b);
//! pairwise max: (max(a.x,a.y), max(a.z,a.w), max(b.x,b.y), max(b.z,b.w)); single fmaxp on NEON.
//! NaN lanes give platform-dependent results (finite inputs are identical everywhere)
VECTORCALL VECMATH_FINLINE vec4f v_max_pairs(vec4f a, vec4f b);
//! pairwise add: (a.x+a.y, a.z+a.w, b.x+b.y, b.z+b.w); single faddp on NEON
VECTORCALL VECMATH_FINLINE vec4f v_add_pairs(vec4f a, vec4f b);
//! pairwise integer add: (a.x+a.y, a.z+a.w, b.x+b.y, b.z+b.w); single addp on NEON
VECTORCALL VECMATH_FINLINE vec4i v_addi_pairs(vec4i a, vec4i b);
//! pairwise signed integer min: (min(a.x,a.y), min(a.z,a.w), min(b.x,b.y), min(b.z,b.w)); single sminp on NEON
VECTORCALL VECMATH_FINLINE vec4i v_mini_pairs(vec4i a, vec4i b);
//! pairwise signed integer max: (max(a.x,a.y), max(a.z,a.w), max(b.x,b.y), max(b.z,b.w)); single smaxp on NEON
VECTORCALL VECMATH_FINLINE vec4i v_maxi_pairs(vec4i a, vec4i b);
//! return x*y*z*w
VECTORCALL VECMATH_FINLINE vec4f v_hmul(vec4f a);
//! return x*y*z
VECTORCALL VECMATH_FINLINE vec4f v_hmul3(vec3f a);

//! 1/sqrt_est(a), very unprecise, fastest available on platform (SSE: ~11.9 bits, NEON: ~8 bits precision)
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_unprecise(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_unprecise_x(vec4f a);
//! 1/sqrt_est(a), hardware estimate with N-R refinement, faster than IEEE, useful precision, similar between platforms
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_est(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_est_x(vec4f a);
//! .x = precise 1/sqrt(a) IEEE-754 precise
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_safe(vec4f a, vec4f def);

//! sqrt(a), very unprecise, fastest available on platform (SSE: ~11.9 bits, NEON: ~8 bits precision)
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_unprecise(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_unprecise_x(vec4f a);
//! sqrt(a), hardware estimate with N-R refinement, faster than IEEE, useful precision, similar between platforms
//! a=0 returns NaN; use v_sqrt for zero-safe behavior
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_est(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_est_x(vec4f a);
//! sqrt(a), IEEE-754 precise
VECTORCALL VECMATH_FINLINE vec4f v_sqrt(vec4f a);
//! .x = sqrt(a.x), IEEE-754 precise
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_exp(vec4f x);
VECTORCALL VECMATH_INLINE vec4f v_exp2(vec4f x);
VECTORCALL VECMATH_FINLINE vec4f v_log(vec4f x);
VECTORCALL VECMATH_FINLINE vec4f v_pow(vec4f x, vec4f y);

//! cyclic rotate
VECTORCALL VECMATH_FINLINE vec4f v_rot_1(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rot_2(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_rot_3(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_roti_1(vec4i a);
VECTORCALL VECMATH_FINLINE vec4i v_roti_2(vec4i a);
VECTORCALL VECMATH_FINLINE vec4i v_roti_3(vec4i a);

//! permutations (mostly used in common implementation), grouped by cost of the NEON lowering;
//! on SSE almost all cost a single shuffle
// 1 op on NEON (native zip/uzp/trn/ext/rev64/dup or a single lane insert)
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzwx(vec4f a); //< alias for v_rot_1()
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwxy(vec4f a); //< alias for v_rot_2()
VECTORCALL VECMATH_FINLINE vec4f v_perm_wxyz(vec4f a); //< alias for v_rot_3()
VECTORCALL VECMATH_FINLINE vec4f v_perm_yxwz(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxyy(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zzww(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxxx(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_yyyy(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zzzz(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_wwww(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xycd(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xayb(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zcwd(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_yxxc(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_bzxx(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_caxx(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzbx(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzya(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_yaxx(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zayx(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwzw(vec4f b);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxxb(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxzz(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_yyww(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyxy(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywyw(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzxz(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzz(vec4f a);
// 2 ops on NEON
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxy(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxyw(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zxzx(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_wwyy(vec4f a);
// 3 ops on NEON
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxw(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzxx(vec4f a);

// two-vector permutations; 1 op on NEON
VECTORCALL VECMATH_FINLINE vec4f v_perm_xzac(vec4f xyzw, vec4f abcd); //< uzp1
VECTORCALL VECMATH_FINLINE vec4f v_perm_ywbd(vec4f xyzw, vec4f abcd); //< uzp2
//! transpose pairs: {x,a,z,c} / {y,b,w,d}; single trn1/trn2 on NEON, 2 cheap ops on SSE
VECTORCALL VECMATH_FINLINE vec4f v_perm_xazc(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_ybwd(vec4f xyzw, vec4f abcd);
//! two-vector lane rotate: {y,z,w,a} / {z,w,a,b} / {w,a,b,c}; single ext on NEON, 1 op on SSE
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzwa(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwab(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_wabc(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyab(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwcd(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_ayzw(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xbzw(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xycw(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xyzd(vec4f xyzw, vec4f abcd);
// 2 ops on NEON
VECTORCALL VECMATH_FINLINE vec4f v_perm_xaxa(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_yybb(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_xxab(vec4f xyzw, vec4f abcd);
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzab(vec4f xyzw, vec4f abcd);
// 3 ops on NEON
VECTORCALL VECMATH_FINLINE vec4f v_perm_bbyx(vec4f xyzw, vec4f abcd);

// integer variants: single-source ones use pshufd on SSE (int domain, non-destructive);
// two-source ones are free bitcasts around the float perms, same cost as the float form
VECTORCALL VECMATH_FINLINE vec4i v_permi_xzac(vec4i xyzw, vec4i abcd);
VECTORCALL VECMATH_FINLINE vec4i v_permi_xyab(vec4i xyzw, vec4i abcd);
VECTORCALL VECMATH_FINLINE vec4i v_permi_xycd(vec4i xyzw, vec4i abcd);
VECTORCALL VECMATH_FINLINE vec4i v_permi_xbzd(vec4i xyzw, vec4i abcd);
VECTORCALL VECMATH_FINLINE vec4i v_permi_xyzd(vec4i xyzw, vec4i abcd);
VECTORCALL VECMATH_FINLINE vec4i v_permi_xzxz(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_ywyw(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_xyxy(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_zwzw(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_xxyy(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_zzww(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_xxzz(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_yyww(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_wwyy(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_yzxw(vec4i xyzw);
VECTORCALL VECMATH_FINLINE vec4i v_permi_yzxy(vec4i xyzw);


//! pack 8x 32-bit ints into 8x 16-bit ints
VECTORCALL VECMATH_FINLINE vec4i v_packs(vec4i a, vec4i b);
//! pack 8x 32-bit ints (value being duplicated) into 8x 16-bit ints
VECTORCALL VECMATH_FINLINE vec4i v_packs(vec4i a);
//! pack 8x 32-bit ints into 8x unsigned 16-bit ints
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a, vec4i b);
//! pack 8x 32-bit ints (value being duplicated) into 8x unsigned 16-bit ints
VECTORCALL VECMATH_FINLINE vec4i v_packus(vec4i a);
//! pack 16x 16-bit ints into 16x unsigned 8-bit ints
VECTORCALL VECMATH_FINLINE vec4i v_packus16(vec4i a, vec4i b);
//! pack 16x 16-bit ints into 16x unsigned 8-bit ints
VECTORCALL VECMATH_FINLINE vec4i v_packus16(vec4i a);

//! interleave low 8 bytes from a,b: {a0,b0,a1,b1,...,a7,b7}
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i8(vec4i a, vec4i b);
//! interleave high 8 bytes from a,b: {a8,b8,a9,b9,...,a15,b15}
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i8(vec4i a, vec4i b);

//! interleave low 4 shorts from a,b: {a0,b0,a1,b1,a2,b2,a3,b3}
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i16(vec4i a, vec4i b);
//! interleave high 4 shorts from a,b: {a4,b4,a5,b5,a6,b6,a7,b7}
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i16(vec4i a, vec4i b);
//! interleave low 2 ints from a,b: {a0,b0,a1,b1}
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i32(vec4i a, vec4i b);
//! interleave high 2 ints from a,b: {a2,b2,a3,b3}
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i32(vec4i a, vec4i b);
//! interleave low int64 from a,b: {a0,b0}
VECTORCALL VECMATH_FINLINE vec4i v_interleave_lo_i64(vec4i a, vec4i b);
//! interleave high int64 from a,b: {a1,b1}
VECTORCALL VECMATH_FINLINE vec4i v_interleave_hi_i64(vec4i a, vec4i b);

//! dynamic byte shuffle (pshufb semantics on all platforms): for each of the
//! 16 bytes, result[i] = (k[i] & 0x80) ? 0 : t[k[i] & 15]
VECTORCALL VECMATH_FINLINE vec4i v_perm_i8(vec4i t, vec4i k);
//! per-byte compare, returns byte mask (0xFF where equal, 0 elsewhere)
VECTORCALL VECMATH_FINLINE vec4i v_cmp_eqi8(vec4i a, vec4i b);


//
// vector algebra
//
//! dot product: .xyzw = (a.x * b.x + a.y * b.y); a.z, b.z, a.w, b.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec4f v_dot2(vec4f a, vec4f b);
//! dot product: .xyzw = (a.x * b.x + a.y * b.y + a.z * b.z); a.w, b.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec4f v_dot3(vec4f a, vec4f b);
//! dot product: .xyzw = (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w)
VECTORCALL VECMATH_FINLINE vec4f v_dot4(vec4f a, vec4f b);
//! dot product: .x = (a.x * b.x + a.y * b.y); a.z, b.z, a.w, b.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec4f v_dot2_x(vec4f a, vec4f b);
//! dot product: .x = (a.x * b.x + a.y * b.y + a.z * b.z); a.w, b.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec4f v_dot3_x(vec4f a, vec4f b);
//! dot product: .x = (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w)
VECTORCALL VECMATH_FINLINE vec4f v_dot4_x(vec4f a, vec4f b);
//! cross product: a x b; ; a.w,b.w could be anything, even NAN
VECTORCALL VECMATH_FINLINE vec3f v_cross3(vec3f a, vec3f b);

//! make plane from 3 points
VECTORCALL VECMATH_FINLINE plane3f v_make_plane(vec3f p0, vec3f p1, vec3f p2);

//! make plane from point and two dirs from point
VECTORCALL VECMATH_FINLINE plane3f v_make_plane_dir(vec3f p0, vec3f dir0, vec3f dir1);

//! make plane from point and normal of the plane
VECTORCALL VECMATH_FINLINE plane3f v_make_plane_norm(vec3f p0, vec3f norm);

//! transform plane with matrix
VECTORCALL VECMATH_FINLINE plane3f v_transform_plane(plane3f plane, mat44f_cref transform);
VECTORCALL VECMATH_FINLINE plane3f v_norm_plane(plane3f in);
VECTORCALL VECMATH_FINLINE vec3f v_ray_intersect_plane(vec3f A, vec3f B, plane3f P, vec4f &behind, vec4f &t);
VECTORCALL VECMATH_FINLINE vec3f v_unsafe_ray_intersect_plane(vec3f point, vec3f dir, plane3f P);
VECTORCALL VECMATH_FINLINE void v_unsafe_two_plane_intersection(plane3f p1, plane3f p2, vec3f &point, vec3f &dir);

//! distance from point b to plane a: .xyzw = (a.x * b.x + a.y * b.y + a.z * b.z + a.w)
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist(plane3f a, vec3f b);

//! distance from point b to plane a: .x = (a.x * b.x + a.y * b.y + a.z * b.z + a.w)
VECTORCALL VECMATH_FINLINE vec4f v_plane_dist_x(plane3f a, vec3f b);

//! scalar triple product: (a x b) * c = dot3(cross3(a, b), c)
VECTORCALL VECMATH_FINLINE vec3f v_striple3(vec3f a, vec3f b, vec3f c);
//! vector triple product: a x (b x c) = b(a*c) - c(a*b) = cross3(a, cross(b, c)
VECTORCALL VECMATH_FINLINE vec3f v_vtriple3(vec3f a, vec3f b, vec3f c);

//! length squared: .xyzw = a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w
VECTORCALL VECMATH_FINLINE vec4f v_length4_sq(vec4f a);
//! length squared: .xyzw = a.x*a.x + a.y*a.y + a.z*a.z
VECTORCALL VECMATH_FINLINE vec3f v_length3_sq(vec3f a);
//! length squared: .xyzw = a.x*a.x + a.y*a.y
VECTORCALL VECMATH_FINLINE vec4f v_length2_sq(vec4f a);

//! length: .xyzw = sqrt_est(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w)
VECTORCALL VECMATH_FINLINE vec4f v_length4_est(vec4f a);
//! estimate length: .xyzw = sqrt_est(a.x*a.x + a.y*a.y + a.z*a.z)
VECTORCALL VECMATH_FINLINE vec3f v_length3_est(vec3f a);
//! estimate length: .xyzw = sqrt_est(a.x*a.x + a.y*a.y)
VECTORCALL VECMATH_FINLINE vec4f v_length2_est(vec4f a);
//! length: .xyzw = sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w)
VECTORCALL VECMATH_FINLINE vec4f v_length4(vec4f a);
//! length: .xyzw = sqrt(a.x*a.x + a.y*a.y + a.z*a.z)
VECTORCALL VECMATH_FINLINE vec3f v_length3(vec3f a);
//! length: .xyzw = sqrt(a.x*a.x + a.y*a.y)
VECTORCALL VECMATH_FINLINE vec4f v_length2(vec4f a);
//! length squared: .x = a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w
VECTORCALL VECMATH_FINLINE vec4f v_length4_sq_x(vec4f a);
//! length squared: .x = a.x*a.x + a.y*a.y + a.z*a.z, a.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec3f v_length3_sq_x(vec3f a);
//! length squared: .x = a.x*a.x + a.y*a.y, a.z, a.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec4f v_length2_sq_x(vec4f a);

//! estimate length: .x = sqrt_est(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w)
VECTORCALL VECMATH_FINLINE vec4f v_length4_est_x(vec4f a);
//! estimate length: .x = sqrt_est(a.x*a.x + a.y*a.y + a.z*a.z), a.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec3f v_length3_est_x(vec3f a);
//! estimate length: .x = sqrt_est(a.x*a.x + a.y*a.y, a.z, a.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec4f v_length2_est_x(vec4f a);
//! length: .x = sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w)
VECTORCALL VECMATH_FINLINE vec4f v_length4_x(vec4f a);
//! length: .x = sqrt(a.x*a.x + a.y*a.y + a.z*a.z), a.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec3f v_length3_x(vec3f a);
//! length: .x = sqrt(a.x*a.x + a.y*a.y), a.z, a.w could be anything (even NAN)
VECTORCALL VECMATH_FINLINE vec4f v_length2_x(vec4f a);

//! distance between two points
VECTORCALL VECMATH_FINLINE vec4f v_distance_xyz_x(vec4f a, vec4f b);
//! horizontal distance between two points
VECTORCALL VECMATH_FINLINE vec4f v_distance_x0z_x(vec4f a, vec4f b);

//! normalize: a/length(a). will return NaN for zero vector
VECTORCALL VECMATH_FINLINE vec4f v_norm4(vec4f a);
//! normalize: a/length(a), .w could be anything (even NAN). will return NaN for zero vector
VECTORCALL VECMATH_FINLINE vec3f v_norm3(vec3f a);
//! normalize: a/length(a), .z, .w could be anything (even NAN). will return NaN for zero vector
VECTORCALL VECMATH_FINLINE vec4f v_norm2(vec4f a);

//! estimated normalize: a*rsqrt_est(dot), faster and less precise than v_norm*
VECTORCALL VECMATH_FINLINE vec4f v_norm4_est(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_norm3_est(vec3f a);
VECTORCALL VECMATH_FINLINE vec4f v_norm2_est(vec4f a);

//! safe normalize: a/length(a), return def value for zero vector
VECTORCALL VECMATH_FINLINE vec4f v_norm4_safe(vec4f a, vec4f def);
VECTORCALL VECMATH_FINLINE vec4f v_norm3_safe(vec3f a, vec3f def);
VECTORCALL VECMATH_FINLINE vec4f v_norm2_safe(vec4f a, vec4f def);

//! reset NaN values to 0.f component-wise
VECTORCALL VECMATH_FINLINE vec4f v_remove_nan(vec4f a);
//! check for NaN component-wise
VECTORCALL VECMATH_FINLINE vec4f v_is_nan(vec4f a);
//! check that values are finite numbers and not NaN component-wise
VECTORCALL VECMATH_FINLINE vec4f v_is_finite(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_is_not_finite(vec4f a);
  //! check for negative value component-wise
VECTORCALL VECMATH_FINLINE vec4f v_is_neg(vec4f a);
//! check for NaN in any of component
VECTORCALL VECMATH_FINLINE bool v_test_xyzw_nan(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_test_xyz_nan(vec3f a);
VECTORCALL VECMATH_FINLINE bool v_test_mat43_nan(mat44f m);
VECTORCALL VECMATH_FINLINE bool v_test_mat44_nan(mat44f m);
//! check for finite value all selected components
VECTORCALL VECMATH_FINLINE bool v_test_xyzw_finite(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_test_xyz_finite(vec3f a);
VECTORCALL VECMATH_FINLINE bool v_test_xy_finite(vec4f a);
VECTORCALL VECMATH_FINLINE bool v_test_x_finite(vec4f a);
//! nans and infs converted to zero
VECTORCALL VECMATH_FINLINE vec4f v_remove_not_finite(vec4f a);
//! test that all of .xyz less than limit by absolute value; limit must be non-negative
VECTORCALL VECMATH_FINLINE bool v_test_xyz_abs_lt(vec3f a, vec3f limit);


//
// vector transformations
//

//! m * v,  matrix is treated column-major
VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec4(mat44f_cref m, vec4f v);
//! m * v,  matrix is treated column-major, v.w=0
VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec3v(mat44f_cref m, vec3f v);
//! m * v,  matrix is treated column-major, v.w=1
VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec3p(mat44f_cref m, vec3f v);
//! m * v,  matrix is treated column-major
VECTORCALL VECMATH_FINLINE vec3f v_mat33_mul_vec3(mat33f_cref m, vec3f v);
//! m * v,  matrix is treated row-major, v.w=0
VECTORCALL VECMATH_FINLINE vec3f v_mat43_mul_vec3v(mat43f_cref m, vec3f v);
//! m * v,  matrix is treated row-major, v.w=1
VECTORCALL VECMATH_FINLINE vec3f v_mat43_mul_vec3p(mat43f_cref m, vec3f v);

  //! transfrom position and apply max scale to radius
VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_bsph(mat44f_cref m, vec4f bsph);
VECTORCALL VECMATH_FINLINE void v_mat44_mul_bsph(mat44f_cref m, vec4f &bsph_pos, vec4f &bsph_rad_x);

//! LERP matrix a to b using parameter tttt
VECTORCALL VECMATH_FINLINE mat44f v_mat44_lerp(vec4f tttt, mat44f_cref a, mat44f_cref b);

//! q * v, rotate vector using normalized quaternion
VECTORCALL VECMATH_FINLINE vec3f v_quat_mul_vec3(quat4f q, vec3f v);

//! q1 * q2, effectively multiply rotations
VECTORCALL VECMATH_FINLINE quat4f v_quat_mul_quat(quat4f q1, quat4f q2);

//! compute conjugate quaternion
VECTORCALL VECMATH_FINLINE quat4f v_quat_conjugate(quat4f q);
VECTORCALL VECMATH_FINLINE quat4f v_quat_180_from_unit(vec3f v0);
VECTORCALL VECMATH_FINLINE void v_quat_decompose_swing_twist(quat4f q, vec3f dir, quat4f& swing, quat4f& twist);
VECTORCALL VECMATH_FINLINE void v_quat_decompose_twist_swing(quat4f q, vec3f dir, quat4f& twist, quat4f& swing);

//
// matrix algebra
//

//! identity matrix 4x4
VECTORCALL VECMATH_FINLINE void v_mat44_ident(mat44f &dest);
//! identity matrix 4x4 with swapped X and Z columns
VECTORCALL VECMATH_FINLINE void v_mat44_ident_swapxz(mat44f &dest);
//! identity matrix 3x3
VECTORCALL VECMATH_FINLINE void v_mat33_ident(mat33f &dest);
//! identity matrix 4x4 with swapped X and Z columns
VECTORCALL VECMATH_FINLINE void v_mat33_ident_swapxz(mat33f &dest);

//! make perspective matrix 4x4
VECTORCALL VECMATH_FINLINE void v_mat44_make_persp_reverse(mat44f &dest, float wk, float hk, float zn, float zf);//zn - 1, zf - 0
VECTORCALL VECMATH_FINLINE void v_mat44_make_persp_forward(mat44f &dest, float wk, float hk, float zn, float zf);//zn - 0, zf - 1
VECTORCALL VECMATH_FINLINE void v_mat44_make_persp(mat44f &dest, float wk, float hk, float zn, float zf);// same as reverse
//! make view matrix 3x3 from look_dir
VECTORCALL VECMATH_FINLINE void v_mat33_make_from_look(mat33f &dest, vec4f look_dir);
//! make view submatrix 3x3 of 4x3 from look_dir
VECTORCALL VECMATH_FINLINE void v_mat44_make33_from_look(mat44f &dest, vec4f look_dir);
VECTORCALL VECMATH_FINLINE void v_view_matrix_from_tangentZ(mat33f &dest, vec4f look_dir);
//! make view submatrix 3x3 of 4x3 from look_dir
VECTORCALL VECMATH_FINLINE void v_mat44_make_look_at(mat44f &dest, vec4f eye, vec4f at, vec4f up);
//! make orthogonal projection matrix
VECTORCALL VECMATH_FINLINE void v_mat44_make_ortho(mat44f &dest, float w, float h, float zn, float zf);

//! make rotational matrix 3x3 to rotate 'ang' radians around 'v' clockwise
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw(mat33f &dest, vec3f v, vec4f ang);
//! make rotational matrix 3x3 to rotate 'ang' radians around X axis clockwise
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw_x(mat33f &dest, vec4f ang);
//! make rotational matrix 3x3 to rotate 'ang' radians around Y axis clockwise
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw_y(mat33f &dest, vec4f ang);
//! make rotational matrix 3x3 to rotate 'ang' radians around Z axis clockwise
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw_z(mat33f &dest, vec4f ang);
//! make composite matrix 3x3: =rot_z(ang.z)*rot_y(ang.y)*rot_x(ang.x)
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw_zyx(mat33f &dest, vec4f ang_xyz);

//! T(m), 4x4 -> 4x4
VECTORCALL VECMATH_FINLINE void v_mat44_transpose(mat44f &dest, mat44f src);
VECTORCALL VECMATH_FINLINE void v_mat44_transpose(vec4f &r0, vec4f &r1, vec4f &r2, vec4f &r3);
//! T(m), 3x3 -> 3x3
VECTORCALL VECMATH_FINLINE void v_mat33_transpose(mat33f &dest, vec3f src_col0, vec3f src_col1, vec3f src_col2);
VECTORCALL VECMATH_FINLINE void v_mat33_transpose(mat33f &dest, mat33f_cref src);
//! T(m), 4x4 -> 3x3 (transpose of the 3x3 part; last column ignored, .w lanes zeroed)
VECTORCALL VECMATH_FINLINE void v_mat44_transpose_to_mat33(mat33f &dest, mat44f src);

//! T(m), 4x4 column major -> 4x3 row major
VECTORCALL VECMATH_FINLINE void v_mat44_transpose_to_mat43(mat43f &dest, mat44f src);
//! T(m), 4x3 row major -> 4x4 column major
VECTORCALL VECMATH_FINLINE void v_mat43_transpose_to_mat44(mat44f &dest, mat43f src);
//! same from a mat43f in memory, fusing the load and the transpose (single ld4 on NEON).
//! UNSAFE: reads 16 bytes past m43 - only for mat43f inside bigger arrays/pools, never the
//! last 48 bytes of an allocation; all .w lanes of the result are undefined
NO_ASAN_INLINE void v_mat44_load_from_mat43_overread_unsafe(mat44f &dest, const mat43f &m43);
//! extract rotation/scale transformation from mat44 to mat33
VECTORCALL VECMATH_FINLINE void v_mat33_from_mat44(mat33f &dest, mat44f_cref m);

//! make 3x3 rotation matrix from quaternion
VECTORCALL VECMATH_FINLINE void v_mat33_from_quat(mat33f &dest, quat4f q);
//! make 4x4 matrix from quaternion and position
VECTORCALL VECMATH_FINLINE void v_mat44_from_quat(mat44f &dest, quat4f q, vec4f pos);
//! compose 4x4 matrix from position/rotation/scale
VECTORCALL VECMATH_FINLINE void v_mat44_compose(mat44f &dest, vec4f pos, vec4f scale);
//! compose 4x4 matrix from position/rotation/scale
VECTORCALL VECMATH_FINLINE void v_mat44_compose(mat44f &dest, vec4f pos, quat4f rot, vec4f scale);
//! compose 3x3 matrix from rotation/scale
VECTORCALL VECMATH_FINLINE void v_mat33_compose(mat33f &dest, quat4f rot, vec4f scale);

//! decompose 3x3 matrix to rotation/scale
VECTORCALL VECMATH_FINLINE void v_mat33_decompose(mat33f_cref tm, quat4f &rot, vec4f &scl);
//! decompose 4x4 matrix to position/rotation/scale (assuming ordinary matrix 4x3)
VECTORCALL VECMATH_FINLINE void v_mat4_decompose(mat44f_cref tm, vec3f &pos, quat4f &rot, vec4f &scl);

//! m1+m2
VECTORCALL VECMATH_FINLINE void v_mat44_add(mat44f &dest, mat44f_cref m1, mat44f_cref m2);
//! m1+m2
VECTORCALL VECMATH_FINLINE void v_mat33_add(mat33f &dest, mat33f_cref m1, mat33f_cref m2);
//! m1-m2
VECTORCALL VECMATH_FINLINE void v_mat44_sub(mat44f &dest, mat44f_cref m1, mat44f_cref m2);
//! m1-m2
VECTORCALL VECMATH_FINLINE void v_mat33_sub(mat33f &dest, mat33f_cref m1, mat33f_cref m2);
//! -m
VECTORCALL VECMATH_FINLINE void v_mat33_neg(mat44f &dest, mat44f_cref m);
//! -m
VECTORCALL VECMATH_FINLINE void v_mat33_neg(mat33f &dest, mat33f_cref m);
//! per-elem multiply m1 and m2
VECTORCALL VECMATH_FINLINE void v_mat44_mul_elem(mat44f &dest, mat44f_cref m1, mat44f_cref m2);
//! per-elem multiply m1 and m2
VECTORCALL VECMATH_FINLINE void v_mat33_mul_elem(mat33f &dest, mat33f_cref m1, mat33f_cref m2);

//! m1*m2
VECTORCALL VECMATH_FINLINE void v_mat44_mul(mat44f &dest, mat44f_cref m1, mat44f_cref m2);
//! m1*m2, assuming m2.col1.w=m2.col2.w=m2.col0.w=0, m2.col3=0,0,0,1
VECTORCALL VECMATH_FINLINE void v_mat44_mul43(mat44f &dest, mat44f_cref m1, mat44f_cref m2);
//! m1*m2, assuming m2.col1.w=m2.col2.w=m2.col0.w=0, m2.col3=0,0,0,1.
//Faster, then v_mat44_mul43(dest, m1, v_mat43_transpose_to_mat44(m2));
VECTORCALL VECMATH_FINLINE void v_mat44_mul43(mat44f &dest, mat44f_cref m1, mat43f_cref m2);
//! m1*m2, taking only rotational matrix from m2
VECTORCALL VECMATH_FINLINE void v_mat44_mul33r(mat44f &dest, mat44f_cref m1, mat44f_cref m2);
//! m1*m2, assuming m2 is pure rotational matrix 3x3
VECTORCALL VECMATH_FINLINE void v_mat44_mul33(mat44f &dest, mat44f_cref m1, mat33f_cref m2);
//! m1*m2
VECTORCALL VECMATH_FINLINE void v_mat33_mul(mat33f &dest, mat33f_cref m1, mat33f_cref m2);
//! m1*m2, taking only rotational matrix from m2
VECTORCALL VECMATH_FINLINE void v_mat33_mul33r(mat33f &dest, mat33f_cref m1, mat44f_cref m2);
//! orthonormalize 3x3 part of 4x4 matrix
VECTORCALL VECMATH_FINLINE void v_mat44_orthonormalize33(mat44f &dest, mat44f_cref m);
//! orthonormalize 3x3 matrix
VECTORCALL VECMATH_FINLINE void v_mat33_orthonormalize(mat33f &dest, mat33f_cref m);
//! normalize columns of a 3x3 matrix (removes scale, keeps shear)
VECTORCALL VECMATH_FINLINE void v_mat33_remove_scale(mat33f &dest, mat33f_cref m);
//! 1/m
VECTORCALL VECMATH_FINLINE void v_mat44_inverse(mat44f &dest, mat44f_cref m);
//! 1/m, assuming col1.w=col2.w=col0.w=0, col3=0,0,0,1. Resulting matrix will not conform same assumption (i.e. it will be 43 matrix, not 44)!
VECTORCALL VECMATH_FINLINE void v_mat44_inverse43(mat44f &dest, mat44f_cref m);
//! 1/m, assuming col1.w=col2.w=col0.w=0, col3=0,0,0,1. Resulting matrix is correct 4x4 matrix, col0.w = col1.w = col2.w = 0; col3.w = 1
VECTORCALL VECMATH_FINLINE void v_mat44_inverse43_to44(mat44f &dest, mat44f_cref m);
//! 1/m
VECTORCALL VECMATH_FINLINE void v_mat33_inverse(mat33f &dest, mat33f_cref m);
//! 1/m == T33(m), col3 = -T33(m)*col3, with m assumed being orthonormal 43 matrix (i.e. col0.w=col1.w=col2.w = 0; col3.w = 1). Resulting matrix will not be 44 matrix, having garbage in col3.w!
VECTORCALL VECMATH_FINLINE void v_mat44_orthonormal_inverse43(mat44f &dest, mat44f_cref m);
//! 1/m == T33(m), col3 = -T33(m)*col3, with m assumed being orthonormal 43 matrix (i.e. col0.w=col1.w=col2.w = 0; col3.w = 1). Resulting matrix will be valid 44 matrix, with col3.w == 1.
VECTORCALL VECMATH_FINLINE void v_mat44_orthonormal_inverse43_to44(mat44f &dest, mat44f_cref m);

//! force the affine bottom row: col0..2.w = 0, col3.w = 1 (e.g. after ops leaving .w undefined)
VECTORCALL VECMATH_FINLINE void v_mat44_make_affine(mat44f &m);

//! 1/m == T(m), with m being orthonormal
VECTORCALL VECMATH_FINLINE void v_mat33_orthonormal_inverse(mat33f &dest, mat33f_cref m);

//! Determinant(m)
VECTORCALL VECMATH_FINLINE vec4f v_mat44_det(mat44f_cref m);
//! Determinant(m), assuming col1.w=col2.w=col0.w=0, col3=0,0,0,1
VECTORCALL VECMATH_FINLINE vec4f v_mat44_det43(mat44f_cref m);
//! Determinant(m)
VECTORCALL VECMATH_FINLINE vec4f v_mat33_det(mat33f_cref m);
//! Calculate maximum scale of 3 axes
VECTORCALL VECMATH_FINLINE vec4f v_mat44_max_scale43_sq(mat44f_cref tm);
VECTORCALL VECMATH_FINLINE vec4f v_mat44_max_scale43(mat44f_cref tm);
VECTORCALL VECMATH_FINLINE vec4f v_mat44_max_scale43_x(mat44f_cref tm);
//! .xyz = scales of 3 axes
VECTORCALL VECMATH_FINLINE vec3f v_mat44_scale43_sq(mat44f_cref tm);
//! apply scale from .xyz to 3 axes
VECTORCALL VECMATH_FINLINE void v_mat44_apply_scale33(mat44f &tm, vec3f scale);
//! normalize columns 0..2 (removes scale, keeps shear and translation col3)
VECTORCALL VECMATH_FINLINE void v_mat44_remove_scale33(mat44f &dest, mat44f_cref m);

//! stores mat33f to unaligned Matrix3
VECTORCALL VECMATH_FINLINE void v_mat_33cu_from_mat33(float * __restrict m33, const mat33f& tm);

//! mat44f from 16 floats of unaligned memory holding the four basis vectors in
//! column order (a TMatrix4 has the same layout)
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_44cu(mat44f &tmV, const float *const __restrict m44);
//! same from 16-byte aligned memory
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_44ca(mat44f &tmV, const float *const __restrict m44);
//! stores mat44f as 16 floats to unaligned memory (e.g. a TMatrix4)
VECTORCALL VECMATH_FINLINE void v_mat_44cu_from_mat44(float * __restrict m44, const mat44f &tm);
//! same to 16-byte aligned memory
VECTORCALL VECMATH_FINLINE void v_mat_44ca_from_mat44(float * __restrict m44, const mat44f &tm);

//! stores mat44f to aligned TMatrix from mat44f
VECTORCALL VECMATH_FINLINE void v_mat_43ca_from_mat44(float * __restrict m43, const mat44f &tm);

//! stores mat44f to unaligned TMatrix
VECTORCALL VECMATH_FINLINE void v_mat_43cu_from_mat44(float * __restrict m43, const mat44f &tm);

//! stores mat43f to aligned TMatrix; one interleaving store (single st3 on NEON) -
//! the rows' .w lanes are exactly the translation triple the TMatrix layout ends with
VECTORCALL VECMATH_FINLINE void v_mat_43ca_from_mat43(float * __restrict m43, mat43f_cref tm);

//! stores mat43f to unaligned TMatrix
VECTORCALL VECMATH_FINLINE void v_mat_43cu_from_mat43(float * __restrict m43, mat43f_cref tm);

//! mat33f from unaligned Matrix3 (9 floats); cols' .w = 0. Reads one float past the matrix
//! like v_ldu_p3 does
VECTORCALL VECMATH_FINLINE void v_mat33_make_from_33cu(mat33f &tmV, const float *const __restrict m33);
//! same, reads exactly the 9 floats; use it only when v_mat33_make_from_33cu can crash
VECTORCALL VECMATH_FINLINE void v_mat33_make_from_33cu_safe(mat33f &tmV, const float *const __restrict m33);

//! mat44f from unaligned TMatrix
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu(mat44f &tmV, const float *const __restrict m43);

//! mat44f from unaligned TMatrix but don't set row3 to {0,0,0,1} (will be undefined)
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43cu_unsafe(mat44f &tmV, const float *const __restrict m43);

//! mat44f from memaligned TMatrix
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_43ca(mat44f &tmV, const float *const __restrict m43);

//! mat43f (transposed rows) from unaligned TMatrix; TMatrix column triples are exactly
//! the interleaved form of the rows, so this is one deinterleaving load (single ld3 on NEON)
VECTORCALL VECMATH_FINLINE void v_mat43_make_from_43cu(mat43f &tmV, const float *const __restrict m43);

//! same, but the rows' .w lanes are undefined (rotation columns only); cheaper on SSE
VECTORCALL VECMATH_FINLINE void v_mat43_make_from_43cu_unsafe(mat43f &tmV, const float *const __restrict m43);
VECTORCALL VECMATH_FINLINE void v_mat43_sub(mat43f &dest, mat43f_cref m1, mat43f_cref m2);

//! orthonormal inverse (1/m == T33(m), col3 = -T33(m)*pos) straight from an unaligned TMatrix;
//! m must be orthonormal; result is a valid 44 matrix (col0..2.w = 0, col3.w = 1)
VECTORCALL VECMATH_FINLINE void v_mat44_orthonormal_inverse_from_43cu(mat44f &dest, const float *const __restrict m43);

//! same, but all .w lanes of the result are undefined
VECTORCALL VECMATH_FINLINE void v_mat44_orthonormal_inverse_from_43cu_unsafe(mat44f &dest, const float *const __restrict m43);

//! .xyz = last column of matrix (.w of each row)
VECTORCALL VECMATH_FINLINE vec3f v_mat43_extract_pos(mat43f_cref mat);

//
// Axis-aligned bounding boxes
//

//! init as empty
VECTORCALL VECMATH_FINLINE void v_bbox3_init_empty(bbox3f &b);
//! init as ident (+-0.5 each coord)
VECTORCALL VECMATH_FINLINE void v_bbox3_init_ident(bbox3f &b);
//! init with point
VECTORCALL VECMATH_FINLINE void v_bbox3_init(bbox3f &b, vec3f p);
VECTORCALL VECMATH_FINLINE bool v_bbox3_is_empty(bbox3f bbox);
//! init with bb2 rotated/scaled by 3 column vectors (no translation)
VECTORCALL VECMATH_FINLINE void v_bbox3_rotate_init(bbox3f &b, vec3f col0, vec3f col1, vec3f col2, bbox3f_cref bb2);
//! init with bb2 transformed with 3x3 matrix m (rotation/scale only, no translation)
VECTORCALL VECMATH_FINLINE void v_bbox3_init(bbox3f &b, mat33f_cref m, bbox3f bb2);
//! init with bb2 transformed with 4x4 matrix m
VECTORCALL VECMATH_FINLINE void v_bbox3_init(bbox3f &b, mat44f_cref m, bbox3f bb2);
//! init with bsph
VECTORCALL VECMATH_FINLINE void v_bbox3_init_by_bsph(bbox3f &b, vec3f bsph_center, vec3f bsph_radius);
//! init with ray
VECTORCALL VECMATH_FINLINE void v_bbox3_init_by_ray(bbox3f &b, vec3f from, vec3f dir, vec4f len);
//! init with segment
VECTORCALL VECMATH_FINLINE void v_bbox3_init_by_segment(bbox3f &b, vec4f from, vec4f to);

//! extend bbox to enclose point
VECTORCALL VECMATH_FINLINE void v_bbox3_add_pt(bbox3f &b, vec3f p);
//! extend bbox to enclose bb2
VECTORCALL VECMATH_FINLINE void v_bbox3_add_box(bbox3f &b, bbox3f b2);
//! extend bbox to enclose bb2 transformed with 4x4 matrix m
VECTORCALL VECMATH_FINLINE void v_bbox3_add_transformed_box(bbox3f &b, mat44f_cref m, bbox3f b2);
//! extend bbox to enclose ray
VECTORCALL VECMATH_FINLINE void v_bbox3_add_ray(bbox3f &b, vec3f from, vec3f dir, vec4f len);
//! return b1+b2
VECTORCALL VECMATH_FINLINE bbox3f v_bbox3_sum(bbox3f b1, bbox3f b2);
//! return c ? b2 : b1 component-wise
VECTORCALL VECMATH_FINLINE bbox3f v_bbox3_sel(bbox3f b1, bbox3f b2, vec4f c);

//! .xyz = bbox dimensions; for empty bbox dimensions will be invalid (negative)
VECTORCALL VECMATH_FINLINE vec3f v_bbox3_size(bbox3f b);
//! .xyzw = bbox max dimension
VECTORCALL VECMATH_FINLINE vec3f v_bbox3_max_size(bbox3f b);
//! scale bbox by size_factor
VECTORCALL VECMATH_FINLINE bbox3f v_bbox3_scale(bbox3f b, vec4f size_factor);
//! .xyz = bbox center
VECTORCALL VECMATH_FINLINE vec3f v_bbox3_center(bbox3f b);
// get bbox vertex by index [0;7]
VECTORCALL VECMATH_FINLINE vec3f v_bbox3_point(bbox3f b, int idx);
//! .x = radius of outer sphere
VECTORCALL VECMATH_FINLINE vec4f v_bbox3_outer_rad(bbox3f b);
//! .x = radius of inner sphere
VECTORCALL VECMATH_FINLINE vec4f v_bbox3_inner_rad(bbox3f b);
//! .x = diameter of inner sphere
VECTORCALL VECMATH_FINLINE vec4f v_bbox3_inner_diameter(bbox3f b);
//! .x = radius of outer sphere from zero coords
VECTORCALL VECMATH_FINLINE vec4f v_bbox3_outer_rad_from_zero(bbox3f b);

//! tests whether point is inside box and returns boolean (0 or non-0)
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_pt_inside(bbox3f b, vec3f p);
//! tests whether point is inside box xz and returns boolean (0 or non-0)
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_pt_inside_xz(bbox3f b, vec3f p);
//! tests whether box b2 is fully inside box and returns boolean
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_box_inside(bbox3f b1, bbox3f b2);
//! tests whether boxes intersect and returns boolean (0 or non-0). if one box is whole world, and the other is inverse full (empty) - returns non-intersection
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_box_intersect(bbox3f b1, bbox3f b2);

//! tests whether boxes intersect and returns boolean (0 or non-0). safe for above case
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_box_intersect_safe(bbox3f b1, bbox3f b2);

//! tests whether AABB and OBB intersect and returns boolean (0 or non-0)
VECTORCALL inline bool v_bbox3_test_trasformed_box_intersect(const bbox3f& box0, const bbox3f& box1, const mat44f& tm1);
//! same as previous, but assumes that boxes already culled by caller and will likely have intersection
VECTORCALL inline bool v_bbox3_test_trasformed_box_likely_intersect(const bbox3f& box0, const bbox3f& box1, const mat44f& tm1);

//! tests whether OBB box0 and OBB box1 intersect and returns boolean (0 or non-0). size_factor fattens
//! both boxes. assumes boxes may be far apart: rejects distant pairs by bounding spheres before the test.
//! tm0 must be invertible (well-conditioned); the test is done in box0's frame via inverse(tm0)*tm1.
//! callers that pre-culled and have a relative matrix should use the box0/box1/tm1 _likely form instead.
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_trasformed_box_intersect(bbox3f box0, const mat44f& tm0, bbox3f box1,
                                                                      const mat44f& tm1, vec4f size_factor = v_splats(1.0f));
//! get box = box0 & box1
VECTORCALL VECMATH_FINLINE bbox3f v_bbox3_get_box_intersection(bbox3f box0, bbox3f box1);
//! tests whether box intersecs sphere and returns boolean
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_sph_intersect(bbox3f box, vec4f bsph_center, vec4f bsph_r2_x);
//! extend box in all 3 dimensions by ext
VECTORCALL VECMATH_FINLINE void v_bbox3_extend(bbox3f &b, vec3f ext);
//! extend 2d box component-wise by ext
VECTORCALL VECMATH_FINLINE vec4f v_bbox2_extend(vec4f box2d, vec4f ext);

//! create empty bounding sphere (radius < 0)
VECMATH_FINLINE void v_bsph_init_empty(vec4f &sph);
//! create bounding sphere with specified center and radius
VECMATH_FINLINE void v_bsph_init(vec4f &sph, vec3f center, vec4f radius_x);
//! create bounding sphere with specified center and radius, add transformation matrix
VECMATH_FINLINE void v_bsph_init(vec4f &sph, mat44f_cref tm, vec3f center, vec4f radius_x);
//! create minimal bounding sphere containing bbox (.xyz=bbox_center .w=radius)
VECMATH_FINLINE void v_bsph_init_by_bbox3(vec4f &sph, bbox3f b);

//! cehck that bsph is empty (radius < 0)
VECMATH_FINLINE bool v_bsph_is_empty(vec4f sph);
//! extract bsph radius to all components of result
VECMATH_FINLINE vec4f v_bsph_radius(vec4f sph);
VECMATH_FINLINE vec4f v_bsph_radius_sq(vec4f sph);

//! check that two bounding spheres intersects
VECMATH_FINLINE bool v_test_bsph_bsph_intersection(vec4f a, vec4f b);
//! check that sphere b is inside sphere a
VECMATH_FINLINE bool v_bsph_test_sph_inside(vec4f sph_a, vec4f sph_a_rad_sq_x, vec4f sph_b, vec4f sph_b_rad_x);
//! check that bbox inside sphere
VECMATH_FINLINE bool v_bsph_test_box_inside(vec4f sph, vec4f sph_rad_sq_x, bbox3f b);

//! create minimal sphere containing sphere and point
VECMATH_FINLINE vec4f v_bsph_pt_best_sum(vec4f bsph, vec3f pt);
//! create minimal sphere containing two spheres
VECMATH_FINLINE vec4f v_bsph_bsph_best_sum(vec4f a, vec4f b);
//! create minimal sphere containing sphere and bbox
VECMATH_FINLINE vec4f v_bsph_bbox_best_sum(vec4f bsph, bbox3f bbox);

// Same as above, but all containers should not be empty
VECMATH_FINLINE vec4f v_bsph_pt_best_sum_unsafe(vec4f bsph, vec3f pt);
VECMATH_FINLINE vec4f v_bsph_bsph_best_sum_unsafe(vec4f a, vec4f b);
VECMATH_FINLINE vec4f v_bsph_bbox_best_sum_unsafe(vec4f bsph, bbox3f bbox);

//! extend bsphere to enclose point p
VECMATH_FINLINE void v_bsph_add_pt(vec4f& sph, vec3f p);
//! extend bsphere to enclose bsph b
VECMATH_FINLINE void v_bsph_add_bsph(vec4f& a, vec4f b);
//! extend bsphere to enclose bounding box b
VECMATH_FINLINE void v_bsph_add_bbox(vec4f& sph, bbox3f b);

// Same as above, but all containers should not be empty
VECMATH_FINLINE void v_bsph_add_pt_unsafe(vec4f& sph, vec3f p);
VECMATH_FINLINE void v_bsph_add_bsph_unsafe(vec4f& a, vec4f b);
VECMATH_FINLINE void v_bsph_add_bbox_unsafe(vec4f& sph, bbox3f b);

//! .x = squared MINIMUM distance from point c to bbox (bmin, bmax)
VECTORCALL VECMATH_FINLINE vec4f v_distance_sq_to_bbox_x(vec4f bmin, vec4f bmax, vec4f c);

//! .x = squared MINIMUM 2d (xz) distance from point c to bbox (bmin, bmax)
VECTORCALL VECMATH_FINLINE vec4f v_distance_sq_to_bbox_2d_x(vec4f bmin, vec4f bmax, vec4f c);

//! .x = squared MAXIMUM 3d distance from point c to bbox (bmin, bmax)
VECTORCALL VECMATH_FINLINE vec4f v_max_dist_sq_to_bbox_x(vec4f bmin, vec4f bmax, vec4f c);

//! .x = squared MINIMUM 3d distance from between two bbox
VECTORCALL VECMATH_FINLINE vec4f v_distance_sq_box_to_box_x(vec3f centerA, vec3f extentA, vec3f centerB, vec3f extentB);

//! .x = squared MINIMUM 3d distance from between two bbox
VECTORCALL VECMATH_FINLINE vec4f v_distance_sq_box_to_box_x_scaled(vec3f cA, vec3f extentA, vec3f cB, vec3f extentB, vec3f axis_scale);

//returns point on infinite line which is closes to point
VECTORCALL VECMATH_FINLINE vec3f v_closest_point_on_line(vec3f point, vec3f a, vec3f dir);
//returns point on segment which is closes to point
VECTORCALL VECMATH_FINLINE vec3f v_closest_point_on_segment(vec3f point, vec3f a, vec3f b);
//distance to line in x component
VECTORCALL VECMATH_FINLINE vec4f v_distance_to_line_x(vec3f point, vec3f a, vec3f dir);
//distance to segment in x component
VECTORCALL VECMATH_FINLINE vec4f v_distance_to_seg_x(vec3f point, vec3f a, vec3f b);
//get closest to c bbox point. return c, if c is inside bbox. undefined for empty boxes
VECTORCALL VECMATH_FINLINE vec3f v_closest_bbox_point(vec3f bmin, vec3f bmax, vec3f c);

//returns 1 if segment with start, start + dir*tmax intersects box
VECTORCALL VECMATH_FINLINE bool v_test_ray_box_intersection(vec3f start, vec3f dir, vec3f len_x, bbox3f box);
//same as previous but without support of empty bboxes
VECTORCALL VECMATH_FINLINE bool v_test_ray_box_intersection_unsafe(vec3f start, vec3f dir, vec3f len_x, bbox3f box);
//returns 1 if segment with start, start + dir*tmax intersects box, tmax will contain distance along ray
VECTORCALL VECMATH_FINLINE bool v_ray_box_intersection(vec3f start, vec3f dir, vec3f &t_x, bbox3f box);
//same as previous but without support of empty bboxes
VECTORCALL VECMATH_FINLINE bool v_ray_box_intersection_unsafe(vec3f start, vec3f dir, vec3f &t_x, bbox3f box);
//returns 1 if segment with start, start + end intersects box
VECTORCALL VECMATH_FINLINE bool v_test_segment_box_intersection(vec3f start, vec3f end, bbox3f box);

//returns distance to box intersection
//.x - closest dist (tmin), .y farthest dist (tmax)
// both can be negative
// ray intersects box if tmax >= max(ray.tmin, tmin) && tmin <= ray.tmax
VECTORCALL VECMATH_INLINE  vec4f v_ray_box_intersect_dist(vec3f bmin, vec3f bmax, vec3f ray_origin, vec3f ray_dir, vec3f is_empty_box);
//approximate distance to box intersection (uses v_rcp_est instead of 1/rayDir). A bit faster, v_rcp_est error is < 1e-12
VECTORCALL VECMATH_INLINE  vec4f v_ray_box_intersect_dist_est(vec3f bmin, vec3f bmax, vec3f ray_origin, vec3f ray_dir, vec3f is_empty_box);

// return -1 if no intersection found, or box side index in [0; 5]
// out_at_min and out_at_max in range [0.0; 1.0] set for closest and furthest intersections
// compiler didn't calculate 'out_at_max' if it's unused
VECTORCALL inline int v_segment_box_intersection_side(vec3f start, vec3f end, const bbox3f& box, float& out_at_min, float& out_at_max);

// check ray or segment intersection with sphere
VECTORCALL VECMATH_FINLINE bool v_test_ray_sphere_intersection(vec3f p0, vec3f dir, vec4f len, vec4f sphere_center, vec4f sphere_r2_x);
VECTORCALL VECMATH_FINLINE bool v_test_segment_sphere_intersection(vec3f p0, vec3f p1, vec3f sphere_center, vec4f sphere_r2_x);

// return two intersection distances if ray intersect sphere. both distances can be negative.
VECMATH_FINLINE bool v_ray_sphere_intersection_dist(vec3f start, vec3f dir, vec3f sphere_center, vec4f sphere_r2_x, vec4f &out_p1_t, vec4f &out_p2_t);
// check that ray intersect sphere on distance [0; t] from start and set T of closest intersection (0 if start inside sphere)
VECMATH_FINLINE bool v_ray_sphere_intersection(vec3f start, vec3f dir, vec4f &t_x, vec3f sphere_center, vec4f sphere_r2_x);

//visibility
///construct 6 planes from worldviewproj matrix, with reversed z-buffer camPlanes5 far, camPlanes4 near
VECTORCALL VECMATH_FINLINE void v_construct_camplanes(mat44f_cref clip,
  vec4f &camPlanes0, vec4f &camPlanes1, vec4f &camPlanes2, vec4f &camPlanes3, vec4f &camPlanes4, vec4f &camPlanes5);

//v_frustum_box. assume it has correct frustum planes intersection.
VECTORCALL VECMATH_FINLINE void v_frustum_box_unsafe(bbox3f& box,
                                 vec4f camPlanes0, vec4f camPlanes1, vec4f camPlanes2,
                                 const vec4f &camPlanes3, const vec4f &camPlanes4, const vec4f &camPlanes5);

//0,1,2 returns if sphere is outside (0), inisde(1), intersect(2) frustum, accepts 4 transponed planes and two others in regular
VECTORCALL VECMATH_FINLINE int v_sphere_intersect(vec3f center, vec3f r,
                  vec3f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W,
                  const vec4f& plane4, const vec4f& plane5);

//0,1 returns if sphere is visible in frustum, accepts 4 transponed planes and two others in regular
VECTORCALL VECMATH_FINLINE int v_is_visible_sphere(vec3f center, vec3f r,
                  vec4f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W,
                  const vec4f& plane4, const vec4f& plane5);

///universal visibility function (accepts worldviewproj matrix)
///return zero if not visible. nonzero, otherwise
// center and extent should be multiplied by 2.
// it is fastest implementation
VECTORCALL VECMATH_FINLINE int v_is_visible_extent_fast(vec3f center, vec3f extent, mat44f_cref clip);

// same as above (and will call v_is_visible_extent_fast), but accepts bbox)
VECTORCALL VECMATH_FINLINE int v_is_visible_b_fast(vec3f bmin, vec3f bmax, mat44f_cref clip);
VECTORCALL VECMATH_FINLINE int v_frustum_intersect(vec3f center, vec3f extent, mat44f_cref clip);
VECTORCALL VECMATH_FINLINE void v_frustum_corners_4(vec4f ax, vec4f ay, vec4f az, vec4f aw, vec4f bx, vec4f by, vec4f bz, vec4f bw, vec4f abx, vec4f aby, vec4f abz, vec4f c, vec4f &px, vec4f &py, vec4f &pz);
VECTORCALL VECMATH_FINLINE int v_is_frustum_nout_8(const vec4f cs0[4], vec4f cs0_negw, const vec4f cs1[4], vec4f cs1_negw);
VECTORCALL VECMATH_FINLINE int v_box_frustum_intersect_extent2(vec3f center, vec3f extent, vec4f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W2, const vec4f& plane4W2, const vec4f& plane5W2);
VECTORCALL VECMATH_FINLINE int v_is_visible_b_fast_8planes(vec3f bmin, vec3f bmax, mat44f_cref clip);
VECTORCALL VECMATH_FINLINE int v_is_visible_box_extent2(vec3f center, vec3f extent, vec4f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W2, const vec4f& plane4W2, const vec4f& plane5W2);
VECTORCALL VECMATH_FINLINE int v_is_visible_extent_fast_8planes(vec3f center, vec3f extent, mat44f_cref clip);
VECTORCALL VECMATH_FINLINE void v_is_transform_box_8(vec3f bmin, vec3f bmax, mat44f_cref clip, vec4f cs0[4], vec4f cs1[4], vec4f &cs0_negw, vec4f &cs1_negw);
VECTORCALL VECMATH_FINLINE void v_is_transform_points_4(vec4f* dest, vec4f x, vec4f y, vec4f z, mat44f_cref mat);
VECTORCALL VECMATH_FINLINE void v_is_transform_points_4(vec4f* dest, vec4f x, vec4f y, mat44f_cref mat);
VECTORCALL VECMATH_FINLINE vec3f v_obb_sat_group_separated(vec3f dist, vec3f ra, vec3f rb);
VECTORCALL VECMATH_FINLINE void v_is_screen_project_minmax(const vec4f cs[4], vec4f &minXY, vec4f &maxXY);

///universal visibility function (accepts worldviewproj matrix)
///return zero if not visible in frustum or if unclamped bbox screen size is smaller, than threshold. not zero, otherwise
// also, out screen_box is minX, maxX, minY, maxY - in clipspace coordinates  (-1, -1) .. (1,1)
VECTORCALL VECMATH_INLINE int v_screen_size_b(vec3f bmin, vec3f bmax, vec3f threshold, vec4f &screen_box, mat44f_cref clip);

/// same as previous but return w min, w max
/// return zero if not visible in frustum or if bbox screen size is smaller, than threshold. 1, if all vertex are in front of near plane, -1 (and fullscreen rect) otherwise
/// also, screen_box is minX, maxX, minY, maxY - in clipspace coordinates  (-1, -1) .. (1,1), minmax_w is wWwW (minw, maxw, minw, maxw)
VECTORCALL VECMATH_INLINE int
v_screen_size_b(vec3f bmin, vec3f bmax, vec3f threshold, vec4f &screen_box, vec4f &minmax_w, mat44f_cref clip);

/// as v_screen_size_b (the minmax_w form) for a box the caller has ALREADY frustum-classified as at least
/// partially visible: skips the 6-plane frustum test and the clip-Z transform row entirely (clip z feeds
/// only the frustum planes; the screen rect and minmax_w read x, y, w). Same FP ops for the shared parts,
/// so the outputs are bit-identical to v_screen_size_b whenever the contract holds. For a box fully
/// outside the frustum the outputs are meaningless (but safe to consume) instead of the 0 return.
VECTORCALL VECMATH_INLINE int
v_screen_size_visible_b(vec3f bmin, vec3f bmax, vec3f threshold, vec4f &screen_box, vec4f &minmax_w, mat44f_cref clip);

///universal visibility function (accepts worldviewproj matrix)
///return zero if not visible. nonzero, otherwise
///it is slower than v_is_visible_b_fast, so do not call it if you don't know what are you doing
///for branching: (~v_test_vec_x_eqi_0(v_is_visible(bmin, bmax, clip)))&1 will be 1 if visible, zero otherwise;
///or use v_is_visible_b (it is faster on SSE)
VECTORCALL VECMATH_INLINE vec4f v_is_visible(vec3f bmin, vec3f bmax, mat44f_cref clip);


///for branching: (~v_test_vec_x_eqi_0( v_is_visible(bmin, bmax, clip)))&1 will be 1 if visible, zero otherwise;
VECTORCALL VECMATH_INLINE int v_is_visible_b(vec3f bmin, vec3f bmax, mat44f_cref clip);

//! returns triangle vs sphere intersection
// last parameter is squared radius!
VECTORCALL VECMATH_INLINE int v_test_triangle_sphere_intersection(vec3f A, vec3f B, vec3f C, vec4f sph_c, vec4f sph_r2_x);

//! gets perfect triangle bounding sphere center.
// Warning - can work incorrectly on degenerative triangles (can produce nans)
VECTORCALL VECMATH_INLINE vec3f v_triangle_bounding_sphere_center(vec3f p1, vec3f p2, vec3f p3);

// check is point p inside triangle with vertices t1,t2,t3 in 2D space
VECTORCALL VECMATH_INLINE bool v_is_point_in_triangle_2d(vec4f p, vec4f t1, vec4f t2, vec4f t3);

//
// Quaternion math
//

//! make normalized quaternion from any non-degenerate 3x3/4x3 matrix: removes per-column scale,
//! flips col2 on a mirrored (det < 0) basis and normalizes, so it accepts scaled, sheared and
//! mirrored matrices. For a known pure rotation use v_quat_from_orthonormal_mat* (faster).
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_mat(vec3f col0, vec3f col1, vec3f col2);
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_mat33(mat33f_cref m);
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_mat43(mat44f_cref m);
//! faster: REQUIRES orthonormal (scale-free) columns; still returns a normalized quaternion,
//! but skips scale removal, so it is wrong for scaled input. Use only for known pure rotations.
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_orthonormal_mat(vec3f col0, vec3f col1, vec3f col2);
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_orthonormal_mat33(mat33f_cref m);
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_orthonormal_mat43(mat44f_cref m);

//! make (unnormalized) quaternion to rotate 'ang' radians around normalized 'v';
VECTORCALL inline quat4f v_quat_from_unit_vec_ang(vec3f v, vec4f ang);
//! make (unnormalized) quaternion to rotate acos(ang_cos) radians around normalized 'v';
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_unit_vec_cos(vec3f v, vec4f ang_cos);
//! make (unnormalized) quaternion to rotate 'v0' to 'v1'; both 'v0' and 'v1' must be normalized
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_unit_arc(vec3f v0, vec3f v1);
//! make (unnormalized) quaternion to rotate 'v0' to 'v1', both CAN be not normalized
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_arc(vec3f v0, vec3f v1);
//! make (unnormalized) quaternion from heading, attitude, bank angles in .xyz
VECTORCALL inline quat4f v_quat_from_euler(vec3f angles);
//! make heading, attitude, bank angles from quaternion
VECTORCALL inline vec3f v_euler_from_quat(quat4f quat);

//! linear interpolation between normalized quaternions using parameter tttt
VECTORCALL VECMATH_FINLINE quat4f v_quat_lerp(vec4f tttt, quat4f a, quat4f b);
//! spherical interpolation between normalized quaternions
VECTORCALL VECMATH_FINLINE quat4f v_quat_slerp(vec4f t, quat4f a, quat4f b);
//! fast quasi-spherical linear interpolation between normalized quaternions
VECTORCALL VECMATH_FINLINE quat4f v_quat_qslerp(float t, quat4f a, quat4f b);
//! fast quasi-spherical quadrangle interpolation between normalized quaternions
VECTORCALL VECMATH_FINLINE quat4f v_quat_qsquad(float t,
  const quat4f &q0, const quat4f &q1, const quat4f &q2, const quat4f &q3);

//
// Trigonometry
//

//! angle cos between two not-normalized vectors
VECTORCALL VECMATH_FINLINE vec4f v_angle_cos(vec3f a, vec3f b);
VECTORCALL VECMATH_FINLINE vec4f v_angle_cos_x(vec3f a, vec3f b);

//! angle cos between two not-normalized not-zero vectors
VECTORCALL VECMATH_FINLINE vec4f v_angle_cos_unsafe(vec3f a, vec3f b);
VECTORCALL VECMATH_FINLINE vec4f v_angle_cos_unsafe_x(vec3f a, vec3f b);

//! angle between two not-normalized vectors
VECTORCALL VECMATH_FINLINE vec4f v_angle(vec3f a, vec3f b);
VECTORCALL VECMATH_FINLINE vec4f v_angle_x(vec3f a, vec3f b);

//! angle between two not-normalized not-zero vectors
VECTORCALL VECMATH_FINLINE vec4f v_angle_unsafe(vec3f a, vec3f b);
VECTORCALL VECMATH_FINLINE vec4f v_angle_unsafe_x(vec3f a, vec3f b);

//! degrees <-> radians
VECTORCALL VECMATH_FINLINE vec4f v_deg_to_rad(vec4f deg);
VECTORCALL VECMATH_FINLINE vec4f v_rad_to_deg(vec4f rad);

//! normalize angle to (-PI;PI]
VECTORCALL VECMATH_FINLINE vec4f v_norm_s_angle(vec4f angle);

//! dir<>angle converters
VECTORCALL VECMATH_FINLINE vec4f v_dir_to_angles(vec3f dir);
VECTORCALL VECMATH_FINLINE vec3f v_angles_to_dir(vec4f angles);

//! calculate sine or cosine of same angle
VECTORCALL VECMATH_FINLINE vec4f v_sin_from_cos(vec4f c);
VECTORCALL VECMATH_FINLINE vec4f v_cos_from_sin(vec4f s);
VECTORCALL VECMATH_FINLINE vec4f v_sin_from_cos_x(vec4f c);
VECTORCALL VECMATH_FINLINE vec4f v_cos_from_sin_x(vec4f s);

//! compute sine and cosine for all components: for C={xyzw}  s.C = sin(a.C); c.C = cos(a.C);
VECTORCALL VECMATH_FINLINE void v_sincos(vec4f a, vec4f& s, vec4f& c);

//! compute sine and cosine for .x: s.x = sin(a.x); c.x = cos(a.x);
VECTORCALL VECMATH_FINLINE void v_sincos_x(vec4f a, vec4f& s, vec4f& c);

//! compute sine, cosine, tangent or arc for all components
VECTORCALL VECMATH_FINLINE vec4f v_sin(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_cos(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_tan(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_asin(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_acos(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_atan(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_atan2(vec4f x, vec4f y); // handles +-(0;0) like libc

//! compute sine, cosine, tangent or arc for .x component
VECTORCALL VECMATH_FINLINE vec4f v_sin_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_cos_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_tan_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_asin_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_acos_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_atan_x(vec4f a);
VECTORCALL VECMATH_FINLINE vec4f v_atan2_x(vec4f x, vec4f y);

//! safe asin/acos by angle clamping to [-1;1]
VECTORCALL VECMATH_FINLINE vec4f v_safe_asin(vec4f ang);
VECTORCALL VECMATH_FINLINE vec4f v_safe_acos(vec4f ang);
VECTORCALL VECMATH_FINLINE vec4f v_safe_asin_x(vec4f ang);
VECTORCALL VECMATH_FINLINE vec4f v_safe_acos_x(vec4f ang);

// compute approximate atan |error| is < 0.00045
VECTORCALL VECMATH_FINLINE vec4f v_atan_est(vec4f x);  // any x

// compute approximate atan2 |error| is < 0.0004
// ~40% faster then v_atan2
// NOTE: does not handle any of the following inputs:
// (+0, +0), (+0, -0), (-0, +0), (-0, -0)
VECTORCALL VECMATH_INLINE  vec4f v_atan2_est(vec4f y, vec4f x);

//exp,log, pow
VECTORCALL VECMATH_INLINE  vec4f v_exp2_est(vec4f x);
VECTORCALL VECMATH_INLINE  vec4f v_exp2_est_p2(vec4f x);//polynom of 2
VECTORCALL VECMATH_INLINE  vec4f v_exp2_est_p3(vec4f x);
VECTORCALL VECMATH_INLINE  vec4f v_exp2_est_p4(vec4f x);
VECTORCALL VECMATH_INLINE  vec4f v_exp2_est_p5(vec4f x);//polynom of 5
VECTORCALL VECMATH_INLINE  vec4f v_log2_est(vec4f x);
VECTORCALL VECMATH_INLINE  vec4f v_log2_est_p2(vec4f x);//polynom of 2
VECTORCALL VECMATH_INLINE  vec4f v_log2_est_p3(vec4f x);
VECTORCALL VECMATH_INLINE  vec4f v_log2_est_p4(vec4f x);
VECTORCALL VECMATH_INLINE  vec4f v_log2_est_p5(vec4f x);//polynom of 5

VECTORCALL VECMATH_FINLINE vec4f v_pow_est(vec4f x, vec4f y);////exp_polynom_of_4(log_polynom_of_5(x)*y)

// uv sampling
//
// addr will contain addresses in assumption that data is (1<<size_bits)&2, linearily in memory)
// addr = {LT.x+(LT.y<<size_bits), RT.x+(RT.y<<size_bits), LB.x+(LB.y<<size_bits), RB.x+(RB.y<<size_bits)}
// uv_frac will contain fractional part of texel coord
// center_ofs = V_C_HALF will make typical HW filtering (center in 0.5,0.5 of pixel coordinate)
VECTORCALL VECMATH_INLINE void v_get_bilinear_wrap_addr_pow2(vec4i &addr, vec4f &uv_frac, vec4f uv_wrap, int size_bits, vec4f center_ofs = v_zero());

// this works with non-pow-of-2 textures.
// addr = {LT.x+(LT.y*size), RT.x+(RT.y*size), LB.x+(LB.y*size), RB.x+(RB.y*size)}
VECTORCALL VECMATH_INLINE void v_get_bilinear_wrap_addr(vec4i &addr, vec4f &uv_frac, vec4f uv_wrap, int size, vec4f center_ofs = v_zero());
//
// vector -> scalar transformation (often incurs penalty!)
//

//! extracts fp32 x-component of float[4]
VECTORCALL VECMATH_FINLINE float v_extract_x(vec4f v);
//! extracts fp32 y-component of float[4]
VECTORCALL VECMATH_FINLINE float v_extract_y(vec4f v);
//! extracts fp32 z-component of float[4]
VECTORCALL VECMATH_FINLINE float v_extract_z(vec4f v);
//! extracts fp32 w-component of float[4]
VECTORCALL VECMATH_FINLINE float v_extract_w(vec4f v);

//! extract N element
VECTORCALL VECMATH_FINLINE float v_extract(vec4f v, int idx);

//! extracts i16 x-component of short[8]
VECTORCALL VECMATH_FINLINE short v_extract_xi16(vec4i v);

//! extracts i64 x-component of i64[2]
VECTORCALL VECMATH_FINLINE int64_t v_extract_xi64(vec4i a);
//! extracts i64 y-component of i64[2]
VECTORCALL VECMATH_FINLINE int64_t v_extract_yi64(vec4i a);

//! extracts ith-component of short[8]
VECTORCALL VECMATH_FINLINE short v_extract_i16(vec4i v, int i);

//! extracts i32 x-component of int[4]
VECTORCALL VECMATH_FINLINE int v_extract_xi(vec4i v);
//! extracts i32 y-component of int[4]
VECTORCALL VECMATH_FINLINE int v_extract_yi(vec4i v);
//! extracts i32 z-component of int[4]
VECTORCALL VECMATH_FINLINE int v_extract_zi(vec4i v);
//! extracts i32 w-component of int[4]
VECTORCALL VECMATH_FINLINE int v_extract_wi(vec4i v);

//! insert scalar to vector by index
VECTORCALL VECMATH_FINLINE vec4f v_insert(vec4f v, float x, int idx);
VECTORCALL VECMATH_FINLINE vec4i v_inserti(vec4i v, int x, int idx);

// sign extend
VECTORCALL VECMATH_FINLINE vec4f is_neg_special(vec4f a);

//
// support for slow but sometimes necessary branching
//
//! (int(v.x) == int(a.x)) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eqi(vec3f v, vec3f a);
//! (int(v.x) == 0) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eqi_0(vec3f v);

//! (v.x == a.x) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eq(vec3f v, vec3f a);
//! (v.x  > a.x) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_gt(vec3f v, vec3f a);
//! (v.x >= a.x) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_ge(vec3f v, vec3f a);
//! (v.x  < a.x) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_lt(vec3f v, vec3f a);
//! (v.x <= a.x) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_le(vec3f v, vec3f a);

//! (v.x == 0) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_eq_0(vec3f v);
//! (v.x  > 0) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_gt_0(vec3f v);
//! (v.x >= 0) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_ge_0(vec3f v);
//! (v.x  < 0) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_lt_0(vec3f v);
//! (v.x <= 0) ? 1 : 0
VECTORCALL VECMATH_FINLINE int v_test_vec_x_le_0(vec3f v);

//
// Float16 SUPPORT
//
// Native F16C is supported on the vast majority of modern x86-64 platforms (>96%) including XBox One and PS4.
// Not every processor with AVX supports F16C, but every processor with F16C support also has AVX. AVX2 implies F16C support.
// Default Dagor builds using SSE4 or AVX will fall back to software-emulated Float16 conversions unless F16C is explicitly
// enabled for a specific *.cpp in jamfile. If you want use HW acceleration, manual check for F16C flag support and fallback
// to software emulated code also required.
// On ARM (AArch64), all processors supported by vecmath library provide fast native half-to-float and float-to-half (RTNE)
// conversions. Other rounding modes are partially emulated in software.
// All hw-accelerated v_fc16_* functions handles Inf and NaN values.

//! convert 4 float32 to 4 float16 using different rounding modes, results stored in low halfs of .xyzw
//! 'specials' version handles infs/nans, others doesn't
//! HW-accelerated RTNE version recommended for best performance and compatibility
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_trunc(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_specials(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_rtne(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_up(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_down(vec4f a);

//! same as previous, but 4 results packed in low 64-bit part (.xy) of vec4i
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_trunc_lo(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_specials_lo(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_rtne_lo(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_up_lo(vec4f a);
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_down_lo(vec4f a);

//! convert 4 float16 to 4 float32, source elements should be located in low halfs of .xyzw, NaN and inf NOT handled
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float(vec4i a);
//! convert 4 float16 to 4 float32, source elements should be located in low halfs of .xyzw, NaN and inf handled
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials(vec4i a);
//! convert 4 float16 to 4 float32, source elements should be located in low 64-bits (.xy) of vec4i, NaN and inf NOT handled
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_lo(vec4i a);
//! convert 4 float16 to 4 float32, source elements should be located in low 64-bits (.xy) of vec4i, NaN and inf handled
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials_lo(vec4i a);

// All listed functions will use fastest HW-accelerated version automatically when available.
// Also explicit HW and Software versions present with 'v_fc16' or 'v_sw' prefix:
VECTORCALL VECMATH_FINLINE vec4f v_fc16_half_to_float_lo(vec4i v);
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_rtne_lo(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_down_lo(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_up_lo(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_trunc_lo(vec4f v);

VECTORCALL VECMATH_FINLINE vec4f v_fc16_half_to_float(vec4i v);
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_rtne(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_down(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_up(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_trunc(vec4f v);

VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float_lo(vec4i v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_rtne_lo(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_down_lo(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_up_lo(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_trunc_lo(vec4f v);

VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float(vec4i v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_rtne(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_down(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_up(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_trunc(vec4f v);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half(vec4f f);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_specials(vec4f f);
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_specials_lo(vec4f v);
VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float_specials(vec4i h);
VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float_specials_lo(vec4i a);

//
// vec4d - 4D double-precision vector (see vec4d in dag_vecMathDecl.h for representation)
//
VECTORCALL VECMATH_FINLINE vec4d vd_zero();
VECTORCALL VECMATH_FINLINE vec4d vd_splats(double a);                             //!< broadcast scalar to all lanes
//! set lanes x,y,z,w
VECTORCALL VECMATH_FINLINE vec4d vd_make_vec4d(double x, double y, double z, double w);
//! aligned load/store of 4 doubles. Both halves are moved separately, so 16-byte alignment is
//! enough on every build (not 32) - but only the non-AVX build faults on a violation, since on
//! AVX the pair folds into one unaligned 256-bit move. Use vd_ldu/vd_stu when unsure.
vec4d vd_ld(const double *m);
VECTORCALL VECMATH_FINLINE void vd_st(double *m, vec4d a);
vec4d vd_ldu(const double *m);                                                    //!< unaligned load 4 doubles
VECTORCALL VECMATH_FINLINE void vd_stu(double *m, vec4d a);                       //!< unaligned store 4 doubles
//! load DPoint3 layout (3 doubles) fast by one 4x64 bit load; reads 8 bytes past the 3rd, .w = garbage
NO_ASAN_INLINE vec4d vd_ldu_p3(const double *m);
//! load 3 doubles safe (.w=0), use it only when vd_ldu_p3 can cause memory access crashes
NO_ASAN_INLINE vec4d vd_ldu_p3_safe(const double *m);
//! store DPoint3 layout: writes exactly 3 doubles
VECTORCALL VECMATH_FINLINE void vd_stu_p3(double *p3, vec4d v);
VECTORCALL VECMATH_FINLINE vec4d vd_cvt_from_vec4f(vec4f a);                      //!< widen float4 -> double4
VECTORCALL VECMATH_FINLINE vec4f vd_cvt_to_vec4f(vec4d a);                        //!< narrow double4 -> float4
VECTORCALL VECMATH_FINLINE vec4d vd_cvt_from_vec4i(vec4i a);                      //!< int4 -> double4, exact
//! double4 -> int4, truncating toward zero like v_cvt_vec4i; out-of-range input is platform-specific
VECTORCALL VECMATH_FINLINE vec4i vd_cvt_to_vec4i(vec4d a);

//! read one lane out
VECTORCALL VECMATH_FINLINE double vd_extract_x(vec4d a);
VECTORCALL VECMATH_FINLINE double vd_extract_y(vec4d a);
VECTORCALL VECMATH_FINLINE double vd_extract_z(vec4d a);
VECTORCALL VECMATH_FINLINE double vd_extract_w(vec4d a);
//! replace one lane, other lanes are kept
VECTORCALL VECMATH_FINLINE vec4d vd_insert_x(vec4d a, double x);
VECTORCALL VECMATH_FINLINE vec4d vd_insert_y(vec4d a, double y);
VECTORCALL VECMATH_FINLINE vec4d vd_insert_z(vec4d a, double z);
VECTORCALL VECMATH_FINLINE vec4d vd_insert_w(vec4d a, double w);

VECTORCALL VECMATH_FINLINE vec4d vd_add(vec4d a, vec4d b);
VECTORCALL VECMATH_FINLINE vec4d vd_sub(vec4d a, vec4d b);
VECTORCALL VECMATH_FINLINE vec4d vd_mul(vec4d a, vec4d b);
VECTORCALL VECMATH_FINLINE vec4d vd_div(vec4d a, vec4d b);
VECTORCALL VECMATH_FINLINE vec4d vd_neg(vec4d a);
//! min(a,b): per component a < b ? a : b (b wins on NaN), identical on all platforms
VECTORCALL VECMATH_FINLINE vec4d vd_min(vec4d a, vec4d b);
//! max(a,b): per component a > b ? a : b (b wins on NaN), identical on all platforms
VECTORCALL VECMATH_FINLINE vec4d vd_max(vec4d a, vec4d b);
//! clamp values in [min; max] range component-wise
VECTORCALL VECMATH_FINLINE vec4d vd_clamp(vec4d t, vec4d min_val, vec4d max_val);
VECTORCALL VECMATH_FINLINE vec4d vd_sqrt(vec4d a);
VECTORCALL VECMATH_FINLINE vec4d vd_sqrt_x(vec4d a);                              //!< sqrt of .x only, .yzw undefined
// no lane-wise 3-component forms: use the 4-component op and ignore .w (see dag_vecMath_double.h)

//! Horizontal sums add lanes left to right, the same association scalar x+y+z+w has, so these
//! and vd_dot* reproduce scalar double math bit-for-bit when built with -ffp-contract=off
//! (as the physics libs are). A pairwise tree would be one add shorter but would not match.
VECTORCALL VECMATH_FINLINE vec4d vd_hadd4(vec4d a);                               //!< x+y+z+w, broadcast to all lanes
VECTORCALL VECMATH_FINLINE vec4d vd_hadd4_x(vec4d a);                             //!< x+y+z+w in .x
VECTORCALL VECMATH_FINLINE vec4d vd_hadd3(vec4d a);                               //!< x+y+z (ignores .w), broadcast
VECTORCALL VECMATH_FINLINE vec4d vd_hadd3_x(vec4d a);                             //!< x+y+z in .x

VECTORCALL VECMATH_FINLINE vec4d vd_dot4(vec4d a, vec4d b);                       //!< xyzw dot, broadcast to all lanes
VECTORCALL VECMATH_FINLINE vec4d vd_dot4_x(vec4d a, vec4d b);                     //!< xyzw dot in .x
VECTORCALL VECMATH_FINLINE vec4d vd_dot3(vec4d a, vec4d b);                       //!< xyz dot (ignores .w), broadcast
VECTORCALL VECMATH_FINLINE vec4d vd_dot3_x(vec4d a, vec4d b);                     //!< xyz dot in .x
VECTORCALL VECMATH_FINLINE vec4d vd_cross3(vec4d a, vec4d b);                     //!< xyz cross (ignores .w), .w unspecified

VECTORCALL VECMATH_FINLINE vec4d vd_length4_sq(vec4d a);
VECTORCALL VECMATH_FINLINE vec4d vd_length3_sq(vec4d a);
VECTORCALL VECMATH_FINLINE vec4d vd_length4(vec4d a);
VECTORCALL VECMATH_FINLINE vec4d vd_length4_x(vec4d a);
VECTORCALL VECMATH_FINLINE vec4d vd_length3(vec4d a);
VECTORCALL VECMATH_FINLINE vec4d vd_length3_x(vec4d a);

#include "dag_vecMath_const.h"

#if _TARGET_SIMD_SSE
  #include "dag_vecMath_pc_sse.h"
#elif _TARGET_SIMD_NEON
  #include "dag_vecMath_neon.h"
#else
 !error! unsupported target
#endif

#include "dag_vecMath_common.h"
#include "dag_vecMath_trig.h"
#include "dag_vecMath_double.h"
