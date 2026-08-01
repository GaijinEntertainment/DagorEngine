//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_vecMathCompatibility.h>
#include <gameMath/mathUtils.h>

struct Ray3
{
  Point3 start = {};
  Point3 dir = VEC_UNIT_FWD;
  float length = 0.f;
  Point3 end() const { return start + dir * length; }

  Ray3() = default;
  Ray3(const Point3 &s, const Point3 &d, float l) : start(s), dir(d), length(l) {}
};

inline Ray3 operator*(mat44f_cref tm, const Ray3 &r)
{
  Ray3 outRay;
  v_stu_p3(&outRay.start.x, v_mat44_mul_vec3p(tm, v_ldu_p3(&r.start.x)));
  v_stu_p3(&outRay.dir.x, v_norm3(v_mat44_mul_vec3v(tm, v_ldu_p3(&r.dir.x))));
  outRay.length = r.length;
  return outRay;
}

inline Ray3 make_ray_from_segment(const Point3 &p1, const Point3 &p2)
{
  Ray3 ray;
  ray.start = p1;
  ray.dir = ::normalize(p2 - p1);
  ray.length = ::length(p2 - p1);
  return ray;
}

inline Ray3 make_ray_from_segment_and_t(const Point3 &p1, const Point3 &p2, float t)
{
  Ray3 ray;
  ray.start = p1;
  ray.dir = ::normalize(p2 - p1);
  ray.length = t;
  return ray;
}
