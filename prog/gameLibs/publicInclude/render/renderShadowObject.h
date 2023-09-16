//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_bounds3.h>

struct RenderShadowObject
{
  BBox3 box;
  Point3 sphc;
  real rad;
  enum
  {
    CAR_OBJECT,
    RENDINST_OBJECT,
    SCENE_PHYS_OBJECT,
  };
  enum
  {
    SHADOW_CASTER = 1,
    SHADOW_RECEIVER = 2,
    SHADOW_ONLY_VISIBLE_CASTER = 4, // if true, objectType is childrencount,object - is pointer to first child.
    SHADOW_VISIBLE_CASTER = 5,      // if true, objectType is childrencount,object - is pointer to first child.
                                    // Children could be casters, receivers, or both, according to flag
    SHADOW_BOX_IS_SPHERE = 8,
    SHADOW_BOTH = SHADOW_CASTER | SHADOW_RECEIVER,
  };
  unsigned int flags : 6;
  unsigned int objectType : 26;
  int object;
  RenderShadowObject() : flags(0), object(0), objectType(0) {}
  RenderShadowObject(const BBox3 &box_, const Point3 &sphc_, float rad_, unsigned int type, int userdata,
    unsigned flags_ = SHADOW_BOTH) :
    box(box_), objectType(type), object(userdata), flags(flags_), sphc(sphc_), rad(rad_)
  {}
};
