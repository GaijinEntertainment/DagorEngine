// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/pluginService/de_IDagorPhys.h>


class DeDagorPhys : public IDagorPhys
{
public:
  bool traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt) const override;
  bool traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt, Point3 &norm) const override;
  bool rayHitStatic(const Point3 &p, const Point3 &dir, real maxt) const override;

  real clipCapsuleStatic(Capsule &c, Point3 &cap_pt, Point3 &world_pt) const override;

  void closeCollision() const override;

  BBox3 getBoundingBox() const override;

  void visitClipTriangles(const BBox3 &box, const ClipTriVisitor &visitor) const override;
};
