// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/ray3.h>

#include <math/dag_TMatrix.h>

Ray3 operator*(const TMatrix &tm, const Ray3 &r) { return {tm * as_point3(&r.start), normalize(tm % as_point3(&r.dir)), r.length}; }
Ray3 operator*(mat44f_cref tm, const Ray3 &r)
{
  return {v_mat44_mul_vec3p(tm, r.start), v_norm3(v_mat44_mul_vec3v(tm, r.dir)), r.length};
}

Ray3 make_ray_from_segment(const Point3 &p1, const Point3 &p2) { return {p1, ::normalize(p2 - p1), ::length(p2 - p1)}; }

Ray3 make_ray_from_segment_and_t(const Point3 &p1, const Point3 &p2, float t) { return {p1, ::normalize(p2 - p1), t}; }
