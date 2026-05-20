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
// Precision-tolerant variants for callers feeding quantized verts (e.g. BVH UnquantVL decode).
// dp_cull_threshold: front-face-cull tolerance on dot(triNormal, dir). Default -0.001f matches
//   the historical FRT raw-vertex behavior.
// h_zero_eps: tolerance for treating "h" (signed distance from ray origin to triangle plane,
//   along the triangle normal) as zero in the edge-pass back-side rejection. Default 0 matches
//   FRT raw-vertex behavior; with vert21-decoded verts a flat plane at y=0 can drift to y=-5e-5,
//   making h negative when geometrically it should be 0. That spuriously triggers the
//   `(h < 0 && dist < 0)` skip in the per-edge sweep test, dropping grazing-edge hits.
VECTORCALL bool test_capsule_triangle_intersection(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float radius, float &t,
  vec3f &out_norm, vec3f &out_pos, bool no_cull, float dp_cull_threshold = -0.001f, float h_zero_eps = 0.f);
VECTORCALL bool test_capsule_triangle_hit(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float radius, float t, bool no_cull,
  float dp_cull_threshold = -0.001f, float h_zero_eps = 0.f);
