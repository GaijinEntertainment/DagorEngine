//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_vecMath.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4800)
//we use direct conversion to bool from int
#endif

// Fast but still UNSAFE function for float[3] point loading by one 16-bytes read. Only for FALSE data-race fixing!
// It's can be used to suppress false-positive TSAN warnings, for example when multiple threads
// simultaneously read and write to DIFFERENT elements of same float[3] array.
// TSAN catches data race on 4 bytes after loaded float[3], but readed .w component really is not used.
// You can safely use that function for data on stack, but loading of last element of float[3] array
// may be unsafe when it's allocated on heap. Sometimes access to memory after array may cause app crash.
// Don't use that function to fix warnings like "heap use after free".
NO_ASAN_INLINE vec3f v_ldu_p3(const float *m)
{
#if defined(DAGOR_TSAN_ENABLED) || defined(DAGOR_ASAN_ENABLED)
  return v_ldu_p3_safe(m);
#else
  return v_ldu(m);
#endif
}
NO_ASAN_INLINE vec4i v_ldui_p3(const int *m)
{
#if defined(DAGOR_TSAN_ENABLED) || defined(DAGOR_ASAN_ENABLED)
  return v_ldui_p3_safe(m);
#else
  return v_ldui(m);
#endif
}

VECTORCALL VECMATH_FINLINE vec4f v_make_vec4f_mask(uint8_t bitmask)
{
  vec4i lookup = v_make_vec4i(1 << 0, 1 << 1, 1 << 2, 1 << 3);
  vec4i isolated = v_andi(v_splatsi(bitmask), lookup);
  return v_cast_vec4f(v_cmp_eqi(isolated, lookup));
}

VECTORCALL VECMATH_FINLINE bool v_check_xy_all_true(vec4f a) { return v_extract_xi64(v_cast_vec4i(a)) == int64_t(-1); }
VECTORCALL VECMATH_FINLINE bool v_check_xy_all_false(vec4f a) { return v_extract_xi64(v_cast_vec4i(a)) == 0; }
VECTORCALL VECMATH_FINLINE bool v_check_xy_any_true(vec4f a) { return v_extract_xi64(v_cast_vec4i(a)) != 0; }

#if _TARGET_SIMD_SSE
VECTORCALL VECMATH_FINLINE bool v_check_xz_all_true(vec4f a) { return (v_truemask(a) & 0b101) == 0b101; }
#else
VECTORCALL VECMATH_FINLINE bool v_check_xz_all_true(vec4f a) { return v_check_xyzw_all_true(v_perm_xxzz(a)); }
#endif

//! number of true (all-ones) lanes among x,y,z of a canonical mask, 0..3; the w lane is ignored
VECTORCALL VECMATH_FINLINE int v_count_true_xyz(vec4f a) { return v_count_true(v_perm_xyzd(a, v_zero())); }

VECTORCALL VECMATH_FINLINE vec4f v_bool_to_mask(bool bool_mask) { return v_cast_vec4f(v_splatsi(bool_mask ? -1 : 0)); }
VECTORCALL VECMATH_FINLINE vec4f v_bool_to_msbit(bool param) { return v_cast_vec4f(v_splatsi((param ? 1 : 0) << 31)); }
VECTORCALL VECMATH_FINLINE vec4f v_sel_b(vec4f a, vec4f b, bool c) { return v_sel(a, b, v_bool_to_mask(c)); }
VECTORCALL VECMATH_FINLINE vec4i v_seli_b(vec4i a, vec4i b, bool c) { return v_seli(a, b, v_cast_vec4i(v_bool_to_mask(c))); }

VECTORCALL VECMATH_FINLINE vec4f v_distance_xyz_x(vec4f a, vec4f b) { return v_length3_x(v_sub(a, b)); }
VECTORCALL VECMATH_FINLINE vec4f v_distance_x0z_x(vec4f a, vec4f b) { return v_length2_x(v_perm_xzxz(v_sub(a, b))); }

VECTORCALL VECMATH_FINLINE vec4f v_insert_x(vec4f a, float x) { return v_perm_ayzw(a, v_set_x(x)); }
VECTORCALL VECMATH_FINLINE vec4f v_insert_y(vec4f a, float y) { return v_perm_xbzw(a, v_splats(y)); }
VECTORCALL VECMATH_FINLINE vec4f v_insert_z(vec4f a, float z) { return v_perm_xycw(a, v_splats(z)); }
VECTORCALL VECMATH_FINLINE vec4f v_insert_w(vec4f a, float w) { return v_perm_xyzd(a, v_splats(w)); }

VECTORCALL VECMATH_FINLINE float v_extract(vec4f v, int idx)
{
  alignas(16) float tmp[4];
  v_st(tmp, v);
  return tmp[idx];
}

alignas(64) static vec4f _vec_insert_mask[4] = {V_CI_MASK1000, V_CI_MASK0100, V_CI_MASK0010, V_CI_MASK0001};
VECTORCALL VECMATH_FINLINE vec4f v_insert(vec4f v, float x, int idx)
{
  return v_sel(v, v_splats(x), _vec_insert_mask[idx]);
}
VECTORCALL VECMATH_FINLINE vec4i v_inserti(vec4i v, int x, int idx)
{
  return v_seli(v, v_splatsi(x), v_cast_vec4i(_vec_insert_mask[idx]));
}

VECTORCALL VECMATH_FINLINE vec4f v_add_x(vec4f a, float x) { return v_perm_ayzw(a, v_add_x(a, v_set_x(x))); }
VECTORCALL VECMATH_FINLINE vec4f v_add_y(vec4f a, float y) { return v_perm_xbzw(a, v_add(a, v_splats(y))); }
VECTORCALL VECMATH_FINLINE vec4f v_add_z(vec4f a, float z) { return v_perm_xycw(a, v_add(a, v_splats(z))); }
VECTORCALL VECMATH_FINLINE vec4f v_add_w(vec4f a, float w) { return v_perm_xyzd(a, v_add(a, v_splats(w))); }

VECTORCALL VECMATH_FINLINE vec4f v_mul_x(vec4f a, float x) { return v_perm_ayzw(a, v_mul_x(a, v_set_x(x))); }
VECTORCALL VECMATH_FINLINE vec4f v_mul_y(vec4f a, float y) { return v_perm_xbzw(a, v_mul(a, v_splats(y))); }
VECTORCALL VECMATH_FINLINE vec4f v_mul_z(vec4f a, float z) { return v_perm_xycw(a, v_mul(a, v_splats(z))); }
VECTORCALL VECMATH_FINLINE vec4f v_mul_w(vec4f a, float w) { return v_perm_xyzd(a, v_mul(a, v_splats(w))); }

VECTORCALL VECMATH_FINLINE vec4f v_cmp_le(vec4f a, vec4f b) { return v_cmp_ge(b, a); }
VECTORCALL VECMATH_FINLINE vec4f v_cmp_lt(vec4f a, vec4f b) { return v_cmp_gt(b, a); }

VECTORCALL VECMATH_FINLINE vec4f v_clamp(vec4f t, vec4f min_val, vec4f max_val)
{
  return v_max(v_min(t, max_val), min_val);
}

VECTORCALL VECMATH_FINLINE vec4i v_clampi(vec4i t, vec4i min_val, vec4i max_val)
{
  return v_maxi(v_mini(t, max_val), min_val);
}

VECTORCALL VECMATH_FINLINE vec4f v_cmp_relative_equal(vec4f a, vec4f b, vec4f max_diff, vec4f max_rel_diff)
{
  vec4f diff = v_abs_diff(a, b);
  vec4f m = v_max(v_abs(a), v_abs(b));
  return v_cmp_le(diff, v_min(max_diff, v_mul(m, max_rel_diff)));
}

VECTORCALL VECMATH_FINLINE bool v_is_relative_equal_vec3f(vec4f a, vec4f b)
{
  return v_check_xyz_all_true(v_cmp_relative_equal(a, b));
}

VECTORCALL VECMATH_FINLINE bool v_is_relative_equal_vec4f(vec4f a, vec4f b)
{
  return v_check_xyzw_all_true(v_cmp_relative_equal(a, b));
}

VECTORCALL VECMATH_FINLINE vec4f v_is_unsafe_positive_divisor(vec4f a)
{
  return v_cmp_lt(a, V_C_VERY_SMALL_VAL);
}

VECTORCALL VECMATH_FINLINE vec4f v_is_unsafe_divisor(vec4f a)
{
  return v_is_unsafe_positive_divisor(v_abs(a));
}

VECTORCALL VECMATH_FINLINE vec4f v_safediv(vec4f a, vec4f b, vec4f def)
{
  vec4f isDiv0 = v_is_unsafe_divisor(b);
  return v_sel(v_div(a, b), def, isDiv0);
}

VECTORCALL VECMATH_FINLINE vec4f v_rcp(vec4f a)
{
  //! Works only with -fno-reciprocal-math (clang) and -mno-recip (gcc)
  return v_div(V_C_ONE, a);
}

VECTORCALL VECMATH_FINLINE vec4f v_rcp_x(vec4f a)
{
  return v_div_x(V_C_ONE, a);
}

VECTORCALL VECMATH_FINLINE vec4f v_rcp_safe(vec4f a, vec4f def)
{
  vec4f isDiv0 = v_is_unsafe_divisor(a);
  return v_sel(v_rcp(a), def, isDiv0);
}

VECTORCALL VECMATH_FINLINE vec4f v_rsqrt(vec4f a) { return v_rcp(v_sqrt(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_x(vec4f a) { return v_rcp_x(v_sqrt_x(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_rsqrt_safe(vec4f a, vec4f def)
{
  vec4f isDiv0 = v_is_unsafe_positive_divisor(a);
  return v_sel(v_rsqrt(a), def, isDiv0);
}

VECTORCALL VECMATH_FINLINE vec4f v_mod(vec4f a, vec4f aDiv)
{
  vec4f c = v_div(a, aDiv);
  vec4f cTrunc = v_cvt_vec4f(v_cvt_vec4i(c));
  vec4f base = v_mul(cTrunc, aDiv);
  vec4f r = v_sub(a, base);
  return r;
}

VECTORCALL VECMATH_FINLINE vec4f v_lerp_vec4f(vec4f tttt, vec4f a, vec4f b)
{
  return v_madd(v_sub(b, a), tttt, a);
}

VECTORCALL VECMATH_FINLINE vec4f v_saturate(vec4f a)
{
  return v_clamp(a, v_zero(), V_C_ONE);
}

VECTORCALL VECMATH_FINLINE vec4f v_hand(vec4f a)
{
  a = v_and(a, v_rot_1(a));
  return v_and(a, v_rot_2(a));
}
VECTORCALL VECMATH_FINLINE vec4f v_hor(vec4f a)
{
  a = v_or(a, v_rot_1(a));
  return v_or(a, v_rot_2(a));
}
VECTORCALL VECMATH_FINLINE vec4f v_hand3(vec3f a)
{
  return v_and(v_splat_x(a), v_and(v_splat_y(a), v_splat_z(a)));
}
VECTORCALL VECMATH_FINLINE vec4f v_hor3(vec3f a)
{
  return v_or(v_splat_x(a), v_or(v_splat_y(a), v_splat_z(a)));
}
VECTORCALL VECMATH_FINLINE vec4f v_hmul(vec4f a)
{
  a = v_mul(a, v_rot_1(a));
  return v_mul(a, v_rot_2(a));
}
VECTORCALL VECMATH_FINLINE vec4f v_hmul3(vec3f a)
{
  return v_mul(v_splat_x(a), v_mul(v_splat_y(a), v_splat_z(a)));
}
VECTORCALL VECMATH_FINLINE vec4f v_sqr(vec4f a)
{
  return v_mul(a, a);
}
VECTORCALL VECMATH_FINLINE vec4f v_sqr_x(vec4f a)
{
  return v_mul_x(a, a);
}
VECTORCALL VECMATH_FINLINE vec4f v_midp(vec4f a, vec4f b)
{
  return v_mul(v_add(a, b), V_C_HALF);
}

VECTORCALL VECMATH_FINLINE vec4f v_perm_xxxx(vec4f a) { return v_splat_x(a); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yyyy(vec4f a) { return v_splat_y(a); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zzzz(vec4f a) { return v_splat_z(a); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wwww(vec4f a) { return v_splat_w(a); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_yzwx(vec4f a) { return v_rot_1(a); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_zwxy(vec4f a) { return v_rot_2(a); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_wxyz(vec4f a) { return v_rot_3(a); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_zcwd(vec4f xyzw, vec4f abcd) { return v_merge_lw(xyzw, abcd); }
VECTORCALL VECMATH_FINLINE vec4f v_perm_xayb(vec4f xyzw, vec4f abcd) { return v_merge_hw(xyzw, abcd); }

VECTORCALL VECMATH_FINLINE vec4f v_sqrt_unprecise(vec4f a)   { return v_mul(a, v_rsqrt_unprecise(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_unprecise_x(vec4f a) { return v_mul_x(a, v_rsqrt_unprecise_x(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_est(vec4f a)         { return v_mul(a, v_rsqrt_est(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_sqrt_est_x(vec4f a)       { return v_mul_x(a, v_rsqrt_est_x(a)); }

VECTORCALL VECMATH_FINLINE vec4f v_length4_sq(vec4f a) { return v_dot4(a, a); }
VECTORCALL VECMATH_FINLINE vec3f v_length3_sq(vec3f a) { return v_dot3(a, a); }
VECTORCALL VECMATH_FINLINE vec4f v_length2_sq(vec4f a) { return v_dot2(a, a); }
VECTORCALL VECMATH_FINLINE vec4f v_length4_sq_x(vec4f a) { return v_dot4_x(a, a); }
VECTORCALL VECMATH_FINLINE vec3f v_length3_sq_x(vec3f a) { return v_dot3_x(a, a); }
VECTORCALL VECMATH_FINLINE vec4f v_length2_sq_x(vec4f a) { return v_dot2_x(a, a); }

VECTORCALL VECMATH_FINLINE vec4f v_length4_x(vec4f a)     { return v_sqrt_x(v_length4_sq_x(a)); }
VECTORCALL VECMATH_FINLINE vec3f v_length3_x(vec3f a)     { return v_sqrt_x(v_length3_sq_x(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_length2_x(vec4f a)     { return v_sqrt_x(v_length2_sq_x(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_length4_est_x(vec4f a) { return v_sqrt_est_x(v_length4_sq_x(a)); }
VECTORCALL VECMATH_FINLINE vec3f v_length3_est_x(vec3f a) { return v_sqrt_est_x(v_length3_sq_x(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_length2_est_x(vec4f a) { return v_sqrt_est_x(v_length2_sq_x(a)); }

// broadcast variants reuse the _x forms: scalar sqrt on lane 0 + splat is never slower
// than the packed sqrt and is faster on Jaguar consoles and little ARM cores
VECTORCALL VECMATH_FINLINE vec4f v_length4(vec4f a) { return v_splat_x(v_length4_x(a)); }
VECTORCALL VECMATH_FINLINE vec3f v_length3(vec3f a) { return v_splat_x(v_length3_x(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_length2(vec4f a) { return v_splat_x(v_length2_x(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_length4_est(vec4f a) { return v_splat_x(v_length4_est_x(a)); }
VECTORCALL VECMATH_FINLINE vec3f v_length3_est(vec3f a) { return v_splat_x(v_length3_est_x(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_length2_est(vec4f a) { return v_splat_x(v_length2_est_x(a)); }

VECTORCALL VECMATH_FINLINE vec3f v_striple3(vec3f a, vec3f b, vec3f c) { return v_dot3(v_cross3(a, b), c); }
VECTORCALL VECMATH_FINLINE vec3f v_vtriple3(vec3f a, vec3f b, vec3f c)
{
  vec3f ac = v_dot3(a, c);
  vec3f ab = v_dot3(a, b);
  return v_nmsub(c, ab, v_mul(b, ac));
}

VECTORCALL VECMATH_FINLINE vec4f v_norm4(vec4f a) { return v_div(a, v_length4(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_norm3(vec4f a) { return v_div(a, v_length3(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_norm2(vec4f a) { return v_div(a, v_length2(a)); }

// _est: a * rsqrt_est(len^2) using the hardware reciprocal-sqrt estimate (+1 NR); no
// division, faster and less precise than v_norm*.
VECTORCALL VECMATH_FINLINE vec4f v_norm4_est(vec4f a) { return v_mul(a, v_splat_x(v_rsqrt_est_x(v_length4_sq_x(a)))); }
VECTORCALL VECMATH_FINLINE vec4f v_norm3_est(vec4f a) { return v_mul(a, v_splat_x(v_rsqrt_est_x(v_length3_sq_x(a)))); }
VECTORCALL VECMATH_FINLINE vec4f v_norm2_est(vec4f a) { return v_mul(a, v_splat_x(v_rsqrt_est_x(v_length2_sq_x(a)))); }

VECTORCALL VECMATH_FINLINE vec4f v_norm4_safe(vec4f a, vec4f def)
{
  vec4f lenSq = v_length4_sq(a);
  vec4f len = v_splat_x(v_sqrt_x(lenSq));
  return v_sel(v_div(a, len), def, v_is_unsafe_positive_divisor(lenSq));
}
VECTORCALL VECMATH_FINLINE vec4f v_norm3_safe(vec3f a, vec3f def)
{
  vec4f lenSq = v_length3_sq(a);
  vec4f len = v_splat_x(v_sqrt_x(lenSq));
  return v_sel(v_div(a, len), def, v_is_unsafe_positive_divisor(lenSq));
}
VECTORCALL VECMATH_FINLINE vec4f v_norm2_safe(vec4f a, vec4f def)
{
  vec4f lenSq = v_length2_sq(a);
  vec4f len = v_splat_x(v_sqrt_x(lenSq));
  return v_sel(v_div(a, len), def, v_is_unsafe_positive_divisor(lenSq));
}

VECTORCALL VECMATH_FINLINE vec3f v_cross3(vec3f a, vec3f b)
{
  // (a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
  vec3f yzxw = v_perm_yzxw(a);
  vec3f bcad = v_perm_yzxw(b);
  return v_perm_yzxy(v_nmsub(yzxw, b, v_mul(a, bcad)));
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_persp_forward(mat44f &dest, float wk, float hk, float zn, float zf)
{
  float q = zf/(zf-zn);
  dest.col0 = v_make_vec4f(wk, 0, 0, 0);
  dest.col1 = v_make_vec4f(0, hk, 0, 0);
  dest.col2 = v_make_vec4f(0, 0, q, 1.f);
  dest.col3 = v_make_vec4f(0, 0, -q*zn, 0);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_persp_reverse(mat44f &dest, float wk, float hk, float zn, float zf)
{
  dest.col0 = v_make_vec4f(wk, 0, 0, 0);
  dest.col1 = v_make_vec4f(0, hk, 0, 0);
  dest.col2 = v_make_vec4f(0, 0, zn/(zn-zf), 1.f);
  dest.col3 = v_make_vec4f(0, 0, (zn*zf)/(zf-zn), 0);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_persp(mat44f &dest, float wk, float hk, float zn, float zf)
{
  return v_mat44_make_persp_reverse(dest, wk, hk, zn, zf);
}

VECTORCALL VECMATH_FINLINE void v_mat33_transpose(mat33f &dest, vec3f col0, vec3f col1, vec3f col2)
{
  vec4f tmp0, tmp1, tmp2, tmp3;
  tmp0 = v_merge_hw(col0, col2);
  tmp1 = v_merge_hw(col1, v_zero());//may be v_zero can be replaced with col2
  tmp2 = v_merge_lw(col0, col2);
  tmp3 = v_merge_lw(col1, v_zero());//may be v_zero can be replaced with col2
  dest.col0 = v_merge_hw(tmp0, tmp1);
  dest.col1 = v_merge_lw(tmp0, tmp1);
  dest.col2 = v_merge_hw(tmp2, tmp3);
}

VECTORCALL VECMATH_FINLINE void v_mat33_from_mat44(mat33f &dest, mat44f_cref m2)
{
  dest.col0 = m2.col0;
  dest.col1 = m2.col1;
  dest.col2 = m2.col2;
}

// core 4x4 transpose; all other transpose variants delegate to it. the 3-output
// variants let dead-code elimination drop the unused 4th row (same shuffle count).
VECTORCALL VECMATH_FINLINE void v_mat44_transpose(vec4f &r0, vec4f &r1, vec4f &r2, vec4f &r3)
{
  vec4f tmp0 = v_merge_hw(r0, r2);
  vec4f tmp1 = v_merge_hw(r1, r3);
  vec4f tmp2 = v_merge_lw(r0, r2);
  vec4f tmp3 = v_merge_lw(r1, r3);
  r0 = v_merge_hw(tmp0, tmp1);
  r1 = v_merge_lw(tmp0, tmp1);
  r2 = v_merge_hw(tmp2, tmp3);
  r3 = v_merge_lw(tmp2, tmp3);
}

VECTORCALL VECMATH_FINLINE void v_mat44_transpose_to_mat33(mat33f &dest, mat44f src)
{
  vec4f r3 = v_zero(); // src.col3 is ignored; zero keeps the transposed .w lanes clean
  v_mat44_transpose(src.col0, src.col1, src.col2, r3);
  v_mat33_from_mat44(dest, src);
}

VECTORCALL VECMATH_FINLINE void v_mat44_transpose(mat44f &dest, mat44f src)
{
  dest = src;
  v_mat44_transpose(dest.col0, dest.col1, dest.col2, dest.col3);
}

VECTORCALL VECMATH_FINLINE void v_mat43_transpose_to_mat44(mat44f &dest, mat43f src)
{
  dest.col0 = src.row0;
  dest.col1 = src.row1;
  dest.col2 = src.row2;
  dest.col3 = V_C_UNIT_0001; // gives bottom row (0,0,0,1) for free -> proper affine 4x4
  v_mat44_transpose(dest.col0, dest.col1, dest.col2, dest.col3);
}

NO_ASAN_INLINE void v_mat44_load_from_mat43_overread_unsafe(mat44f &dest, const mat43f &m43)
{
#if defined(DAGOR_TSAN_ENABLED) || defined(DAGOR_ASAN_ENABLED) || (_TARGET_SIMD_SSE && !_TARGET_PC_WIN)
  // non-Windows SSE is the dedicated server (a dev build shipped to production) - never
  // over-read there; sanitizers take the exact 48-byte transpose too
  v_mat43_transpose_to_mat44(dest, m43);
#else
  // in memory the mat44 columns sit at stride 4 across the mat43f rows - exactly the ld4
  // access pattern, so the load and the transpose fuse into one instruction on NEON. NEON
  // (all targets) and the Windows x86-64 dev majority keep the over-read so those developers
  // hit the same out-of-bounds a NEON build would, instead of it only surfacing to few NEON devs
  v_ld_soa4((const float *)&m43, dest.col0, dest.col1, dest.col2, dest.col3);
#endif
}

VECTORCALL VECMATH_FINLINE void v_mat44_transpose_to_mat43(mat43f &dest, mat44f src)
{
  vec4f r3 = src.col3; // mat43f has no 4th row; the transposed row3 is discarded
  dest.row0 = src.col0;
  dest.row1 = src.col1;
  dest.row2 = src.col2;
  v_mat44_transpose(dest.row0, dest.row1, dest.row2, r3);
}

VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec4(mat44f_cref m, vec4f v)
{
  vec4f tmp0 = v_mul(m.col0, v_splat_x(v));
  vec4f tmp1 = v_mul(m.col1, v_splat_y(v));
  tmp0 = v_madd(m.col2, v_splat_z(v), tmp0);
  tmp1 = v_madd(m.col3, v_splat_w(v), tmp1);
  return v_add(tmp0, tmp1);
}

VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec3v(mat44f_cref m, vec3f v)
{
  vec4f res = v_mul(m.col0, v_splat_x(v));
  res = v_madd(m.col1, v_splat_y(v), res);
  return v_madd(m.col2, v_splat_z(v), res);
}

VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_vec3p(mat44f_cref m, vec3f v)
{
  vec4f tmp0 = v_mul(m.col0, v_splat_x(v));
  vec4f tmp1 = v_madd(m.col1, v_splat_y(v), m.col3);
  return v_add(v_madd(m.col2, v_splat_z(v), tmp0), tmp1);
}

VECTORCALL VECMATH_FINLINE vec3f v_mat33_mul_vec3(mat33f_cref m, vec3f v)
{
  vec4f res = v_mul(m.col0, v_splat_x(v));
  res = v_madd(m.col1, v_splat_y(v), res);
  return v_madd(m.col2, v_splat_z(v), res);
}

VECTORCALL VECMATH_FINLINE vec4f v_remove_not_finite(vec4f a)
{
  return v_andnot(v_is_not_finite(a), a);
}

VECTORCALL VECMATH_FINLINE vec4f v_remove_nan(vec4f a)
{
  return v_andnot(v_is_nan(a), a);
}

VECTORCALL VECMATH_FINLINE vec4f v_is_nan(vec4f a)
{
  volatile vec4f v = a;
#if !defined(_MSC_VER) || defined(__clang__)
  return v_cmp_neq(a, v);
#else
  return v_cmp_neq(a, (vec4f &)v);
#endif
}
// Note: intentionally not const since some compliers (e.g. clang>=19) optimizes it off otherwise
alignas(16) inline volatile int __infMask[4] = {0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000};
VECTORCALL VECMATH_FINLINE vec4f v_is_not_finite(vec4f a)
{
  vec4i ai = v_cast_vec4i(a);
  vec4i im = v_ldi((int *)__infMask);
  return v_cast_vec4f(v_cmp_eqi(v_andi(ai, im), im));
}
VECTORCALL VECMATH_FINLINE vec4f v_is_finite(vec4f a) { return v_not(v_is_not_finite(a)); }
VECTORCALL VECMATH_FINLINE vec4f v_is_neg(vec4f a) { return v_cast_vec4f(v_srai(v_cast_vec4i(a), 31)); }
VECTORCALL VECMATH_FINLINE bool v_test_xyzw_nan(vec4f a) { return v_test_any_bit_set(v_is_nan(a)); }
VECTORCALL VECMATH_FINLINE bool v_test_xyz_nan(vec3f a) { return v_check_xyz_any_true(v_is_nan(a)); }
VECTORCALL VECMATH_FINLINE bool v_test_mat43_nan(mat44f m)
{
  return v_test_xyz_nan(v_add(v_add(m.col0, m.col1), v_add(m.col2, m.col3)));
}
VECTORCALL VECMATH_FINLINE bool v_test_xyzw_finite(vec4f a) { return v_check_xyzw_all_false(v_is_not_finite(a)); }
VECTORCALL VECMATH_FINLINE bool v_test_xyz_finite(vec3f a) { return v_check_xyz_all_false(v_is_not_finite(a)); }
VECTORCALL VECMATH_FINLINE bool v_test_xy_finite(vec4f a) {return v_check_xy_all_false(v_is_not_finite(a)); }
VECTORCALL VECMATH_FINLINE bool v_test_x_finite(vec4f a) { return !v_extract_xi(v_cast_vec4i(v_is_not_finite(a))); }
VECTORCALL VECMATH_FINLINE bool v_test_mat44_nan(mat44f m)
{
  return v_test_xyzw_nan(v_add(v_add(m.col0, m.col1), v_add(m.col2, m.col3)));
}
VECTORCALL VECMATH_FINLINE bool v_test_xyz_abs_lt(vec3f a, vec3f limit) { return v_check_xyz_all_true(v_cmp_abs_gt(limit, a)); }

VECTORCALL VECMATH_FINLINE void v_mat33_transpose(mat33f &dest, mat33f_cref src)
{
  v_mat33_transpose(dest, src.col0, src.col1, src.col2);
}

VECTORCALL VECMATH_FINLINE void v_mat33_make_from_look(mat33f &dest, vec4f look_dir)
{
  vec4f vx = v_cross3(V_C_UNIT_0100, look_dir);
  vec4f def = v_norm3(v_cross3(v_btsel(V_C_UNIT_0010, look_dir, v_msbit()), look_dir));
  dest.col0 = v_norm3_safe(vx, def);
  dest.col1 = v_norm3(v_cross3(look_dir, dest.col0));
  dest.col2 = look_dir;
}

VECTORCALL VECMATH_INLINE  void v_view_matrix_from_tangentZ( vec3f &left, vec3f &up, vec4f vdir )
{
  up = v_sel(V_C_UNIT_0010, V_C_UNIT_1000, v_cmp_abs_gt(v_splat_z(vdir), v_splats(0.999f)));
  left = v_norm3(v_cross3(up, vdir));
  up = v_cross3(vdir, left);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make33_from_look(mat44f &dest, vec4f look_dir)
{
  mat33f m;
  v_mat33_make_from_look(m, look_dir);
  dest.set33(m);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_look_at(mat44f &dest, vec4f eye, vec4f at, vec4f up)
{
  dest.col2 = v_norm3(v_sub(at, eye));
  dest.col0 = v_norm3(v_cross3(up, dest.col2));
  dest.col1 = v_cross3(dest.col2, dest.col0);
  dest.col3 = eye;
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_ortho(mat44f &dest, float _w, float _h, float _zn, float _zf)
{
  v_mat44_ident(dest);
  vec4f zn = v_splats(_zn);
  vec4f w = v_splats(_w);
  vec4f h = v_splats(_h);
  vec4f r_zr = v_rcp(v_sub(v_splats(_zf), zn));

  dest.col2 = v_mul(dest.col2, r_zr);
  dest.col0 = v_mul(v_add(dest.col0, dest.col0), v_rcp(w));
  dest.col1 = v_mul(v_add(dest.col1, dest.col1), v_rcp(h));
  dest.col3 = v_madd(zn, dest.col2, dest.col3);
}

VECTORCALL VECMATH_FINLINE void v_mat44_add(mat44f &dest, mat44f_cref m1, mat44f_cref m2)
{
  dest.col0 = v_add(m1.col0, m2.col0);
  dest.col1 = v_add(m1.col1, m2.col1);
  dest.col2 = v_add(m1.col2, m2.col2);
  dest.col3 = v_add(m1.col3, m2.col3);
}
VECTORCALL VECMATH_FINLINE void v_mat33_add(mat33f &dest, mat33f_cref m1, mat33f_cref m2)
{
  dest.col0 = v_add(m1.col0, m2.col0);
  dest.col1 = v_add(m1.col1, m2.col1);
  dest.col2 = v_add(m1.col2, m2.col2);
}
VECTORCALL VECMATH_FINLINE void v_mat43_sub(mat43f &dest, mat43f_cref m1, mat43f_cref m2)
{
  dest.row0 = v_sub(m1.row0, m2.row0);
  dest.row1 = v_sub(m1.row1, m2.row1);
  dest.row2 = v_sub(m1.row2, m2.row2);
}
VECTORCALL VECMATH_FINLINE void v_mat44_sub(mat44f &dest, mat44f_cref m1, mat44f_cref m2)
{
  dest.col0 = v_sub(m1.col0, m2.col0);
  dest.col1 = v_sub(m1.col1, m2.col1);
  dest.col2 = v_sub(m1.col2, m2.col2);
  dest.col3 = v_sub(m1.col3, m2.col3);
}
VECTORCALL VECMATH_FINLINE void v_mat33_sub(mat33f &dest, mat33f_cref m1, mat33f_cref m2)
{
  dest.col0 = v_sub(m1.col0, m2.col0);
  dest.col1 = v_sub(m1.col1, m2.col1);
  dest.col2 = v_sub(m1.col2, m2.col2);
}

VECTORCALL VECMATH_FINLINE void v_mat33_neg(mat44f &dest, mat44f_cref m)
{
  vec4f zero = v_zero();
  dest.col0 = v_sub(zero, m.col0);
  dest.col1 = v_sub(zero, m.col1);
  dest.col2 = v_sub(zero, m.col2);
}
VECTORCALL VECMATH_FINLINE void v_mat33_neg(mat33f &dest, mat33f_cref m)
{
  vec4f zero = v_zero();
  dest.col0 = v_sub(zero, m.col0);
  dest.col1 = v_sub(zero, m.col1);
  dest.col2 = v_sub(zero, m.col2);
}
VECTORCALL VECMATH_FINLINE void v_mat44_mul_elem(mat44f &dest, mat44f_cref m1, mat44f_cref m2)
{
  dest.col0 = v_mul(m1.col0, m2.col0);
  dest.col1 = v_mul(m1.col1, m2.col1);
  dest.col2 = v_mul(m1.col2, m2.col2);
  dest.col3 = v_mul(m1.col3, m2.col3);
}
VECTORCALL VECMATH_FINLINE void v_mat33_mul_elem(mat33f &dest, mat33f_cref m1, mat33f_cref m2)
{
  dest.col0 = v_mul(m1.col0, m2.col0);
  dest.col1 = v_mul(m1.col1, m2.col1);
  dest.col2 = v_mul(m1.col2, m2.col2);
}

VECTORCALL VECMATH_FINLINE vec3f v_mat43_mul_vec3v(mat43f_cref m, vec3f v)
{
  mat44f m44;
  v_mat43_transpose_to_mat44(m44, m);
  return v_mat44_mul_vec3v(m44, v);
}

VECTORCALL VECMATH_FINLINE vec3f v_mat43_mul_vec3p(mat43f_cref m, vec3f v)
{
  mat44f m44;
  v_mat43_transpose_to_mat44(m44, m);
  return v_mat44_mul_vec3p(m44, v);
}

VECTORCALL VECMATH_FINLINE void v_mat44_mul(mat44f &dest, mat44f_cref m1, mat44f_cref m2)
{
  vec4f col0 = v_mat44_mul_vec4(m1, m2.col0);
  vec4f col1 = v_mat44_mul_vec4(m1, m2.col1);
  vec4f col2 = v_mat44_mul_vec4(m1, m2.col2);
  vec4f col3 = v_mat44_mul_vec4(m1, m2.col3);
  dest.col0 = col0;
  dest.col1 = col1;
  dest.col2 = col2;
  dest.col3 = col3;
}
VECTORCALL VECMATH_FINLINE void v_mat44_mul43(mat44f &dest, mat44f_cref m1, mat44f_cref m2)
{
  vec4f col0 = v_mat44_mul_vec3v(m1, m2.col0);
  vec4f col1 = v_mat44_mul_vec3v(m1, m2.col1);
  vec4f col2 = v_mat44_mul_vec3v(m1, m2.col2);
  vec4f col3 = v_mat44_mul_vec3p(m1, m2.col3);
  dest.col0 = col0;
  dest.col1 = col1;
  dest.col2 = col2;
  dest.col3 = col3;
}

VECTORCALL VECMATH_FINLINE void v_mat44_mul43(mat44f &dest, mat44f_cref m1, mat43f_cref m2)
{
  vec4f xxxx, yyyy, zzzz;
  #define SPLAT_MAT_N_COL0(N, COL) \
    xxxx = v_splat_##N(m2.row0); yyyy = v_splat_##N(m2.row1); zzzz = v_splat_##N(m2.row2);\
    vec4f COL = v_add(v_add(v_mul(xxxx, m1.col0), v_mul(yyyy, m1.col1)), v_mul(zzzz, m1.col2));

  #define SPLAT_MAT_N_COL1(N, COL) \
    xxxx = v_splat_##N(m2.row0); yyyy = v_splat_##N(m2.row1); zzzz = v_splat_##N(m2.row2);\
    vec4f COL = v_add(v_add(v_mul(xxxx, m1.col0), v_mul(yyyy, m1.col1)), v_add(v_mul(zzzz, m1.col2), m1.col3));

  SPLAT_MAT_N_COL0(x, col0)
  SPLAT_MAT_N_COL0(y, col1)
  SPLAT_MAT_N_COL0(z, col2)
  SPLAT_MAT_N_COL1(w, col3)
  dest.col0 = col0;
  dest.col1 = col1;
  dest.col2 = col2;
  dest.col3 = col3;
  #undef SPLAT_MAT_N_COL0
  #undef SPLAT_MAT_N_COL1
}

VECTORCALL VECMATH_FINLINE void v_mat44_mul33r(mat44f &dest, mat44f_cref m1, mat44f_cref m2)
{
  vec4f col0 = v_mat44_mul_vec3v(m1, m2.col0);
  vec4f col1 = v_mat44_mul_vec3v(m1, m2.col1);
  vec4f col2 = v_mat44_mul_vec3v(m1, m2.col2);
  dest.col0 = col0;
  dest.col1 = col1;
  dest.col2 = col2;
  dest.col3 = m1.col3;
}
VECTORCALL VECMATH_FINLINE void v_mat44_mul33(mat44f &dest, mat44f_cref m1, mat33f_cref m2)
{
  vec4f col0 = v_mat44_mul_vec3v(m1, m2.col0);
  vec4f col1 = v_mat44_mul_vec3v(m1, m2.col1);
  vec4f col2 = v_mat44_mul_vec3v(m1, m2.col2);
  dest.col0 = col0;
  dest.col1 = col1;
  dest.col2 = col2;
  dest.col3 = m1.col3;
}
VECTORCALL VECMATH_FINLINE void v_mat33_mul(mat33f &dest, mat33f_cref m1, mat33f_cref m2)
{
  vec4f col0 = v_mat33_mul_vec3(m1, m2.col0);
  vec4f col1 = v_mat33_mul_vec3(m1, m2.col1);
  vec4f col2 = v_mat33_mul_vec3(m1, m2.col2);
  dest.col0 = col0;
  dest.col1 = col1;
  dest.col2 = col2;
}
VECTORCALL VECMATH_FINLINE void v_mat33_mul33r(mat33f &dest, mat33f_cref m1, mat44f_cref m2)
{
  vec4f col0 = v_mat33_mul_vec3(m1, m2.col0);
  vec4f col1 = v_mat33_mul_vec3(m1, m2.col1);
  vec4f col2 = v_mat33_mul_vec3(m1, m2.col2);
  dest.col0 = col0;
  dest.col1 = col1;
  dest.col2 = col2;
}
VECTORCALL VECMATH_FINLINE void v_mat33_orthonormalize(mat33f &dest, mat33f_cref m)
{
  vec4f c2 = v_norm3(v_cross3(m.col0, m.col1));
  vec4f c1 = v_norm3(v_cross3(c2, m.col0));
  vec4f c0 = v_norm3(v_cross3(c1, c2));
  dest.col2 = c2;
  dest.col1 = c1;
  dest.col0 = c0;
}
VECTORCALL VECMATH_FINLINE void v_mat44_orthonormalize33(mat44f &dest, mat44f_cref m)
{
  vec4f c2 = v_norm3(v_cross3(m.col0, m.col1));
  vec4f c1 = v_norm3(v_cross3(c2, m.col0));
  vec4f c0 = v_norm3(v_cross3(c1, c2));
  dest.col2 = c2;
  dest.col1 = c1;
  dest.col0 = c0;
}
VECTORCALL VECMATH_FINLINE void v_mat33_remove_scale(mat33f &dest, mat33f_cref m)
{
  vec4f sq0 = v_mul(m.col0, m.col0);
  vec4f sq1 = v_mul(m.col1, m.col1);
  vec4f sq2 = v_mul(m.col2, m.col2);
  vec4f dummy = v_zero();
  v_mat44_transpose(sq0, sq1, sq2, dummy);
  vec4f invLen = v_rsqrt(v_add(v_add(sq0, sq1), sq2));
  dest.col0 = v_mul(m.col0, v_splat_x(invLen));
  dest.col1 = v_mul(m.col1, v_splat_y(invLen));
  dest.col2 = v_mul(m.col2, v_splat_z(invLen));
}

VECTORCALL VECMATH_FINLINE void v_mat33_orthonormal_inverse(mat33f &dest, mat33f_cref m)
{
  v_mat33_transpose(dest, m);
}

VECTORCALL VECMATH_FINLINE void v_mat44_orthonormal_inverse43(mat44f &dest, mat44f_cref m)
{
  mat33f m3;
  v_mat44_transpose_to_mat33(m3, m);
  dest.set33(m3);
  dest.col3 = v_neg(v_mat44_mul_vec3v(dest, m.col3));
}

VECTORCALL VECMATH_FINLINE void v_mat44_orthonormal_inverse43_to44(mat44f &dest, mat44f_cref m)
{
  mat33f m3;
  v_mat44_transpose_to_mat33(m3, m);
  dest.set33(m3);
  dest.col3 = v_perm_xyzd(v_neg(v_mat44_mul_vec3v(dest, m.col3)), V_C_UNIT_0001);
}

VECTORCALL VECMATH_FINLINE void v_mat43_make_from_43cu(mat43f &tmV, const float *const __restrict m43)
{
  v_ldu_soa3(m43, tmV.row0, tmV.row1, tmV.row2);
}

VECTORCALL VECMATH_FINLINE void v_mat_43ca_from_mat43(float * __restrict m43, mat43f_cref tm)
{
  v_st_soa3(m43, tm.row0, tm.row1, tm.row2);
}

VECTORCALL VECMATH_FINLINE void v_mat_43cu_from_mat43(float * __restrict m43, mat43f_cref tm)
{
  v_stu_soa3(m43, tm.row0, tm.row1, tm.row2);
}

VECTORCALL VECMATH_FINLINE void v_mat44_make_affine(mat44f &m)
{
  m.col0 = v_perm_xyzd(m.col0, v_zero());
  m.col1 = v_perm_xyzd(m.col1, v_zero());
  m.col2 = v_perm_xyzd(m.col2, v_zero());
  m.col3 = v_perm_xyzd(m.col3, V_C_UNIT_0001);
}

// same, but all .w lanes are undefined
VECTORCALL VECMATH_FINLINE void v_mat44_orthonormal_inverse_from_43cu_unsafe(mat44f &dest, const float *const __restrict m43)
{
  // the deinterleaved rows are the columns of the transposed rotation, with the source
  // translation left in the .w lanes - exactly what -T33(m)*pos needs
  v_ldu_soa3(m43, dest.col0, dest.col1, dest.col2);
  vec4f t = v_mul(dest.col0, v_splat_w(dest.col0));
  t = v_madd(dest.col1, v_splat_w(dest.col1), t);
  t = v_madd(dest.col2, v_splat_w(dest.col2), t);
  dest.col3 = v_neg(t);
}

VECTORCALL VECMATH_FINLINE void v_mat44_orthonormal_inverse_from_43cu(mat44f &dest, const float *const __restrict m43)
{
  v_mat44_orthonormal_inverse_from_43cu_unsafe(dest, m43);
  v_mat44_make_affine(dest);
}

VECTORCALL VECMATH_FINLINE vec4f v_mat33_det(mat33f_cref m)
{
  return v_dot3(m.col2, v_cross3(m.col0, m.col1));
}

VECTORCALL VECMATH_FINLINE void v_mat33_inverse(mat33f &dest, mat33f_cref m)
{
  // adjugate = transpose of the cofactor cross-products, scaled by 1/det
  vec4f tmp2 = v_cross3(m.col0, m.col1);
  vec4f tmp0 = v_cross3(m.col1, m.col2);
  vec4f tmp1 = v_cross3(m.col2, m.col0);
  vec4f invdet = v_rcp_safe(v_dot3(tmp2, m.col2));
  mat33f adj;
  v_mat33_transpose(adj, tmp0, tmp1, tmp2);
  dest.col0 = v_mul(adj.col0, invdet);
  dest.col1 = v_mul(adj.col1, invdet);
  dest.col2 = v_mul(adj.col2, invdet);
}

VECTORCALL VECMATH_FINLINE void v_mat44_inverse(mat44f &dest, mat44f_cref m)
{
  vec4f tmp0 = v_perm_xzac(m.col0, m.col1);
  vec4f tmp1 = v_perm_xzac(m.col2, m.col3);
  vec4f tmp2 = v_perm_ywbd(m.col0, m.col1);
  vec4f tmp3 = v_perm_ywbd(m.col2, m.col3);
  vec4f row0 = v_perm_xzac(tmp0, tmp1);
  vec4f row1 = v_perm_xzac(tmp3, tmp2);
  vec4f row2 = v_perm_ywbd(tmp0, tmp1);
  vec4f row3 = v_perm_ywbd(tmp3, tmp2);

  vec4f minor0, minor1, minor2, minor3, t;

  t = v_perm_yxwz(v_mul(row2, row3));
  minor0 = v_mul(row1, t);
  minor1 = v_mul(row0, t);
  t = v_rot_2(t);
  minor0 = v_msub(row1, t, minor0);
  minor1 = v_msub(row0, t, minor1);
  minor1 = v_rot_2(minor1);

  t = v_perm_yxwz(v_mul(row1, row2));
  minor0 = v_madd(row3, t, minor0);
  minor3 = v_mul(row0, t);
  t = v_rot_2(t);
  minor0 = v_nmsub(row3, t, minor0);
  minor3 = v_msub(row0, t, minor3);
  minor3 = v_rot_2(minor3);

  t = v_perm_yxwz(v_mul(v_rot_2(row1), row3));
  row2 = v_rot_2(row2);
  minor0 = v_madd(row2, t, minor0);
  minor2 = v_mul(row0, t);
  t = v_rot_2(t);
  minor0 = v_nmsub(row2, t, minor0);
  minor2 = v_msub(row0, t, minor2);
  minor2 = v_rot_2(minor2);

  t = v_perm_yxwz(v_mul(row0, row1));
  minor2 = v_madd(row3, t, minor2);
  minor3 = v_msub(row2, t, minor3);
  t = v_rot_2(t);
  minor2 = v_msub(row3, t, minor2);
  minor3 = v_nmsub(row2, t, minor3);

  t = v_perm_yxwz(v_mul(row0, row3));
  minor1 = v_nmsub(row2, t, minor1);
  minor2 = v_madd(row1, t, minor2);
  t = v_rot_2(t);
  minor1 = v_madd(row2, t, minor1);
  minor2 = v_nmsub(row1, t, minor2);

  t = v_perm_yxwz(v_mul(row0, row2));
  minor1 = v_madd(row3, t, minor1);
  minor3 = v_nmsub(row1, t, minor3);
  t = v_rot_2(t);
  minor1 = v_nmsub(row3, t, minor1);
  minor3 = v_madd(row1, t, minor3);

  vec4f det = v_rcp_safe(v_dot4(row0, minor0));
  dest.col0 = v_mul(det, minor0);
  dest.col1 = v_mul(det, minor1);
  dest.col2 = v_mul(det, minor2);
  dest.col3 = v_mul(det, minor3);
}

VECTORCALL VECMATH_FINLINE vec4f v_mat44_det(mat44f_cref m)
{
  vec4f tmp0 = v_perm_xzac(m.col0, m.col1);
  vec4f tmp1 = v_perm_xzac(m.col2, m.col3);
  vec4f tmp2 = v_perm_ywbd(m.col0, m.col1);
  vec4f tmp3 = v_perm_ywbd(m.col2, m.col3);
  vec4f t0 = v_perm_xzac(tmp0, tmp1);
  vec4f t1 = v_perm_xzac(tmp3, tmp2);
  vec4f t2 = v_perm_ywbd(tmp0, tmp1);
  vec4f t3 = v_perm_ywbd(tmp3, tmp2);

  vec4f t23 = v_perm_yxwz(v_mul(t2, t3));
  vec4f cof0 = v_mul(t1, t23);
  cof0 = v_neg(v_nmsub(t1, v_rot_2(t23), cof0));
  vec4f t12 = v_perm_yxwz(v_mul(t1, t2));
  cof0 = v_madd(t3, t12, cof0);
  cof0 = v_nmsub(t3, v_rot_2(t12), cof0);
  vec4f t1r = v_rot_2(t1);
  vec4f t2r = v_rot_2(t2);
  vec4f t1r3 = v_perm_yxwz(v_mul(t1r, t3));
  cof0 = v_madd(t2r, t1r3, cof0);
  cof0 = v_nmsub(t2r, v_rot_2(t1r3), cof0);
  return v_dot4(t0, cof0);
}
VECTORCALL VECMATH_FINLINE void v_mat44_inverse43(mat44f &dest, mat44f_cref m)
{
  mat33f m3;
  m3.col0 = m.col0;
  m3.col1 = m.col1;
  m3.col2 = m.col2;
  v_mat33_inverse(m3, m3);
  dest.set33(m3);
  dest.col3 = v_neg(v_mat44_mul_vec3v(dest, m.col3));
}

VECTORCALL VECMATH_FINLINE void v_mat44_inverse43_to44(mat44f &dest, mat44f_cref m)
{
  mat33f m3;
  m3.col0 = m.col0;
  m3.col1 = m.col1;
  m3.col2 = m.col2;
  v_mat33_inverse(m3, m3);
  dest.set33(m3);
  vec4f zero = v_zero();
  dest.col0 = v_perm_xyzd(dest.col0, zero);
  dest.col1 = v_perm_xyzd(dest.col1, zero);
  dest.col2 = v_perm_xyzd(dest.col2, zero);
  dest.col3 = v_perm_xyzd(v_neg(v_mat44_mul_vec3v(dest, m.col3)), V_C_UNIT_0001);
}

VECTORCALL VECMATH_FINLINE vec4f v_mat44_det43(mat44f_cref m)
{
  return v_dot3(m.col2, v_cross3(m.col0, m.col1));
}
VECTORCALL VECMATH_FINLINE vec3f v_mat44_scale43_sq(mat44f_cref tm)
{
  // transpose puts col0..col2 x/y/z into rows, so one wide mul + 2 madd yields all
  // three squared column lengths at once (FMA-friendly, unlike 3 horizontal dots)
  vec4f r0 = tm.col0, r1 = tm.col1, r2 = tm.col2, r3 = v_zero();
  v_mat44_transpose(r0, r1, r2, r3);
  return v_madd(r2, r2, v_madd(r1, r1, v_mul(r0, r0)));
}
VECTORCALL VECMATH_FINLINE vec4f v_mat44_max_scale43_sq(mat44f_cref tm)
{
  // full 4-lane hmax is valid: scale43_sq has w = 0 and squared scales are >= 0
  return v_hmax(v_mat44_scale43_sq(tm));
}
VECTORCALL VECMATH_FINLINE void v_mat44_apply_scale33(mat44f &tm, vec3f scale)
{
  tm.col0 = v_mul(tm.col0, v_splat_x(scale));
  tm.col1 = v_mul(tm.col1, v_splat_y(scale));
  tm.col2 = v_mul(tm.col2, v_splat_z(scale));
}
VECTORCALL VECMATH_FINLINE void v_mat44_remove_scale33(mat44f &dest, mat44f_cref m)
{
  vec4f invLen = v_rsqrt(v_mat44_scale43_sq(m)); // w lane is rsqrt(0), never read
  dest = m;
  v_mat44_apply_scale33(dest, invLen);
}
VECTORCALL VECMATH_FINLINE vec4f v_mat44_max_scale43_x(mat44f_cref tm)
{
  return v_sqrt_x(v_mat44_max_scale43_sq(tm));
}
VECTORCALL VECMATH_FINLINE vec4f v_mat44_max_scale43(mat44f_cref tm)
{
  return v_splat_x(v_mat44_max_scale43_x(tm)); // scalar sqrt + splat, see v_length*
}

VECTORCALL VECMATH_FINLINE vec4f v_mat44_mul_bsph(mat44f_cref m, vec4f bsph)
{
  return v_perm_xyzd(v_mat44_mul_vec3p(m, bsph), v_mul(bsph, v_mat44_max_scale43(m)));
}

VECTORCALL VECMATH_FINLINE void v_mat44_mul_bsph(mat44f_cref m, vec4f &bsph_pos, vec4f &bsph_rad_x)
{
  bsph_pos = v_mat44_mul_vec3p(m, bsph_pos);
  bsph_rad_x = v_mul_x(bsph_rad_x, v_mat44_max_scale43_x(m));
}

VECTORCALL VECMATH_FINLINE mat44f v_mat44_lerp(vec4f tttt, mat44f_cref a, mat44f_cref b)
{
  quat4f aq = v_quat_from_mat43(a);
  quat4f bq = v_quat_from_mat43(b);
  quat4f rq = v_quat_slerp(tttt, aq, bq);
  vec3f rpos = v_lerp_vec4f(tttt, a.col3, b.col3);
  mat44f tm;
  v_mat44_from_quat(tm, rq, rpos);
  return tm;
}

VECTORCALL VECMATH_FINLINE void v_bbox3_init_empty(bbox3f &b)
{
  b.bmin = V_C_MAX_VAL;
  b.bmax = v_sub(v_zero(), b.bmin);
}
VECTORCALL VECMATH_FINLINE void v_bbox3_init_ident(bbox3f &b)
{
  b.bmin = v_neg(V_C_HALF);
  b.bmax = V_C_HALF;
}
VECTORCALL VECMATH_FINLINE void v_bbox3_init(bbox3f &b, vec3f p) { b.bmin = b.bmax = p; }
VECTORCALL VECMATH_FINLINE void v_bbox3_rotate_init(bbox3f &b, vec3f col0, vec3f col1, vec3f col2, bbox3f_cref b2)
{
  // AABB of the rotated box: transformed center +/- |M| * half-extent (Ericson RTCD 4.2.6).
  vec4f center = v_bbox3_center(b2);
  vec4f extent = v_mul(v_bbox3_size(b2), V_C_HALF);
  vec4f c = v_mul(col0, v_splat_x(center));
  c = v_madd(col1, v_splat_y(center), c);
  c = v_madd(col2, v_splat_z(center), c);
  vec4f e = v_mul(v_abs(col0), v_splat_x(extent));
  e = v_madd(v_abs(col1), v_splat_y(extent), e);
  e = v_madd(v_abs(col2), v_splat_z(extent), e);
  b.bmin = v_sub(c, e);
  b.bmax = v_add(c, e);
}

VECTORCALL VECMATH_FINLINE void v_bbox3_init(bbox3f &b, mat33f_cref m, bbox3f b2)
{
  v_bbox3_rotate_init(b, m.col0, m.col1, m.col2, b2);
}

VECTORCALL VECMATH_FINLINE void v_bbox3_init(bbox3f &b, mat44f_cref m, bbox3f b2)
{
  v_bbox3_rotate_init(b, m.col0, m.col1, m.col2, b2);
  b.bmin = v_add(b.bmin, m.col3);
  b.bmax = v_add(b.bmax, m.col3);
}

VECTORCALL VECMATH_FINLINE void v_bbox3_init_by_bsph(bbox3f &b, vec3f bsph_center, vec3f bsph_radius)
{
  b.bmin = v_sub(bsph_center, bsph_radius);
  b.bmax = v_add(bsph_center, bsph_radius);
}

VECTORCALL VECMATH_FINLINE void v_bbox3_init_by_ray(bbox3f &b, vec3f from, vec3f dir, vec4f len)
{
  v_bbox3_init_by_segment(b, from, v_madd(dir, len, from));
}

VECTORCALL VECMATH_FINLINE void v_bbox3_init_by_segment(bbox3f &b, vec4f from, vec4f to)
{
  b.bmin = v_min(from, to);
  b.bmax = v_max(from, to);
}

//return all mask if empty
VECTORCALL VECMATH_FINLINE vec3f v_bbox3_center(bbox3f b)
{
  return v_mul(v_add(b.bmin, b.bmax), V_C_HALF);
}
VECTORCALL VECMATH_FINLINE vec4f v_bbox3_outer_rad(bbox3f b)
{
  return v_mul_x(V_C_HALF, v_length3_x(v_bbox3_size(b)));
}
VECTORCALL VECMATH_FINLINE vec4f v_bbox3_inner_diameter(bbox3f b)
{
  return v_hmin3(v_bbox3_size(b));
}
VECTORCALL VECMATH_FINLINE vec4f v_bbox3_inner_rad(bbox3f b)
{
  return v_mul_x(V_C_HALF, v_bbox3_inner_diameter(b));
}
VECTORCALL VECMATH_FINLINE vec4f v_bbox3_outer_rad_from_zero(bbox3f b)
{
  return v_length3_x(v_max(v_neg(b.bmin), b.bmax));
}
VECTORCALL VECMATH_FINLINE vec3f v_bbox3_point(bbox3f b, int idx)
{
  return v_sel(b.bmin, b.bmax, v_make_vec4f_mask(uint8_t(idx)));
}

VECTORCALL VECMATH_FINLINE void v_bbox3_add_pt(bbox3f &b, vec3f p)
{
  b.bmin = v_min(b.bmin, p);
  b.bmax = v_max(b.bmax, p);
}
VECTORCALL VECMATH_FINLINE void v_bbox3_add_box(bbox3f &b, bbox3f b2)
{
  b.bmin = v_min(b.bmin, b2.bmin);
  b.bmax = v_max(b.bmax, b2.bmax);
}

VECTORCALL VECMATH_FINLINE void v_bbox3_add_transformed_box(bbox3f &b, mat44f_cref m, bbox3f b2)
{
  bbox3f temp;
  v_bbox3_init(temp, m, b2);
  v_bbox3_add_box(b, temp);
}

VECTORCALL VECMATH_FINLINE void v_bbox3_add_ray(bbox3f &b, vec3f from, vec3f dir, vec4f len)
{
  v_bbox3_add_pt(b, from);
  v_bbox3_add_pt(b, v_madd(dir, len, from));
}

VECTORCALL VECMATH_FINLINE bbox3f v_bbox3_sum(bbox3f b1, bbox3f b2)
{
  bbox3f b;
  b.bmin = v_min(b1.bmin, b2.bmin);
  b.bmax = v_max(b1.bmax, b2.bmax);
  return b;
}

VECTORCALL VECMATH_FINLINE bbox3f v_bbox3_sel(bbox3f b1, bbox3f b2, vec4f c)
{
  bbox3f b;
  b.bmin = v_sel(b1.bmin, b2.bmin, c);
  b.bmax = v_sel(b1.bmax, b2.bmax, c);
  return b;
}

VECTORCALL VECMATH_FINLINE vec3f v_bbox3_size(bbox3f b) { return v_sub(b.bmax, b.bmin); }

VECTORCALL VECMATH_FINLINE vec3f v_bbox3_max_size(bbox3f b)
{
  return v_hmax3(v_bbox3_size(b));
}
VECTORCALL VECMATH_FINLINE bbox3f v_bbox3_scale(bbox3f b, vec4f size_factor)
{
  vec3f center = v_bbox3_center(b);
  return bbox3f
  {
    v_lerp_vec4f(size_factor, center, b.bmin),
    v_lerp_vec4f(size_factor, center, b.bmax)
  };
}
VECTORCALL VECMATH_FINLINE bool v_bbox3_is_empty(bbox3f bbox)
{
  vec4f invalid = v_cmp_gt(bbox.bmin, bbox.bmax);
  return v_check_xyz_any_true(invalid);
}
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_pt_inside(bbox3f b, vec3f p)
{
  vec3f inside = v_and(v_cmp_ge(p, b.bmin), v_cmp_ge(b.bmax, p));
  return v_check_xyz_all_true(inside);
}
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_box_inside(bbox3f b1, bbox3f b2)
{
  vec3f insideMin = v_and(v_cmp_ge(b2.bmin, b1.bmin), v_cmp_ge(b1.bmax, b2.bmin));
  vec3f insideMax = v_and(v_cmp_ge(b2.bmax, b1.bmin), v_cmp_ge(b1.bmax, b2.bmax));
  return v_check_xyz_all_true(v_and(insideMin, insideMax));
}

VECTORCALL VECMATH_FINLINE bool v_bbox3_test_box_intersect(bbox3f b1, bbox3f b2)
{
  vec3f m = v_and(v_cmp_ge(b2.bmax, b1.bmin), v_cmp_ge(b1.bmax, b2.bmin));
  return v_check_xyz_all_true(m);
}
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_box_intersect_safe(bbox3f b1, bbox3f b2)
{
  vec3f m = v_and(v_and(v_cmp_ge(b1.bmax, b1.bmin), v_cmp_ge(b2.bmax, b2.bmin)),
    v_and(v_cmp_ge(b2.bmax, b1.bmin), v_cmp_ge(b1.bmax, b2.bmin)));
  return v_check_xyz_all_true(m);
}
VECTORCALL VECMATH_FINLINE bool v_bbox3_test_pt_inside_xz(bbox3f b, vec3f p)
{
  vec3f m = v_and(v_cmp_ge(p, b.bmin), v_cmp_ge(b.bmax, p));
  return !v_test_vec_x_eqi_0(v_and(m, v_splat_z(m)));
}

// One SAT axis separates iff |projection of center offset| > sum of the two boxes' extents on it.
// All three terms scale linearly with |axis|, so axes need not be normalized: a zero-length axis
// (parallel edges) yields 0 > 0 and does not separate. The (1 + eps) slack biases the predicate toward
// reporting intersection, so a just-touching pair is not spuriously separated by float rounding; the
// bias is conservative (may keep a barely-separated pair) rather than an exactness guarantee. Each vec3f
// packs one group of 3 sibling axes, so a lane holds one axis' verdict.
VECTORCALL VECMATH_FINLINE vec3f v_obb_sat_group_separated(vec3f dist, vec3f ra, vec3f rb)
{
  return v_cmp_gt(dist, v_mul(v_add(ra, rb), v_splats(1.0000001f)));
}

// AABB-vs-OBB overlap via the separating axis theorem (Ericson, RTCD 4.4.1), generalized to an arbitrary
// tm1: B's axes are the columns of tm1 (scale/shear included) with half-extents kept in local space, so
// no orthonormality is assumed. Returns true when box0 (axis-aligned in this frame) overlaps box1
// transformed by tm1 into this frame. A complete overlap test in one call, with no pre-reject: this is
// the _likely variant, for callers that already culled non-overlapping pairs (so almost all reaching here
// overlap and no axis separates). The 15 axes are evaluated branchlessly in 5 SIMD groups of 3 and OR-ed
// into a single verdict. It is a float predicate with a conservative tolerance (see v_obb_sat_group_
// separated), not a bit-exact oracle; validated against a double-precision SAT over randomized and
// adversarial near-parallel/large-magnitude sweeps without observed false negatives.
VECTORCALL inline bool v_bbox3_test_trasformed_box_likely_intersect(const bbox3f& box0, const bbox3f& box1, const mat44f& tm1)
{
  vec3f hafA = v_mul(v_bbox3_size(box0), V_C_HALF);
  vec3f hafB = v_mul(v_bbox3_size(box1), V_C_HALF);
  vec3f t = v_sub(v_mat44_mul_vec3p(tm1, v_bbox3_center(box1)), v_bbox3_center(box0));
  vec3f aHafBx = v_splat_x(hafB), aHafBy = v_splat_y(hafB), aHafBz = v_splat_z(hafB);
  vec3f aHafAx = v_splat_x(hafA), aHafAy = v_splat_y(hafA), aHafAz = v_splat_z(hafA);

  vec3f bx = tm1.col0, by = tm1.col1, bz = tm1.col2;
  vec3f absBx = v_abs(bx), absBy = v_abs(by), absBz = v_abs(bz);

  // A's face normals e_x,e_y,e_z: projections are componentwise, so all 3 axes fit one vec3f.
  // rowX/Y/Z are B's axes seen per component (rows of [bx by bz]); |L.b_k| on axis e_i reduces to |b_k[i]|.
  mat33f rows;
  v_mat33_transpose(rows, bx, by, bz);
  vec3f rowX = rows.col0, rowY = rows.col1, rowZ = rows.col2;
  vec3f absRowX = v_abs(rowX), absRowY = v_abs(rowY), absRowZ = v_abs(rowZ);
  vec3f rbA = v_madd(absBx, aHafBx, v_madd(absBy, aHafBy, v_mul(absBz, aHafBz)));
  vec3f sep = v_obb_sat_group_separated(v_abs(t), hafA, rbA);

  // B's face normals = cross products of its edge pairs (handles a sheared box as a parallelepiped).
  // Each is orthogonal to two of B's edges, so B's extent on it collapses to hafB[k]*|det(B)|.
  vec3f c0 = v_cross3(by, bz), c1 = v_cross3(bz, bx), c2 = v_cross3(bx, by);
  vec3f absDet = v_abs(v_dot3(bx, c0));
  mat33f ct;
  v_mat33_transpose(ct, c0, c1, c2);
  vec3f distB = v_abs(v_mat33_mul_vec3(ct, t));
  mat33f absCt = { v_abs(ct.col0), v_abs(ct.col1), v_abs(ct.col2) };
  vec3f raB = v_mat33_mul_vec3(absCt, hafA);
  sep = v_or(sep, v_obb_sat_group_separated(distB, raB, v_mul(hafB, absDet)));

  // 9 edge-edge axes e_i x b_j, grouped by i (3 per vec3f, over j). L = e_i x b_j has a zero component,
  // so dist/ra use the two nonzero rows and rb sums hafB[k]*|L.b_k| over the 3 columns.
  vec3f tx = v_splat_x(t), ty = v_splat_y(t), tz = v_splat_z(t);
  // i = x: L = (0, -b_j.z, b_j.y)
  vec3f dist0 = v_abs_diff(v_mul(rowY, tz), v_mul(rowZ, ty));
  vec3f ra0 = v_madd(absRowZ, aHafAy, v_mul(absRowY, aHafAz));
  vec3f rb0 = v_madd(v_abs_diff(v_mul(rowY, v_splat_x(rowZ)), v_mul(rowZ, v_splat_x(rowY))), aHafBx,
              v_madd(v_abs_diff(v_mul(rowY, v_splat_y(rowZ)), v_mul(rowZ, v_splat_y(rowY))), aHafBy,
                 v_mul(v_abs_diff(v_mul(rowY, v_splat_z(rowZ)), v_mul(rowZ, v_splat_z(rowY))), aHafBz)));
  sep = v_or(sep, v_obb_sat_group_separated(dist0, ra0, rb0));
  // i = y: L = (b_j.z, 0, -b_j.x)
  vec3f dist1 = v_abs_diff(v_mul(rowZ, tx), v_mul(rowX, tz));
  vec3f ra1 = v_madd(absRowZ, aHafAx, v_mul(absRowX, aHafAz));
  vec3f rb1 = v_madd(v_abs_diff(v_mul(rowZ, v_splat_x(rowX)), v_mul(rowX, v_splat_x(rowZ))), aHafBx,
              v_madd(v_abs_diff(v_mul(rowZ, v_splat_y(rowX)), v_mul(rowX, v_splat_y(rowZ))), aHafBy,
                 v_mul(v_abs_diff(v_mul(rowZ, v_splat_z(rowX)), v_mul(rowX, v_splat_z(rowZ))), aHafBz)));
  sep = v_or(sep, v_obb_sat_group_separated(dist1, ra1, rb1));
  // i = z: L = (-b_j.y, b_j.x, 0)
  vec3f dist2 = v_abs_diff(v_mul(rowX, ty), v_mul(rowY, tx));
  vec3f ra2 = v_madd(absRowY, aHafAx, v_mul(absRowX, aHafAy));
  vec3f rb2 = v_madd(v_abs_diff(v_mul(rowX, v_splat_x(rowY)), v_mul(rowY, v_splat_x(rowX))), aHafBx,
              v_madd(v_abs_diff(v_mul(rowX, v_splat_y(rowY)), v_mul(rowY, v_splat_y(rowX))), aHafBy,
                 v_mul(v_abs_diff(v_mul(rowX, v_splat_z(rowY)), v_mul(rowY, v_splat_z(rowX))), aHafBz)));
  sep = v_or(sep, v_obb_sat_group_separated(dist2, ra2, rb2));

  return v_check_xyz_all_false(sep); // intersecting iff no axis separated
}

// same as the _likely variant but for callers that have NOT pre-culled the pair: rejects distant pairs
// with a cheap transformed-AABB test before the SAT. Prefer _likely when the caller already culled.
VECTORCALL inline bool v_bbox3_test_trasformed_box_intersect(const bbox3f& box0, const bbox3f& box1, const mat44f& tm1)
{
  bbox3f box1AABB;
  v_bbox3_init(box1AABB, tm1, box1);
  if (!v_bbox3_test_box_intersect(box0, box1AABB))
    return false;
  return v_bbox3_test_trasformed_box_likely_intersect(box0, box1, tm1);
}

VECTORCALL VECMATH_FINLINE bool v_bbox3_test_trasformed_box_intersect(bbox3f box0, const mat44f& tm0, bbox3f box1, const mat44f& tm1,
                                                                      vec4f size_factor)
{
  // scale up front (around each box center) so the reject sphere and the SAT operate on the same
  // boxes; deriving the radius from the pre-scale box would be wrong for off-center boxes when
  // size_factor < 1. A negative size_factor flips min/max and is rejected by the validity check below.
  box0 = v_bbox3_scale(box0, size_factor);
  box1 = v_bbox3_scale(box1, size_factor);

  // validate: any dimension negative (also catches a negative size_factor)
  vec3f width0 = v_bbox3_size(box0);
  vec3f width1 = v_bbox3_size(box1);
  if (v_check_xyz_any_true(v_cmp_lt(v_min(width0, width1), v_zero())))
    return false;

  // boxes may be far apart, so reject by bounding spheres first: skips the matrix inverse + SAT
  // (~half the total cost) for distant pairs. sphere centers are the two column3 positions; the radius
  // bounds every corner via sum(boxMax_i * |col_i|), which stays conservative under shear -- |tm*corner|
  // can exceed |tm*boxMax| when tm's columns are not orthogonal (the all-positive corner cancels).
  vec4f box0max = v_max(v_abs(box0.bmin), v_abs(box0.bmax));
  vec4f box1max = v_max(v_abs(box1.bmin), v_abs(box1.bmax));
  float r0 = v_extract_x(box0max) * v_extract_x(v_length3_x(tm0.col0)) +
             v_extract_y(box0max) * v_extract_x(v_length3_x(tm0.col1)) +
             v_extract_z(box0max) * v_extract_x(v_length3_x(tm0.col2));
  float r1 = v_extract_x(box1max) * v_extract_x(v_length3_x(tm1.col0)) +
             v_extract_y(box1max) * v_extract_x(v_length3_x(tm1.col1)) +
             v_extract_z(box1max) * v_extract_x(v_length3_x(tm1.col2));
  float dist = v_extract_x(v_distance_xyz_x(tm1.col3, tm0.col3));
  if (dist > r0 + r1)
    return false;

  // bring box1 into box0's frame and run the exact SAT (boxes already scaled)
  mat44f b1_to_b0;
  v_mat44_inverse43(b1_to_b0, tm0);
  v_mat44_mul43(b1_to_b0, b1_to_b0, tm1);
  return v_bbox3_test_trasformed_box_likely_intersect(box0, box1, b1_to_b0);
}

VECTORCALL VECMATH_FINLINE bbox3f v_bbox3_get_box_intersection(bbox3f box0, bbox3f box1)
{
  return bbox3f { v_max(box0.bmin, box1.bmin), v_min(box0.bmax, box1.bmax) };
}

VECTORCALL VECMATH_FINLINE bool v_bbox3_test_sph_intersect(bbox3f box, vec4f bsph_center, vec4f bsph_r2_x)
{
  vec4f distSq = v_length3_sq_x(v_add(v_max(v_sub(box.bmin, bsph_center), v_zero()),
                                      v_max(v_sub(bsph_center, box.bmax), v_zero()))); // Dist from sph center to bounding box squared
  return v_test_vec_x_le_0(v_sub_x(distSq, bsph_r2_x));
}

VECTORCALL VECMATH_FINLINE void v_bbox3_extend(bbox3f &b, vec3f ext)
{
  b.bmin = v_sub(b.bmin, ext);
  b.bmax = v_add(b.bmax, ext);
}

VECTORCALL VECMATH_FINLINE vec4f v_bbox2_extend(vec4f box2d, vec4f ext)
{
  return v_perm_xycd(v_sub(box2d, ext), v_add(box2d, ext));
}

VECMATH_FINLINE void v_bsph_init_empty(vec4f &sph)
{
  sph = v_make_vec4f(0, 0, 0, -1);
}

VECMATH_FINLINE void v_bsph_init(vec4f &sph, vec3f center, vec4f radius_x)
{
  sph = v_perm_xyzd(center, v_splat_x(radius_x));
}

VECMATH_FINLINE void v_bsph_init(vec4f &sph, mat44f_cref tm, vec3f center, vec4f radius_x)
{
  vec4f scale = v_mat44_max_scale43_x(tm);
  vec3f c = v_mat44_mul_vec3p(tm, center);
  vec4f r = v_mul_x(radius_x, scale);
  sph = v_perm_xyzd(c, v_splat_x(r));
}

VECMATH_FINLINE void v_bsph_init_by_bbox3(vec4f &sph, bbox3f b)
{
  sph = v_mul(V_C_HALF, v_perm_xyzd(v_add(b.bmin, b.bmax), v_length3(v_bbox3_size(b))));
}

VECMATH_FINLINE bool v_bsph_is_empty(vec4f sph)
{
  return v_extract_w(sph) < 0;
}

VECMATH_FINLINE vec4f v_bsph_radius(vec4f sph)
{
  return v_splat_w(sph);
}

VECMATH_FINLINE vec4f v_bsph_radius_sq(vec4f sph)
{
  return v_sqr(v_bsph_radius(sph));
}

VECMATH_FINLINE bool v_test_bsph_bsph_intersection(vec4f a, vec4f b)
{
  vec4f distSq = v_length3_sq_x(v_sub(a, b));
  vec4f radSum = v_splat_w(v_add(a, b));
  return v_test_vec_x_le(distSq, v_sqr(radSum));
}

VECMATH_FINLINE bool v_bsph_test_sph_inside(vec4f sph_a, vec4f sph_a_rad_sq_x, vec4f sph_b, vec4f sph_b_rad_x)
{
  vec4f dist = v_length3_x(v_sub(sph_a, sph_b));
  return v_test_vec_x_lt(v_sqr_x(v_add_x(dist, sph_b_rad_x)), sph_a_rad_sq_x);
}

VECMATH_FINLINE bool v_bsph_test_box_inside(vec4f sph, vec4f sph_rad_sq_x, bbox3f b)
{
  vec4f d1 = v_abs_diff(sph, b.bmin);
  vec4f d2 = v_abs_diff(sph, b.bmax);
  vec4f r = v_length3_sq_x(v_max(d1, d2));
  return v_test_vec_x_lt(r, sph_rad_sq_x);
}

VECMATH_FINLINE vec4f v_bsph_pt_best_sum(vec4f bsph, vec3f pt)
{
  if (v_bsph_is_empty(bsph))
    return v_perm_xyzd(pt, v_zero());
  return v_bsph_pt_best_sum_unsafe(bsph, pt);
}

VECMATH_FINLINE vec4f v_bsph_bsph_best_sum(vec4f a, vec4f b)
{
  vec3f cd = v_sub(b, a);
  vec4f rd = v_length3(cd);
  vec4f ar = v_splat_w(a);
  vec4f br = v_splat_w(b);
  vec4f isEmptyA = v_cmp_lt(v_splat_w(a), v_zero());
  vec4f isEmptyB = v_cmp_lt(v_splat_w(b), v_zero());
  vec4f insideA = v_cmp_ge(ar, v_add(rd, br));
  vec4f insideB = v_cmp_ge(br, v_add(rd, ar));
  vec4f rad = v_div(v_add(rd, v_add(ar, br)), V_C_TWO);
  vec4f center = v_madd(cd, v_div(v_sub(rad, ar), rd), a);
  vec4f sph = v_perm_xyzd(center, rad);
  vec4f forceA = v_or(insideA, isEmptyB);
  vec4f forceB = v_or(insideB, isEmptyA);
  return v_sel(v_sel(sph, b, forceB), a, forceA);
}

VECMATH_FINLINE vec4f v_bsph_bbox_best_sum(vec4f bsph, bbox3f bbox)
{
  if (v_bsph_is_empty(bsph))
  {
    vec4f sphB;
    v_bsph_init_by_bbox3(sphB, bbox);
    return sphB;
  }
  if (v_bbox3_is_empty(bbox))
    return bsph;
  return v_bsph_bbox_best_sum_unsafe(bsph, bbox);
}

VECMATH_FINLINE vec4f v_bsph_pt_best_sum_unsafe(vec4f bsph, vec3f pt)
{
  vec3f cd = v_sub(pt, bsph);
  vec4f rd = v_length3(cd);
  vec4f r = v_splat_w(bsph);
  vec4f inside = v_cmp_ge(r, rd);
  vec4f rad = v_div(v_add(rd, r), V_C_TWO);
  vec4f center = v_madd(cd, v_div(v_sub(rad, r), rd), bsph);
  vec4f sph = v_perm_xyzd(center, rad);
  return v_sel(sph, bsph, inside);
}

VECMATH_FINLINE vec4f v_bsph_bsph_best_sum_unsafe(vec4f a, vec4f b)
{
  vec3f cd = v_sub(b, a);
  vec4f rd = v_length3(cd);
  vec4f ar = v_splat_w(a);
  vec4f br = v_splat_w(b);
  vec4f insideA = v_cmp_ge(ar, v_add(rd, br));
  vec4f insideB = v_cmp_ge(br, v_add(rd, ar));
  vec4f rad = v_div(v_add(rd, v_add(ar, br)), V_C_TWO);
  vec4f center = v_madd(cd, v_div(v_sub(rad, ar), rd), a);
  vec4f sph = v_perm_xyzd(center, rad);
  return v_sel(v_sel(sph, b, insideB), a, insideA);
}

VECMATH_FINLINE vec4f v_bsph_bbox_best_sum_unsafe(vec4f bsph, bbox3f bbox)
{
  vec3f mind = v_abs_diff(bbox.bmin, bsph);
  vec3f maxd = v_abs_diff(bbox.bmax, bsph);
  vec3f p = v_sel(bbox.bmax, bbox.bmin, v_cmp_ge(mind, maxd));
  return v_bsph_pt_best_sum_unsafe(bsph, p);
}

VECMATH_FINLINE void v_bsph_add_pt(vec4f& sph, vec3f p)
{
  vec4f isEmpty = v_cmp_lt(v_splat_w(sph), v_zero());
  v_bsph_add_pt_unsafe(sph, p);
  sph = v_sel(sph, v_perm_xyzd(p, v_zero()), isEmpty);
}

VECMATH_FINLINE void v_bsph_add_bsph(vec4f& a, vec4f b)
{
  vec4f initialA = a;
  vec4f isEmptyA = v_cmp_lt(v_splat_w(a), v_zero());
  vec4f isEmptyB = v_cmp_lt(v_splat_w(b), v_zero());
  v_bsph_add_bsph_unsafe(a, b);
  a = v_sel(v_sel(a, initialA, isEmptyB), b, isEmptyA);
}

VECMATH_FINLINE void v_bsph_add_bbox(vec4f& sph, bbox3f b)
{
  vec4f sphA = sph;
  vec4f sphB;
  v_bsph_init_by_bbox3(sphB, b);
  vec4f isEmptyA = v_cmp_lt(v_splat_w(sph), v_zero());
  vec4f isEmptyB = v_cmp_gt(b.bmin, b.bmax);;
  v_bsph_add_bbox_unsafe(sph, b);
  sph = v_sel(v_sel(sph, sphA, isEmptyB), sphB, isEmptyA);
}

VECMATH_FINLINE void v_bsph_add_pt_unsafe(vec4f& sph, vec3f p)
{
  vec4f r = v_length3(v_sub(sph, p));
  sph = v_perm_xyzd(sph, v_max(sph, r));
}

VECMATH_FINLINE void v_bsph_add_bsph_unsafe(vec4f& a, vec4f b)
{
  vec4f r = v_add(v_length3(v_sub(a, b)), v_splat_w(b));
  a = v_perm_xyzd(a, v_max(a, r));
}

VECMATH_FINLINE void v_bsph_add_bbox_unsafe(vec4f& sph, bbox3f b)
{
  vec4f d1 = v_abs_diff(sph, b.bmin);
  vec4f d2 = v_abs_diff(sph, b.bmax);
  vec4f r = v_length3(v_max(d1, d2));
  sph = v_perm_xyzd(sph, v_max(sph, r));
}

VECTORCALL VECMATH_FINLINE quat4f v_quat_lerp(vec4f tttt, quat4f a, quat4f b)
{
  return v_lerp_vec4f(tttt, a, b);
}

VECTORCALL VECMATH_FINLINE quat4f v_quat_conjugate(quat4f q) { return v_perm_xyzd(v_neg(q), q); }

VECTORCALL VECMATH_FINLINE vec3f v_quat_mul_vec3(quat4f q, vec3f v)
{
  vec3f t = v_cross3(q, v);
  t = v_add(t,t);//*2
  return v_add(v, v_madd(v_splat_w(q), t, v_cross3(q, t)));//v + q.w * t + v_cross3(q.xyz, t);
}

// fast path: columns MUST be orthonormal (scale-free). Result is a normalized rotation.
// For scaled/unnormalized input use v_quat_from_mat*, which removes scale first.
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_orthonormal_mat(vec3f col0, vec3f col1, vec3f col2)
{
  /* compute quaternion for each case */
  vec4f _yz_ = v_perm_xycd(col1, col2);
  vec4f xyz = v_perm_ayzw(_yz_, col0);
  vec4f yzx = v_perm_xyab(v_perm_yzwx(_yz_), col0);
  vec4f zxy = v_perm_zxyw(xyz);

  vec4f diagSum = v_add(v_add(xyz, yzx), zxy);
  vec4f diagDiff = v_sub(v_sub(xyz, yzx), zxy);
  vec4f radicand = v_add(v_perm_xyzd(diagDiff, v_splat_x(diagSum)), V_C_ONE);
  vec4f invSqrt = v_rsqrt(v_sel(radicand, V_C_ONE, v_cmp_ge(v_zero(), radicand)));

  vec4f zy_xz_yx = v_perm_zayx(v_perm_xycw(col0, col1), col2);
  vec4f yz_zx_xy = v_perm_bzxx(v_perm_ayzw(col0, col1), col2);
  vec4f sum = v_add(zy_xz_yx, yz_zx_xy);
  vec4f diff = v_sub(zy_xz_yx, yz_zx_xy);

  vec4f scale = v_mul(invSqrt, V_C_HALF);
  vec4f res0 = v_mul(v_perm_ayzw(v_perm_xzya(sum, diff), radicand), v_splat_x(scale));
  vec4f res1 = v_mul(v_perm_xbzw(v_perm_zxxb(sum, diff), radicand), v_splat_y(scale));
  vec4f res2 = v_mul(v_perm_xycw(v_perm_yxxc(sum, diff), radicand), v_splat_z(scale));
  vec4f res3 = v_mul(v_perm_xyzd(diff, radicand), v_splat_w(scale));

  /* determine case and select answer */
  vec4f xx = v_splat_x(col0);
  vec4f yy = v_splat_y(col1);
  vec4f zz = v_splat_z(col2);

  vec4f q = v_sel(res0, res1, v_cmp_gt(yy, xx));
  q = v_sel(q, res2, v_and(v_cmp_gt(zz, xx), v_cmp_gt(zz, yy)));
  // no final normalize: 0.5*rsqrt(radicand) already yields a unit quaternion for orthonormal
  // input (unit to 4e-7, rotation error 1.6e-6)
  return v_sel(q, res3, v_cmp_ge(v_splat_x(diagSum), v_zero()));
}

VECTORCALL VECMATH_FINLINE quat4f v_quat_from_orthonormal_mat33(mat33f_cref m)
{
  return v_quat_from_orthonormal_mat(m.col0, m.col1, m.col2);
}
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_orthonormal_mat43(mat44f_cref m)
{
  return v_quat_from_orthonormal_mat(m.col0, m.col1, m.col2);
}

// robust path: accepts any non-degenerate matrix. Removes per-column scale (one packed rsqrt via
// v_mat33_remove_scale), flips col2 on a mirrored basis (det < 0, which a quat cannot represent -
// same convention v_mat33_decompose uses, folding negative det into scale.z), and normalizes the
// result, since scale removal does not fix sheared (non-orthogonal) columns.
// Prefer v_quat_from_orthonormal_mat* for known pure rotations - it skips all three extra steps.
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_mat33(mat33f_cref m)
{
  mat33f n;
  v_mat33_remove_scale(n, m);
  vec4f mirrored = v_cmp_lt(v_mat33_det(m), v_zero());
  n.col2 = v_sel(n.col2, v_neg(n.col2), mirrored);
  return v_norm4(v_quat_from_orthonormal_mat(n.col0, n.col1, n.col2));
}
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_mat(vec3f col0, vec3f col1, vec3f col2)
{
  mat33f m;
  m.col0 = col0;
  m.col1 = col1;
  m.col2 = col2;
  return v_quat_from_mat33(m);
}
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_mat43(mat44f_cref m)
{
  return v_quat_from_mat(m.col0, m.col1, m.col2);
}

// 180 deg rotation taking a unit v0 to -v0 (opposite-vectors fallback of the arc quaternions);
// result has w == 0 exactly. The axis is perpendicular to v0 to within |v0.x|, which is bounded
// by the unsafe-divisor threshold below - a negligible (~1e-19) angular error.
VECTORCALL VECMATH_FINLINE quat4f v_quat_180_from_unit(vec3f v0)
{
  // (v0.y, -v0.x, 0, 0) is exactly perpendicular to v0; its length x^2+y^2 stays >= the
  // safe-under-squaring threshold unless |v0.x| is tiny, where the exactly-unit X axis (1,0,0,0)
  // is used - perpendicular to within |v0.x|. The threshold keeps the length safe under FTZ.
  vec4f perpXY = v_perm_xyab(v_perm_yaxx(v0, v_neg(v0)), v_zero());
  return v_norm4(v_sel(perpXY, V_C_UNIT_1000, v_is_unsafe_divisor(v_splat_x(v0))));
}

//! make quaternion to rotate 'v0' to 'v1'; v0 and v1 must be normalized
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_unit_arc(vec3f v0, vec3f v1)
{
  vec4f cosAngle = v_dot3(v0, v1);
  vec4f cosAngleX2Plus2 = v_madd_x(cosAngle, V_C_TWO, V_C_TWO); // 2(1+cos) = |(cross, 1+cos)|^2
  if (v_extract_x(cosAngleX2Plus2) > 1e-4f)
  {
    // q = (cross, 1+cos) normalized by its length sqrt(k)
    vec4f len = v_splat_x(v_sqrt_x(cosAngleX2Plus2));
    vec4f onePlusCos = v_splat_x(v_mul_x(cosAngleX2Plus2, V_C_HALF)); // splat so perm_xyzd sees it in .w
    quat4f q = v_perm_xyzd(v_cross3(v0, v1), onePlusCos);
    return v_div(q, len);
  }
  return v_quat_180_from_unit(v0); // opposite vectors
}

//! make quaternion to rotate 'v0' to 'v1'
VECTORCALL VECMATH_FINLINE quat4f v_quat_from_arc(vec3f v0, vec3f v1)
{
  vec4f inv_len_product = v_rsqrt_x(v_mul_x(v_length3_sq_x(v0), v_length3_sq_x(v1)));
  vec4f cosAngle = v_mul_x(v_dot3(v0, v1), inv_len_product);
  vec4f cosAngleX2Plus2 = v_madd_x(cosAngle, V_C_TWO, V_C_TWO); // 2(1+cos)
  if (v_extract_x(cosAngleX2Plus2) > 1e-4f)
  {
    // cross has magnitude len0*len1*sin, so scale it to unit by inv_len_product before normalizing
    vec3f crossVec = v_mul(v_cross3(v0, v1), v_splat_x(inv_len_product));
    vec4f len = v_splat_x(v_sqrt_x(cosAngleX2Plus2));
    vec4f onePlusCos = v_splat_x(v_mul_x(cosAngleX2Plus2, V_C_HALF)); // splat so perm_xyzd sees it in .w
    quat4f q = v_perm_xyzd(crossVec, onePlusCos);
    return v_div(q, len);
  }
  return v_quat_180_from_unit(v_norm3(v0)); // opposite vectors; normalize so the X-axis fallback is valid
}

//! make quaternion to rotate 'ang' radians around 'v' vector; v must be normalized
VECTORCALL inline quat4f v_quat_from_unit_vec_ang(vec3f v, vec4f ang)
{
  vec4f s, c;
  v_sincos(v_mul(ang, V_C_HALF), s, c);
  return v_perm_xyzd(v_mul(v, s), c);
}

VECTORCALL VECMATH_FINLINE quat4f v_quat_from_unit_vec_cos(vec3f v, vec4f ang_cos)
{
  vec4f half_cos = v_sqrt(v_mul(v_add(V_C_ONE, ang_cos), V_C_HALF));
  vec4f half_sin = v_sqrt(v_mul(v_sub(V_C_ONE, ang_cos), V_C_HALF));
  return v_perm_xyzd(v_mul(v, half_sin), half_cos);
}

// .xyz = heading, attitude, bank
VECTORCALL inline quat4f v_quat_from_euler(vec3f angles)
{
  vec4f s, c;
  v_sincos(v_mul(angles, V_C_HALF), s, c);
  vec4f q1 = v_mul(v_mul(v_perm_xycd(v_splat_x(s), v_splat_x(c)),
                         v_perm_xaxa(v_splat_y(s), v_splat_y(c))),
                   v_splat_z(c));
  vec4f q2 = v_mul(v_mul(v_perm_xycd(v_splat_x(c), v_splat_x(s)),
                         v_perm_xaxa(v_splat_y(c), v_splat_y(s))),
                   v_splat_z(s));
  return v_perm_xycd(v_add(q1, q2), v_sub(q1, q2));
}

// .xyz = heading, attitude, bank
VECTORCALL inline vec3f v_euler_from_quat(quat4f quat)
{
  vec4f test = v_dot2(v_perm_xzxz(quat), v_perm_ywyw(quat));
  vec4f testSign = v_and(test, V_CI_SIGN_MASK);
  vec4f testAbs = v_xor(test, testSign);
  vec4f specialCase = v_cmp_ge(testAbs, v_splats(0.49999f));
  vec4f m1 = v_perm_xxyy(quat);
  vec4f m2 = v_perm_zwzw(quat);
  vec4f m = v_mul(m1, m2);
  vec4f s = v_rot_2(v_perm_yyww(m));
  vec4f x = v_sub(s, m);
  vec4f qSq = v_sqr(quat);
  vec4f y = v_sub(V_C_HALF, v_add(v_perm_yzxw(qSq), v_splat_z(qSq)));
  x = v_sel(x, quat, specialCase);
  y = v_sel(y, v_splat_w(quat), specialCase);
  vec4f attX = v_add(test, test);
  vec4f attY = v_cos_from_sin_x(attX);
  x = v_sel(x, attX, V_CI_MASK0100);
  y = v_sel(y, v_splat_x(attY), V_CI_MASK0100);
  vec4f angles = v_atan2(x, y);
  vec4f specialAngles = v_perm_xycd(v_or(v_perm_xaxa(v_add_x(angles, angles), V_C_HALFPI), testSign), v_zero());
  return v_sel(angles, specialAngles, specialCase);
}

VECTORCALL inline quat4f v_quat_slerp(vec4f t, quat4f a, quat4f b)
{
  vec4f f = v_dot4(a, b);
  vec4f absF = v_abs(f);
  if (v_test_vec_x_ge(absF, v_set_x(0.9999f)))
  {
    vec4f p = v_add(a, v_mul(v_sub(b, a), t));
    vec4f n = v_msub(v_add(a, b), t, a);
    return v_norm4(v_sel(p, n, v_cmp_lt(f, v_zero())));
  }
  b = v_xor(b, v_and(f, v_msbit()));
  vec4f w = v_acos(absF);
  // pack (1-t, t, 1, 1) so one sin yields both numerators and sin(w); computing sin(w) from the
  // same polynomial as the numerators cancels error in the ratio
  vec4f s = v_sin(v_mul(w, v_perm_xyab(v_merge_hw(v_sub(V_C_ONE, t), t), V_C_ONE)));
  vec4f l = v_mul(a, v_splat_x(s));
  vec4f r = v_mul(b, v_splat_y(s));
  return v_div(v_add(l, r), v_splat_z(s));
}

VECTORCALL VECMATH_FINLINE quat4f v_quat_qslerp(float t, quat4f l, quat4f r)
{
  float ca = v_extract_x(v_dot4_x(l, r));
  float d = v_extract_x(v_abs(v_set_x(ca)));
  float k = 0.931872f + d * (-1.25654f + d * 0.331442f);
  float ot = t + t * (t - 0.5f) * (t - 1) * k;

  float lt = 1 - ot;
  float rt = ca > 0 ? ot : -ot;

  return v_norm4(v_add(v_mul(l, v_splats(lt)), v_mul(r, v_splats(rt))));
}

VECTORCALL VECMATH_FINLINE quat4f v_quat_qsquad(float t,
  const quat4f &q0, const quat4f &q1, const quat4f &q2, const quat4f &q3)
{
  quat4f tmp0, tmp1;
  tmp0 = v_quat_qslerp(t, q0, q3);
  tmp1 = v_quat_qslerp(t, q1, q2);
  return v_quat_qslerp(2*t*(1-t), tmp0, tmp1);
}

VECTORCALL VECMATH_FINLINE quat4f v_quat_mul_quat(quat4f q1, quat4f q2)
{
  vec4f qv, tmp0, tmp1, tmp2, tmp3;
  vec4f product, l_wxyz, r_wxyz, xy, qw;
  tmp0 = v_perm_yzxw(q1);
  tmp1 = v_perm_zxyw(q2);
  tmp2 = v_perm_zxyw(q1);
  tmp3 = v_perm_yzxw(q2);
  qv = v_mul(v_splat_w(q1), q2);
  qv = v_madd(v_splat_w(q2), q1, qv);
  qv = v_madd(tmp0, tmp1, qv);
  qv = v_nmsub(tmp2, tmp3, qv);
  product = v_mul(q1, q2);
  l_wxyz = v_perm_wxyz(q1);
  r_wxyz = v_perm_wxyz(q2);
  qw = v_nmsub(l_wxyz, r_wxyz, product);
  xy = v_madd(l_wxyz, r_wxyz, product);
  qw = v_sub(qw, v_perm_zwxy(xy));
  return v_perm_xyzd(qv, qw);
}

//! Decomposes 'q' into a rotational component around 'dir' = twist; and a perpendicular component = swing
//! q = swing * twist
//! dir is assumed to be normalized
VECTORCALL VECMATH_FINLINE void v_quat_decompose_swing_twist(quat4f q, vec3f dir, quat4f& swing, quat4f& twist)
{
  // http://www.euclideanspace.com/maths/geometry/rotations/for/decomposition/
  vec3f p = v_mul(v_dot3(q, dir), dir);
  twist = v_norm4_safe(v_perm_xyzd(p, q), V_C_UNIT_0001);
  swing = v_quat_mul_quat(q, v_quat_conjugate(twist));
}

//! Decomposes 'q' into a rotational component around 'dir' = twist; and a perpendicular component = swing
//! q = twist * swing
//! dir is assumed to be normalized
VECTORCALL VECMATH_FINLINE void v_quat_decompose_twist_swing(quat4f q, vec3f dir, quat4f& twist, quat4f& swing)
{
  vec3f p = v_mul(v_dot3(q, dir), dir);
  twist = v_norm4_safe(v_perm_xyzd(p, q), V_C_UNIT_0001);
  swing = v_quat_mul_quat(v_quat_conjugate(twist), q);
}

//! make 3x3 rotation matrix from quaternion
VECTORCALL VECMATH_FINLINE void v_mat33_from_quat(mat33f &dest, quat4f rot)
{
  vec4f xyzw_2, wwww, yzxw, zxyw, yzxw_2, zxyw_2;
  vec4f tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;

  xyzw_2 = v_add(rot, rot);
  wwww = v_splat_w(rot);
  yzxw = v_perm_yzxw(rot); yzxw_2 = v_perm_yzxw(xyzw_2);
  zxyw = v_perm_zxyw(rot); zxyw_2 = v_perm_zxyw(xyzw_2);
  tmp0 = v_mul(yzxw_2, wwww);
  tmp1 = v_nmsub(yzxw, yzxw_2, V_C_ONE);
  tmp2 = v_mul(yzxw, xyzw_2);
  tmp0 = v_madd(zxyw, xyzw_2, tmp0);
  tmp1 = v_nmsub(zxyw, zxyw_2, tmp1);
  tmp2 = v_nmsub(zxyw_2, wwww, tmp2);
  tmp3 = v_perm_ayzw(tmp0, tmp1);
  tmp4 = v_perm_ayzw(tmp1, tmp2);
  tmp5 = v_perm_ayzw(tmp2, tmp0);
  dest.col0 = v_perm_xycw(tmp3, tmp2);
  dest.col1 = v_perm_xycw(tmp4, tmp0);
  dest.col2 = v_perm_xycw(tmp5, tmp1);
}

//! compose 3x3 matrix from rotation/scale
VECTORCALL VECMATH_FINLINE void v_mat33_compose(mat33f &dest, quat4f rot, vec4f scale)
{
  vec4f xyzw_2, wwww, yzxw, zxyw, yzxw_2, zxyw_2;
  vec4f tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;

  xyzw_2 = v_add(rot, rot);
  wwww = v_splat_w(rot);
  yzxw = v_perm_yzxw(rot); yzxw_2 = v_perm_yzxw(xyzw_2);
  zxyw = v_perm_zxyw(rot); zxyw_2 = v_perm_zxyw(xyzw_2);
  tmp0 = v_mul(yzxw_2, wwww);
  tmp1 = v_nmsub(yzxw, yzxw_2, V_C_ONE);
  tmp2 = v_mul(yzxw, xyzw_2);
  tmp0 = v_madd(zxyw, xyzw_2, tmp0);
  tmp1 = v_nmsub(zxyw, zxyw_2, tmp1);
  tmp2 = v_nmsub(zxyw_2, wwww, tmp2);
  tmp3 = v_perm_ayzw(tmp0, tmp1);
  tmp4 = v_perm_ayzw(tmp1, tmp2);
  tmp5 = v_perm_ayzw(tmp2, tmp0);
  dest.col0 = v_mul(v_perm_xycw(tmp3, tmp2), v_splat_x(scale));
  dest.col1 = v_mul(v_perm_xycw(tmp4, tmp0), v_splat_y(scale));
  dest.col2 = v_mul(v_perm_xycw(tmp5, tmp1), v_splat_z(scale));
}

//! make 4x4 matrix from quaternion and position
VECTORCALL VECMATH_FINLINE void v_mat44_from_quat(mat44f &dest, quat4f q, vec4f pos)
{
  mat33f m;
  v_mat33_from_quat(m, q);
  dest.set33(m);
  dest.col3 = pos;
}

//! compose 4x4 matrix from position and scale
VECTORCALL VECMATH_FINLINE void v_mat44_compose(mat44f &dest, vec4f pos, vec4f scale)
{
  dest.col0 = v_perm_ayzw(v_zero(), scale);
  dest.col1 = v_perm_xbzw(v_zero(), scale);
  dest.col2 = v_perm_xycw(v_zero(), scale);
  dest.col3 = pos;
}

//! compose 4x4 matrix from position/rotation/scale
VECTORCALL VECMATH_FINLINE void v_mat44_compose(mat44f &dest, vec4f pos, quat4f rot, vec4f scale)
{
  vec4f xyzw_2, wwww, yzxw, zxyw, yzxw_2, zxyw_2;
  vec4f tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
  vec4f select_xyz = (vec4f)V_CI_MASK1110;

  xyzw_2 = v_add(rot, rot);
  wwww = v_splat_w(rot);
  yzxw = v_perm_yzxw(rot); yzxw_2 = v_perm_yzxw(xyzw_2);
  zxyw = v_perm_zxyw(rot); zxyw_2 = v_perm_zxyw(xyzw_2);
  tmp0 = v_mul(yzxw_2, wwww);
  tmp1 = v_nmsub(yzxw, yzxw_2, V_C_ONE);
  tmp2 = v_mul(yzxw, xyzw_2);
  tmp0 = v_madd(zxyw, xyzw_2, tmp0);
  tmp1 = v_nmsub(zxyw, zxyw_2, tmp1);
  tmp2 = v_nmsub(zxyw_2, wwww, tmp2);
  tmp3 = v_perm_ayzw(tmp0, tmp1);
  tmp4 = v_perm_ayzw(tmp1, tmp2);
  tmp5 = v_perm_ayzw(tmp2, tmp0);
  dest.col0 = v_and(v_mul(v_perm_xycw(tmp3, tmp2), v_splat_x(scale)), select_xyz);
  dest.col1 = v_and(v_mul(v_perm_xycw(tmp4, tmp0), v_splat_y(scale)), select_xyz);
  dest.col2 = v_and(v_mul(v_perm_xycw(tmp5, tmp1), v_splat_z(scale)), select_xyz);
  dest.col3 = v_perm_xyzd(pos, V_C_ONE);
}

//! decompose 3x3 matrix to rotation/scale
VECTORCALL VECMATH_FINLINE void v_mat33_decompose(mat33f_cref tm, quat4f &rot, vec4f &scl)
{
  mat44f m{tm.col0, tm.col1, tm.col2};
  scl = v_sqrt(v_mat44_scale43_sq(m));
  if (v_test_vec_x_lt_0(v_mat33_det(tm)))
    scl = v_perm_xycw(scl, v_neg(scl));

  // divide scale out; the basis can still be sheared or degenerate, which the
  // orthonormal path does not tolerate, so keep the contract that rot is unit
  v_mat44_apply_scale33(m, v_rcp_safe(scl));
  rot = v_norm4(v_quat_from_orthonormal_mat43(m));
}

//! decompose 4x4 matrix to position/rotation/scale
VECTORCALL VECMATH_FINLINE void v_mat4_decompose(mat44f_cref tm, vec3f &pos, quat4f &rot, vec4f &scl)
{
  pos = tm.col3;
  mat33f m3;
  v_mat33_from_mat44(m3, tm);
  v_mat33_decompose(m3, rot, scl);
}

//! make rotational matrix 3x3 to rotate 'ang' radians around 'v'
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw(mat33f &dest, vec3f v, vec4f ang)
{
  vec4f s, c, oneMinusC, axisS, negAxisS, xxxx, yyyy, zzzz, tmp0, tmp1, tmp2;
  v_sincos(ang, s, c);
  xxxx = v_splat_x(v);
  yyyy = v_splat_y(v);
  zzzz = v_splat_z(v);
  oneMinusC = v_sub(V_C_ONE, c);
  axisS = v_mul(v, s);
  negAxisS = v_neg(axisS);
  tmp0 = v_perm_xzbx(axisS, negAxisS);
  tmp1 = v_perm_caxx(axisS, negAxisS);
  tmp2 = v_perm_yaxx(axisS, negAxisS);
  tmp0 = v_perm_ayzw(tmp0, c);
  tmp1 = v_perm_xbzw(tmp1, c);
  tmp2 = v_perm_xycw(tmp2, c);

  vec4f axisOneMinusC = v_mul(v, oneMinusC);
  dest.col0 = v_madd(xxxx, axisOneMinusC, tmp0);
  dest.col1 = v_madd(yyyy, axisOneMinusC, tmp1);
  dest.col2 = v_madd(zzzz, axisOneMinusC, tmp2);
}

//! make rotational matrix 3x3 to rotate 'ang' radians around X axis
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw_x(mat33f &dest, vec4f ang)
{
  vec4f s, c, res1, res2;
  vec4f select_y = (vec4f)V_CI_MASK0100;
  v_sincos(ang, s, c);

  dest.col0 = V_C_UNIT_1000;
  res1 = v_and(c, select_y);
  dest.col1 = v_perm_xycw(res1, s);
  res2 = v_and(v_neg(s), select_y);
  dest.col2 = v_perm_xycw(res2, c);
}

//! make rotational matrix 3x3 to rotate 'ang' radians around Y axis
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw_y(mat33f &dest, vec4f ang)
{
  vec4f s, c, res0, res2;
  vec4f select_x = (vec4f)V_CI_MASK1000;
  v_sincos(ang, s, c);

  res0 = v_and(c, select_x);
  dest.col0 = v_perm_xycw(res0, v_neg(s));
  dest.col1 = V_C_UNIT_0100;
  res2 = v_and(s, select_x);
  dest.col2 = v_perm_xycw(res2, c);
}

//! make rotational matrix 3x3 to rotate 'ang' radians around Z axis
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw_z(mat33f &dest, vec4f ang)
{
  vec4f s, c, res0, res1;
  vec4f select_x = (vec4f)V_CI_MASK1000;
  v_sincos(ang, s, c);

  res0 = v_and(c, select_x);
  dest.col0 = v_perm_xbzw(res0, s);
  res1 = v_and(v_neg(s), select_x);
  dest.col1 = v_perm_xbzw(res1, c);
  dest.col2 = V_C_UNIT_0010;
}

//! make composite matrix 3x3: =rot_z(ang.z)*rot_y(ang.y)*rot_x(ang.x)
VECTORCALL VECMATH_FINLINE void v_mat33_make_rot_cw_zyx(mat33f &dest, vec4f ang_xyz)
{
  vec4f s, negS, c, X0, X1, Y0, Y1, Z0, Z1, tmp;
  v_sincos(v_and(ang_xyz, (vec4f)V_CI_MASK1110), s, c);
  negS = v_neg(s);
  Z0 = v_merge_lw(c, s);
  Z1 = v_merge_lw(negS, c);
  Z1 = v_and(Z1, (vec4f)V_CI_MASK1110);
  Y0 = v_perm_bbyx(negS, c);
  Y1 = v_perm_bbyx(c, s);
  X0 = v_splat_x(s);
  X1 = v_splat_x(c);
  tmp = v_mul( Z0, Y1);
  dest.col0 = v_mul(Z0, Y0);
  dest.col1 = v_madd(Z1, X1, v_mul(tmp, X0));
  dest.col2 = v_nmsub(Z1, X0, v_mul(tmp, X1));
}

VECTORCALL VECMATH_FINLINE plane3f v_norm_plane(plane3f in) { return v_norm3(in); }

VECTORCALL VECMATH_FINLINE vec3f v_ray_intersect_plane(vec3f A, vec3f B, plane3f P, vec4f &behind, vec4f &t)
{
  vec3f dir = v_sub(B, A);
  vec4f dot = v_dot3(dir, P);
  t = v_div(v_sub(v_neg(v_splat_w(P)),v_dot3(A, P)), dot);
  behind = v_or(v_cmp_gt(v_zero(), t), v_cmp_eq(dot, v_zero()));
  return v_madd(t, dir, A);
}

VECTORCALL VECMATH_FINLINE vec3f three_plane_intersection(plane3f p0, plane3f p1, plane3f p2, vec4f &invalid)
{
  vec4f n1_n2 = v_cross3(p1, p2), n2_n0 = v_cross3(p2, p0), n0_n1 = v_cross3(p0, p1);
  vec4f cosTheta = v_dot3(p0, n1_n2);
  invalid = v_is_unsafe_divisor(cosTheta);
  vec4f secTheta = v_rcp(cosTheta);

  vec4f intersectPt;
  intersectPt = v_nmsub(n1_n2, v_splat_w(p0), v_zero());
  intersectPt = v_nmsub(n2_n0, v_splat_w(p1), intersectPt);
  intersectPt = v_nmsub(n0_n1, v_splat_w(p2), intersectPt);
  return v_mul(intersectPt, secTheta);
}

VECTORCALL VECMATH_FINLINE void v_unsafe_two_plane_intersection(plane3f p1, plane3f p2, vec3f &point, vec3f &dir)
{
  dir = v_cross3(p1, p2);//p3_normal
  // calculate the final (point, normal)
  point = v_add(v_mul(v_cross3(dir, p2), v_splat_w(p1)), v_mul(v_cross3(p1, dir), v_splat_w(p2)));
  //vec4f det = v_dot3(dir, dir);
  //for safe check if abs(det) is bigger then eps
  point = v_div(point, v_dot3(dir, dir));
}

VECTORCALL VECMATH_FINLINE vec3f v_unsafe_ray_intersect_plane(vec3f point, vec3f dir, plane3f P)
{
  vec3f t = v_div(v_sub(v_neg(v_splat_w(P)), v_dot3(point, P)), v_dot3(dir, P));
  //for safe check if abs(v_dot3(dir, P)) is bigger than eps
  return v_madd(t, dir, point);
}

VECTORCALL VECMATH_FINLINE vec3f v_closest_point_on_segment(vec3f point, vec3f a, vec3f b)
{
  vec3f ap = v_sub(point, a);
  vec3f ab = v_sub(b, a);
  vec3f lenSq = v_length3_sq(ab);
  vec3f t = v_clamp(v_dot3(ap, ab), v_zero(), lenSq);
  return v_madd(v_safediv(ab, lenSq), t, a);
}

VECTORCALL VECMATH_FINLINE void v_is_transform_points_4(vec4f* dest, vec4f x, vec4f y, vec4f z, mat44f_cref mat)
{
#define COMP(c, attr) \
  vec4f res_ ## c = v_splat_##attr(mat.col3); \
  res_ ## c = v_madd(z, v_splat_##attr(mat.col2), res_ ## c); \
  res_ ## c = v_madd(y, v_splat_##attr(mat.col1), res_ ## c); \
  res_ ## c = v_madd(x, v_splat_##attr(mat.col0), res_ ## c); \
  dest[c] = res_ ## c;
  COMP(0,x);
  COMP(1,y);
  COMP(2,z);
  COMP(3,w);
#undef COMP
}

VECTORCALL VECMATH_FINLINE void v_is_transform_points_4(vec4f* dest, vec4f x, vec4f y, mat44f_cref mat)
{
#define COMP(c, attr) \
  vec4f res_ ## c = v_splat_##attr(mat.col3); \
  res_ ## c = v_madd(y, v_splat_##attr(mat.col1), res_ ## c); \
  res_ ## c = v_madd(x, v_splat_##attr(mat.col0), res_ ## c); \
  dest[c] = res_ ## c;
  COMP(0,x);
  COMP(1,y);
  COMP(2,z);
  COMP(3,w);
#undef COMP
}

// shared core of the v_is_visible* / v_screen_size_b family

// transform the 8 aabb corners to clip space as two z-slices, each SoA: [xxxx],[yyyy],[zzzz],[wwww]
// one clip-matrix row applied to 4 xy points at two z levels; the z slices share x and y,
// so the x/y part is built once and each z is added on top
VECTORCALL VECMATH_FINLINE void v_is_transform_row_2z(vec4f x, vec4f y, vec4f minz, vec4f maxz,
  vec4f row_x, vec4f row_y, vec4f row_z, vec4f row_w, vec4f &out0, vec4f &out1)
{
  vec4f base = v_madd(x, row_x, row_w);
  base = v_madd(y, row_y, base);
  out0 = v_madd(minz, row_z, base);
  out1 = v_madd(maxz, row_z, base);
}

VECTORCALL VECMATH_FINLINE void v_is_transform_box_8(vec3f bmin, vec3f bmax, mat44f_cref clip,
  vec4f cs0[4], vec4f cs1[4], vec4f &cs0_negw, vec4f &cs1_negw)
{
  vec4f minmax_x = v_perm_xaxa(bmin, bmax);
  vec4f minmax_y = v_perm_yybb(bmin, bmax);
  vec4f minz = v_splat_z(bmin);
  vec4f maxz = v_splat_z(bmax);
  v_is_transform_row_2z(minmax_x, minmax_y, minz, maxz,
    v_splat_x(clip.col0), v_splat_x(clip.col1), v_splat_x(clip.col2), v_splat_x(clip.col3), cs0[0], cs1[0]);
  v_is_transform_row_2z(minmax_x, minmax_y, minz, maxz,
    v_splat_y(clip.col0), v_splat_y(clip.col1), v_splat_y(clip.col2), v_splat_y(clip.col3), cs0[1], cs1[1]);
  v_is_transform_row_2z(minmax_x, minmax_y, minz, maxz,
    v_splat_z(clip.col0), v_splat_z(clip.col1), v_splat_z(clip.col2), v_splat_z(clip.col3), cs0[2], cs1[2]);
  v_is_transform_row_2z(minmax_x, minmax_y, minz, maxz,
    v_splat_w(clip.col0), v_splat_w(clip.col1), v_splat_w(clip.col2), v_splat_w(clip.col3), cs0[3], cs1[3]);
  cs0_negw = v_neg(cs0[3]);
  cs1_negw = v_neg(cs1[3]);
}

// mask of the 8 points (two slices) on the inner side of one clip plane, i.e. coord > bound
VECTORCALL VECMATH_FINLINE vec4f v_is_plane_inside_mask(vec4f coord0, vec4f bound0, vec4f coord1, vec4f bound1)
{
  return v_or(v_cmp_gt(coord0, bound0), v_cmp_gt(coord1, bound1));
}

// -1 if some point is on the inner side of every clip plane (x,y in [-w,w], z in [0,w]), 0 otherwise
VECTORCALL VECMATH_FINLINE int v_is_frustum_nout_8(const vec4f cs0[4], vec4f cs0_negw, const vec4f cs1[4], vec4f cs1_negw)
{
  vec4f inXlo = v_is_plane_inside_mask(cs0[0], cs0_negw, cs1[0], cs1_negw);  // -w <= x
  vec4f inXhi = v_is_plane_inside_mask(cs0[3], cs0[0],   cs1[3], cs1[0]);    //  x <= w
  vec4f inYlo = v_is_plane_inside_mask(cs0[1], cs0_negw, cs1[1], cs1_negw);  // -w <= y
  vec4f inYhi = v_is_plane_inside_mask(cs0[3], cs0[1],   cs1[3], cs1[1]);    //  y <= w
  vec4f inZlo = v_is_plane_inside_mask(cs0[2], v_zero(), cs1[2], v_zero());  //  0 <= z
  vec4f inZhi = v_is_plane_inside_mask(cs0[3], cs0[2],   cs1[3], cs1[2]);    //  z <= w
  return v_is_merge_planes_nout(inXlo, inXhi, inYlo, inYhi, inZlo, inZhi);
}

// project 4 clip-space points to NDC and reduce to per-lane xy min/max (result still holds 2 points)
VECTORCALL VECMATH_FINLINE void v_is_screen_project_minmax(const vec4f cs[4], vec4f &minXY, vec4f &maxXY)
{
  vec4f inv = v_rcp(cs[3]);
  vec4f xxxx = v_mul(cs[0], inv);
  vec4f yyyy = v_mul(cs[1], inv);
  vec4f point01 = v_merge_hw(xxxx, yyyy);//xy, xy
  vec4f point23 = v_merge_lw(xxxx, yyyy);//xy, xy
  minXY = v_min(point01, point23);
  maxXY = v_max(point01, point23);
}

// reduce xy min/max to screen_box (minX, maxX, minY, maxY); 0 if smaller than threshold in x or y, else 1
VECTORCALL VECMATH_FINLINE int v_is_screen_finish(vec4f minXY, vec4f maxXY, vec3f threshold, vec4f &screen_box)
{
  minXY = v_min(minXY, v_rot_2(minXY));
  maxXY = v_max(maxXY, v_rot_2(maxXY));
  screen_box = v_merge_hw(minXY, maxXY);
  vec4f tooSmall = v_cmp_ge(threshold, v_sub(maxXY, minXY));
  return v_check_xy_any_true(tooSmall) ? 0 : 1;
}

//universal visibility function (accepts worldviewproj matrix)

///return zero if not visible. 0xff in all bytes, otherwise
///really fast, ~0.075 us per test on PS3

///for branching: (~v_test_vec_x_eqi_0(v_is_visible(bmin, bmax, clip)))&1 will be 1 if visible, zero otherwise;
VECTORCALL VECMATH_INLINE vec4f v_is_visible(vec3f bmin, vec3f bmax, mat44f_cref clip)
{
  vec4f cs0[4], cs1[4], cs0_negw, cs1_negw;
  v_is_transform_box_8(bmin, bmax, clip, cs0, cs1, cs0_negw, cs1_negw);
  // reduce each plane to a bit and broadcast the 0/-1 result once
  return v_cast_vec4f(v_splatsi(v_is_frustum_nout_8(cs0, cs0_negw, cs1, cs1_negw)));
}

VECTORCALL VECMATH_INLINE int v_is_visible_b(vec3f bmin, vec3f bmax, mat44f_cref clip)
{
  vec4f cs0[4], cs1[4], cs0_negw, cs1_negw;
  v_is_transform_box_8(bmin, bmax, clip, cs0, cs1, cs0_negw, cs1_negw);
  return -v_is_frustum_nout_8(cs0, cs0_negw, cs1, cs1_negw); // 0 or 1
}

VECTORCALL VECMATH_FINLINE void v_construct_camplanes(mat44f_cref clip,
  vec4f &camPlanes0, vec4f &camPlanes1, vec4f &camPlanes2, vec4f &camPlanes3, vec4f &camPlanes4, vec4f &camPlanes5)
{
  mat44f m2;
  v_mat44_transpose(m2, clip);
  camPlanes0 = v_sub(m2.col3, m2.col0);  // right
  camPlanes1 = v_add(m2.col3, m2.col0);  // left
  camPlanes2 = v_sub(m2.col3, m2.col1);  // top
  camPlanes3 = v_add(m2.col3, m2.col1);  // bottom
  camPlanes4 = v_sub(m2.col3, m2.col2);  // far if forward depth (near otherwise)
  camPlanes5 = m2.col2;  // near if forward depth (far otherwise)
}

// solve 4 three-plane intersections at once: lane i is the corner of planes (a_i, b_i, c);
// p = -(a.w*(b x c) + b.w*(c x a) + c.w*(a x b)) / (a . (b x c)), a x b passed in (shared by batches)
VECTORCALL VECMATH_FINLINE void v_frustum_corners_4(vec4f ax, vec4f ay, vec4f az, vec4f aw,
  vec4f bx, vec4f by, vec4f bz, vec4f bw, vec4f abx, vec4f aby, vec4f abz, vec4f c,
  vec4f &px, vec4f &py, vec4f &pz)
{
  vec4f cx = v_splat_x(c), cy = v_splat_y(c), cz = v_splat_z(c), cw = v_splat_w(c);
  vec4f bcx = v_nmsub(bz, cy, v_mul(by, cz)); // b x c
  vec4f bcy = v_nmsub(bx, cz, v_mul(bz, cx));
  vec4f bcz = v_nmsub(by, cx, v_mul(bx, cy));
  vec4f cax = v_nmsub(cz, ay, v_mul(cy, az)); // c x a
  vec4f cay = v_nmsub(cx, az, v_mul(cz, ax));
  vec4f caz = v_nmsub(cy, ax, v_mul(cx, ay));
  vec4f den = v_madd(ax, bcx, v_madd(ay, bcy, v_mul(az, bcz)));
  vec4f inv = v_rcp(den);
  px = v_mul(v_nmsub(abx, cw, v_nmsub(cax, bw, v_nmsub(bcx, aw, v_zero()))), inv);
  py = v_mul(v_nmsub(aby, cw, v_nmsub(cay, bw, v_nmsub(bcy, aw, v_zero()))), inv);
  pz = v_mul(v_nmsub(abz, cw, v_nmsub(caz, bw, v_nmsub(bcz, aw, v_zero()))), inv);
}

VECTORCALL VECMATH_FINLINE void v_frustum_box_unsafe(bbox3f& box,
                                 vec4f camPlanes0, vec4f camPlanes1, vec4f camPlanes2,
                                 const vec4f &camPlanes3, const vec4f &camPlanes4, const vec4f &camPlanes5)
{
  // the 8 corners are (P5|P4, P2|P3, P1|P0) triples; solve them lane-parallel in two
  // batches of 4 with a = (P5,P4,P5,P4), b = (P2,P2,P3,P3) and c = P1 / P0
  vec4f m01 = v_merge_hw(camPlanes5, camPlanes4); // 5x 4x 5y 4y
  vec4f m23 = v_merge_lw(camPlanes5, camPlanes4); // 5z 4z 5w 4w
  vec4f ax = v_perm_xyxy(m01), ay = v_perm_zwcd(m01, m01);
  vec4f az = v_perm_xyxy(m23), aw = v_perm_zwcd(m23, m23);
  vec4f n01 = v_merge_hw(camPlanes2, camPlanes3); // 2x 3x 2y 3y
  vec4f n23 = v_merge_lw(camPlanes2, camPlanes3); // 2z 3z 2w 3w
  vec4f bx = v_perm_xxyy(n01), by = v_perm_zzww(n01);
  vec4f bz = v_perm_xxyy(n23), bw = v_perm_zzww(n23);

  vec4f abx = v_nmsub(az, by, v_mul(ay, bz)); // a x b
  vec4f aby = v_nmsub(ax, bz, v_mul(az, bx));
  vec4f abz = v_nmsub(ay, bx, v_mul(ax, by));

  vec4f px0, py0, pz0, px1, py1, pz1;
  v_frustum_corners_4(ax, ay, az, aw, bx, by, bz, bw, abx, aby, abz, camPlanes1, px0, py0, pz0);
  v_frustum_corners_4(ax, ay, az, aw, bx, by, bz, bw, abx, aby, abz, camPlanes0, px1, py1, pz1);

  vec4f minx = v_min(px0, px1), maxx = v_max(px0, px1);
  vec4f miny = v_min(py0, py1), maxy = v_max(py0, py1);
  vec4f minz = v_min(pz0, pz1), maxz = v_max(pz0, pz1);
  // pairwise trees reduce the three components and pack the bound in one go: (x, y, z, z)
  box.bmin = v_min_pairs(v_min_pairs(minx, miny), v_min_pairs(minz, minz));
  box.bmax = v_max_pairs(v_max_pairs(maxx, maxy), v_max_pairs(maxz, maxz));
}

VECTORCALL VECMATH_FINLINE int v_is_visible_sphere(vec3f center, vec3f r,
                  vec4f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W,
                  const vec4f& plane4, const vec4f& plane5)
{
  vec4f res03;
  res03 = v_madd(v_splat_x(center), plane03X, plane03W);
  res03 = v_madd(v_splat_y(center), plane03Y, res03);
  res03 = v_madd(v_splat_z(center), plane03Z, res03);
  res03 = v_add(res03, r);
  res03 = v_or(res03, v_add(v_add(v_dot3(center, plane4), r), v_splat_w(plane4)));
  res03 = v_or(res03, v_add(v_add(v_dot3(center, plane5), r), v_splat_w(plane5)));

  return !v_is_any_neg_b(res03); // inside if no plane distance is negative
}

VECTORCALL VECMATH_FINLINE int v_sphere_intersect(vec3f center, vec3f r,
                  vec3f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W,
                  const vec4f& plane4, const vec4f& plane5)
{
  // the +r and -r bounds share the center's plane distances; compute them once
  vec4f base = v_madd(v_splat_x(center), plane03X, plane03W);
  base = v_madd(v_splat_y(center), plane03Y, base);
  base = v_madd(v_splat_z(center), plane03Z, base);
  vec4f d4 = v_dot3(center, plane4), w4 = v_splat_w(plane4);
  vec4f d5 = v_dot3(center, plane5), w5 = v_splat_w(plane5);

  vec4f res = v_add(base, r);
  res = v_or(res, v_add(v_add(d4, r), w4));
  res = v_or(res, v_add(v_add(d5, r), w5));
  if (v_is_any_neg_b(res)) // near bound outside a plane -> sphere fully outside
    return 0;

  res = v_sub(base, r);
  res = v_or(res, v_add(v_sub(d4, r), w4));
  res = v_or(res, v_add(v_sub(d5, r), w5));
  return v_is_any_neg_b(res) + 1; // far bound crosses a plane -> intersects, else fully inside
}

// not 100% correct: OUTSIDE cases may be reported as INTERSECT (in practice this is VERY rare, mostly for huge boxes)
VECTORCALL VECMATH_FINLINE int v_is_visible_box_extent2(vec3f center, vec3f extent,//center and extent should be multiplied by 2
                  vec4f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W2,
                  const vec4f& plane4W2, const vec4f& plane5W2)
{
  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  vec4f res03;
  res03 = v_madd(v_add(v_splat_x(center), v_xor(v_splat_x(extent), v_and(plane03X, signmask))), plane03X, plane03W2);
  res03 = v_madd(v_add(v_splat_y(center), v_xor(v_splat_y(extent), v_and(plane03Y, signmask))), plane03Y, res03);
  res03 = v_madd(v_add(v_splat_z(center), v_xor(v_splat_z(extent), v_and(plane03Z, signmask))), plane03Z, res03);
  res03 = v_or(res03, v_add(v_dot3(v_add(v_xor(extent, v_and(plane4W2, signmask)), center), plane4W2), v_splat_w(plane4W2)));
  res03 = v_or(res03, v_add(v_dot3(v_add(v_xor(extent, v_and(plane5W2, signmask)), center), plane5W2), v_splat_w(plane5W2)));

  return !v_is_any_neg_b(res03); // inside if no plane distance is negative
}

// not 100% correct: OUTSIDE cases may be reported as INTERSECT (in practice this is VERY rare, mostly for huge boxes)
VECTORCALL VECMATH_FINLINE int v_box_frustum_intersect_extent2(vec3f center, vec3f extent, // center and extent should be multiplied by
                                                                                           // 2
                  vec4f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W2,
                  const vec4f& plane4W2, const vec4f& plane5W2)
{
  // sign(plane)*extent per axis is the same for the p- and n-vertex; compute it (and the
  // center splats / plane w) once and reuse across both bounds
  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  vec4f cx = v_splat_x(center), cy = v_splat_y(center), cz = v_splat_z(center);
  vec4f eX = v_xor(v_splat_x(extent), v_and(plane03X, signmask));
  vec4f eY = v_xor(v_splat_y(extent), v_and(plane03Y, signmask));
  vec4f eZ = v_xor(v_splat_z(extent), v_and(plane03Z, signmask));
  vec4f e4 = v_xor(extent, v_and(plane4W2, signmask)), w4 = v_splat_w(plane4W2);
  vec4f e5 = v_xor(extent, v_and(plane5W2, signmask)), w5 = v_splat_w(plane5W2);

  vec4f res03;
  res03 = v_madd(v_add(cx, eX), plane03X, plane03W2);
  res03 = v_madd(v_add(cy, eY), plane03Y, res03);
  res03 = v_madd(v_add(cz, eZ), plane03Z, res03);
  res03 = v_or(res03, v_add(v_dot3(v_add(e4, center), plane4W2), w4));
  res03 = v_or(res03, v_add(v_dot3(v_add(e5, center), plane5W2), w5));

  if (v_is_any_neg_b(res03)) // p-vertex outside a plane -> box fully outside
    return 0;

  res03 = v_madd(v_sub(cx, eX), plane03X, plane03W2);
  res03 = v_madd(v_sub(cy, eY), plane03Y, res03);
  res03 = v_madd(v_sub(cz, eZ), plane03Z, res03);
  res03 = v_or(res03, v_add(v_dot3(v_sub(center, e4), plane4W2), w4));
  res03 = v_or(res03, v_add(v_dot3(v_sub(center, e5), plane5W2), w5));

  return v_is_any_neg_b(res03) + 1;
}

VECTORCALL VECMATH_FINLINE int v_is_visible_extent_fast(vec3f center, vec3f extent, mat44f_cref clip)//center and extent should be multiplied by 2
{
  //construct frustum
  vec3f plane03X, plane03Y, plane03Z, plane03W2, plane4W2, plane5W2;
  v_construct_camplanes(clip, plane03X, plane03Y, plane03Z, plane03W2, plane4W2, plane5W2);
  v_mat44_transpose(plane03X, plane03Y, plane03Z, plane03W2);
  plane03W2 = v_add(plane03W2, plane03W2);
  plane4W2 = v_perm_xyzd(plane4W2, v_add(plane4W2, plane4W2));
  plane5W2 = v_perm_xyzd(plane5W2, v_add(plane5W2, plane5W2));

  //perform test
  return v_is_visible_box_extent2(center, extent, plane03X, plane03Y, plane03Z, plane03W2, plane4W2, plane5W2);
}

VECTORCALL VECMATH_FINLINE int v_frustum_intersect(vec3f center, vec3f extent, mat44f_cref clip)//center and extent should be multiplied by 2
{
  //construct frustum
  vec3f plane03X, plane03Y, plane03Z, plane03W2, plane4W2, plane5W2;
  v_construct_camplanes(clip, plane03X, plane03Y, plane03Z, plane03W2, plane4W2, plane5W2);
  v_mat44_transpose(plane03X, plane03Y, plane03Z, plane03W2);
  plane03W2 = v_add(plane03W2, plane03W2);
  plane4W2 = v_perm_xyzd(plane4W2, v_add(plane4W2, plane4W2));
  plane5W2 = v_perm_xyzd(plane5W2, v_add(plane5W2, plane5W2));
  //perform test
  return v_box_frustum_intersect_extent2(center, extent, plane03X, plane03Y, plane03Z, plane03W2, plane4W2, plane5W2);
}

VECTORCALL VECMATH_FINLINE int v_is_visible_b_fast(vec3f bmin, vec3f bmax, mat44f_cref clip)
{
  vec4f center = v_add(bmax, bmin);
  vec4f extent = v_sub(bmax, bmin);
  return v_is_visible_extent_fast(center, extent, clip);
}

VECTORCALL VECMATH_FINLINE int v_is_visible_box_extent2(vec3f center, vec3f extent,//center and extent should be multiplied by 2
                  vec4f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W2,
                  const vec4f& plane47X, const vec4f& plane47Y, const vec4f& plane47Z, const vec4f& plane47W2)
{
  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  vec4f res03, res47;
  res03 = v_madd(v_add(v_splat_x(center), v_xor(v_splat_x(extent), v_and(plane03X, signmask))), plane03X, plane03W2);
  res03 = v_madd(v_add(v_splat_y(center), v_xor(v_splat_y(extent), v_and(plane03Y, signmask))), plane03Y, res03);
  res03 = v_madd(v_add(v_splat_z(center), v_xor(v_splat_z(extent), v_and(plane03Z, signmask))), plane03Z, res03);

  res47 = v_madd(v_add(v_splat_x(center), v_xor(v_splat_x(extent), v_and(plane47X, signmask))), plane47X, plane47W2);
  res47 = v_madd(v_add(v_splat_y(center), v_xor(v_splat_y(extent), v_and(plane47Y, signmask))), plane47Y, res47);
  res47 = v_madd(v_add(v_splat_z(center), v_xor(v_splat_z(extent), v_and(plane47Z, signmask))), plane47Z, res47);

  return !v_is_any_neg_b(v_or(res03, res47)); // inside if no plane distance is negative
}

VECTORCALL VECMATH_FINLINE int v_box_frustum_intersect_extent2(vec3f center, vec3f extent,//center and extent should be multiplied by 2
                  vec4f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W2,
                  const vec4f& plane47X, const vec4f& plane47Y, const vec4f& plane47Z, const vec4f& plane47W2)
{
  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  vec4f res03Base, res47Base, res03Add, res47Add;
  res03Base = v_madd(v_splat_x(center), plane03X, plane03W2);
  res03Base = v_madd(v_splat_y(center), plane03Y, res03Base);
  res03Base = v_madd(v_splat_z(center), plane03Z, res03Base);
  res03Add =  v_mul(v_xor(v_splat_x(extent), v_and(plane03X, signmask)), plane03X);
  res03Add = v_madd(v_xor(v_splat_y(extent), v_and(plane03Y, signmask)), plane03Y, res03Add);
  res03Add = v_madd(v_xor(v_splat_z(extent), v_and(plane03Z, signmask)), plane03Z, res03Add);

  res47Base = v_madd(v_splat_x(center), plane47X, plane47W2);
  res47Base = v_madd(v_splat_y(center), plane47Y, res47Base);
  res47Base = v_madd(v_splat_z(center), plane47Z, res47Base);
  res47Add =  v_mul(v_xor(v_splat_x(extent), v_and(plane47X, signmask)), plane47X);
  res47Add = v_madd(v_xor(v_splat_y(extent), v_and(plane47Y, signmask)), plane47Y, res47Add);
  res47Add = v_madd(v_xor(v_splat_z(extent), v_and(plane47Z, signmask)), plane47Z, res47Add);

  if (v_is_any_neg_b(v_or(v_add(res03Base, res03Add), v_add(res47Base, res47Add))))
    return 0;

  return v_is_any_neg_b(v_or(v_sub(res03Base, res03Add), v_sub(res47Base, res47Add))) + 1;
}

VECTORCALL VECMATH_FINLINE int v_is_visible_sphere(vec3f center, vec3f r,
                  vec3f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W,
                  const vec4f& plane47X, const vec4f& plane47Y, const vec4f& plane47Z, const vec4f& plane47W)
{
  vec4f res03, res47;
  res03 = v_madd(v_splat_x(center), plane03X, plane03W);
  res03 = v_madd(v_splat_y(center), plane03Y, res03);
  res03 = v_madd(v_splat_z(center), plane03Z, res03);
  res47 = v_madd(v_splat_x(center), plane47X, plane47W);
  res47 = v_madd(v_splat_y(center), plane47Y, res47);
  res47 = v_madd(v_splat_z(center), plane47Z, res47);
  res03 = v_or(v_add(res03, r), v_add(res47, r));
  return !v_is_any_neg_b(res03); // inside if no plane distance is negative
}

VECTORCALL VECMATH_FINLINE int v_sphere_intersect(vec3f center, vec3f r,
                  vec3f plane03X, const vec4f& plane03Y, const vec4f& plane03Z, const vec4f& plane03W,
                  const vec4f& plane47X, const vec4f& plane47Y, const vec4f& plane47Z, const vec4f& plane47W)
{
  vec4f res03, res47;
  res03 = v_madd(v_splat_x(center), plane03X, plane03W);
  res03 = v_madd(v_splat_y(center), plane03Y, res03);
  res03 = v_madd(v_splat_z(center), plane03Z, res03);
  res47 = v_madd(v_splat_x(center), plane47X, plane47W);
  res47 = v_madd(v_splat_y(center), plane47Y, res47);
  res47 = v_madd(v_splat_z(center), plane47Z, res47);
  vec4f resOut = v_or(v_add(res03, r), v_add(res47, r));
  if (v_is_any_neg_b(resOut))
    return 0;

  vec4f resIn = v_or(v_sub(res03, r), v_sub(res47, r));
  return v_is_any_neg_b(resIn) + 1;
}

VECTORCALL VECMATH_FINLINE int v_is_visible_extent_fast_8planes(vec3f center, vec3f extent, mat44f_cref clip)//just correct box extents, not multiplied
{
  //construct frustum
  vec3f plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W;
  v_construct_camplanes(clip, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y);
  v_mat44_transpose(plane03X, plane03Y, plane03Z, plane03W);
  plane47Z = plane47X, plane47W = plane47Y;//we can use some useful planes instead of replicating
  v_mat44_transpose(plane47X, plane47Y, plane47Z, plane47W);
  //perform test
  return v_is_visible_box_extent2(center, extent, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W);
}

VECTORCALL VECMATH_FINLINE int v_is_visible_b_fast_8planes(vec3f bmin, vec3f bmax, mat44f_cref clip)
{
  vec4f center = v_mul(v_add(bmax, bmin), V_C_HALF);
  vec4f extent = v_sub(bmax, center);
  return v_is_visible_extent_fast_8planes(center, extent, clip);
}

//todo: move to consts
alignas(16) static float v_screen_full_screen[4] = {-2, 2, -2, 2};
alignas(16) static float v_screen_div_eps[4] = {0.0001f,0.0001f,0.0001f,0.0001f};

//universal visibility function (accepts worldviewproj matrix)

///return zero if not visible in frustum or if bbox screen size is smaller, than threshold. not zero, otherwise
// also, screen_box is minX, maxX, minY, maxY - in clipspace coordinates  (-1, -1) .. (1,1)

VECTORCALL VECMATH_INLINE int v_screen_size_b(vec3f bmin, vec3f bmax, vec3f threshold, vec4f &screen_box, mat44f_cref clip)
{
  vec4f cs0[4], cs1[4], cs0_negw, cs1_negw;
  v_is_transform_box_8(bmin, bmax, clip, cs0, cs1, cs0_negw, cs1_negw);

  if (!v_is_frustum_nout_8(cs0, cs0_negw, cs1, cs1_negw)) // outside at least one plane
    return 0;

  vec4f eps = *(vec4f*)v_screen_div_eps;
  vec4f valid_cs = v_and(v_cmp_gt(cs0[3], eps), v_cmp_gt(cs1[3], eps));
  if (!v_check_xyzw_all_true(valid_cs))
  {
    screen_box = *(vec4f*)v_screen_full_screen;
    return -1;
  }

  vec4f min0, max0, min1, max1;
  v_is_screen_project_minmax(cs0, min0, max0);
  v_is_screen_project_minmax(cs1, min1, max1);
  return v_is_screen_finish(v_min(min0, min1), v_max(max0, max1), threshold, screen_box);
}

///return zero if not visible in frustum or if bbox screen size is smaller, than threshold. 1, if all vertex are in front of near plane, -1 (and fullscreen rect) otherwise
// also, screen_box is minX, maxX, minY, maxY - in clipspace coordinates  (-1, -1) .. (1,1), minmax_w is wWwW (minw, maxw, minw, maxw)
VECTORCALL VECMATH_INLINE int
  v_screen_size_b(vec3f bmin, vec3f bmax, vec3f threshold, vec4f &screen_box, vec4f &minmax_w, mat44f_cref clip)
{
  vec4f cs0[4], cs1[4], cs0_negw, cs1_negw;
  v_is_transform_box_8(bmin, bmax, clip, cs0, cs1, cs0_negw, cs1_negw);

  if (!v_is_frustum_nout_8(cs0, cs0_negw, cs1, cs1_negw)) // outside at least one plane
    return 0;

  // horizontal reduce of the 8 corner w's; on NEON these fold to fminv/fmaxv
  vec4f min_w = v_hmin(v_min(cs0[3], cs1[3]));
  vec4f max_w = v_hmax(v_max(cs0[3], cs1[3]));
  minmax_w = v_perm_xaxa(min_w, max_w);

  vec4f eps = *(vec4f*)v_screen_div_eps;
  if (v_test_vec_x_lt(min_w, eps))
  {
    screen_box = *(vec4f*)v_screen_full_screen;
    return -1;
  }

  vec4f min0, max0, min1, max1;
  v_is_screen_project_minmax(cs0, min0, max0);
  v_is_screen_project_minmax(cs1, min1, max1);
  return v_is_screen_finish(v_min(min0, min1), v_max(max0, max1), threshold, screen_box);
}

// as v_screen_size_b above, contract: the box is already frustum-classified as at least partially
// visible. Only the x, y, w clip rows are computed (clip z is consumed solely by the frustum planes,
// which are skipped), so the minmax_w / screen rect math is the same FP ops as v_screen_size_b and the
// outputs are bit-identical whenever the contract holds; a fully-outside box yields meaningless (but
// safe) outputs instead of the 0 return.
VECTORCALL VECMATH_INLINE int
  v_screen_size_visible_b(vec3f bmin, vec3f bmax, vec3f threshold, vec4f &screen_box, vec4f &minmax_w, mat44f_cref clip)
{
  vec4f minmax_x = v_perm_xaxa(bmin, bmax);
  vec4f minmax_y = v_perm_yybb(bmin, bmax);
  vec4f minz = v_splat_z(bmin);
  vec4f maxz = v_splat_z(bmax);
  vec4f cs0[4], cs1[4]; // only the x, y, w rows are filled; [2] stays untouched (never read below)
  v_is_transform_row_2z(minmax_x, minmax_y, minz, maxz, v_splat_x(clip.col0), v_splat_x(clip.col1), v_splat_x(clip.col2),
    v_splat_x(clip.col3), cs0[0], cs1[0]);
  v_is_transform_row_2z(minmax_x, minmax_y, minz, maxz, v_splat_y(clip.col0), v_splat_y(clip.col1), v_splat_y(clip.col2),
    v_splat_y(clip.col3), cs0[1], cs1[1]);
  v_is_transform_row_2z(minmax_x, minmax_y, minz, maxz, v_splat_w(clip.col0), v_splat_w(clip.col1), v_splat_w(clip.col2),
    v_splat_w(clip.col3), cs0[3], cs1[3]);

  vec4f min_w = v_hmin(v_min(cs0[3], cs1[3]));
  vec4f max_w = v_hmax(v_max(cs0[3], cs1[3]));
  minmax_w = v_perm_xaxa(min_w, max_w);

  vec4f eps = *(vec4f *)v_screen_div_eps;
  if (v_test_vec_x_lt(min_w, eps))
  {
    screen_box = *(vec4f *)v_screen_full_screen;
    return -1;
  }

  vec4f min0, max0, min1, max1;
  v_is_screen_project_minmax(cs0, min0, max0);
  v_is_screen_project_minmax(cs1, min1, max1);
  return v_is_screen_finish(v_min(min0, min1), v_max(max0, max1), threshold, screen_box);
}

//universal visibility function (accepts worldviewproj matrix)

///return zero if not visible in frustum or if bbox screen size is smaller, than threshold. not zero, otherwise
// also, screen_box is minX, maxX, minY, maxY - in clipspace coordinates  (-1, -1) .. (1,1)

VECTORCALL VECMATH_INLINE int v_screen_size_b(vec3f box2_xyXY, vec3f threshold, vec4f &screen_box, mat44f_cref clip)
{
  // get aabb points (SoA); a single z-slice, box2_xyXY is a 2D box
  vec4f minmax_x = v_perm_xzxz(box2_xyXY);
  vec4f minmax_y = v_perm_yyww(box2_xyXY);

  vec4f points_cs_0[4];
  v_is_transform_points_4(points_cs_0, minmax_x, minmax_y, clip);
  vec4f points_cs_0_negw = v_neg(points_cs_0[3]);

  vec4f *cs = points_cs_0;
  if (!v_is_merge_planes_nout(v_cmp_gt(cs[0], points_cs_0_negw),  // -w <= x
        v_cmp_gt(cs[3], cs[0]),                                  //  x <= w
        v_cmp_gt(cs[1], points_cs_0_negw),                       // -w <= y
        v_cmp_gt(cs[3], cs[1]),                                  //  y <= w
        v_cmp_gt(cs[2], v_zero()),                               //  0 <= z
        v_cmp_gt(cs[3], cs[2])))                                 //  z <= w
    return 0; // outside at least one plane

  vec4f eps = *(vec4f*)v_screen_div_eps;
  if (!v_test_all_bits_ones(v_cmp_gt(points_cs_0[3], eps)))
  {
    screen_box = *(vec4f*)v_screen_full_screen;
    return -1;
  }

  vec4f minXY, maxXY;
  v_is_screen_project_minmax(points_cs_0, minXY, maxXY);
  return v_is_screen_finish(minXY, maxXY, threshold, screen_box);
}


// estimated dist
//.x - closest dist (tmin), .y farthest dist (tmax)
// both can be negative
// ray intersects box if tmax >= max(ray.tmin, tmin) && tmin < ray.tmax
VECTORCALL VECMATH_INLINE  vec4f v_ray_box_intersect_dist_est(vec3f bmin, vec3f bmax, vec3f ray_origin, vec3f ray_dir, vec3f is_empty_box)
{
  vec3f invRay = v_rcp_est(ray_dir);//we can't use 1/x or v_rcp, as we have to ensure that invRay has infs if rayDir is 0 in some direction
  vec3f firstPlaneIntersect  = v_mul(v_sub(bmin, ray_origin), invRay);
  vec3f secondPlaneIntersect = v_mul(v_sub(bmax, ray_origin), invRay);
  vec3f furthestPlane = v_max(firstPlaneIntersect, secondPlaneIntersect);
  vec3f closestPlane = v_min(firstPlaneIntersect, secondPlaneIntersect);
  vec3f furthestDistance = v_hmin3(furthestPlane);
  vec3f closestDistance = v_hmax3(v_sel(closestPlane, V_C_INF, is_empty_box)); // closest dist invalidation will be enough to fail intersection
  return v_perm_xaxa(closestDistance, furthestDistance);
}

//.x - closest dist (tmin), .y farthest dist (tmax)
// both can be negative
// ray intersects box if tmax >= max(ray.tmin, tmin) && tmin <= ray.tmax
VECTORCALL VECMATH_INLINE  vec4f v_ray_box_intersect_dist(vec3f bmin, vec3f bmax, vec3f ray_origin, vec3f ray_dir, vec3f is_empty_box)
{
  vec3f invRay = v_rcp_safe(ray_dir, V_C_MAX_VAL);
  vec3f firstPlaneIntersect = v_mul(v_sub(bmin, ray_origin), invRay);
  vec3f secondPlaneIntersect = v_mul(v_sub(bmax, ray_origin), invRay);
  vec3f furthestPlane = v_max(firstPlaneIntersect, secondPlaneIntersect);
  vec3f closestPlane = v_min(firstPlaneIntersect, secondPlaneIntersect);
  vec3f furthestDistance = v_hmin3(furthestPlane);
  vec3f closestDistance = v_hmax3(v_sel(closestPlane, V_C_INF, is_empty_box));
  return v_perm_xaxa(closestDistance, furthestDistance);
}

// test that tmax_x intersect segment [lmin; lmax] and update it to max(0, lmin) if yes
VECTORCALL VECMATH_INLINE  bool v_segment_test_internal(vec3f lmin_lmax, vec3f& tmax_x)
{
  vec4f clampedMin = v_max(lmin_lmax, v_zero());
  vec4f isect = v_and(v_cmp_ge(v_splat_y(lmin_lmax), clampedMin), v_cmp_ge(tmax_x, lmin_lmax));
  tmax_x = v_sel(tmax_x, clampedMin, isect);
  return v_extract_xi(v_cast_vec4i(isect)) != 0;
}

VECTORCALL VECMATH_FINLINE bool v_ray_box_intersection(vec3f start, vec3f dir, vec3f &t_x, bbox3f box)
{
  vec3f isEmptyBox = v_cmp_gt(box.bmin, box.bmax);
  vec4f at = v_ray_box_intersect_dist(box.bmin, box.bmax, start, dir, isEmptyBox);
  return v_segment_test_internal(at, t_x);
}

VECTORCALL VECMATH_FINLINE bool v_ray_box_intersection_unsafe(vec3f start, vec3f dir, vec3f &t_x, bbox3f box)
{
  vec3f isEmptyBox = v_zero();
  vec4f at = v_ray_box_intersect_dist(box.bmin, box.bmax, start, dir, isEmptyBox);
  return v_segment_test_internal(at, t_x);
}

VECTORCALL VECMATH_FINLINE bool v_test_ray_box_intersection(vec3f start, vec3f dir, vec3f len_x, bbox3f box)
{
  vec3f isEmptyBox = v_cmp_gt(box.bmin, box.bmax);
  return v_segment_test_internal(v_ray_box_intersect_dist(box.bmin, box.bmax, start, dir, isEmptyBox), len_x);
}

VECTORCALL VECMATH_FINLINE bool v_test_ray_box_intersection_unsafe(vec3f start, vec3f dir, vec3f len_x, bbox3f box)
{
  vec3f isEmptyBox = v_zero();
  return v_segment_test_internal(v_ray_box_intersect_dist(box.bmin, box.bmax, start, dir, isEmptyBox), len_x);
}

VECTORCALL VECMATH_FINLINE bool v_test_segment_box_intersection(vec3f start, vec3f end, bbox3f box)
{
  vec4f t = V_C_ONE;
  return v_test_ray_box_intersection(start, v_sub(end, start), t, box);
}

// return -1 if no intersection found, or box side index in [0; 5] and output param 'at' in range [0.0; 1.0] for closest/furthest intersection
VECTORCALL inline int v_segment_box_intersection_side(vec3f start, vec3f end, const bbox3f& box, float& out_at_min, float& out_at_max)
{
  vec3f dir = v_sub(end, start);
  vec3f isDiv0 = v_is_unsafe_divisor(dir);
  vec3f t0 = v_div(v_sub(box.bmin, start), dir);
  vec3f t1 = v_div(v_sub(box.bmax, start), dir);

  // axis parallel to the segment: unbounded slab if start is inside it,
  // empty interval [+MAX, -MAX] (guaranteed miss) if outside
  vec3f outside = v_or(v_cmp_lt(start, box.bmin), v_cmp_gt(start, box.bmax));
  vec3f parMin = v_sel(v_neg(V_C_MAX_VAL), V_C_MAX_VAL, outside);
  vec3f parMax = v_sel(V_C_MAX_VAL, v_neg(V_C_MAX_VAL), outside);
  vec3f tmin3 = v_sel(v_min(t0, t1), parMin, isDiv0);
  vec3f tmax3 = v_sel(v_max(t0, t1), parMax, isDiv0);

  vec4f vEnter = v_hmax3(tmin3);
  vec4f vExit = v_hmin3(tmax3);
  vec4f validEnter = v_and(v_cmp_ge(vEnter, v_zero()), v_cmp_le(vEnter, V_C_ONE));
  vec4f validExit = v_and(v_cmp_ge(vExit, v_zero()), v_cmp_le(vExit, V_C_ONE));
  // no face crossing within the segment: miss, or segment fully inside the box
  if (v_check_xyzw_any_true(v_or(v_cmp_gt(vEnter, vExit), v_not(v_or(validEnter, validExit)))))
    return -1;

  // side of the first crossing: entry face if the entry is on the segment,
  // otherwise (start inside the box) the exit face
  vec4f vSide = v_sel(vExit, vEnter, validEnter);
  vec3f ref = v_sel(tmax3, tmin3, validEnter);
  out_at_min = v_extract_x(vSide);
  out_at_max = v_extract_x(v_sel(vEnter, vExit, validExit));

  int axisMask = v_truemask(v_cmp_eq(ref, vSide)) & 0b111;
#if defined(__clang__) || defined(__GNUC__)
  int axis = __builtin_ctz(axisMask);
#else
  unsigned long axis;
  _BitScanForward(&axis, axisMask);
#endif

  // prefer the bmin face on ties (t0 == t1), matching the original scan order
  int isMax = (~v_truemask(v_cmp_eq(ref, t0)) >> axis) & 1;
  return axis + isMax * 3;
}

VECTORCALL VECMATH_FINLINE bool v_test_ray_sphere_intersection(vec3f p0,
                                                               vec3f dir,
                                                               vec4f len,
                                                               vec4f sphere_center,
                                                               vec4f sphere_r2_x)
{
  vec3f ap = v_sub(sphere_center, p0);
  vec4f t = v_min(v_max(v_dot3(ap, dir), v_zero()), len);
  return v_test_vec_x_le(v_length3_sq(v_sub(v_mul(dir, t), ap)), sphere_r2_x);
}

VECTORCALL VECMATH_FINLINE bool v_test_segment_sphere_intersection(vec3f p0,
                                                                   vec3f p1,
                                                                   vec3f sphere_center,
                                                                   vec4f sphere_r2_x)
{
  vec3f dir = v_sub(p1, p0);
  vec4f lenSq = v_length3_sq(dir);
  vec3f ap = v_sub(sphere_center, p0);
  vec4f t = v_min(v_max(v_div(v_dot3(ap, dir), v_max(lenSq, v_splats(0.00001f))), v_zero()), V_C_ONE);
  return v_test_vec_x_le(v_length3_sq(v_sub(v_mul(dir, t), ap)), sphere_r2_x);
}

VECMATH_FINLINE bool v_ray_sphere_intersection_dist(vec3f start, vec3f dir, vec3f sphere_center, vec4f sphere_r2_x, vec4f &out_p1_t, vec4f &out_p2_t)
{
  vec3f p = v_sub(start, sphere_center);
  vec4f c = v_sub_x(v_length3_sq_x(p), sphere_r2_x);
  vec4f b = v_dot3_x(p, dir);
  c = v_add_x(c, c);
  c = v_add_x(c, c);
  b = v_add_x(b, b);
  vec4f d = v_sub_x(v_mul_x(b, b), c);
  if (v_test_vec_x_ge_0(d))
  {
    vec4f sq = v_sqrt_x(d);
    out_p1_t = v_mul_x(v_add_x(b, sq), v_set_x(-0.5f));
    out_p2_t = v_mul_x(v_sub_x(b, sq), v_set_x(-0.5f));
    return true;
  }
  return false;
}

VECMATH_FINLINE bool v_ray_sphere_intersection(vec3f start, vec3f dir, vec4f &t_x, vec3f sphere_center, vec4f sphere_r2_x)
{
  vec4f p1t, p2t;
  if (v_ray_sphere_intersection_dist(start, dir, sphere_center, sphere_r2_x, p1t, p2t) && v_test_vec_x_lt(p1t, t_x) && v_test_vec_x_ge_0(p2t))
  {
    t_x = v_max(p1t, v_zero());
    return true;
  }
  return false;
}

VECTORCALL VECMATH_FINLINE void v_mat33_make_from_33cu(mat33f &tmV, const float *const __restrict m33)
{
  // v_ldu_p3 keeps the trailing column read sanitizer-clean
  tmV.col0 = v_perm_xyzd(v_ldu(m33 + 0), v_zero());
  tmV.col1 = v_perm_xyzd(v_ldu(m33 + 3), v_zero());
  tmV.col2 = v_perm_xyzd(v_ldu_p3(m33 + 6), v_zero());
}

VECTORCALL VECMATH_FINLINE void v_mat33_make_from_33cu_safe(mat33f &tmV, const float *const __restrict m33)
{
  // the first two loads stay within the 9 floats, only the tail needs the exact form
  tmV.col0 = v_perm_xyzd(v_ldu(m33 + 0), v_zero());
  tmV.col1 = v_perm_xyzd(v_ldu(m33 + 3), v_zero());
  tmV.col2 = v_perm_xyzd(v_ldu_p3_safe(m33 + 6), v_zero());
}

VECTORCALL VECMATH_FINLINE void v_mat_44cu_from_mat44(float* __restrict m44, const mat44f& tm)
{
  v_stu(m44 + 0, tm.col0);
  v_stu(m44 + 4, tm.col1);
  v_stu(m44 + 8, tm.col2);
  v_stu(m44 + 12, tm.col3);
}

VECTORCALL VECMATH_FINLINE void v_mat_44ca_from_mat44(float* __restrict m44, const mat44f& tm)
{
  v_st(m44 + 0, tm.col0);
  v_st(m44 + 4, tm.col1);
  v_st(m44 + 8, tm.col2);
  v_st(m44 + 12, tm.col3);
}

//mat44f from unaligned 4x4 matrix
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_44cu(mat44f &tmV, const float *const __restrict m44)
{
  tmV.col0 = v_ldu(m44+0);
  tmV.col1 = v_ldu(m44+4);
  tmV.col2 = v_ldu(m44+8);
  tmV.col3 = v_ldu(m44+12);
}

//mat44f from aligned 4x4 matrix
VECTORCALL VECMATH_FINLINE void v_mat44_make_from_44ca(mat44f &tmV, const float *const __restrict m44)
{
  tmV.col0 = v_ld(m44+0);
  tmV.col1 = v_ld(m44+4);
  tmV.col2 = v_ld(m44+8);
  tmV.col3 = v_ld(m44+12);
}

VECTORCALL VECMATH_FINLINE vec4f v_div_est(vec4f a, vec4f b) {return v_mul(a, v_rcp_est(b));}

#define POLY0(x, c0) v_splats(c0)
#define POLY1(x, c0, c1) v_add(v_mul(POLY0(x, c1), x), v_splats(c0))
#define POLY2(x, c0, c1, c2) v_add(v_mul(POLY1(x, c1, c2), x), v_splats(c0))
#define POLY3(x, c0, c1, c2, c3) v_add(v_mul(POLY2(x, c1, c2, c3), x), v_splats(c0))
#define POLY4(x, c0, c1, c2, c3, c4) v_add(v_mul(POLY3(x, c1, c2, c3, c4), x), v_splats(c0))
#define POLY5(x, c0, c1, c2, c3, c4, c5) v_add(v_mul(POLY4(x, c1, c2, c3, c4, c5), x), v_splats(c0))

#define EXP_DEF_PART\
   vec4i ipart;\
   vec4f fpart, expipart, expfpart;\
   x = v_min(x, v_splats( 129.00000f));\
   x = v_max(x, v_splats(-126.99999f));\
   ipart = v_cvt_roundi(v_sub(x, V_C_HALF));\
   fpart = v_sub(x, v_cvt_vec4f(ipart));\
   expipart = v_cast_vec4f(v_slli(v_addi(ipart, v_splatsi(127)), 23));\

   /* minimax polynomial fit of 2**x, in range [-0.5, 0.5[ */
VECTORCALL VECMATH_INLINE  vec4f v_exp2_est_p5(vec4f x)
{
  EXP_DEF_PART
  expfpart = POLY5(fpart, 9.9999994e-1f, 6.9315308e-1f, 2.4015361e-1f, 5.5826318e-2f, 8.9893397e-3f, 1.8775767e-3f);
  return v_mul(expipart, expfpart);
}

VECTORCALL VECMATH_INLINE  vec4f v_exp2_est_p4(vec4f x)
{
  EXP_DEF_PART
  expfpart = POLY4(fpart, 1.0000026f, 6.9300383e-1f, 2.4144275e-1f, 5.2011464e-2f, 1.3534167e-2f);
  return v_mul(expipart, expfpart);
}
VECTORCALL VECMATH_INLINE  vec4f v_exp2_est_p3(vec4f x)
{
  EXP_DEF_PART
  expfpart = POLY3(fpart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
  return v_mul(expipart, expfpart);
}
VECTORCALL VECMATH_INLINE  vec4f v_exp2_est_p2(vec4f x)
{
  EXP_DEF_PART
  expfpart = POLY2(fpart, 1.0017247f, 6.5763628e-1f, 3.3718944e-1f);
  return v_mul(expipart, expfpart);
}

VECTORCALL VECMATH_INLINE  vec4f v_exp2(vec4f x)
{
  vec4i ipart;
  vec4f fpart, expipart, expfpart;
  x = v_min(x, v_splats( 129.00000f));
  x = v_max(x, v_splats(-126.99999f));
  ipart = v_cvt_roundi(v_sub(x, V_C_HALF_MINUS_EPS));
  fpart = v_sub(x, v_cvt_vec4f(ipart));
  expipart = v_cast_vec4f(v_slli(v_addi(ipart, v_splatsi(127)), 23));
  expfpart = POLY5(fpart, 9.9999994e-1f, 6.9315308e-1f, 2.4015361e-1f, 5.5826318e-2f, 8.9893397e-3f, 1.8775767e-3f);
  return v_sel(v_mul(expipart, expfpart), expipart, v_cmp_eq(fpart, v_zero()));//ensure that exp2(int) = 2^int
}

#undef EXP_DEF_PART

#define LOG_DEF_PART\
   static const vec4i exp = v_splatsi(0x7F800000);\
   static const vec4i mant = v_splatsi(0x007FFFFF);\
   vec4i i = v_cast_vec4i(x);\
   vec4f e = v_cvt_vec4f(v_subi(v_srli(v_andi(i, exp), 23), v_splatsi(127)));\
   vec4f m = v_or(v_cast_vec4f(v_andi(i, mant)), V_C_ONE);\
   vec4f p;

VECTORCALL VECMATH_INLINE  vec4f v_log2_est_p5(vec4f x)
{
   LOG_DEF_PART
   p = POLY5( m, 3.1157899f, -3.3241990f, 2.5988452f, -1.2315303f,  3.1821337e-1f, -3.4436006e-2f);
   /* This effectively increases the polynomial degree by one, but ensures that log2(1) == 0*/
   return v_add(v_mul(p, v_sub(m, V_C_ONE)), e);
}

VECTORCALL VECMATH_INLINE  vec4f v_log2_est_p4(vec4f x)
{
   LOG_DEF_PART
   p = POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f,
                1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
   return v_add(v_mul(p, v_sub(m, V_C_ONE)), e);
}
VECTORCALL VECMATH_INLINE  vec4f v_log2_est_p3(vec4f x)
{
   LOG_DEF_PART
   p = POLY3(m, 2.61761038894603480148f, -1.75647175389045657003f, 0.688243882994381274313f, -0.107254423828329604454f);
   return v_add(v_mul(p, v_sub(m, V_C_ONE)), e);
}
VECTORCALL VECMATH_INLINE  vec4f v_log2_est_p2(vec4f x)
{
   LOG_DEF_PART
   p = POLY2(m, 2.28330284476918490682f, -1.04913055217340124191f, 0.204446009836232697516f);
   return v_add(v_mul(p, v_sub(m, V_C_ONE)), e);
}

#undef LOG_DEF_PART

VECTORCALL VECMATH_INLINE  vec4f v_exp2_est(vec4f x) {return v_exp2_est_p4(x);}
VECTORCALL VECMATH_INLINE  vec4f v_log2_est(vec4f x) {return v_log2_est_p4(x);}

VECTORCALL VECMATH_FINLINE vec4f v_pow_est(vec4f x, vec4f y)
{
   return v_exp2_est_p4(v_mul(v_log2_est_p5(x), y));
}
//natural log
VECTORCALL VECMATH_FINLINE vec4f v_log(vec4f x){return v_mul(v_log2_est_p5(x), v_splats(0.6931471805599453f));}
//natural exponent
VECTORCALL VECMATH_FINLINE vec4f v_exp(vec4f x){return v_exp2(v_mul(x, v_splats(1.4426950408889634073599f)));}//log2(e)
//safer pow. checks for y == 0
VECTORCALL VECMATH_FINLINE vec4f v_pow(vec4f x, vec4f y)
{
   vec4f ret = v_exp2(v_mul(v_log2_est_p5(x), y));
   ret = v_sel(ret, V_C_ONE, v_cmp_eq(y, v_zero()));
   return ret;
}

#undef POLY0
#undef POLY1
#undef POLY2
#undef POLY3
#undef POLY4
#undef POLY5

VECTORCALL VECMATH_FINLINE plane3f v_make_plane_dir(vec3f p0, vec3f dir0, vec3f dir1)
{
  vec3f n = v_cross3(dir0, dir1);
  return v_make_plane_norm(p0, n);
}
VECTORCALL VECMATH_FINLINE plane3f v_make_plane(vec3f p0, vec3f p1, vec3f p2)
{
  return v_make_plane_dir(p0, v_sub(p1,p0), v_sub(p2, p0));
}

VECTORCALL VECMATH_FINLINE plane3f v_make_plane_norm(vec3f p0, vec3f norm)
{
  vec3f d = v_neg(v_dot3(norm, p0));
  return v_perm_xyzd(norm, d);
}

VECTORCALL VECMATH_FINLINE plane3f v_transform_plane(plane3f plane, mat44f_cref transform)
{
  // simple way
  vec3f p0 = v_mul(plane, v_neg(v_splat_w(plane))); // p0 = norm * -d
  return v_make_plane_norm(v_mat44_mul_vec3p(transform, p0), v_mat44_mul_vec3v(transform, plane));
}

VECTORCALL VECMATH_FINLINE vec4f v_distance_sq_to_bbox_x(vec4f bmin, vec4f bmax, vec4f c)
{
  vec4f diff = v_max(v_max(v_sub(bmin, c), v_sub(c, bmax)), v_zero());
  return v_dot3_x(diff, diff);
}

VECTORCALL VECMATH_FINLINE vec4f v_distance_sq_to_bbox_2d_x(vec4f bmin, vec4f bmax, vec4f c)
{
  vec4f diff = v_max(v_max(v_sub(bmin, c), v_sub(c, bmax)), v_zero());
  vec3f diffSq = v_mul(diff, diff);
  return v_add_x(diffSq, v_splat_z(diffSq));
}

VECTORCALL VECMATH_FINLINE vec4f v_distance_sq_box_to_box_x(vec3f centerA, vec3f extentA, vec3f centerB, vec3f extentB)
{
  vec3f axisDistances = v_max(v_sub(v_abs_diff(centerB, centerA), v_add(extentA, extentB)), v_zero());
  return v_dot3_x(axisDistances, axisDistances);
}

VECTORCALL VECMATH_FINLINE vec4f v_distance_sq_box_to_box_x_scaled(vec3f centerA, vec3f extentA, vec3f centerB, vec3f extentB, vec3f scale)
{
  vec3f axisDistances = v_max(v_sub(v_abs_diff(centerB, centerA), v_add(extentA, extentB)), v_zero());
  axisDistances = v_mul(axisDistances, scale);
  return v_dot3_x(axisDistances, axisDistances);
}

VECTORCALL VECMATH_FINLINE vec4f v_max_dist_sq_to_bbox_x(vec4f bmin, vec4f bmax, vec4f c)
{
  vec4f diff = v_max(v_max(v_sub(c, bmin), v_sub(bmax, c)), v_zero());
  return v_dot3_x(diff, diff);
}

VECTORCALL VECMATH_FINLINE vec3f v_closest_bbox_point(vec3f bmin, vec3f bmax, vec3f c)
{
  c = v_sel(c, bmin, v_cmp_gt(bmin, c));
  c = v_sel(c, bmax, v_cmp_gt(c, bmax));
  return c;
}


//returns point on infinite line which is closes to point
VECTORCALL VECMATH_FINLINE vec3f v_closest_point_on_line(vec3f point, vec3f a, vec3f dir)
{
  vec3f t = v_dot3(v_sub(point, a), dir);// t param along line
  return v_madd(dir, t, a);//pt is point on line
}

VECTORCALL VECMATH_FINLINE vec4f v_distance_to_line_x(vec3f point, vec3f a, vec3f dir)
{
  vec3f pa = v_sub(point, a);
  vec3f t = v_dot3(pa, dir);// t param along line
  return v_length3_x(v_sub(pa, v_mul(dir, t)));
}

VECTORCALL VECMATH_FINLINE vec4f v_distance_to_seg_x(vec3f point, vec3f a, vec3f b)
{
  vec3f pt = v_closest_point_on_segment(point, a, b);
  return v_length3_x(v_sub(point, pt));
}


#if _TARGET_HAS_FC16
VECTORCALL VECMATH_FINLINE vec4f v_fc16_half_to_float(vec4i v) {return v_fc16_half_to_float_lo(v_packus(v));}
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_rtne(vec4f v) {return v_cvt_lo_ush_vec4i(v_fc16_float_to_half_rtne_lo(v));}
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_down(vec4f v) {return v_cvt_lo_ush_vec4i(v_fc16_float_to_half_down_lo(v));}
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_up(vec4f v) {return v_cvt_lo_ush_vec4i(v_fc16_float_to_half_up_lo(v));}
VECTORCALL VECMATH_FINLINE vec4i v_fc16_float_to_half_trunc(vec4f v) {return v_cvt_lo_ush_vec4i(v_fc16_float_to_half_trunc_lo(v));}

// tunc is most closer in results to v_float_to_half
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half(vec4f a) {return v_fc16_float_to_half_trunc(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_trunc(vec4f a) {return v_fc16_float_to_half_trunc(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_specials(vec4f a) {return v_fc16_float_to_half_trunc(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_rtne(vec4f a) {return v_fc16_float_to_half_rtne(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_up(vec4f a) {return v_fc16_float_to_half_up(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_down(vec4f a) {return v_fc16_float_to_half_down(a);}

VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_trunc_lo(vec4f a) {return v_fc16_float_to_half_trunc_lo(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_specials_lo(vec4f a) {return v_fc16_float_to_half_trunc_lo(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_rtne_lo(vec4f a) {return v_fc16_float_to_half_rtne_lo(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_up_lo(vec4f a) {return v_fc16_float_to_half_up_lo(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_down_lo(vec4f a) {return v_fc16_float_to_half_down_lo(a);}

VECTORCALL VECMATH_FINLINE vec4f v_half_to_float(vec4i a) {return v_fc16_half_to_float(a);}
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials(vec4i a) {return v_fc16_half_to_float(a);}
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_lo(vec4i a) {return v_fc16_half_to_float_lo(a);}
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials_lo(vec4i a) {return v_fc16_half_to_float_lo(a);}
#else

//https://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
//https://gist.github.com/rygorous/2156668
//doesn't round correctly, basically truncs to zero
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_trunc(vec4f f)
{
  vec4i mask_fabs  = v_splatsi(0x7fffffff);
  vec4i c_f16max   = v_splatsi((127 + 16) << 23);
  vec4i c_magic    = v_splatsi(15 << 23);

  vec4f mabs        = v_cast_vec4f(mask_fabs);
  vec4f fabs        = v_and(mabs, f);
  vec4f justsign    = v_xor(f, fabs);

  vec4f f16max      = v_cast_vec4f(c_f16max);
  vec4f clamped     = v_min(f16max, fabs);
  vec4f scaled      = v_mul(clamped, v_cast_vec4f(c_magic));
  vec4f merged      = scaled;

  vec4i shifted     = v_srli(v_cast_vec4i(merged), 13);
  vec4i signshifted = v_srli(v_cast_vec4i(justsign), 16);
  vec4i final       = v_ori(shifted, signshifted);

  // ~15 SSE2 ops
  return final;
}

VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half(vec4f f) {return v_sw_float_to_half_trunc(f);}

//handles infs/nans, doesn't round correctly (truncs)
//it (incorrectly) translates some of sNaNs into infinity, so be careful!
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_specials(vec4f f)
{
  vec4i mask_fabs  = v_splatsi(0x7fffffff);
  vec4i c_f32infty = v_splatsi((255 << 23));
  vec4i c_expinf   = v_splatsi((255 ^ 31) << 23);
  vec4i c_f16max   = v_splatsi((127 + 16) << 23);
  vec4i c_magic    = v_splatsi(15 << 23);

  vec4f mabs        = v_cast_vec4f(mask_fabs);
  vec4f fabs        = v_and(mabs, f);
  vec4f justsign    = v_xor(f, fabs);

  vec4f f16max      = v_cast_vec4f(c_f16max);
  vec4f expinf      = v_cast_vec4f(c_expinf);
  vec4f infnancase  = v_xor(expinf, fabs);
  vec4f clamped     = v_min(f16max, fabs);
  vec4f scaled      = v_mul(clamped, v_cast_vec4f(c_magic));

  vec4f b_notnormal = v_not(v_cmp_lt(fabs, v_cast_vec4f(c_f32infty)));//can be replaced with one _mm_cmpnlt_ps
  vec4f merge1      = v_and(infnancase, b_notnormal);
  vec4f merge2      = v_andnot(b_notnormal, scaled);
  vec4f merged      = v_or(merge1, merge2);

  vec4i shifted     = v_srli(v_cast_vec4i(merged), 13);
  vec4i signshifted = v_srli(v_cast_vec4i(justsign), 16);
  vec4i final       = v_ori(shifted, signshifted);
  return final;
}

// round-to-nearest-even, doesn't handle nans
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_rtne(vec4f f)
{
  vec4i mask_sign       = v_splatsi(0x80000000u);
  vec4i c_f16max        = v_splatsi((127 + 16) << 23); // all FP32 values >=this round to +inf
  vec4i c_infty_as_fp16 = v_splatsi(0x7c00);
  vec4i c_min_normal    = v_splatsi((127 - 14) << 23); // smallest FP32 that yields a normalized FP16
  vec4i c_subnorm_magic = v_splatsi(((127 - 15) + (23 - 10) + 1) << 23);
  vec4i c_normal_bias   = v_splatsi(0xfff - ((127 - 15) << 23)); // adjust exponent and add mantissa rounding

  vec4f msign       = v_cast_vec4f(mask_sign);
  vec4f justsign    = v_and(msign, f);
  vec4f absf        = v_xor(f, justsign);
  vec4i absf_int    = v_cast_vec4i(absf); // the cast is "free" (extra bypass latency, but no thruput hit)
  vec4i f16max      = c_f16max;

  vec4i b_isregular = v_cmp_gti(f16max, absf_int); // (sub)normalized or special?
  vec4i inf_or_nan  = c_infty_as_fp16; // output for specials

  vec4i min_normal  = c_min_normal;
  vec4i b_issub     = v_cmp_gti(min_normal, absf_int);

  // "result is subnormal" path
  vec4f subnorm1    = v_add(absf, v_cast_vec4f(c_subnorm_magic)); // magic value to round output mantissa
  vec4i subnorm2    = v_subi(v_cast_vec4i(subnorm1), c_subnorm_magic); // subtract out bias

  // "result is normal" path
  vec4i mantoddbit  = v_slli(absf_int, 31 - 13); // shift bit 13 (mantissa LSB) to sign
  vec4i mantodd     = v_srai(mantoddbit, 31); // -1 if FP16 mantissa odd, else 0

  vec4i round1      = v_addi(absf_int, c_normal_bias);
  vec4i round2      = v_subi(round1, mantodd); // if mantissa LSB odd, bias towards rounding up (RTNE)
  vec4i normal      = v_srli(round2, 13); // rounded result

  // combine the two non-specials
  vec4i nonspecial  = v_ori(v_andi(subnorm2, b_issub), v_andnoti(b_issub, normal));

  // merge in specials as well
  vec4i joined      = v_ori(v_andi(nonspecial, b_isregular), v_andnoti(b_isregular, inf_or_nan));

  vec4i sign_shift  = v_srli(v_cast_vec4i(justsign), 16);
  vec4i final       = v_ori(joined, sign_shift);

  // ~28 SSE2 ops
  return final;
}

// fp16 denormals (zero exponent field): mantissa * 2^-24 through exact int->float and an
// exact power-of-two scale. The magic-multiply path below would push them through FLOAT
// denormal arithmetic, making the result depend on the thread's FTZ/DAZ state (flushed to
// zero in the FTZ+DAZ mode init_math() sets) and disagree with the F16C hardware path,
// which converts fp16 denormals exactly regardless of MXCSR.
VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float_subnorm(vec4i h)
{
  return v_mul(v_cvti_vec4f(v_andi(h, v_splatsi(0x3ff))), v_cast_vec4f(v_splatsi(103 << 23))); // mant * 2^-24, both exact
}

VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float(vec4i h)
{
  const vec4f magic = v_cast_vec4f(v_splatsi((254 - 15) << 23));
  vec4i oi = v_srl(v_sll(h, 17), 4); // exponent/mantissa bits
  vec4f of = v_mul(v_cast_vec4f(oi), magic);// exponent adjust; exact for normals (result >= 2^-14)
  vec4i isSub = v_cmp_eqi(v_andi(h, v_splatsi(0x7c00)), v_zeroi());
  of = v_sel(of, v_sw_half_to_float_subnorm(h), v_cast_vec4f(isSub));
  return v_or(of, v_cast_vec4f(v_sll(v_srl(h, 15), 31)));
}

VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float_specials(vec4i h)
{
  const vec4f magic = v_cast_vec4f(v_splatsi((254 - 15) << 23));
  const vec4f was_infnan = v_cast_vec4f(v_splatsi((127 + 16) << 23));
  vec4i oi = v_srl(v_sll(h, 17), 4); // exponent/mantissa bits
  vec4f of = v_mul(v_cast_vec4f(oi), magic);// exponent adjust; exact for normals (result >= 2^-14)
  vec4i isSub = v_cmp_eqi(v_andi(h, v_splatsi(0x7c00)), v_zeroi());
  of = v_sel(of, v_sw_half_to_float_subnorm(h), v_cast_vec4f(isSub));
  of = v_or(of, v_and(v_cmp_ge(of, was_infnan), v_cast_vec4f(v_splatsi(255 << 23))));
  return v_or(of, v_cast_vec4f(v_sll(v_srl(h, 15), 31)));
}

VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float_lo(vec4i a)
{
  return v_sw_half_to_float(v_cvt_lo_ush_vec4i(a));
}

VECTORCALL VECMATH_FINLINE vec4f v_sw_half_to_float_specials_lo(vec4i a)
{
  return v_sw_half_to_float_specials(v_cvt_lo_ush_vec4i(a));
}

//not checking for NANs
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_up(vec4f a)
{
  vec4i c = v_sw_float_to_half_rtne(a);
  vec4f incMask = v_cmp_lt(v_half_to_float(c), a);
  vec4i signMask = v_srli(c, 15); signMask = v_subi(v_addi(signMask,signMask), v_splatsi(1));
  return v_seli(c, v_subi(c, signMask), v_cast_vec4i(incMask));
}

//not checking for NANs
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_down(vec4f a)
{
  vec4i c = v_sw_float_to_half_rtne(a);//
  vec4f decMask = v_cmp_gt(v_half_to_float(c), a);
  vec4i signMask = v_srli(c, 15); signMask = v_subi(v_addi(signMask,signMask), v_splatsi(1));
  return v_seli(c, v_addi(c, signMask), v_cast_vec4i(decMask));
}

VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_specials_lo(vec4f v) {return v_packus(v_sw_float_to_half_specials(v));}
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_trunc_lo(vec4f v) {return v_packus(v_sw_float_to_half_trunc(v));}
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_down_lo(vec4f v) {return v_packus(v_sw_float_to_half_down(v));}
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_up_lo(vec4f v) {return v_packus(v_sw_float_to_half_up(v));}
VECTORCALL VECMATH_FINLINE vec4i v_sw_float_to_half_rtne_lo(vec4f v) {return v_packus(v_sw_float_to_half_rtne(v));}

// tunc is most closer in results to v_float_to_half
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half(vec4f a) {return v_sw_float_to_half_trunc(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_trunc(vec4f a) {return v_sw_float_to_half_trunc(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_specials(vec4f a) {return v_sw_float_to_half_trunc(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_rtne(vec4f a) {return v_sw_float_to_half_rtne(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_up(vec4f a) {return v_sw_float_to_half_up(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_down(vec4f a) {return v_sw_float_to_half_down(a);}

VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_trunc_lo(vec4f a) {return v_sw_float_to_half_trunc_lo(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_specials_lo(vec4f a) {return v_sw_float_to_half_trunc_lo(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_rtne_lo(vec4f a) {return v_sw_float_to_half_rtne_lo(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_up_lo(vec4f a) {return v_sw_float_to_half_up_lo(a);}
VECTORCALL VECMATH_FINLINE vec4i v_float_to_half_down_lo(vec4f a) {return v_sw_float_to_half_down_lo(a);}

VECTORCALL VECMATH_FINLINE vec4f v_half_to_float(vec4i a) {return v_sw_half_to_float(a);}
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials(vec4i a) {return v_sw_half_to_float(a);}
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_lo(vec4i a) {return v_sw_half_to_float_lo(a);}
VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials_lo(vec4i a) {return v_sw_half_to_float_lo(a);}
#endif

VECTORCALL VECMATH_FINLINE void v_float_to_half(uint16_t* __restrict m, const vec4f v)
{
  v_stui_half(m, v_float_to_half_trunc_lo(v));
}

VECTORCALL VECMATH_FINLINE vec4f v_half_to_float(const uint16_t* __restrict m)
{
  return v_half_to_float_lo(v_ldui_half(m));
}

VECTORCALL VECMATH_FINLINE vec4f v_half_to_float_specials(const uint16_t* __restrict m)
{
  return v_half_to_float_specials_lo(v_ldui_half(m));
}

VECMATH_FINLINE uint32_t v_float_to_byte ( vec4f x )
{
  vec4i y = v_cvt_roundi(x);
  y = v_packs(y);
  y = v_packus16(y, y);
  return v_extract_xi(y);
}

VECMATH_FINLINE vec4f v_byte_to_float ( uint32_t x )
{
  vec4i y = v_cvt_byte_vec4i(x);
  return v_cvt_vec4f(y);
}

VECTORCALL VECMATH_INLINE int v_test_triangle_sphere_intersection(vec3f A, vec3f B, vec3f C, vec4f sph_c, vec4f sph_r2_x)
{
  A = v_sub(A, sph_c);
  B = v_sub(B, sph_c);
  C = v_sub(C, sph_c);
  vec3f AB = v_sub(B, A);
  vec3f CA = v_sub(A, C);
  vec3f V = v_cross3(AB, CA);
  vec3f d = v_dot3_x(A, V);
  vec3f e = v_dot3_x(V, V);
  if (v_test_vec_x_gt(v_mul_x(d, d), v_mul_x(sph_r2_x, e)))
    return 0;
  vec3f aa = v_dot3(A, A);
  vec3f ab = v_dot3(A, B);
  vec3f ac = v_dot3(A, C);
  vec3f bb = v_dot3(B, B);
  vec3f bc = v_dot3(B, C);
  vec3f cc = v_dot3(C, C);
  if (!v_test_vec_x_eq_0(v_or(v_or(v_and(v_and(v_cmp_gt(bb, sph_r2_x), v_cmp_gt(ab, bb)), v_cmp_gt(bc, bb)),
                                   v_and(v_and(v_cmp_gt(cc, sph_r2_x), v_cmp_gt(ac, cc)), v_cmp_gt(bc, cc))),
                                   v_and(v_and(v_cmp_gt(aa, sph_r2_x), v_cmp_gt(ab, aa)), v_cmp_gt(ac, aa))))
     )
    return 0;

  vec3f BC = v_sub(C, B);
  vec3f d1 = v_sub(ab, aa);
  vec3f d2 = v_sub(bc, bb);
  vec3f d3 = v_sub(ac, cc);
  vec3f e1 = v_dot3(AB, AB);
  vec3f e2 = v_dot3(BC, BC);
  vec3f e3 = v_dot3(CA, CA);
  vec3f Q1 = v_sub(v_mul(A, e1), v_mul(d1, AB));
  vec3f Q2 = v_sub(v_mul(B, e2), v_mul(d2, BC));
  vec3f Q3 = v_sub(v_mul(C, e3), v_mul(d3, CA));
  vec3f QC = v_sub(v_mul(C, e1), Q1);
  vec3f QA = v_sub(v_mul(A, e2), Q2);
  vec3f QB = v_sub(v_mul(B, e3), Q3);

  return v_test_vec_x_eq_0(v_or(v_or(v_and(v_cmp_gt(v_dot3_x(Q1, Q1), v_mul_x(sph_r2_x, v_mul_x(e1, e1))), v_cmp_gt(v_dot3_x(Q1, QC), v_zero())),
                                     v_and(v_cmp_gt(v_dot3_x(Q2, Q2), v_mul_x(sph_r2_x, v_mul_x(e2, e2))), v_cmp_gt(v_dot3_x(Q2, QA), v_zero()))),
                                     v_and(v_cmp_gt(v_dot3_x(Q3, Q3), v_mul_x(sph_r2_x, v_mul_x(e3, e3))), v_cmp_gt(v_dot3_x(Q3, QB), v_zero()))));
}

VECTORCALL VECMATH_INLINE vec3f v_triangle_bounding_sphere_center(vec3f p1, vec3f p2, vec3f p3)
{
  vec3f edge1 = v_sub(p2, p1);
  vec3f edge2 = v_sub(p3, p1);
  vec3f edge3 = v_sub(p2, p3);

  vec4f nd1 = v_dot3(edge1, edge2);
  vec4f nd2 = v_dot3(edge3, edge1);
  vec4f nd3 = v_neg(v_dot3(edge2, edge3));
  vec3f d1, d2;
  vec4f mask1, mask;
  d1 = d2 = v_zero();
  mask1 = v_cmp_gt(v_zero(), nd1);
  d1 = v_and(mask1, p3);
  d2 = v_and(mask1, edge3);

  mask = v_cmp_gt(v_zero(), nd2);
  d1 = v_sel(d1, p1, mask);
  d2 = v_sel(d2, edge2, mask);
  mask1 = v_or(mask1, mask);

  mask = v_cmp_gt(v_zero(), nd3);
  d1 = v_sel(d1, p1, mask);
  d2 = v_sel(d2, edge1, mask);
  mask1 = v_or(mask1, mask);
  if (!v_test_vec_x_eqi_0(mask1))
    return v_add(d1, v_mul(d2, V_C_HALF));//radius = 0.5*length(d2);
  vec3f c1  = v_mul(nd2, nd3);
  vec3f c2  = v_mul(nd3, nd1);
  vec3f c3  = v_mul(nd1, nd2);
  vec3f cmul2   = v_add(v_add(c1, c2), c3);
  cmul2 = v_add(cmul2, cmul2);

  return v_div(v_add(
                     v_madd(p1, v_add(c2, c3), v_mul(p2, v_add(c3, c1))),
                     v_mul(p3, v_add(c1, c2))
                     ),
               cmul2);//r       = 0.5f * sqrt( fabsf(safediv((nd1 + nd2) * (nd2 + nd3) * (nd3 + nd1), c)) );
}

VECTORCALL VECMATH_INLINE bool v_is_point_in_triangle_2d(vec4f p, vec4f t1, vec4f t2, vec4f t3)
{
  vec3f t123y = v_perm_xyab(v_perm_xaxa(v_splat_y(t1), v_splat_y(t2)), v_splat_y(t3));
  vec3f t123x = v_perm_xyab(v_perm_xaxa(t1, t2), t3);
  vec3f t231y = v_perm_yzxw(t123y);
  vec3f t231x = v_perm_yzxw(t123x);
  vec3f a = v_sub(t231y, t123y);
  vec3f b = v_sub(t231x, t123x);
  vec3f c = v_sub(v_mul(b, t123y), v_mul(a, t123x));
  vec3f d = v_sub(v_mul(a, v_splat_x(p)), v_mul(b, v_splat_y(p)));
  vec3f e = v_add(c, d);
  int signMask = v_signmask(e) & (1 | 2 | 4);
  return signMask == 0 || signMask == (1 | 2 | 4);
}

//this is ~3 times faster for valid floats (not nans, infs, etc), than int(floorf())
VECTORCALL VECMATH_INLINE  int vec_floori(float x)
{
  return v_extract_xi(v_cvt_floori(v_splats(x)));
}

//this is ~2 times faster for valid floats (not nans, infs, etc), than int(floorf())
VECTORCALL VECMATH_INLINE  int vec_float_to_int(float x)
{
  return v_extract_xi(v_cvt_vec4i(v_splats(x)));
}

VECMATH_INLINE float invsqrt(float x) { return v_extract_x(v_rsqrt_x(v_set_x(x))); }

VECTORCALL VECMATH_FINLINE vec4i v_permi_xzac(vec4i xyzw, vec4i abcd) { return v_cast_vec4i(v_perm_xzac(v_cast_vec4f(xyzw), v_cast_vec4f(abcd))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xyab(vec4i xyzw, vec4i abcd) { return v_cast_vec4i(v_perm_xyab(v_cast_vec4f(xyzw), v_cast_vec4f(abcd))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xycd(vec4i xyzw, vec4i abcd) { return v_cast_vec4i(v_perm_xycd(v_cast_vec4f(xyzw), v_cast_vec4f(abcd))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xbzd(vec4i xyzw, vec4i abcd) { return v_cast_vec4i(v_perm_xayb(v_perm_xzxz(v_cast_vec4f(xyzw)), v_perm_ywyw(v_cast_vec4f(abcd)))); }
VECTORCALL VECMATH_FINLINE vec4i v_permi_xyzd(vec4i xyzw, vec4i abcd) { return v_cast_vec4i(v_perm_xyzd(v_cast_vec4f(xyzw), v_cast_vec4f(abcd))); }

VECTORCALL VECMATH_INLINE void v_get_bilinear_wrap_addr_pow2(vec4i &uv_idx, vec4f &uv_frac, vec4f uv_wrap, int size_bits, vec4f center_ofs)
{
  const int size = 1<<size_bits;
  vec4i dmapi = v_splatsi(size);
  vec4f dmap = v_cvt_vec4f(dmapi);
// don't use a pixel centered bilinear filtration because it creates shifts for different
  uv_wrap = v_mul(uv_wrap, dmap);
  uv_wrap = v_sub(uv_wrap, center_ofs);
  vec4f floored = v_floor(uv_wrap);
  vec4i uvIdx = v_cvt_vec4i(floored);
  uv_frac = v_sub(uv_wrap, floored);

  vec4i bitMask = v_splatsi(size - 1);
  uvIdx = v_andi(bitMask, uvIdx); // It is actually needed due to floating point imprecision
  uvIdx = v_permi_xyab(uvIdx, v_andi(v_addi(uvIdx, V_CI_1), bitMask));

  uv_idx = v_addi(v_permi_xzxz(uvIdx), v_permi_yyww(v_sll(uvIdx, size_bits)));
}

VECTORCALL VECMATH_INLINE void v_get_bilinear_wrap_addr(vec4i &uv_idx, vec4f &uv_frac, vec4f uv_wrap, int size, vec4f center_ofs)
{
  vec4i dmapi = v_splatsi(size);
  vec4f dmap = v_cvt_vec4f(dmapi);
// don't use a pixel centered bilinear filtration because it creates shifts for different
  uv_wrap = v_sub(uv_wrap, v_div(center_ofs, dmap));
  uv_wrap = v_mul(v_sub(uv_wrap, v_floor(uv_wrap)), dmap);
  vec4i uvIdx = v_cvt_vec4i(uv_wrap);
  uv_frac = v_sub(uv_wrap, v_cvt_vec4f(uvIdx));

  uvIdx = v_seli(uvIdx, v_zeroi(), v_cmp_lti(uvIdx, dmapi)); // It is actually needed due to floating point imprecision
  vec4i uvIdx1 = v_addi(uvIdx, V_CI_1);
  vec4i uvNext = v_seli(uvIdx1, v_zeroi(), v_cmp_lti(uvIdx1, dmapi));
  uvIdx = v_permi_xyab(uvIdx, uvNext);

  uv_idx = v_addi(v_permi_xzxz(uvIdx), v_permi_yyww(v_muli(uvIdx, dmapi)));
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
