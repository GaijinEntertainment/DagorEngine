//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>

class TMatrix;

struct Ray3
{
  Point3 start = {0.f, 0.f, 0.f};
  Point3 dir = {1.f, 0.f, 0.f};
  float length = 0.f;
  Point3 end() const { return start + dir * length; }

  Ray3() = default;
  Ray3(const Point3 &s, const Point3 &d, float l) : start(s), dir(d), length(l) {}
};

Ray3 operator*(const TMatrix &tm, const Ray3 &r);

Ray3 make_ray_from_segment(const Point3 &p1, const Point3 &p2);
Ray3 make_ray_from_segment_and_t(const Point3 &p1, const Point3 &p2, float t);
