//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
class Plane3;
class Point3;
// As in TFrustum, plane normals are oriented inwards.

/// Constructs an occluder for a quadrilateral. No requirements for clockwise/contraclockwise.
/// Will return false, if it is seen as a line from the eye position.
///  \code
/// occluder[2]---------------------------occluder[3]
///          |                            |
///          |                            |
///          |         ordering:          |
///          |      corner = Y*2 + X,     |
///          |       X, Y is 0 or 1       |
///          |                            |
///          |                            |
/// occluder[0]---------------------------occluder[1]
///  \endcode
///  @param occluder
///      four points (in one plane)
///  @param plane
///      normalized plane for occluder param
///  @param out_frustum_planes
///      pointer to five frustum planes.
///  @return
///      false, if there is no frustum. true, if out_frustum_planes contains valid planes.

bool frustum_for_4p_occluder(const Point3 occluder[4], const Plane3 &plane, const Point3 &eye, plane3f out_frustum_planes[5]);

__forceinline int box_plane_side(vec3f center, vec3f size, const plane3f &plane)
{
  vec3f d1, d2;
  d1 = v_plane_dist_x(plane, center); // plane.distance( center );
  d2 = v_mul(size, plane);
  d2 = v_abs(d2);
  d2 = v_dot3_x(d2, V_C_ONE);
  return v_test_vec_x_lt(d1, d2);
}

__forceinline bool is_box_occluded(vec3f center, vec3f size, const plane3f frustum_planes[], int planes_count)
{
  for (int i = 0; i < planes_count; ++i)
    if (box_plane_side(center, size, frustum_planes[i]))
      return false;
  return true;
}

__forceinline int box_plane_side_full(const vec3f center, const vec3f size, const plane3f &plane)
{
  vec3f d1, d2;
  d1 = v_plane_dist(plane, center);
  d2 = v_mul(size, plane);
  d2 = v_abs(d2);
  d2 = v_dot3(d2, V_C_ONE);
#if _TARGET_X360
  unsigned cr = 0;
  __vcmpgefpR(d2, d1, &cr);
  if (cr & (1 << 5))
    return 1;
  unsigned cr = 0;
  __vcmpgtfpR(d1, v_neg(d2), &cr);
  if (cr & (1 << 5))
    return -1;
#else
  if (v_test_vec_x_gt(d1, d2))
    return 1;
  if (v_test_vec_x_lt(d1, v_neg(d2)))
    return -1;
#endif
  return 0;
  // positive = v_sub(d1,d2);
  // negative = v_add(d1,d2);
  // if ( v_test_vec_x_gt_0(positive) )
  //   return 1;//PLANESIDE_FRONT;
  // if ( v_test_vec_x_lt_0(negative) )
  //   return -1;//PLANESIDE_BACK;
}

// 0 - never will occlude
// 1 - not occlude
// 2 - occludes
enum
{
  NEVER_OCCLUDE = 0,
  NOT_OCCLUDE = 1,
  ALWAYS_OCCLUDE = 2
};
__forceinline int is_box_occluded_full(vec3f center, vec3f size, const plane3f frustum_planes[], int planes_count)
{
  unsigned ret = 0xFFFFFFFF;
  for (unsigned i = 0; i < planes_count; ++i)
  {
    unsigned val = (unsigned)box_plane_side_full(center, size, frustum_planes[i]);
    if (val & (1 << 31))
      return NEVER_OCCLUDE;
    ret &= val;
  }
  if (!ret)
    return NOT_OCCLUDE;
  return ALWAYS_OCCLUDE;
}
