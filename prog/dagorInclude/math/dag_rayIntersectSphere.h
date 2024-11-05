//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath_common.h>
#include <util/dag_globDef.h>

__forceinline bool rayIntersectSphere(const Point3 &p, const Point3 &normalized_dir, const Point3 &sphere_center, real r2, real mint)
{
  return v_test_ray_sphere_intersection(v_ldu(&p.x), v_ldu(&normalized_dir.x), v_splats(mint), v_ldu(&sphere_center.x), v_set_x(r2));
}

__forceinline bool rayIntersectSphere(const Point3 &p, const Point3 &normalized_dir, const Point3 &sphere_center, real r2)
{
  Point3 pc = sphere_center - p;
  real c = pc * pc - r2;
  if (c >= 0)
  {
    real b = (pc * normalized_dir);
    if (b < 0)
      return false;
    // b *= 2.0f;
    real d = b * b - c; //*4.0f;
    if (d < 0)
      return false;
  }
  return true;
}

__forceinline bool notNormalizedRayIntersectSphere(const Point3 &p, const Point3 &dir, const Point3 &sphere_center, real r2)
{
  Point3 pc = sphere_center - p;
  real c = pc * pc - r2;
  if (c >= 0)
  {
    real b = (pc * dir);
    if (b < 0)
      return false;
    // b *= 2.0f;
    real d = b * b / lengthSq(dir) - c; //*4.0f;
    if (d < 0)
      return false;
  }
  return true;
}

// <0 if there is no intersection
__forceinline float rayIntersectSphereDist(const Point3 &ray_start, const Point3 &ray_normalized_dir, const Point3 &sphere_center,
  float sphere_radius)
{
  Point3 p = ray_start - sphere_center;

  float b = 2.f * (p * ray_normalized_dir);
  float c = p * p - sphere_radius * sphere_radius;
  float d = b * b - 4 * c;
  if (d >= 0)
  {
    float sq = sqrtf(d);
    float v0 = (-b - sq) * 0.5f;
    float v1 = (-b + sq) * 0.5f;

    return v0 >= 0.f ? v0 : max(v0, v1);
  }

  return -1.f;
}

__forceinline float rayIntersectSphereDist(vec3f ray_start, vec3f ray_normalized_dir, vec3f sphere_center, float sphere_radius)
{
  vec3f p = v_sub(ray_start, sphere_center);

  float c = v_extract_x(v_length3_sq_x(p)) - sphere_radius * sphere_radius;
  if (c < 0.f)
    return 0.f;
  float b = 2.f * v_extract_x(v_dot3_x(p, ray_normalized_dir));
  float d = b * b - 4 * c;
  if (d >= 0)
  {
    float sq = sqrtf(d);
    float v0 = (-b - sq) * 0.5f;
    float v1 = (-b + sq) * 0.5f;

    return v0 >= 0.f ? v0 : max(v0, v1);
  }

  return -1.f;
}
