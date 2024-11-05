// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_frustum.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_plane3.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_globDef.h>

#define ALMOST_ZERO(F) (fabsf(F) <= 2.0f * FLT_EPSILON)
#define IS_SPECIAL(F)  ((FP_BITS(F) & 0x7f800000L) == 0x7f800000L)

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
//  PlaneIntersection
//    computes the point where three planes intersect
//    returns whether or not the point exists.
static __forceinline vec4f planeIntersection(plane3f p0, plane3f p1, plane3f p2)
{
  vec4f invalid;
  vec4f result = three_plane_intersection(p0, p1, p2, invalid);
  result = v_andnot(invalid, result);
  // G_ASSERT(v_test_vec_x_eqi_0(invalid));//frustum should be always valid (i.e. have these intersections)!
  // return v_sel(result, v_zero(), invalid);
  return result;
}

//  build a frustum from a camera (projection, or viewProjection) matrix
void Frustum::construct(const mat44f &matrix)
{
  v_construct_camplanes(matrix, camPlanes[0], camPlanes[1], camPlanes[2], camPlanes[3], camPlanes[5], camPlanes[4]);
  for (int p = 0; p < 6; p++)
    camPlanes[p] = v_norm3(camPlanes[p]); // .w is divided by length(xyz) as side effect

  //  build a bit-field that will tell us the indices for the nearest and farthest vertices from each plane...
  plane03X = camPlanes[0];
  plane03Y = camPlanes[1];
  plane03Z = camPlanes[2];
  plane03W = camPlanes[3];
  v_mat44_transpose(plane03X, plane03Y, plane03Z, plane03W);
  plane03W2 = v_add(plane03W, plane03W);

  plane4W2 = camPlanes[4];
  plane5W2 = camPlanes[5];
  plane4W2 = v_perm_xyzd(plane4W2, v_add(plane4W2, plane4W2));
  plane5W2 = v_perm_xyzd(plane5W2, v_add(plane5W2, plane5W2));
}

void Frustum::calcFrustumBBox(bbox3f &box) const
{
  box.bmax = box.bmin = planeIntersection(camPlanes[5], camPlanes[2], camPlanes[1]);
  v_bbox3_add_pt(box, planeIntersection(camPlanes[4], camPlanes[2], camPlanes[1]));
  v_bbox3_add_pt(box, planeIntersection(camPlanes[5], camPlanes[3], camPlanes[1]));
  v_bbox3_add_pt(box, planeIntersection(camPlanes[4], camPlanes[3], camPlanes[1]));
  v_bbox3_add_pt(box, planeIntersection(camPlanes[5], camPlanes[2], camPlanes[0]));
  v_bbox3_add_pt(box, planeIntersection(camPlanes[4], camPlanes[2], camPlanes[0]));
  v_bbox3_add_pt(box, planeIntersection(camPlanes[5], camPlanes[3], camPlanes[0]));
  v_bbox3_add_pt(box, planeIntersection(camPlanes[4], camPlanes[3], camPlanes[0]));
}

void Frustum::generateAllPointFrustm(vec3f *pntList) const
{
  pntList[0] = planeIntersection(camPlanes[5], camPlanes[2], camPlanes[1]);
  pntList[1] = planeIntersection(camPlanes[4], camPlanes[2], camPlanes[1]);
  pntList[2] = planeIntersection(camPlanes[5], camPlanes[3], camPlanes[1]);
  pntList[3] = planeIntersection(camPlanes[4], camPlanes[3], camPlanes[1]);
  pntList[4] = planeIntersection(camPlanes[5], camPlanes[2], camPlanes[0]);
  pntList[5] = planeIntersection(camPlanes[4], camPlanes[2], camPlanes[0]);
  pntList[6] = planeIntersection(camPlanes[5], camPlanes[3], camPlanes[0]);
  pntList[7] = planeIntersection(camPlanes[4], camPlanes[3], camPlanes[0]);
}

//  Tests if an AABB is inside/intersecting the view frustum
// todo:
int Frustum::testSphere(const Point3 &center, float radius, unsigned int &last_plane, unsigned int in_mask,
  unsigned int &out_mask) const
{
  G_ASSERT(last_plane < 6 && in_mask <= ALL_PLANES_MASK);

  if (in_mask != ALL_PLANES_MASK)
  {
    bool intersect = false;
    unsigned int k = 1 << last_plane;
    out_mask = 0;
    if (k & in_mask)
    {
      float dist = as_plane3(&camPlanes[last_plane]).distance(center);
      if (dist < -radius)
        return OUTSIDE;
      if (dist < radius)
      {
        out_mask |= k;
        intersect = true;
      }
    }

    for (unsigned int planeNo = 0, planeBit = 1; planeBit <= in_mask; planeNo++, planeBit += planeBit)
    {
      if (planeBit & in_mask && (planeNo != last_plane))
      {
        float dist = as_plane3(&camPlanes[planeNo]).distance(center);
        if (dist < -radius)
          return OUTSIDE;
        if (dist < radius)
        {
          out_mask |= k;
          intersect = true;
        }
      }
    }
    return intersect ? INTERSECT : INSIDE;
  }
  else // fast path
  {
    // fast case
    float dist = as_plane3(&camPlanes[last_plane]).distance(center);
    if (dist < -radius)
    {
      out_mask = 0;
      return OUTSIDE;
    }

    float outside = -1.0f; // false
    float intersect = -1.0f;
    float maskBit = 1.0f;
    float maskFlt = 0.0f;
    for (int i = 0; i < 6; i++)
    {
      const float dist_ = as_plane3(&camPlanes[i]).distance(center);
      outside = fsel(dist_ - (-radius), outside, 1.0f);        //(dist_ < -rad) -> 1.0f
      intersect = fsel(dist_ - radius, intersect, 1.0f);       //(dist_ < rad)  -> 1.0f
      const float mergeMsk = fsel(dist_ - radius, 0.0f, 1.0f); //(dist_ < rad)  -> 1.0f
      maskFlt += maskBit * mergeMsk;
      maskBit += maskBit;
    }

    if (outside >= 0.0f)
    {
      out_mask = 0;
      return OUTSIDE;
    }
    out_mask = (unsigned int)maskFlt; // LHS!
    return (intersect >= 0.0f) ? INTERSECT : INSIDE;
  }
}

int Frustum::testBox(vec4f bmin, vec4f bmax, unsigned int &last_start_plane, unsigned int in_mask, unsigned int &out_mask) const
{
  int result = INSIDE;
  plane3f sp;
  vec4f neg, pos;
  out_mask = 0;
  unsigned last_mask = 1 << last_start_plane;
  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  vec4f center = v_mul(v_add(bmax, bmin), V_C_HALF);
  vec4f extent = v_mul(v_sub(bmax, bmin), V_C_HALF);

  if (in_mask & last_mask)
  {
    in_mask &= ~last_mask;
    sp = camPlanes[last_start_plane];
    pos = v_plane_dist_x(sp, v_add(center, v_xor(extent, v_and(sp, signmask))));
    neg = v_plane_dist_x(sp, v_sub(center, v_xor(extent, v_and(sp, signmask))));
#if _TARGET_SIMD_SSE
    if (_mm_movemask_ps(pos) & 1)
      return OUTSIDE;

    if (_mm_movemask_ps(neg) & 1)
    {
      out_mask |= last_mask;
      result = INTERSECT;
    }
#else
    if (v_test_vec_x_lt_0(pos))
      return OUTSIDE;

    if (v_test_vec_x_lt_0(neg))
    {
      out_mask |= last_mask;
      result = INTERSECT;
    }
#endif
  }

  for (unsigned i = 0, k = 1; k <= in_mask; i++, k += k)
  {
    if (!(k & in_mask))
      continue;

    sp = camPlanes[i];
    pos = v_plane_dist_x(sp, v_add(center, v_xor(extent, v_and(sp, signmask))));
    neg = v_plane_dist_x(sp, v_sub(center, v_xor(extent, v_and(sp, signmask))));

#if _TARGET_SIMD_SSE
    if (_mm_movemask_ps(pos) & 1)
    {
      last_start_plane = i;
      return OUTSIDE;
    }
    if (_mm_movemask_ps(neg) & 1)
    {
      out_mask |= k;
      result = INTERSECT;
    }
#else
    if (v_test_vec_x_lt_0(pos))
    {
      last_start_plane = i;
      return OUTSIDE;
    }
    if (v_test_vec_x_lt_0(neg))
    {
      out_mask |= k;
      result = INTERSECT;
    }
#endif
  }
  return result;
}


// Frustum::testSweptSphere() tests if the projection of a bounding sphere
// along the light direction intersects the view frustum.

bool Frustum::testSweptSphere(const Point3 &sphere_center, float sphere_radius, const Point3 &sweep_dir) const
{
  return testSweptSphere(v_ldu(reinterpret_cast<const float *>(&sphere_center)), v_splats(sphere_radius),
    v_ldu(reinterpret_cast<const float *>(&sweep_dir)));
}

bool Frustum::testSweptSphere(vec4f sphere_center, vec4f sphere_radius, vec4f sweep_dir) const
{
  // Algorithm -- get all 12 intersection points of the swept sphere with the view frustum
  // for all points >0, displace sphere along the sweep direction. If the displaced sphere
  // is inside the frustum, return TRUE. Else, return FALSE.


  // Common setup.

  vec4f zeroSoa = v_zero();
  vec4f oneSoa = v_splats(1.0f);
  vec4f minusOneSoa = v_splats(-1.0f);
  vec4f epsilonSoa = v_splats(2.0f * FLT_EPSILON);
  vec4f maxFloatSoa = v_splats(1.e32f);

#define FSEL(A, B, C) v_sel(C, B, v_cmp_ge(A, zeroSoa))


  // Prepare data for SOA algorithm.

#define LOAD_PLANE_SOA(N)                        \
                                                 \
  plane3f plane##N##x = v_splat_x(camPlanes[N]); \
  plane3f plane##N##y = v_splat_y(camPlanes[N]); \
  plane3f plane##N##z = v_splat_z(camPlanes[N]); \
  plane3f plane##N##w = v_splat_w(camPlanes[N]);

  LOAD_PLANE_SOA(0);
  LOAD_PLANE_SOA(1);
  LOAD_PLANE_SOA(2);
  LOAD_PLANE_SOA(3);
  LOAD_PLANE_SOA(4);
  LOAD_PLANE_SOA(5);

  vec4f sweepDirX = v_splat_x(sweep_dir);
  vec4f sweepDirY = v_splat_y(sweep_dir);
  vec4f sweepDirZ = v_splat_z(sweep_dir);

  vec4f scX = v_splat_x(sphere_center);
  vec4f scY = v_splat_y(sphere_center);
  vec4f scZ = v_splat_z(sphere_center);

  vec4f srSoa = sphere_radius;
  vec4f sr11 = v_mul(srSoa, v_splats(1.1f));

  vec4f disp03, disp47, disp8b;
  vec4f res03, res47, res8b;


  // Calculate 12 points of sphere-plane intersections.
  //
  // static void SweptSpherePlaneIntersect(float& t0, float& t1, const Plane3& plane, const Point3 &sc, float sr, const Point3&
  // sweepDir)
  //{
  //  float b_dot_n = plane.distance(sc);
  //  float d_dot_n = plane.n*sweepDir;
  //
  //  float tmp0 = ( sr - b_dot_n) / d_dot_n;
  //  float tmp1 = (-sr - b_dot_n) / d_dot_n;
  //  t0 = min(tmp0, tmp1);
  //  t1 = max(tmp0, tmp1);
  //
  //  float almost_zero = 2.0f*FLT_EPSILON - fabsf(d_dot_n);
  //
  //  t0 = fsel(almost_zero, fsel(sr - b_dot_n, 0, -1.0f), t0);
  //  t1 = fsel(almost_zero, fsel(sr - b_dot_n, 1.e32f, -1.0f), t1);
  //}
  //
  // SweptSpherePlaneIntersect(disp[0], disp[1], as_plane3(camPlanes[0]), sc, sr, sweepDir);
  // SweptSpherePlaneIntersect(disp[2], disp[3], as_plane3(camPlanes[1]), sc, sr, sweepDir);
  // SweptSpherePlaneIntersect(disp[4], disp[5], as_plane3(camPlanes[2]), sc, sr, sweepDir);
  // SweptSpherePlaneIntersect(disp[6], disp[7], as_plane3(camPlanes[3]), sc, sr, sweepDir);
  // SweptSpherePlaneIntersect(disp[8], disp[9], as_plane3(camPlanes[4]), sc, sr, sweepDir);
  // SweptSpherePlaneIntersect(disp[10], disp[11], as_plane3(camPlanes[5]), sc, sr, sweepDir);

#define SPHERE_PLANE_INTERSECT_SOA(T0, T1, PlaneX, PlaneY, PlaneZ, PlaneW)                          \
  {                                                                                                 \
    vec4f b_dot_n = v_madd(PlaneX, scX, v_madd(PlaneY, scY, v_madd(PlaneZ, scZ, PlaneW)));          \
                                                                                                    \
    vec4f d_dot_n = v_madd(PlaneX, sweepDirX, v_madd(PlaneY, sweepDirY, v_mul(PlaneZ, sweepDirZ))); \
                                                                                                    \
    vec4f almost_zero = v_sub(epsilonSoa, v_abs(d_dot_n));                                          \
                                                                                                    \
    vec4f d_dot_n_inv = v_rcp(FSEL(almost_zero, oneSoa, d_dot_n));                                  \
                                                                                                    \
    vec4f tmp0 = v_mul(v_sub(srSoa, b_dot_n), d_dot_n_inv);                                         \
    vec4f tmp1 = v_mul(v_neg(v_add(srSoa, b_dot_n)), d_dot_n_inv);                                  \
                                                                                                    \
    T0 = v_min(tmp0, tmp1);                                                                         \
    T1 = v_max(tmp0, tmp1);                                                                         \
                                                                                                    \
    vec4f cmp = v_sub(srSoa, b_dot_n);                                                              \
    T0 = FSEL(almost_zero, FSEL(cmp, zeroSoa, minusOneSoa), T0);                                    \
    T1 = FSEL(almost_zero, FSEL(cmp, maxFloatSoa, minusOneSoa), T1);                                \
  }

  plane3f plane45X = v_perm_xaxa(plane4x, plane5x); // Only first two values of each vector matter.
  plane3f plane45Y = v_perm_xaxa(plane4y, plane5y);
  plane3f plane45Z = v_perm_xaxa(plane4z, plane5z);
  plane3f plane45W = v_perm_xaxa(plane4w, plane5w);

  vec4f disp8a, disp9b;

  SPHERE_PLANE_INTERSECT_SOA(disp03, disp47, plane03X, plane03Y, plane03Z, plane03W);
  SPHERE_PLANE_INTERSECT_SOA(disp8a, disp9b, plane45X, plane45Y, plane45Z, plane45W);

  disp8b = v_perm_xyab(disp8a, disp9b);


  // Do 12 sphere-frustum tests.
  //
  //// returns 0.0f in case of intersection, -1.0f otherwise
  // static float testSphereFloat(const plane3f camPlanes[], const Point3& c, real rad)
  //{
  //   const float dist0 = as_plane3(camPlanes[0]).distance(c);
  //   const float dist1 = as_plane3(camPlanes[1]).distance(c);
  //   const float dist2 = as_plane3(camPlanes[2]).distance(c);
  //   const float dist3 = as_plane3(camPlanes[3]).distance(c);
  //   const float dist4 = as_plane3(camPlanes[4]).distance(c);
  //   const float dist5 = as_plane3(camPlanes[5]).distance(c);
  //
  //   float res = fsel(dist0 + rad,
  //               fsel(dist1 + rad,
  //               fsel(dist2 + rad,
  //               fsel(dist3 + rad,
  //               fsel(dist4 + rad,
  //               fsel(dist5 + rad, 0, -1), -1), -1), -1), -1), -1);
  //
  //   return res;
  // }
  //
  //   float res = 1.0f;
  //
  //   res *= fsel(disp[0], testSphereFloat(camPlanes, sc + sweepDir*disp[0], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[1], testSphereFloat(camPlanes, sc + sweepDir*disp[1], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[2], testSphereFloat(camPlanes, sc + sweepDir*disp[2], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[3], testSphereFloat(camPlanes, sc + sweepDir*disp[3], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[4], testSphereFloat(camPlanes, sc + sweepDir*disp[4], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[5], testSphereFloat(camPlanes, sc + sweepDir*disp[5], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[6], testSphereFloat(camPlanes, sc + sweepDir*disp[6], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[7], testSphereFloat(camPlanes, sc + sweepDir*disp[7], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[8], testSphereFloat(camPlanes, sc + sweepDir*disp[8], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[9], testSphereFloat(camPlanes, sc + sweepDir*disp[9], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[10], testSphereFloat(camPlanes, sc + sweepDir*disp[10], sr * 1.1f), 1.0f);
  //   res *= fsel(disp[11], testSphereFloat(camPlanes, sc + sweepDir*disp[11], sr * 1.1f), 1.0f);

#define CALC_DISTANCE_SOA(N) const vec4f dist##N = v_madd(plane##N##x, x, v_madd(plane##N##y, y, v_madd(plane##N##z, z, plane##N##w)));

#define TEST_SPHERE_SOA(RES, DISP)                                                                                        \
  {                                                                                                                       \
    const vec4f x = v_madd(sweepDirX, DISP, scX);                                                                         \
    const vec4f y = v_madd(sweepDirY, DISP, scY);                                                                         \
    const vec4f z = v_madd(sweepDirZ, DISP, scZ);                                                                         \
                                                                                                                          \
    CALC_DISTANCE_SOA(0);                                                                                                 \
    CALC_DISTANCE_SOA(1);                                                                                                 \
    CALC_DISTANCE_SOA(2);                                                                                                 \
    CALC_DISTANCE_SOA(3);                                                                                                 \
    CALC_DISTANCE_SOA(4);                                                                                                 \
    CALC_DISTANCE_SOA(5);                                                                                                 \
                                                                                                                          \
    RES = FSEL(v_add(dist0, sr11),                                                                                        \
      FSEL(v_add(dist1, sr11),                                                                                            \
        FSEL(v_add(dist2, sr11),                                                                                          \
          FSEL(v_add(dist3, sr11), FSEL(v_add(dist4, sr11), FSEL(v_add(dist5, sr11), zeroSoa, minusOneSoa), minusOneSoa), \
            minusOneSoa),                                                                                                 \
          minusOneSoa),                                                                                                   \
        minusOneSoa),                                                                                                     \
      minusOneSoa);                                                                                                       \
                                                                                                                          \
    RES = FSEL(DISP, RES, oneSoa);                                                                                        \
  }

  TEST_SPHERE_SOA(res03, disp03);
  TEST_SPHERE_SOA(res47, disp47);
  TEST_SPHERE_SOA(res8b, disp8b);


  // Combine results.

  vec4f res = v_and(v_and(res03, res47), res8b);
  res = v_and(v_and(v_and(res, v_splat_y(res)), v_splat_z(res)), v_splat_w(res));
  return (v_test_vec_x_eqi_0(res));

#undef TEST_SPHERE_SOA
#undef CALC_DISTANCE_SOA
#undef SPHERE_PLANE_INTERSECT_SOA
#undef LOAD_PLANE_SOA
#undef FSEL
}
