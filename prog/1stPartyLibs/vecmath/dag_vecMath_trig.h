//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>

#define _MINUS_DP1         -0.78515625f
#define _MINUS_DP2         -2.4187564849853515625e-4f
#define _MINUS_DP3         -3.77489497744594108e-8f

#define _SINCOS_CC0        -0.0013602249f
#define _SINCOS_CC1         0.0416566950f
#define _SINCOS_CC2        -0.4999990225f
#define _SINCOS_SC0        -0.0001950727f
#define _SINCOS_SC1         0.0083320758f
#define _SINCOS_SC2        -0.1666665247f
#define _SINCOS_KC1         1.57079625129f
#define _SINCOS_KC2         7.54978995489e-8f

#define _TAN_B0             1.0e-4f
#define _TAN_B1             1.0e-35f
#define _TAN_P0             9.38540185543e-3f
#define _TAN_P1             3.11992232697e-3f
#define _TAN_P2             2.44301354525e-2f
#define _TAN_P3             5.34112807005e-2f
#define _TAN_P4             1.33387994085e-1f
#define _TAN_P5             3.33331568548e-1f

#define _ASIN_P0            4.2163199048e-2f
#define _ASIN_P1            2.4181311049e-2f
#define _ASIN_P2            4.5470025998e-2f
#define _ASIN_P3            7.4953002686e-2f
#define _ASIN_P4            1.6666752422e-1f

#define _ATAN_Q0            2.414213562373095f
#define _ATAN_Q1            0.414213562373095f
#define _ATAN_P0            8.05374449538e-2f
#define _ATAN_P1           -1.38776856032e-1f
#define _ATAN_P2            1.99777106478e-1f
#define _ATAN_P3           -3.33329491539e-1f

#define _ATAN_EST_T0       -0.91646118527267623468e-1f
#define _ATAN_EST_T1       -0.13956945682312098640e1f
#define _ATAN_EST_T2       -0.94393926122725531747e2f
#define _ATAN_EST_T3        0.12888383034157279340e2f

#define _ATAN_EST_S0        0.12797564625607904396e1f
#define _ATAN_EST_S1        0.21972168858277355914e1f
#define _ATAN_EST_S2        0.68193064729268275701e1f
#define _ATAN_EST_S3        0.28205206687035841409e2f

VECTORCALL VECMATH_FINLINE vec4f v_deg_to_rad(vec4f deg)
{
  return v_mul(deg, v_splats(float(M_PI / 180.0)));
}

VECTORCALL VECMATH_FINLINE vec4f v_rad_to_deg(vec4f rad)
{
  return v_mul(rad, v_splats(float(180.0 / M_PI)));
}

VECTORCALL VECMATH_FINLINE vec4f v_norm_s_angle(vec4f angle)
{
  return v_sub(angle, v_mul(v_floor(v_mul(v_add(angle, V_C_PI), v_rcp(V_C_TWOPI))), V_C_TWOPI));
}

VECTORCALL VECMATH_FINLINE vec4f v_dir_to_angles(vec3f dir)
{
  vec4f z = v_splat_z(dir);
  vec4f d = v_length2_x(v_perm_xzxz(dir));
  vec4f y = v_perm_xaxa(z, v_splat_y(dir));
  vec4f x = v_perm_xaxa(dir, d);
  vec4f a = v_atan2(y, x);
  return v_xor(a, v_cast_vec4f(v_seti_x(0x80000000)));
}

VECTORCALL VECMATH_FINLINE vec3f v_angles_to_dir(vec4f angles)
{
  vec4f s, c;
  vec4f a = v_xor(angles, v_cast_vec4f(v_seti_x(0x80000000)));
  v_sincos(a, s, c);
  return v_make_vec3f(v_extract_x(c) * v_extract_y(c), v_extract_y(s), v_extract_x(s) * v_extract_y(c));
}

VECTORCALL VECMATH_FINLINE vec4f v_sin_from_cos(vec4f c) { return v_sqrt(v_sub(V_C_ONE, v_sqr(c))); }
VECTORCALL VECMATH_FINLINE vec4f v_cos_from_sin(vec4f s) { return v_sin_from_cos(s); }
VECTORCALL VECMATH_FINLINE vec4f v_sin_from_cos_x(vec4f c) { return v_sqrt_x(v_sub_x(V_C_ONE, v_sqr_x(c))); }
VECTORCALL VECMATH_FINLINE vec4f v_cos_from_sin_x(vec4f s) { return v_sin_from_cos_x(s); }

// calculates 4 in ~2.14x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_FINLINE void v_sincos(vec4f ang, vec4f& s, vec4f& c)
{
  vec4f xl, xl2, xl3;
  vec4i q;
  vec4i offsetSin, offsetCos;
  vec4f vzero = v_zero();

  xl = v_mul(ang, V_C_2_DIV_PI);
  xl = v_add(xl, v_btsel(V_C_HALF, ang, v_msbit()));

  q = v_cvt_vec4i(xl);

  offsetSin = v_andi(q, V_CI_3);
  offsetCos = v_addi(V_CI_1, offsetSin);

  vec4f qf = v_cvt_vec4f(q);
  vec4f p1 = v_nmsub(qf, v_splats(_SINCOS_KC1), ang);
  xl = v_nmsub(qf, v_splats(_SINCOS_KC2), p1);

  xl2 = v_mul(xl, xl);
  xl3 = v_mul(xl2, xl);

  vec4f ct1 = v_madd(v_splats(_SINCOS_CC0), xl2, v_splats(_SINCOS_CC1));
  vec4f st1 = v_madd(v_splats(_SINCOS_SC0), xl2, v_splats(_SINCOS_SC1));

  vec4f ct2 = v_madd(ct1, xl2, v_splats(_SINCOS_CC2));
  vec4f st2 = v_madd(st1, xl2, v_splats(_SINCOS_SC2));

  vec4f cx = v_madd(ct2, xl2, V_C_ONE);
  vec4f sx = v_madd(st2, xl3, xl);

  vec4f sinMask = v_cmp_eqi(v_cast_vec4f(v_andi(offsetSin, V_CI_1)), vzero);
  vec4f cosMask = v_cmp_eqi(v_cast_vec4f(v_andi(offsetCos, V_CI_1)), vzero);
  s = v_sel(cx, sx, sinMask);
  c = v_sel(cx, sx, cosMask);

  sinMask = v_cmp_eqi(v_cast_vec4f(v_andi(offsetSin, V_CI_2)), vzero);
  cosMask = v_cmp_eqi(v_cast_vec4f(v_andi(offsetCos, V_CI_2)), vzero);

  s = v_sel(v_neg(s), s, sinMask);
  c = v_sel(v_neg(c), c, cosMask);
}

// calculates 4 in 2x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_FINLINE vec4f v_sin(vec4f ang)
{
  vec4f s, c;
  v_sincos(ang, s, c);
  return s;
}

// calculates 4 in 2x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_FINLINE vec4f v_cos(vec4f ang)
{
  vec4f s, c;
  v_sincos(ang, s, c);
  return c;
}

// calculates 4 in ~1.13x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_FINLINE vec4f v_tan(vec4f ang)
{
  vec4f x = v_abs(ang);
  vec4f signBit = v_and(ang, V_CI_SIGN_MASK);
  vec4f xl = v_mul(x, V_C_4_DIV_PI);
  vec4f cmp = v_cmp_gt(x, v_splats(_TAN_B0));

  vec4i q = v_cvt_vec4i(xl);
  q = v_addi(q, V_CI_1);
  q = v_andi(q, v_splatsi(~1));
  xl = v_cvt_vec4f(q);

  x = v_madd(xl, v_splats(_MINUS_DP1), x);
  x = v_madd(xl, v_splats(_MINUS_DP2), x);
  x = v_madd(xl, v_splats(_MINUS_DP3), x);

  vec4f x2 = v_mul(x, x);

  vec4f y = v_madd(x2, v_splats(_TAN_P0), v_splats(_TAN_P1));
  y = v_madd(y, x2, v_splats(_TAN_P2));
  y = v_madd(y, x2, v_splats(_TAN_P3));
  y = v_madd(y, x2, v_splats(_TAN_P4));
  y = v_madd(y, x2, v_splats(_TAN_P5));
  y = v_mul(y, x2);
  y = v_madd(y, x, x);

  y = v_sel(x, y, cmp);

  q = v_andi(q, V_CI_2);
  q = v_cmp_eqi(q, v_zeroi());
  cmp = v_cast_vec4f(q);

  vec4f z = v_div(v_neg(V_C_ONE), y);
  y = v_sel(z, y, cmp);
  return v_xor(y, signBit);
}

// calculates 4 in ~1.84x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_FINLINE vec4f v_asin(vec4f ang)
{
  vec4f x = v_abs(ang);
  vec4f signBit = v_and(ang, V_CI_SIGN_MASK);
  vec4f invalidMask = v_cmp_gt(x, V_C_ONE);
  vec4f cmp = v_cmp_ge(x, V_C_HALF);

  vec4f z1 = v_mul(V_C_HALF, v_sub(V_C_ONE, x));
  vec4f x1 = v_sqrt(z1);
  vec4f z2 = v_mul(x, x);

  vec4f z = v_sel(z2, z1, cmp);
  x = v_sel(x, x1, cmp);

  vec4f y = v_madd(z, v_splats(_ASIN_P0), v_splats(_ASIN_P1));
  y = v_madd(y, z, v_splats(_ASIN_P2));
  y = v_madd(y, z, v_splats(_ASIN_P3));
  y = v_madd(y, z, v_splats(_ASIN_P4));
  y = v_mul(y, z);
  y = v_madd(y, x, x);

  vec4f y2 = v_sub(V_C_HALFPI, v_add(y, y));
  y2 = v_and(cmp, y2);
  y = v_sel(y, y2, cmp);

  y = v_xor(y, signBit);
  return v_or(y, invalidMask);
}

// calculates 4 in ~1.08x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_FINLINE vec4f v_acos(vec4f ang)
{
  vec4f polyMask1 = v_cmp_ge(v_neg(V_C_HALF), ang);
  vec4f polyMask2 = v_cmp_ge(ang, V_C_HALF);

  vec4f x1 = v_add(V_C_ONE, ang);
  vec4f x2 = v_sub(V_C_ONE, ang);

  x1 = v_and(polyMask1, x1);
  x2 = v_and(polyMask2, x2);
  vec4f x = v_or(x1, x2);
  x = v_sqrt(v_mul(V_C_HALF, x));

  vec4f polyMask3 = v_or(polyMask1, polyMask2);
  vec4f x3 = v_andnot(polyMask3, ang);
  x = v_or(x, x3);

  vec4f signBit = v_and(x, V_CI_SIGN_MASK);
  x = v_abs(x);

  vec4f z = v_mul(x, x);
  vec4f y = v_madd(z, v_splats(_ASIN_P0), v_splats(_ASIN_P1));
  y = v_madd(y, z, v_splats(_ASIN_P2));
  y = v_madd(y, z, v_splats(_ASIN_P3));
  y = v_madd(y, z, v_splats(_ASIN_P4));
  y = v_mul(y, z);
  y = v_madd(y, x, x);

  x = v_xor(y, signBit);

  vec4f fact1 = v_and(polyMask1, v_neg(V_C_TWO));
  vec4f fact2 = v_and(polyMask2, V_C_TWO);
  vec4f fact = v_or(fact1, fact2);
  vec4f fact3 = v_andnot(polyMask3, v_neg(V_C_ONE));
  fact = v_add(fact, fact3);
  x = v_mul(x, fact);
  vec4f offs1 = v_and(polyMask1, V_C_PI);
  vec4f offs3 = v_andnot(polyMask3, V_C_HALFPI);
  offs1 = v_or(offs1, offs3);
  x = v_add(x, offs1);

  vec4f absVal = v_abs(ang);
  vec4f invalidMask = v_cmp_gt(absVal, V_C_ONE);
  return v_or(x, invalidMask);
}

// calculates 4 in ~1.72x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_FINLINE vec4f v_atan(vec4f x)
{
  vec4f signBit = v_and(x, V_CI_SIGN_MASK);
  x = v_abs(x);

  vec4f mask1 = v_cmp_ge(x, v_splats(_ATAN_Q0));
  vec4f mask2 = v_andnot(mask1, v_cmp_ge(x, v_splats(_ATAN_Q1)));
  vec4f mask3 = v_or(mask1, mask2);

  vec4f y = v_and(mask1, V_C_HALFPI);
  y = v_or(y, v_and(mask2, V_C_PI_DIV_4));
  vec4f x1 = v_div(v_neg(V_C_ONE), x);
  vec4f x2 = v_div(v_sub(x, V_C_ONE), v_add(x, V_C_ONE));
  x = v_andnot(mask3, x);
  x = v_or(x, v_and(mask1, x1));
  x = v_or(x, v_and(mask2, x2));

  vec4f z = v_mul(x, x);
  vec4f tmp = v_madd(z, v_splats(_ATAN_P0), v_splats(_ATAN_P1));
  tmp = v_madd(tmp, z, v_splats(_ATAN_P2));
  tmp = v_madd(tmp, z, v_splats(_ATAN_P3));
  tmp = v_mul(tmp, z);
  tmp = v_madd(tmp, x, x);
  tmp = v_add(tmp, y);

  return v_xor(tmp, signBit);
}

// approximate atan_est |error| is < 0.00045
// calculates 4 in ~2.81x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_INLINE  vec4f v_atan_est(vec4f x)  // any x
{
  vec4f xRcp = v_rcp_est(x);

  vec4f isOut1m1 = v_or(v_cmp_gt(x, V_C_ONE), v_cmp_ge(v_neg(V_C_ONE), x));
  vec4f xUsed = v_sel(x, xRcp, isOut1m1);

  vec4f xUsedSq = v_mul(xUsed, xUsed);
  vec4f atanPoly;
  atanPoly = v_add(xUsedSq, v_splats(_ATAN_EST_S0));
  atanPoly = v_mul(v_rcp_est(atanPoly), v_splats(_ATAN_EST_T0));

  atanPoly = v_add(atanPoly, v_add(xUsedSq, v_splats(_ATAN_EST_S1)));
  atanPoly = v_mul(v_rcp_est(atanPoly), v_splats(_ATAN_EST_T1));

  atanPoly = v_add(atanPoly, v_add(xUsedSq, v_splats(_ATAN_EST_S2)));
  atanPoly = v_mul(v_rcp_est(atanPoly), v_splats(_ATAN_EST_T2));

  atanPoly = v_add(atanPoly, v_add(xUsedSq, v_splats(_ATAN_EST_S3)));
  atanPoly = v_mul(v_rcp_est(atanPoly), v_mul(xUsed, v_splats(_ATAN_EST_T3)));

  vec4f res = v_or(v_and(xUsed, v_msbit()), V_C_HALFPI);
  res = v_sub(res, atanPoly);
  return v_sel(atanPoly, res, isOut1m1);
}

// calculates 4 in ~1.47x speed of win libc implementation for 1, with same precision
VECTORCALL VECMATH_FINLINE vec4f v_atan2(vec4f y, vec4f x)
{
  vec4f maskXeq0 = v_is_unsafe_divisor(x);
  vec4f maskXlt0 = v_cast_vec4f(v_cmp_lti(v_cast_vec4i(x), v_zeroi()));
  vec4f maskYeq0 = v_cmp_eq(y, v_zero());
  vec4f signY = v_and(y, V_CI_SIGN_MASK);
  vec4f zeroXres = v_or(signY, v_sel(V_C_HALFPI, v_and(V_C_PI, maskXlt0), maskYeq0));
  vec4f offs = v_or(signY, v_and(V_C_PI, maskXlt0));
  vec4f atan = v_atan(v_div(y, x));
  atan = v_add(atan, offs);
  return v_sel(atan, zeroXres, maskXeq0);
}

// fast approx atan2 version. |error| is < 0.0004
// calculates 4 in ~1.47x+ (untested, faster than v_atan2) speed of win libc implementation for 1
VECTORCALL VECMATH_INLINE vec4f v_atan2_est(vec4f y, vec4f x)
{
  vec4f maskXeq0 = v_is_unsafe_divisor(x);
  vec4f maskXlt0 = v_cast_vec4f(v_cmp_lti(v_cast_vec4i(x), v_zeroi()));
  vec4f maskYneq0 = v_cmp_neq(y, v_zero());
  vec4f signY = v_and(y, V_CI_SIGN_MASK);
  vec4f zeroXres = v_or(signY, v_and(V_C_HALFPI, maskYneq0));
  vec4f offs = v_or(signY, v_and(V_C_PI, maskXlt0));
  vec4f atan = v_atan_est(v_div(y, x));
  atan = v_add(atan, offs);
  return v_sel(atan, zeroXres, maskXeq0);
}

VECTORCALL VECMATH_FINLINE void v_sincos_x(vec4f ang, vec4f& s, vec4f& c)
{
  v_sincos(ang, s, c);
}

VECTORCALL VECMATH_FINLINE vec4f v_sin_x(vec4f ang)
{
  return v_sin(ang);
}

VECTORCALL VECMATH_FINLINE vec4f v_cos_x(vec4f ang)
{
  return v_cos(ang);
}

VECTORCALL VECMATH_FINLINE vec4f v_tan_x(vec4f ang)
{
  return v_tan(ang);
}

VECTORCALL VECMATH_FINLINE vec4f v_asin_x(vec4f ang)
{
  return v_asin(ang);
}

VECTORCALL VECMATH_FINLINE vec4f v_acos_x(vec4f ang)
{
  return v_acos(ang);
}

VECTORCALL VECMATH_FINLINE vec4f v_atan_x(vec4f ang)
{
  return v_atan(ang);
}

VECTORCALL VECMATH_FINLINE vec4f v_atan_est_x(vec4f ang)
{
  return v_atan_est(ang);
}

VECTORCALL VECMATH_FINLINE vec4f v_atan2_x(vec4f y, vec4f x)
{
  return v_atan2(y, x);
}

VECTORCALL VECMATH_FINLINE vec4f v_atan2_est_x(vec4f y, vec4f x)
{
  return v_atan2_est(y, x);
}

VECTORCALL VECMATH_FINLINE vec4f v_safe_asin(vec4f ang)
{
  return v_asin(v_clamp(ang, v_neg(V_C_ONE), V_C_ONE));
}

VECTORCALL VECMATH_FINLINE vec4f v_safe_acos(vec4f ang)
{
  return v_acos(v_clamp(ang, v_neg(V_C_ONE), V_C_ONE));
}

VECTORCALL VECMATH_FINLINE vec4f v_safe_asin_x(vec4f ang)
{
  return v_asin_x(v_clamp(ang, v_neg(V_C_ONE), V_C_ONE));
}

VECTORCALL VECMATH_FINLINE vec4f v_safe_acos_x(vec4f ang)
{
  return v_acos_x(v_clamp(ang, v_neg(V_C_ONE), V_C_ONE));
}

VECTORCALL VECMATH_FINLINE vec4f v_angle_cos(vec3f a, vec3f b)
{
  return v_dot3(v_norm3_safe(a, v_zero()), v_norm3_safe(b, v_zero()));
}

VECTORCALL VECMATH_FINLINE vec4f v_angle_cos_x(vec3f a, vec3f b)
{
  return v_dot3_x(v_norm3_safe(a, v_zero()), v_norm3_safe(b, v_zero()));
}

VECTORCALL VECMATH_FINLINE vec4f v_angle_cos_unsafe(vec3f a, vec3f b)
{
  return v_dot3(v_norm3(a), v_norm3(b));
}

VECTORCALL VECMATH_FINLINE vec4f v_angle_cos_unsafe_x(vec3f a, vec3f b)
{
  return v_dot3_x(v_norm3(a), v_norm3(b));
}

VECTORCALL VECMATH_FINLINE vec4f v_angle(vec3f a, vec3f b)
{
  return v_safe_acos(v_angle_cos(a, b));
}

VECTORCALL VECMATH_FINLINE vec4f v_angle_x(vec3f a, vec3f b)
{
  return v_safe_acos_x(v_angle_cos_x(a, b));
}

VECTORCALL VECMATH_FINLINE vec4f v_angle_unsafe(vec3f a, vec3f b)
{
  return v_safe_acos(v_angle_cos_unsafe(a, b));
}

VECTORCALL VECMATH_FINLINE vec4f v_angle_unsafe_x(vec3f a, vec3f b)
{
  return v_safe_acos_x(v_angle_cos_unsafe_x(a, b));
}
