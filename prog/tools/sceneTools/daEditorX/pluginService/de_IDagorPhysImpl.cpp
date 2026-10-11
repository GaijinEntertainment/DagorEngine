// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_IDagorPhysImpl.h"

#include <oldEditor/de_collision.h>

#include <gameRes/dag_collisionResource.h>
#include <math/dag_capsule.h>


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
void DeDagorPhys::visitClipTriangles(const BBox3 &box, const ClipTriVisitor &visitor) const
{
  const CollisionResource *clip = DagorPhys::getClipCollision();
  if (!clip)
    return;

  clip->visitTrianglesInBox(v_ldu_bbox3(box), CollisionNode::TRACEABLE, [&visitor](vec3f a, vec3f b, vec3f c, int, int) {
    Point3 v0, v1, v2;
    v_stu_p3(&v0.x, a), v_stu_p3(&v1.x, b), v_stu_p3(&v2.x, c);
    visitor(v0, v1, v2);
    return false;
  });
}


//==================================================================================================
void DeDagorPhys::closeCollision() const { DagorPhys::close_collision(); }


//==================================================================================================
BBox3 DeDagorPhys::getBoundingBox() const { return DagorPhys::get_bounding_box(); }
