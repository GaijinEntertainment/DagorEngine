// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_plane3.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_occluder.h>
#include <util/dag_globDef.h>
#include <math/dag_Point4.h>
#include <math/dag_vecMathCompatibility.h>

//--------------------------------------------------------------------------------------

#define CALC_FRUSTUM_EPS_THRESHOLD 0.0001f

#if DAGOR_DBGLEVEL > 0
static inline bool almost_zero(float f) { return fabsf(f) < CALC_FRUSTUM_EPS_THRESHOLD; }

static inline bool almost_equal(float a, float b) { return almost_zero(a - b); }

static inline bool is_near_plane(const Point3 &point, const Plane3 &plane)
{
  // Might not work well for non-normalized planes with ultra-small/ultra-large values of n, d.
  // In this case, the speed is preferred over additional normalization.
  return almost_zero(plane.distance(point));
}
#endif

//--------------------------------------------------------------------------------------

bool frustum_for_4p_occluder(const Point3 occluder[4], const Plane3 &plane, const Point3 &eye, plane3f out_frustum_planes[5])
{
  const Point3 *oc = occluder;

#if DAGOR_DBGLEVEL > 0
  G_ASSERT(is_near_plane(oc[3], plane) && "frustum_for_4p_occluder: four points are not a plane quad");
  G_ASSERT(almost_equal(plane.n.lengthSq(), 1.f));

  Plane3 test_plane(oc[0], oc[1], oc[2]);
  test_plane.normalize();
  // Check for the correspondence of occluder and plane
  G_ASSERT(almost_zero((test_plane.n - plane.n).lengthSq()));

  // Check for having a convex qudrilateral (or that the vertices are correct).
  static int other_triples[3][3] = {{0, 1, 3}, {1, 3, 2}, {3, 2, 0}};
  for (int ti = 0; ti < 3; ++ti)
  {
    Plane3 other_plane(oc[other_triples[ti][0]], oc[other_triples[ti][1]], oc[other_triples[ti][2]]);
    // Normal should be oriented in the same direction
    G_ASSERT(other_plane.n * plane.n > -CALC_FRUSTUM_EPS_THRESHOLD && "frustum_for_4p_occluder: four points are not convex quad");
  }
#endif

  float plane_eye_dist = plane.distance(eye); // Normal of the quadrilateral is directed either to or from the eye (signed distance).
  if (plane_eye_dist > -CALC_FRUSTUM_EPS_THRESHOLD && plane_eye_dist <= CALC_FRUSTUM_EPS_THRESHOLD)
    return false;

  Plane3 planes[5] = {plane, {eye, oc[0], oc[1]}, {eye, oc[1], oc[3]}, {eye, oc[3], oc[2]}, {eye, oc[2], oc[0]}};
  for (int i = 0; i < 5; ++i)
  {
    if (plane_eye_dist > CALC_FRUSTUM_EPS_THRESHOLD)
      planes[i].flip();
    out_frustum_planes[i] = v_ldu(&planes[i].n.x);
  }
  return true;
}
