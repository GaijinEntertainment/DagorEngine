//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_capsule.h>

struct TriangleFace
{
  TriangleFace(const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &norm) : n(norm)

  {
    vp[0] = v0;
    vp[1] = v1;
    vp[2] = v2;
  }
  Point3 n;
  Point3 vp[3];
  void reverse()
  {
    n = -n;
    Point3 a = vp[1];
    vp[1] = vp[2];
    vp[2] = a;
  }
};

bool clipCapsuleTriangle(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const TriangleFace &f);
bool test_capsule_triangle_intersection(const Capsule &c, const TriangleFace &f, float &t, Point3 &isect_pos);
VECTORCALL bool test_capsule_triangle_intersection(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float radius, float &t,
  vec3f &out_norm, vec3f &out_pos, bool no_cull);
VECTORCALL bool test_capsule_triangle_hit(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float radius, float t, bool no_cull);
