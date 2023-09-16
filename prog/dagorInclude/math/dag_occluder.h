//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
class Plane3;
class Point3;
class BBox3;
class BSphere3;
class TMatrix;
// As in TFrustum, plane normals are oriented inwards.

#define BOX_OCCLUDER_PLANES_MAX 7

/// Constructs frustum planes for a box. There is only one "front" plane, which is the avarage of fronts
/// for eye, therefore frustum would not be accurate near the occluder.
/// If the point is near the box or a little bit inside of it, the algorithm works correctly, using only one face of the box for
/// frustum.
///  @param to_box_space
///      matrix, which translates a point to the box space (the box istelf is translated to
///      a unit cube [0..1]^3). Matrix is orthogonal, but not orthonormal.
///  @param box_corners
///      8 points for box corners.
///      box_corners[ix + 2*iy + 4*iz] == inverse(to_box_space) * Point3(ix, iy, iz), where ix, iy, iz - either 0 or 1.
///  @param out_frustum_planes
///      pointer to an array of output planes of frustum, up to BOX_OCCLUDER_PLANES_MAX (about 10) planes.
///      Normals will be inward-oriented.
///  @param out_planes_count
///      number of planes in array to which out_frustum_planes is pointing.
void frustum_for_box_occluder(const TMatrix &to_box_space, const Point3 box_corners[8], const Point3 &eye,
  plane3f out_frustum_planes[BOX_OCCLUDER_PLANES_MAX], int *out_planes_count);

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

/// For testing, if the box is located in the half-space, only one of its vertex can be provided. This vertex has to
/// satisfy the requirement: its projection on the plane normal (the value of dot-product) has to be the smallest among all vertices.
/// It befits when testing many boxes for one frustum is needed.
/// This function finds its index.
char prepare_info_for_plane_and_aabb_testing(const plane3f &plane_normal);

/// Is the box fully in the frustum?
extern bool is_box_occluded(const BBox3 &box, const Plane3 frustum_planes[], const char box_corner_indices_for_plane_testing[],
  int planes_count);

/// Is the sphere fully in the frustum?
extern bool is_sphere_occluded(const BSphere3 &sphere, const Plane3 *frustum_planes, int planes_count);

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
