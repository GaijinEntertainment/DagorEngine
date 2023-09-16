//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_bounds3.h>
#include <sceneRay/dag_sceneRayDecl.h>


class IGenLoad;
class FastRtDump;
struct Capsule;


class IDagorPhys
{
public:
  static constexpr unsigned HUID = 0xED1BCF08u; // IDagorPhys

  virtual StaticSceneRayTracer *loadBinaryRaytracer(IGenLoad &crd) const = 0;

  virtual bool traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt) const = 0;
  virtual bool traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt, Point3 &norm) const = 0;
  virtual bool rayHitStatic(const Point3 &p, const Point3 &dir, real maxt) const = 0;

  virtual real clipCapsuleStatic(Capsule &c, Point3 &cap_pt, Point3 &world_pt) const = 0;
  virtual real clipCapsuleStatic(Capsule &c, Point3 &cap_pt, Point3 &world_pt, Point3 &norm) const = 0;

  virtual FastRtDump *getFastRtDump() const = 0;

  virtual void initClippingBinary(StaticSceneRayTracer *rt) const = 0;

  virtual void closeClipping() const = 0;

  virtual BBox3 getBoundingBox() const = 0;
};
