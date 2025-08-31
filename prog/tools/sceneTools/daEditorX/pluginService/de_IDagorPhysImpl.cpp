// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_IDagorPhysImpl.h"

#include <oldEditor/de_collision.h>

#include <math/dag_capsule.h>


//==================================================================================================
StaticSceneRayTracer *DeDagorPhys::loadBinaryRaytracer(IGenLoad &crd) const { return DagorPhys::load_binary_raytracer(crd); }


//==================================================================================================
bool DeDagorPhys::traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt) const
{
  return DagorPhys::trace_ray_static(p, dir, maxt);
}


//==================================================================================================
bool DeDagorPhys::traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt, Point3 &norm) const
{
  return DagorPhys::trace_ray_static(p, dir, maxt, norm);
}


//==================================================================================================
bool DeDagorPhys::rayHitStatic(const Point3 &p, const Point3 &dir, real maxt) const { return DagorPhys::ray_hit_static(p, dir, maxt); }


//==================================================================================================
real DeDagorPhys::clipCapsuleStatic(Capsule &c, Point3 &cap_pt, Point3 &world_pt) const
{
  return DagorPhys::clip_capsule_static(c, cap_pt, world_pt);
}


//==================================================================================================
real DeDagorPhys::clipCapsuleStatic(Capsule &c, Point3 &cap_pt, Point3 &world_pt, Point3 &norm) const
{
  return DagorPhys::clip_capsule_static(c, cap_pt, world_pt, norm);
}


//==================================================================================================
FastRtDump *DeDagorPhys::getFastRtDump() const { return DagorPhys::getFastRtDump(); }


//==================================================================================================
void DeDagorPhys::initCollisionBinary(StaticSceneRayTracer *rt) const { DagorPhys::init_collision_binary(rt); }


//==================================================================================================
void DeDagorPhys::closeCollision() const { DagorPhys::close_collision(); }


//==================================================================================================
BBox3 DeDagorPhys::getBoundingBox() const { return DagorPhys::get_bounding_box(); }
