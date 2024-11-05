//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_bounds3.h>
#include <vecmath/dag_vecMath.h>

/// @addtogroup math
/// @{
struct Frustum
{
  Frustum() //-V730
  {
    plane03W2 = plane03W = camPlanes[0] = v_zero();
  }
  Frustum(const TMatrix4 &matrix) { construct(matrix); } //-V730
  Frustum(const mat44f &matrix) { construct(matrix); }   //-V730

  void construct(const mat44f &matrix);
  void construct(const TMatrix4 &matrix)
  {
    mat44f globtm;
    globtm.col0 = v_ldu(matrix.m[0]);
    globtm.col1 = v_ldu(matrix.m[1]);
    globtm.col2 = v_ldu(matrix.m[2]);
    globtm.col3 = v_ldu(matrix.m[3]);
    construct(globtm);
  }

  enum
  {
    OUTSIDE = 0,
    INSIDE = 1,
    INTERSECT = 2
  };
  enum
  {
    RIGHT = 0,
    LEFT = 1,
    TOP = 2,
    BOTTOM = 3,
    FARPLANE = 4,
    NEARPLANE = 5
  };
  enum
  {
    NO_PLANES_MASK = 0,
    ALL_PLANES_MASK = 63
  };
  // radius is splatted!
  inline int testSphereOrthoB(vec3f c, vec4f rad) const;     // ortho projection matrix!
  inline int testSphereOrtho(vec3f center, vec3f rad) const; // ortho projection matrix!

  inline int testSphereB(vec3f c, vec4f rad) const;
  inline int testSphere(vec3f c, vec4f rad) const;

  __forceinline int testSphereB(const Point3 &c, real rad) const { return testSphereB(v_ldu(&c.x), v_splats(rad)); }

  __forceinline int testSphere(const Point3 &c, real rad) const { return testSphere(v_ldu(&c.x), v_splats(rad)); }

  int testSphere(const Point3 &center, float radius, unsigned int &last_plane, unsigned int in_mask, unsigned int &out_mask) const;

  bool testSweptSphere(const BSphere3 &sphere, const Point3 &sweep_dir) const
  {
    return testSweptSphere(sphere.c, sphere.r, sweep_dir);
  }
  bool testSweptSphere(const Point3 &sphere_center, float sphere_radius, const Point3 &sweep_dir) const;
  bool testSweptSphere(vec4f sphere_center, vec4f sphere_radius, vec4f sweep_dir) const;

  int testSphere(const BSphere3 &sphere) const { return testSphere(sphere.c, sphere.r); }
  // return only true:false
  int testBoxExtentB(vec4f center2, vec4f extent2) const; //(bmax+bmin), (bmax-bmin)
  int testBoxExtent(vec4f center2, vec4f extent2) const;
  int testBoxB(vec4f bmin, vec4f bmax) const
  {
    vec4f center = v_add(bmax, bmin);
    vec4f extent = v_sub(bmax, bmin);
    return testBoxExtentB(center, extent);
  }
  int testBox(vec4f bmin, vec4f bmax) const
  {
    vec4f center = v_add(bmax, bmin);
    vec4f extent = v_sub(bmax, bmin);
    return testBoxExtent(center, extent);
  }
  // int testBox3(vec4f bmin, vec4f bmax) const;
  // int testBoxB3(vec4f bmin, vec4f bmax) const;
  __forceinline int testBoxB(const BBox3 &box) const
  {
    vec3f bmin = v_ldu(&box[0].x);
    vec3f bmax = v_ldu(&box[1].x);
    return testBoxB(bmin, bmax);
  }

  __forceinline int testBox(const BBox3 &box) const
  {
    vec3f bmin = v_ldu(&box[0].x);
    vec3f bmax = v_ldu(&box[1].x);
    return testBox(bmin, bmax);
  }

  /// obsolete! it is actually ~average 3 time slower then just testBox, except for very early exit!
  int testBox(vec4f bmin, vec4f bmax, unsigned int &last_start_plane, unsigned int in_mask, unsigned int &out_mask) const;

  __forceinline int testBox(const BBox3 &box, unsigned int &last_start_plane, unsigned int in_mask, unsigned int &out_mask) const
  {
    vec3f bmin = v_ldu(&box[0].x);
    vec3f bmax = v_ldu(&box[1].x);
    return testBox(bmin, bmax, last_start_plane, in_mask, out_mask);
  }

  plane3f camPlanes[6];
  vec4f plane03X, plane03Y, plane03Z, plane03W2, plane03W, plane4W2, plane5W2;
  // vec4f plane45X,plane45Y,plane45Z, plane45W;

  // generate 8 points
  void generateAllPointFrustm(vec3f *pntList) const;
  void calcFrustumBBox(bbox3f &box) const;
};

//  test if a sphere is within the view frustum

inline int Frustum::testSphereB(vec3f center, vec3f r) const
{
  return v_is_visible_sphere(center, r, plane03X, plane03Y, plane03Z, plane03W, camPlanes[Frustum::FARPLANE],
    camPlanes[Frustum::NEARPLANE]);
}

inline int Frustum::testSphere(vec3f center, vec3f r) const
{
  return v_sphere_intersect(center, r, plane03X, plane03Y, plane03Z, plane03W, camPlanes[Frustum::FARPLANE],
    camPlanes[Frustum::NEARPLANE]);
}

// in ortho frustum, each odd plane is faced backwards to prev even plane. save dot product
inline int Frustum::testSphereOrthoB(vec3f c, vec3f rad) const
{
  return testSphereB(c, rad); // fixme: make sense for plane4 and plane5 only
  /*  vec4f nrad = v_neg(rad);

    vec4f planeDist, dist, outside;
    planeDist = v_dot3_x(camPlanes[0], c);
    dist = v_add_x(planeDist, v_splat_w(camPlanes[0]));
    outside = v_cmp_gt(nrad, dist);
    dist = v_add_x(v_neg(planeDist), v_splat_w(camPlanes[1]));
    outside = v_or(outside, v_cmp_gt(nrad, dist));
  #define LOOP_STEP(x)\
    planeDist = v_dot3_x(camPlanes[x], c);\
    dist = v_add_x(planeDist, v_splat_w(camPlanes[x]));\
    outside = v_or(outside, v_cmp_gt(nrad, dist));\
    dist = v_add_x(v_neg(planeDist), v_splat_w(camPlanes[x+1]));\
    outside = v_or(outside, v_cmp_gt(nrad, dist));

    LOOP_STEP(2);
    LOOP_STEP(4);

  #undef LOOP_STEP

  #if _TARGET_SIMD_SSE
    return (~_mm_movemask_ps(outside))&1;
  #else
    return v_test_vec_x_eqi_0(outside);
  #endif*/
}

inline int Frustum::testSphereOrtho(vec3f c, vec3f rad) const
{
  return testSphere(c, rad);
  /*  vec4f nrad = v_neg(rad);

    vec4f planeDist, dist, outside, intersect;
    planeDist = v_dot3_x(camPlanes[0], c);
    dist = v_add_x(planeDist, v_splat_w(camPlanes[0]));
    outside = v_cmp_gt(nrad, dist);
    intersect = v_cmp_gt(rad, dist);
    dist = v_add_x(v_neg(planeDist), v_splat_w(camPlanes[1]));
    outside = v_or(outside, v_cmp_gt(nrad, dist));
    intersect = v_or(intersect, v_cmp_gt(rad, dist));

  #define LOOP_STEP(x)\
    planeDist = v_dot3_x(camPlanes[x], c);\
    dist = v_add_x(planeDist, v_splat_w(camPlanes[x]));\
    outside = v_or(outside, v_cmp_gt(nrad, dist));\
    intersect = v_or(intersect, v_cmp_gt(rad, dist));\
    dist = v_add_x(v_neg(planeDist), v_splat_w(camPlanes[x+1]));\
    outside = v_or(outside, v_cmp_gt(nrad, dist));\
    intersect = v_or(intersect, v_cmp_gt(rad, dist));\

    LOOP_STEP(2);
    LOOP_STEP(4);

  #undef LOOP_STEP

  #if _TARGET_SIMD_SSE
    int out_intersect = _mm_movemask_ps(v_merge_hw(outside, intersect));

    return ((1|(0<<2)|(2<<4)|(0<<6)) >> ((out_intersect&3)<<1))&3;
    //if (out_intersect&1)
    //  return 0;
    //return (out_intersect&2) ? 2 : 1;
  #else
    if (!v_test_vec_x_eqi_0(outside))
      return 0;
    return v_test_vec_x_eqi_0(intersect) ? 1 : 2;
  #endif
  */
}


inline int Frustum::testBoxExtentB(vec4f center, vec4f extent) const // center and extent should be multiplied by 2
{
  return v_is_visible_box_extent2(center, extent, plane03X, plane03Y, plane03Z, plane03W2, plane4W2, plane5W2);
}

inline int Frustum::testBoxExtent(vec4f center, vec4f extent) const // center and extent should be multiplied by 2
{
  return v_box_frustum_intersect_extent2(center, extent, plane03X, plane03Y, plane03Z, plane03W2, plane4W2, plane5W2);
}

// zfar_plane should be normalized and faced towards camera origin. camPlanes[Frustum::FARPLANE] in Frustum is of that kind
// v_plane_dist(zfar_plane, cur_view_pos) - should be positive!
inline plane3f shrink_zfar_plane(plane3f zfar_plane, vec4f cur_view_pos, vec4f max_z_far_dist)
{
  vec4f zfarDist = v_plane_dist(zfar_plane, cur_view_pos);
  vec4f newZFarDist = v_min(max_z_far_dist, v_splat_w(zfarDist));
  vec4f ofsDist = v_sub(newZFarDist, zfarDist);
  return v_perm_xyzd(zfar_plane, v_add(zfar_plane, ofsDist));
}

// znear_plane should be normalized and faced backwards  camera origin. camPlanes[Frustum::NEARPLANE] in Frustum is of that kind
// max_z_near_dist - is positive
// v_plane_dist(znear_plane, cur_view_pos) - should be negative!
inline plane3f expand_znear_plane(plane3f znear_plane, vec4f cur_view_pos, vec4f max_z_near_dist)
{
  vec4f znearDist = v_plane_dist(znear_plane, cur_view_pos); // negative
  vec4f newZnearDist = v_min(v_neg(max_z_near_dist), v_splat_w(znearDist));
  vec4f ofsDist = v_sub(newZnearDist, znearDist);
  return v_perm_xyzd(znear_plane, v_add(znear_plane, ofsDist));
}

inline void shrink_frustum_zfar(Frustum &frustum, vec4f cur_view_pos, vec4f max_z_far_dist)
{
  vec4f originalFarPlane = frustum.camPlanes[Frustum::FARPLANE];
  frustum.camPlanes[Frustum::FARPLANE] = shrink_zfar_plane(originalFarPlane, cur_view_pos, max_z_far_dist);
  frustum.plane4W2 = v_perm_xyzd(frustum.plane4W2, v_add(frustum.camPlanes[Frustum::FARPLANE], frustum.camPlanes[Frustum::FARPLANE]));
}

/// World aligned box test
inline bool test_wabox_frustum_intersection(const BBox3 &box, const Frustum &frustum) { return frustum.testBoxB(box); }

// extern void calculate_six_frustum_planes(const Frustum &frustum, Point4 *six_frustum_planes);
inline void calculate_six_frustum_planes(const Frustum &frustum, Point4 *six_frustum_planes)
{
  for (int i = 0; i < 6; ++i)
    v_stu(&six_frustum_planes[i].x, frustum.camPlanes[i]);
}
inline void calculate_six_frustum_planes(const Frustum &frustum, vec4f *six_frustum_planes)
{
  for (int i = 0; i < 6; ++i)
    six_frustum_planes[i] = frustum.camPlanes[i];
}


/// @}
