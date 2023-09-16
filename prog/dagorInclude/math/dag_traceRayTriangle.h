//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath.h>

/*
Why v_rcp isn't used:
vrcpps results differs in 128 and 256 bit mode
On modern AMD 5800H results of v_mul(a, v_rcp(b)) and v_div256(a, b) are binary equal
On some old Intel cpu's (Sandy, Ivy bridge, ...) 256-bit version of vrcpps had significantly worse precision than on modern cpu
In this file we always prefer v_div for better precision
*/

/// intersect_triangle from Moller
#define TRACE_EPSILON_HT          0.00005f
#define TRACE_ONE_PLUS_EPSILON_HT (1.0f + TRACE_EPSILON_HT)

#define TRACE_EPSILON          0.000004f
#define TRACE_ONE_PLUS_EPSILON (1.f + TRACE_EPSILON)

// Branchless version
static bool __forceinline traceRayToTriangle(vec3f p, vec3f dir, float &min_t, float &u, float &v, vec3f vert0, vec3f edge1,
  vec3f edge2, bool no_cull)
{
  vec3f pvec = v_cross3(dir, edge2);
  vec4f det = v_dot3_x(edge1, pvec);

#if _TARGET_PC_WIN && !_TARGET_64BIT
  if (v_extract_x(det) > -FLT_EPSILON && v_extract_x(det) < FLT_EPSILON) // exit before nan exception in win32 fpu build
    return false;
#endif

  vec4i allBits = v_cmp_eqi(v_zeroi(), v_zeroi());
  vec4f absMask = v_cast_vec4f(v_srli_n(allBits, no_cull ? 1 : 0));

  /// if determinant is near zero, ray lies in plane of triangle
  bool res = v_extract_x(v_and(det, absMask)) > TRACE_EPSILON;
  /// calculate distance from vert0 to ray origin
  vec3f tvec = v_sub(p, vert0);

  /// calculate U parameter and test bounds
  u = v_extract_x(v_div_x(v_dot3_x(tvec, pvec), det));
  res &= u > -TRACE_EPSILON;
  res &= u < TRACE_ONE_PLUS_EPSILON;

  /// calculate V parameter and test bounds
  vec3f qvec = v_cross3(tvec, edge1);
  v = v_extract_x(v_div_x(v_dot3_x(dir, qvec), det));
  res &= v > -TRACE_EPSILON;
  res &= u + v < TRACE_ONE_PLUS_EPSILON;

  /// calculate t, scale parameters, ray intersects triangle
  float t = v_extract_x(v_div_x(v_dot3_x(edge2, qvec), det));
  res &= t > 0.f;
  res &= t < min_t;
  min_t = res ? t : min_t;
  return res;
}

static bool __forceinline traceRayToTriangle(vec3f p, vec3f dir, float &min_t, vec3f vert0, vec3f edge1, vec3f edge2, bool no_cull)
{
  float u, v;
  return traceRayToTriangle(p, dir, min_t, u, v, vert0, edge1, edge2, no_cull);
}

static bool __forceinline traceRayToTriangleCullCCW(vec3f p, vec3f dir, float &min_t, vec3f vert0, vec3f edge1, vec3f edge2)
{
  return traceRayToTriangle(p, dir, min_t, vert0, edge1, edge2, false);
}

static bool __forceinline traceRayToTriangleNoCull(vec3f p, vec3f dir, float &min_t, vec3f vert0, vec3f edge1, vec3f edge2)
{
  return traceRayToTriangle(p, dir, min_t, vert0, edge1, edge2, true);
}

static bool __forceinline traceRayToTriangleCullCCW(const Point3 &p, const Point3 &dir, float &min_t, const Point3 &vert0,
  const Point3 &edge1, const Point3 &edge2, float &u, float &v)
{
  return traceRayToTriangle(v_ldu(&p.x), v_ldu(&dir.x), min_t, u, v, v_ldu(&vert0.x), v_ldu(&edge1.x), v_ldu(&edge2.x), false);
}

static bool __forceinline traceRayToTriangleNoCull(const Point3 &p, const Point3 &dir, float &min_t, const Point3 &vert0,
  const Point3 &edge1, const Point3 &edge2, float &u, float &v)
{
  return traceRayToTriangle(v_ldu(&p.x), v_ldu(&dir.x), min_t, u, v, v_ldu(&vert0.x), v_ldu(&edge1.x), v_ldu(&edge2.x), true);
}

static bool __forceinline rayHitToTriangle(vec3f p, const vec3f dir, float len, vec3f vert0, vec3f edge1, vec3f edge2, bool cull)
{
  float u, v;
  return traceRayToTriangle(p, dir, len, u, v, vert0, edge1, edge2, cull);
}

static void __forceinline v_mat43_sub_vec4f(mat43f &dest, mat43f a, vec4f b)
{
  dest.row0 = v_sub(a.row0, v_splat_x(b));
  dest.row1 = v_sub(a.row1, v_splat_y(b));
  dest.row2 = v_sub(a.row2, v_splat_z(b));
}

static vec4f __forceinline v_dot(mat43f a, mat43f b) { return v_madd(a.row2, b.row2, v_madd(a.row1, b.row1, v_mul(a.row0, b.row0))); }

static void __forceinline v_mat43_cross(mat43f &dest, mat43f a, mat43f b)
{
  dest.row0 = v_msub(a.row1, b.row2, v_mul(a.row2, b.row1));
  dest.row1 = v_msub(a.row2, b.row0, v_mul(a.row0, b.row2));
  dest.row2 = v_msub(a.row0, b.row1, v_mul(a.row1, b.row0));
}

// return vector mask of triangles was hit
// min_t will be updated for each triangle independently
static vec4f __forceinline traceray4TrianglesVecMask(vec3f from, vec3f dir, vec4f &min_t, mat43f p0, mat43f p1, mat43f p2,
  bool no_cull)
{
  mat43f e1, e2, Ng;
  v_mat43_sub(e1, p0, p1);
  v_mat43_sub(e2, p2, p0);
  v_mat43_cross(Ng, e1, e2);

  mat43f D, C, R;
  D.row0 = v_splat_x(dir);
  D.row1 = v_splat_y(dir);
  D.row2 = v_splat_z(dir);
  v_mat43_sub_vec4f(C, p0, from);
  v_mat43_cross(R, D, C);
  vec4f det = v_dot(Ng, D);

  vec4i allBits = v_cmp_eqi(v_zeroi(), v_zeroi());
  vec4f absMask = v_cast_vec4f(v_srli_n(allBits, no_cull ? 1 : 0));

  vec4f sgnDet = v_andnot(absMask, det);
  det = v_and(det, absMask);

  vec4f U = v_xor(v_dot(R, e2), sgnDet);
  vec4f V = v_xor(v_dot(R, e1), sgnDet);

  vec4f valid = v_and(v_cmp_ge(U, v_splats(-TRACE_EPSILON)), v_cmp_ge(V, v_splats(-TRACE_EPSILON)));
  vec4f W1 = v_div(v_add(U, V), det);
  valid = v_and(valid, v_cmp_ge(v_splats(TRACE_ONE_PLUS_EPSILON), W1));
  vec4f T = v_xor(v_dot(Ng, C), sgnDet);
  T = v_div(T, det);
  valid = v_and(valid, v_and(v_and(v_cmp_gt(det, v_zero()), v_cmp_ge(T, v_zero())), v_cmp_gt(min_t, T)));
  min_t = v_sel(min_t, T, valid);
  return valid;
}

// return int number of triangle if hit (0..3) or -1 if miss
// min_t will be updated on smallest ray T
static int __forceinline traceray4Triangles(vec3f from, vec3f dir, float &min_t, mat43f p0, mat43f p1, mat43f p2, bool no_cull,
  vec4f mask = V_CI_MASK1111)
{
  vec4f T = v_splats(min_t);
  vec4f valid = traceray4TrianglesVecMask(from, dir, T, p0, p1, p2, no_cull);
  valid = v_and(valid, mask);

#if _TARGET_SIMD_SSE
  int validMask = v_signmask(valid); // slow on neon
  if (validMask == 0x0)
    return -1;
#endif

  T = v_sel(V_C_MAX_VAL, T, valid);
  vec4f minT = v_min(v_rot_1(T), T);
  minT = v_min(v_rot_2(minT), minT);
  minT = v_min(minT, v_splats(min_t));
  min_t = v_extract_x(minT);

  // select one valid element by min t
  vec4f resultMask = v_cmp_eq(minT, T);
#if _TARGET_SIMD_SSE
  int signMask = v_signmask(resultMask);
  return __bsf_unsafe(signMask);
#else
  vec4i iResultMask = v_cast_vec4i(v_and(resultMask, valid));
  int ret = -1;
  if (v_extract_wi(iResultMask))
    ret = 3;
  if (v_extract_zi(iResultMask))
    ret = 2;
  if (v_extract_yi(iResultMask))
    ret = 1;
  if (v_extract_xi(iResultMask))
    ret = 0;
  return ret;
#endif
}

static int __forceinline traceray4TrianglesNoCull(vec3f from, vec3f dir, float &min_t, mat43f p0, mat43f p1, mat43f p2)
{
  return traceray4Triangles(from, dir, min_t, p0, p1, p2, true);
}

static int __forceinline traceray4TrianglesCullCCW(vec3f from, vec3f dir, float &min_t, mat43f p0, mat43f p1, mat43f p2)
{
  return traceray4Triangles(from, dir, min_t, p0, p1, p2, false);
}

// return bit mask of triangles was hit
// min_t will be updated for each triangle independently
static int __forceinline traceray4TrianglesMask(vec3f from, vec3f dir, vec4f &min_t, mat43f p0, mat43f p1, mat43f p2, bool no_cull)
{
  vec4f valid = traceray4TrianglesVecMask(from, dir, min_t, p0, p1, p2, no_cull);
  return v_signmask(valid);
}

static int __forceinline rayhit4Triangles(vec3f from, vec3f dir, float len, mat43f p0, mat43f p1, mat43f p2, bool no_cull,
  vec4f mask = V_CI_MASK1111)
{
  vec4f T = v_splats(len);
  vec4f valid = traceray4TrianglesVecMask(from, dir, T, p0, p1, p2, no_cull);
  return v_signmask(v_and(valid, mask));
}

// return vec4 - 0xFFFFFFFF or 0 for each triangle, and return hts of hits in ht (out param)
template <bool noCull>
static vec4f __forceinline get4TrianglesHt(vec3f from, vec4f &ht, mat43f p0, mat43f p1, mat43f p2)
{
  mat43f e1, e2;
  v_mat43_sub(e1, p1, p0);
  v_mat43_sub(e2, p2, p0);
  vec4f det = v_msub(e1.row2, e2.row0, v_mul(e1.row0, e2.row2));
  vec4f tvecx = v_sub(v_splat_x(from), p0.row0);
  vec4f tvecz = v_sub(v_splat_z(from), p0.row2);
  if (noCull)
    det = v_abs(det);

  vec4f u = v_div(v_msub(tvecz, e2.row0, v_mul(tvecx, e2.row2)), det);
  vec4f v = v_div(v_msub(tvecx, e1.row2, v_mul(tvecz, e1.row0)), det);
  vec4f valid = v_and(v_cmp_ge(u, v_splats(-TRACE_EPSILON_HT)), v_cmp_ge(v, v_splats(-TRACE_EPSILON_HT)));
  valid = v_and(v_cmp_gt(det, v_zero()), v_and(valid, v_cmp_ge(v_splats(TRACE_ONE_PLUS_EPSILON_HT), v_add(u, v))));

  ht = v_add(p0.row1, v_madd(u, e1.row1, v_mul(v, e2.row1)));
  return valid;
}

// return 0 to 3 - indices of triangle, or -1 if failed to hit maxht
template <bool noCull, bool below>
static int __forceinline get4TrianglesMaxHtId(vec3f from, vec4f &maxht, mat43f p0, mat43f p1, mat43f p2)
{
  vec4f ht;
  vec4f valid = get4TrianglesHt<noCull>(from, ht, p0, p1, p2);
  ht = v_sel(V_C_MIN_VAL, ht, valid);
  valid = v_and(valid, v_cmp_gt(ht, maxht));
  if (below)
    valid = v_and(valid, v_cmp_gt(v_splat_y(from), ht));
#if _TARGET_SIMD_SSE
  int validMask = v_signmask(valid); // slow on neon
  if (validMask == 0x0)
    return -1;
#endif

  ht = v_sel(V_C_MIN_VAL, ht, valid);
  vec4f maxHT = v_max(v_rot_1(ht), ht);
  maxHT = v_max(v_rot_2(maxHT), maxHT);
  maxHT = v_max(maxht, maxHT);
  maxht = maxHT;

  // select one valid element by maxHT
  vec4f resultMask = v_cmp_eq(maxHT, ht);
#if _TARGET_SIMD_SSE
  int signMask = v_signmask(resultMask);
  return signMask ? __bsf(signMask) : -1;
#else
  vec4i iResultMask = v_cast_vec4i(v_and(resultMask, valid));
  int ret = -1;
  if (v_extract_wi(iResultMask))
    ret = 3;
  if (v_extract_zi(iResultMask))
    ret = 2;
  if (v_extract_yi(iResultMask))
    ret = 1;
  if (v_extract_xi(iResultMask))
    ret = 0;
  return ret;
#endif
}

// return 0 or 1
template <bool noCull, bool below>
static int __forceinline get4TrianglesMaxHt(vec3f from, vec4f &maxht, mat43f p0, mat43f p1, mat43f p2)
{
  vec4f ht;
  vec4f valid = get4TrianglesHt<noCull>(from, ht, p0, p1, p2);
  ht = v_sel(V_C_MIN_VAL, ht, valid);
  valid = v_and(valid, v_cmp_gt(ht, maxht));
  if (below)
    valid = v_and(valid, v_cmp_gt(v_splat_y(from), ht));
#if _TARGET_SIMD_SSE
  int validMask = _mm_movemask_ps(valid); // slow on neon
  if (validMask == 0x0)
    return 0;
#endif

  ht = v_sel(V_C_MIN_VAL, ht, valid);
  ht = v_max(v_rot_1(ht), ht);
  ht = v_max(v_rot_2(ht), ht);
  maxht = v_max(ht, maxht);

#if _TARGET_SIMD_SSE
  return 1;
#else
  return !v_test_vec_x_eqi(ht, V_C_MIN_VAL);
#endif
}

static void __forceinline v_mat44_transpose_to_mat43(vec4f r0, vec4f r1, vec4f r2, vec4f r3, vec4f &c0, vec4f &c1, vec4f &c2)
{
  vec4f l02 = v_merge_hw(r0, r2);
  vec4f h02 = v_merge_lw(r0, r2);
  vec4f l13 = v_merge_hw(r1, r3);
  vec4f h13 = v_merge_lw(r1, r3);
  c0 = v_merge_hw(l02, l13);
  c1 = v_merge_lw(l02, l13);
  c2 = v_merge_hw(h02, h13);
}

// todo: vectorize getTriangleHt as well (could be SIMDED up to two triangles in one function)
template <bool noCull>
static inline bool getTriangleHtCull(float x, float z, real &maxht, const Point3 &vert0, const Point3 &edge1, const Point3 &edge2)
{
#if _TARGET_SIMD_SSE // early exits are faster on PC
  // const Point3 pvec = dir % edge2;
  //{return Point3(edge2.z,0,-edge2.x);}
  float u, v;

  /// if determinant is near zero, ray lies in plane of triangle
  float det = edge1.z * edge2.x - edge1.x * edge2.z;
  if (noCull)
  {
    if (fabsf(det) < 1e-30) // culling: uncomment this line to make accurate culling - prevent errors on near zero det
      return false;
  }
  else if (det < 1e-30) // culling: uncomment this line to make accurate culling - prevent errors on near zero det
    return false;

  /// calculate distance from vert0 to ray origin
  float tvecx = x - vert0.x;
  float tvecz = z - vert0.z;

  /// calculate U parameter and test bounds
  float invdet = 1.0f / det;
  u = (tvecz * edge2.x - tvecx * edge2.z) * invdet;
  if (u < -TRACE_EPSILON_HT || u > TRACE_ONE_PLUS_EPSILON_HT) // culling would also done here
    return false;

  /// calculate V parameter and test bounds
  v = (tvecx * edge1.z - tvecz * edge1.x) * invdet;
  if (v < -TRACE_EPSILON_HT || u + v > TRACE_ONE_PLUS_EPSILON_HT)
    return false;

  // float qvecx = tvec.y*edge1.z-tvec.z*edge1.y;
  // float qvecz = tvec.x*edge1.y-tvec.y*edge1.x;

  /// calculate t, scale parameters, ray intersects triangle
  // maxht = max((edge2.x * qvecx + edge2.y * qvecy + edge2.z * qvecz) * invdet, maxht);
  maxht = vert0.y + (u * edge1.y + v * edge2.y);
  return true;
#else
  // const Point3 pvec = dir % edge2;
  //{return Point3(edge2.z,0,-edge2.x);}
  float u, v;

  /// if determinant is near zero, ray lies in plane of triangle
  float det = edge1.z * edge2.x - edge1.x * edge2.z;
  float res;
  if (noCull)
    res = fsel(-fabsf(det), 0, 1); // culling: uncomment this line to make accurate culling - prevent errors on near zero det
  else
    res = fsel(-det, 0, 1); // culling: uncomment this line to make accurate culling - prevent errors on near zero det

  /// calculate distance from vert0 to ray origin
  float tvecx = x - vert0.x;
  float tvecz = z - vert0.z;

  /// calculate U parameter and test bounds
  float invdet = safeinv(det);
  u = (tvecz * edge2.x - tvecx * edge2.z) * invdet;
  res = fsel(-TRACE_EPSILON_HT - u, 0, res); // culling would also done here
  // res = fsel(u - det * TRACE_ONE_PLUS_EPSILON_HT, 0, res); ///!No use u+v will also be bigger than 1

  /// calculate V parameter and test bounds
  v = (tvecx * edge1.z - tvecz * edge1.x) * invdet;
  res = fsel(-TRACE_EPSILON_HT - v, 0, res);
  res = fsel(u + v - TRACE_ONE_PLUS_EPSILON_HT, 0, res);
  if (res == 0)
    return false;

  // float qvecx = tvec.y*edge1.z-tvec.z*edge1.y;
  // float qvecz = tvec.x*edge1.y-tvec.y*edge1.x;

  /// calculate t, scale parameters, ray intersects triangle
  // maxht = max((edge2.x * qvecx + edge2.y * qvecy + edge2.z * qvecz) * invdet, maxht);
  maxht = vert0.y + (u * edge1.y + v * edge2.y);
  return true;
#endif
}

static inline vec4f getTriangle4HtCull(vec4f x, vec4f z, vec4f &maxht, const vec3f &vert0, const vec3f &edge1, const vec3f &edge2)
{
  vec4f u, v;
  vec4f edge1_x = v_splat_x(edge1), edge1_z = v_splat_z(edge1), edge2_x = v_splat_x(edge2), edge2_z = v_splat_z(edge2);
  /// if determinant is near zero, ray lies in plane of triangle
  vec4f det = v_splat_x(v_sub_x(v_mul_x(edge1_z, edge2_x), v_mul_x(edge1_x, edge2_z)));

  /// calculate distance from vert0 to ray origin
  vec4f tvecx = v_sub(x, v_splat_x(vert0));
  vec4f tvecz = v_sub(z, v_splat_z(vert0));

  /// calculate U parameter and test bounds
  u = v_div(v_msub(tvecz, edge2_x, v_mul(tvecx, edge2_z)), det);
  v = v_div(v_msub(tvecx, edge1_z, v_mul(tvecz, edge1_x)), det);

  vec4f valid;
  valid = v_and(v_cmp_ge(u, v_splats(-TRACE_EPSILON_HT)), v_cmp_ge(v, v_splats(-TRACE_EPSILON_HT)));
  valid = v_and(v_cmp_gt(det, v_zero()), v_and(valid, v_cmp_ge(v_splats(TRACE_ONE_PLUS_EPSILON_HT), v_add(u, v))));
  maxht = v_add(v_splat_y(vert0), v_madd(u, v_splat_y(edge1), v_mul(v, v_splat_y(edge2))));
  return valid;
}

static bool __forceinline getTriangleHt(float x, float z, real &maxht, const Point3 &vert0, const Point3 &edge1, const Point3 &edge2)
{
  return getTriangleHtCull<true>(x, z, maxht, vert0, edge1, edge2);
}

#if _TARGET_SIMD_SSE
static inline bool
#else
static inline vec4f
#endif
getTrianglesMultiHtCullSOA(const vec4f *__restrict triangles, int tri_count,    // array of vert0, edge1, edge2
  mat44f *__restrict rays_soa, int rays_soa_count, vec4f *__restrict v_outNorm) // array of 4 rays, v_outNorm size is rays_soa_count*4
{
  vec4f valid = v_zero();
  for (const vec3f *endTri = triangles + tri_count * 3; triangles != endTri; triangles += 3)
  {
    vec4f vert0 = triangles[0];
    vec4f edge1 = triangles[1];
    vec4f edge2 = triangles[2];
    vec4f norm = v_perm_xyzd(v_norm3(v_cross3(edge1, edge2)), vert0);
    vec4f *v_norm = v_outNorm;
    for (mat44f *ray4 = rays_soa, *endRay = rays_soa + rays_soa_count; ray4 != endRay; ray4++, v_norm += 4)
    {
      vec4f ht, result;
      result = getTriangle4HtCull(ray4->col0, ray4->col2, ht, vert0, edge1, edge2);
      result = v_and(result, v_cmp_gt(ray4->col1, ht));
      result = v_and(result, v_cmp_gt(ht, ray4->col3));
      valid = v_or(valid, result);
      ray4->col3 = v_sel(ray4->col3, ht, result);
      v_norm[0] = v_sel(v_norm[0], norm, v_splat_x(result));
      v_norm[1] = v_sel(v_norm[1], norm, v_splat_y(result));
      v_norm[2] = v_sel(v_norm[2], norm, v_splat_z(result));
      v_norm[3] = v_sel(v_norm[3], norm, v_splat_w(result));
    }
  }
#if _TARGET_SIMD_SSE
  return _mm_movemask_ps(valid) != 0;
#else
  valid = v_or(valid, v_rot_1(valid)); // xyzw | yzwx = x|y y|z w|z x|w
  valid = v_or(valid, v_rot_2(valid)); // x|y|w|z y|z|x|w w|z|x|y x|w|y|z
  return valid;
#endif
}

static inline bool traceDownTrianglesMultiRay(const vec4f *__restrict triangles, int tri_count, // array of vert0, edge1, edge2
  vec4f *__restrict v_rays_pos_t, int rays_count, vec4f *__restrict v_outNorm, mat44f *__restrict tempSOA) // tempSOA is array of 4
                                                                                                           // rays, v_rays_pos_t,
                                                                                                           // v_norm, is array of
                                                                                                           // [rays_count*4]
{
  for (mat44f *ray4SOA = tempSOA, *endRay4SOA = tempSOA + rays_count, *src4Ray = (mat44f *)v_rays_pos_t; ray4SOA != endRay4SOA;
       ray4SOA++, src4Ray++)
  {
    v_mat44_transpose(*ray4SOA, *src4Ray);
    ray4SOA->col3 = v_sub(ray4SOA->col1, ray4SOA->col3);
  }

#if _TARGET_SIMD_SSE
  bool valid;
#else
  vec4f valid;
#endif
  valid = getTrianglesMultiHtCullSOA(triangles, tri_count, tempSOA, rays_count, v_outNorm);

#if _TARGET_SIMD_SSE
  if (!valid)
    return false;
#else
  if (v_test_vec_x_eqi_0(valid))
    return false;
#endif
  for (mat44f *ray4SOA = tempSOA, *endRay4SOA = tempSOA + rays_count, *src4Ray = (mat44f *)v_rays_pos_t; ray4SOA != endRay4SOA;
       ray4SOA++, src4Ray++)
  {
    ray4SOA->col3 = v_sub(ray4SOA->col1, ray4SOA->col3);
    v_mat44_transpose(*src4Ray, *ray4SOA);
  }

  return true;
}

static int __forceinline traceray4Triangles(vec4f from, vec4f dir, float &min_t, vec4f vertices[4][3], int count, bool no_cull)
{
  mat43f p0, p1, p2;
  v_mat44_transpose_to_mat43(vertices[0][0], vertices[1][0], vertices[2][0], vertices[3][0], p0.row0, p0.row1, p0.row2);
  v_mat44_transpose_to_mat43(vertices[0][1], vertices[1][1], vertices[2][1], vertices[3][1], p1.row0, p1.row1, p1.row2);
  v_mat44_transpose_to_mat43(vertices[0][2], vertices[1][2], vertices[2][2], vertices[3][2], p2.row0, p2.row1, p2.row2);

  unsigned bitMask = (1u << uint8_t(count)) - 1u;
  vec4f vecMask = v_make_vec4f_mask(bitMask);
  return traceray4Triangles(from, dir, min_t, p0, p1, p2, no_cull, vecMask);
}

static int __forceinline rayhit4Triangles(vec4f from, vec4f dir, float len, vec4f vertices[4][3], int count, bool no_cull)
{
  mat43f p0, p1, p2;
  v_mat44_transpose_to_mat43(vertices[0][0], vertices[1][0], vertices[2][0], vertices[3][0], p0.row0, p0.row1, p0.row2);
  v_mat44_transpose_to_mat43(vertices[0][1], vertices[1][1], vertices[2][1], vertices[3][1], p1.row0, p1.row1, p1.row2);
  v_mat44_transpose_to_mat43(vertices[0][2], vertices[1][2], vertices[2][2], vertices[3][2], p2.row0, p2.row1, p2.row2);

  unsigned bitMask = (1u << uint8_t(count)) - 1u;
  vec4f vecMask = v_make_vec4f_mask(bitMask);
  return rayhit4Triangles(from, dir, len, p0, p1, p2, no_cull, vecMask);
}

static int __forceinline traceray4TrianglesCullCCW(vec4f from, vec4f dir, float &min_t, vec4f vertices[4][3], int count)
{
  return traceray4Triangles(from, dir, min_t, vertices, count, true);
}

static int __forceinline rayhit4TrianglesCullCCW(vec4f from, vec4f dir, float len, vec4f vertices[4][3], int count)
{
  return rayhit4Triangles(from, dir, len, vertices, count, false);
}

static int __forceinline traceray4TrianglesMask(vec4f from, vec4f dir, vec4f &min_t, vec4f vertices[4][3], bool no_cull)
{
  mat43f p0, p1, p2;
  v_mat44_transpose_to_mat43(vertices[0][0], vertices[1][0], vertices[2][0], vertices[3][0], p0.row0, p0.row1, p0.row2);
  v_mat44_transpose_to_mat43(vertices[0][1], vertices[1][1], vertices[2][1], vertices[3][1], p1.row0, p1.row1, p1.row2);
  v_mat44_transpose_to_mat43(vertices[0][2], vertices[1][2], vertices[2][2], vertices[3][2], p2.row0, p2.row1, p2.row2);
  return traceray4TrianglesMask(from, dir, min_t, p0, p1, p2, no_cull);
}

static int __forceinline traceray4TrianglesMaskNoCull(vec4f from, vec4f dir, vec4f &min_t, vec4f vertices[4][3])
{
  return traceray4TrianglesMask(from, dir, min_t, vertices, true);
}

#ifdef __AVX__
#include <immintrin.h>

typedef __m256 vec4x2f;
typedef __m256i vec4x2i;

struct mat43x2f
{
  vec4x2f row0, row1, row2;
};

static mat43x2f __forceinline v_mat43x2_sub_vec4x2f(mat43x2f a, vec4x2f b)
{
  mat43x2f dest;
  dest.row0 = _mm256_sub_ps(a.row0, _mm256_permute_ps(b, _MM_SHUFFLE(0, 0, 0, 0)));
  dest.row1 = _mm256_sub_ps(a.row1, _mm256_permute_ps(b, _MM_SHUFFLE(1, 1, 1, 1)));
  dest.row2 = _mm256_sub_ps(a.row2, _mm256_permute_ps(b, _MM_SHUFFLE(2, 2, 2, 2)));
  return dest;
}

static mat43x2f __forceinline v_mat43x2_sub(mat43x2f a, mat43x2f b)
{
  mat43x2f dest;
  dest.row0 = _mm256_sub_ps(a.row0, b.row0);
  dest.row1 = _mm256_sub_ps(a.row1, b.row1);
  dest.row2 = _mm256_sub_ps(a.row2, b.row2);
  return dest;
}

static vec4x2f __forceinline v_mat43x2_dot(mat43x2f a, mat43x2f b)
{
  __m256 r0 = _mm256_mul_ps(a.row0, b.row0);
  __m256 r1 = _mm256_mul_ps(a.row1, b.row1);
  __m256 r2 = _mm256_mul_ps(a.row2, b.row2);
  return _mm256_add_ps(r2, _mm256_add_ps(r0, r1));
}

static mat43x2f __forceinline v_mat43x2_cross(mat43x2f a, mat43x2f b)
{
  mat43x2f dest;
  dest.row0 = _mm256_sub_ps(_mm256_mul_ps(a.row1, b.row2), _mm256_mul_ps(a.row2, b.row1));
  dest.row1 = _mm256_sub_ps(_mm256_mul_ps(a.row2, b.row0), _mm256_mul_ps(a.row0, b.row2));
  dest.row2 = _mm256_sub_ps(_mm256_mul_ps(a.row0, b.row1), _mm256_mul_ps(a.row1, b.row0));
  return dest;
}

static vec4x2f __forceinline traceray8TrianglesVecMask(vec4x2f from, vec4x2f dir, vec4x2f &t, mat43x2f p0, mat43x2f p1, mat43x2f p2,
  bool no_cull)
{
  mat43x2f e1 = v_mat43x2_sub(p0, p1);
  mat43x2f e2 = v_mat43x2_sub(p2, p0);
  mat43x2f Ng = v_mat43x2_cross(e1, e2);

  mat43x2f D;
  D.row0 = _mm256_permute_ps(dir, _MM_SHUFFLE(0, 0, 0, 0));
  D.row1 = _mm256_permute_ps(dir, _MM_SHUFFLE(1, 1, 1, 1));
  D.row2 = _mm256_permute_ps(dir, _MM_SHUFFLE(2, 2, 2, 2));
  mat43x2f C = v_mat43x2_sub_vec4x2f(p0, from);
  mat43x2f R = v_mat43x2_cross(D, C);
  vec4x2f det = v_mat43x2_dot(Ng, D);

  unsigned allBits = ~0u;
  vec4x2f absMask = _mm256_castsi256_ps(_mm256_set1_epi32(allBits >> (no_cull ? 1 : 0)));

  vec4x2f sgnDet = _mm256_andnot_ps(absMask, det);
  det = _mm256_and_ps(det, absMask);

  vec4x2f U = _mm256_xor_ps(v_mat43x2_dot(R, e2), sgnDet);
  vec4x2f V = _mm256_xor_ps(v_mat43x2_dot(R, e1), sgnDet);

  // Ordered cmp gives false for NaN's in input
  vec4x2f valid = _mm256_and_ps(_mm256_cmp_ps(U, _mm256_set1_ps(-TRACE_EPSILON), _CMP_GE_OQ),
    _mm256_cmp_ps(V, _mm256_set1_ps(-TRACE_EPSILON), _CMP_GE_OQ));
  vec4x2f W1 = _mm256_div_ps(_mm256_add_ps(U, V), det);
  valid = _mm256_and_ps(valid, _mm256_cmp_ps(_mm256_set1_ps(TRACE_ONE_PLUS_EPSILON), W1, _CMP_GE_OQ));
  vec4x2f T = _mm256_xor_ps(v_mat43x2_dot(Ng, C), sgnDet);
  T = _mm256_div_ps(T, det);
  valid = _mm256_and_ps(valid, _mm256_and_ps(_mm256_cmp_ps(det, _mm256_setzero_ps(), _CMP_GT_OQ),
                                 _mm256_and_ps(_mm256_cmp_ps(T, _mm256_setzero_ps(), _CMP_GE_OQ), _mm256_cmp_ps(T, t, _CMP_LE_OQ))));
  t = _mm256_blendv_ps(t, T, valid);
  //_mm256_zeroupper(); will be inserted by compiler, don't add it manually
  return valid;
}

static int __forceinline traceray8Triangles(vec4x2f from, vec4x2f dir, float &min_t, mat43x2f p0, mat43x2f p1, mat43x2f p2,
  bool no_cull)
{
  vec4x2f T = _mm256_set1_ps(min_t);
  vec4x2f valid = traceray8TrianglesVecMask(from, dir, T, p0, p1, p2, no_cull);
  if (_mm256_movemask_ps(valid) == 0x0)
    return -1;

  T = _mm256_blendv_ps(_mm256_set_m128(V_C_MAX_VAL, V_C_MAX_VAL), T, valid);
  vec4x2f minT = _mm256_min_ps(_mm256_permute_ps(T, _MM_SHUFFLE(0, 3, 2, 1)), T);
  minT = _mm256_min_ps(_mm256_permute_ps(minT, _MM_SHUFFLE(1, 0, 3, 2)), minT);
  __m128 minL = _mm256_extractf128_ps(minT, 0);
  __m128 minH = _mm256_extractf128_ps(minT, 1);
  __m128 minx4 = _mm_min_ps(_mm_set1_ps(min_t), _mm_min_ps(minL, minH));
  min_t = _mm_cvtss_f32(minx4);

  // select one valid element by min t
  __m256 minx8 = _mm256_set_m128(minx4, minx4);
  __m256 resultMask = _mm256_cmp_ps(minx8, T, _CMP_EQ_OQ);
  int signMask = _mm256_movemask_ps(resultMask);
  return __bsf_unsafe(signMask);
}

static int __forceinline rayhit8Triangles(vec4x2f from, vec4x2f dir, float t, mat43x2f p0, mat43x2f p1, mat43x2f p2, bool no_cull)
{
  vec4x2f T = _mm256_set1_ps(t);
  vec4x2f valid = traceray8TrianglesVecMask(from, dir, T, p0, p1, p2, no_cull);
  return _mm256_movemask_ps(valid);
}

static mat43x2f __forceinline v_mat44x2_transpose_to_mat43x2(vec4x2f r0, vec4x2f r1, vec4x2f r2, vec4x2f r3)
{
  vec4x2f l02 = _mm256_unpacklo_ps(r0, r2);
  vec4x2f h02 = _mm256_unpackhi_ps(r0, r2);
  vec4x2f l13 = _mm256_unpacklo_ps(r1, r3);
  vec4x2f h13 = _mm256_unpackhi_ps(r1, r3);
  mat43x2f dest;
  dest.row0 = _mm256_unpacklo_ps(l02, l13);
  dest.row1 = _mm256_unpackhi_ps(l02, l13);
  dest.row2 = _mm256_unpacklo_ps(h02, h13);
  return dest;
}

static vec4x2f __forceinline v_make_vec4x2f_mask(uint8_t bitmask)
{
  vec4x2f lookup = _mm256_castsi256_ps(_mm256_set_epi32(0x3f800000 + (1 << 7), 0x3f800000 + (1 << 6), 0x3f800000 + (1 << 5),
    0x3f800000 + (1 << 4), 0x3f800000 + (1 << 3), 0x3f800000 + (1 << 2), 0x3f800000 + (1 << 1), 0x3f800000 + (1 << 0)));

  vec4x2f fbitmask = _mm256_castsi256_ps(_mm256_set1_epi32(bitmask | 0x3f800000)); // for float cmp on avx1
  vec4x2f isolated = _mm256_and_ps(fbitmask, lookup);
  return _mm256_cmp_ps(isolated, lookup, _CMP_EQ_OQ);
}

struct bbox3x2f
{
  vec4x2f bmin, bmax;
};

static void __forceinline v_bbox3x2_init(bbox3x2f &b, vec4x2f p)
{
  b.bmin = p;
  b.bmax = p;
}

static void __forceinline v_bbox3x2_add_pt(bbox3x2f &b, vec4x2f p)
{
  b.bmin = _mm256_min_ps(b.bmin, p);
  b.bmax = _mm256_max_ps(b.bmax, p);
}

template <bool check_bounding>
static bool __forceinline checkBoxBoundingAndTranspose(vec4f from, vec4f dir, float t, vec4f vertices[8][3], mat43x2f &p0,
  mat43x2f &p1, mat43x2f &p2)
{
  vec4x2f v[3][4];
  bbox3x2f dbbox;
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      v[i][j] = _mm256_set_m128(vertices[j + 4][i], vertices[j][i]);
      if (i == 0 && j == 0)
        v_bbox3x2_init(dbbox, v[i][j]);
      else
        v_bbox3x2_add_pt(dbbox, v[i][j]);
    }
  }

  bbox3f bbox0 = {_mm256_extractf128_ps(dbbox.bmin, 0), _mm256_extractf128_ps(dbbox.bmax, 0)};
  bbox3f bbox1 = {_mm256_extractf128_ps(dbbox.bmin, 1), _mm256_extractf128_ps(dbbox.bmax, 1)};
  if (check_bounding && !v_test_ray_box_intersection_unsafe(from, dir, v_set_x(t), v_bbox3_sum(bbox0, bbox1)))
    return false;

  p0 = v_mat44x2_transpose_to_mat43x2(v[0][0], v[0][1], v[0][2], v[0][3]);
  p1 = v_mat44x2_transpose_to_mat43x2(v[1][0], v[1][1], v[1][2], v[1][3]);
  p2 = v_mat44x2_transpose_to_mat43x2(v[2][0], v[2][1], v[2][2], v[2][3]);
  return true;
}

template <bool check_bounding>
static int __forceinline traceray8Triangles(vec4f from, vec4f dir, float &min_t, vec4f vertices[8][3], bool no_cull)
{
  mat43x2f p0, p1, p2;
  if (!checkBoxBoundingAndTranspose<check_bounding>(from, dir, min_t, vertices, p0, p1, p2))
    return -1;
  return traceray8Triangles(_mm256_set_m128(from, from), _mm256_set_m128(dir, dir), min_t, p0, p1, p2, no_cull);
}

template <bool check_bounding>
static int __forceinline rayhit8Triangles(vec4f from, vec4f dir, float t, vec4f vertices[8][3], bool no_cull)
{
  mat43x2f p0, p1, p2;
  if (!checkBoxBoundingAndTranspose<check_bounding>(from, dir, t, vertices, p0, p1, p2))
    return 0;
  return rayhit8Triangles(_mm256_set_m128(from, from), _mm256_set_m128(dir, dir), t, p0, p1, p2, no_cull);
}

template <bool check_bounding>
static int __forceinline traceray8TrianglesCullCCW(vec4f from, vec4f dir, float &min_t, vec4f vertices[8][3])
{
  return traceray8Triangles<check_bounding>(from, dir, min_t, vertices, false);
}

template <bool check_bounding>
static int __forceinline rayhit8TrianglesCullCCW(vec4f from, vec4f dir, float t, vec4f vertices[8][3])
{
  return rayhit8Triangles<check_bounding>(from, dir, t, vertices, false);
}

static int __forceinline traceray8TrianglesMask(vec4x2f from, vec4x2f dir, vec4x2f &t, mat43x2f p0, mat43x2f p1, mat43x2f p2,
  bool no_cull)
{
  vec4x2f valid = traceray8TrianglesVecMask(from, dir, t, p0, p1, p2, no_cull);
  return _mm256_movemask_ps(valid);
}

template <bool check_bounding>
static int __forceinline traceray8TrianglesMask(vec4f from, vec4f dir, float in_t, float (&out_t)[8], vec4f vertices[8][3],
  bool no_cull)
{
  mat43x2f p0, p1, p2;
  if (!checkBoxBoundingAndTranspose<check_bounding>(from, dir, in_t, vertices, p0, p1, p2))
    return 0;

  vec4x2f vOutT = _mm256_set1_ps(in_t);
  int ret = traceray8TrianglesMask(_mm256_set_m128(from, from), _mm256_set_m128(dir, dir), vOutT, p0, p1, p2, no_cull);
  _mm256_store_ps(out_t, vOutT);
  return ret;
}

#endif

#undef TRACE_EPSILON
#undef TRACE_ONE_PLUS_EPSILON
