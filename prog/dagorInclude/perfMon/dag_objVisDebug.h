//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>

#if DAGOR_DBGLEVEL > 0

class IObjectVisibilityDebugging
{
public:
  virtual void startupVisDist() = 0;
  virtual void shutdownVisDist() = 0;

  virtual int addObjSphere(void *obj, BSphere3 &sphere, Point3 &pos_cam, bool visible, real vis_dist) = 0;

  virtual void renderObjSpheres() = 0;
};

namespace ObjVisDebug
{
extern IObjectVisibilityDebugging *obj_vis_debug;


__forceinline void startup_vis_dist()
{
  if (obj_vis_debug)
    obj_vis_debug->startupVisDist();
}

__forceinline void shutdown_vis_dist()
{
  if (obj_vis_debug)
    obj_vis_debug->shutdownVisDist();
}

__forceinline int add_obj_sphere(void *obj, BSphere3 &sphere, Point3 &pos_cam, bool visible, real vis_dist)
{
  if (!obj_vis_debug)
    return -1;
  return obj_vis_debug->addObjSphere(obj, sphere, pos_cam, visible, vis_dist);
}

__forceinline void render_obj_spheres()
{
  if (obj_vis_debug)
    obj_vis_debug->renderObjSpheres();
}
}; // namespace ObjVisDebug

#else

namespace ObjVisDebug
{
__forceinline void startup_vis_dist() {}
__forceinline void shutdown_vis_dist() {}

__forceinline int add_obj_sphere(void *, BSphere3 &, Point3 &, bool, real) { return -1; }

__forceinline void render_obj_spheres() {}
}; // namespace ObjVisDebug

#endif
