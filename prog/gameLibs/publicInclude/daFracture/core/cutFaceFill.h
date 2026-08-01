//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_plane3.h>
#include <math/dag_bounds2.h>
#include <vecmath/dag_vecMath.h>
#include <dag/dag_vector.h>
#include <memory/dag_memPtrAllocator.h>


namespace frx
{

struct DestrMesh;
struct DestrContext;

struct PlaneBasis
{
  Point3 U, V;
  Plane3 plane;
  PlaneBasis() = default;
  explicit PlaneBasis(plane3f v_plane)
  {
    v_stu(&plane.n.x, v_plane); // Plane3 has same layout (n.xyz, d)
    const float nLen = length(plane.n);
    if (nLen > 1e-12f)
    {
      plane.n *= 1.f / nLen;
      plane.d *= 1.f / nLen;
    }
    U = normalize(cross(plane.n, fabsf(plane.n.y) < 0.9f ? Point3(0, 1, 0) : Point3(1, 0, 0)));
    V = cross(U, plane.n);
  }
  Point3 origin() const { return -plane.n * plane.d; }
  Point2 project(Point3 p) const { return Point2(dot(U, p), dot(V, p)); }
  Point3 unProject(Point2 uv) const { return U * uv.x + V * uv.y + origin(); }
};

struct CutFaceData
{
  PlaneBasis basis;
  Point2 tcOrigin = Point2::ZERO;
  float worldToTc = 0;

  struct EdgeIndices
  {
    uint32_t a, b;
  };
  dag::Vector<EdgeIndices> edges;
  dag::Vector<vec4f> verts;
};

} // namespace frx
