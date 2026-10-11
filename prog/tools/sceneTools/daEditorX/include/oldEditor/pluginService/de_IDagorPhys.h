//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_bounds3.h>
#include <EASTL/fixed_function.h>


struct Capsule;


class IDagorPhys
{
public:
  static constexpr unsigned HUID = 0xED1BCF08u; // IDagorPhys

  virtual bool traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt) const = 0;
  virtual bool traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt, Point3 &norm) const = 0;
  virtual bool rayHitStatic(const Point3 &p, const Point3 &dir, real maxt) const = 0;

  virtual real clipCapsuleStatic(Capsule &c, Point3 &cap_pt, Point3 &world_pt) const = 0;

  virtual void closeCollision() const = 0;

  virtual BBox3 getBoundingBox() const = 0;

  // every traceable triangle of the editor clip whose box overlaps the query box, once each and in no promised order,
  // and none while no clip is loaded; box and corners are in world space, the corners valid inside the visitor call only,
  // decoded from each node's own quantized verts;
  // in the clip's winding, so the triangle's normal is normalize((v1 - v0) % (v2 - v0))
  using ClipTriVisitor = eastl::fixed_function<sizeof(void *) * 6, void(const Point3 &v0, const Point3 &v1, const Point3 &v2)>;
  virtual void visitClipTriangles(const BBox3 &box, const ClipTriVisitor &visitor) const = 0;
};
