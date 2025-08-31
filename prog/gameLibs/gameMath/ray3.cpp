// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/ray3.h>

#include <math/dag_TMatrix.h>

Ray3 operator*(const TMatrix &tm, const Ray3 &r) { return {tm * r.start, normalize(tm % r.dir), r.length}; }

Ray3 make_ray_from_segment(const Point3 &p1, const Point3 &p2)
{
  Ray3 ray;
  ray.start = p1;
  ray.dir = ::normalize(p2 - p1);
  ray.length = ::length(p2 - p1);
  return ray;
}

Ray3 make_ray_from_segment_and_t(const Point3 &p1, const Point3 &p2, float t)
{
  Ray3 ray;
  ray.start = p1;
  ray.dir = ::normalize(p2 - p1);
  ray.length = t;
  return ray;
}
