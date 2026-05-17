//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_vecMathCompatibility.h>

class TMatrix;

struct Ray3
{
  vec3f start;
  vec3f dir;
  float length = 0.f;

  Point3 end() const { return as_point3(&start) + as_point3(&dir) * length; }

  Ray3() : start(v_zero()), dir(v_make_vec4f(1.f, 0.f, 0.f, 0.f)) {}
  Ray3(const Point3 &s, const Point3 &d, float l) : start(v_ldu_p3(&s.x)), dir(v_ldu_p3(&d.x)), length(l) {}
  Ray3(vec3f s, vec3f d, float l) : start(s), dir(d), length(l) {}
};

Ray3 operator*(const TMatrix &tm, const Ray3 &r);
Ray3 operator*(mat44f_cref tm, const Ray3 &r);

Ray3 make_ray_from_segment(const Point3 &p1, const Point3 &p2);
Ray3 make_ray_from_segment_and_t(const Point3 &p1, const Point3 &p2, float t);
