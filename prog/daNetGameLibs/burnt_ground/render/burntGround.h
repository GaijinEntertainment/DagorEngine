// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <EASTL/functional.h>

struct BurntGroundDecalInfo
{
  Point3 worldPos;
  float decalSize;
  float progress;
};

extern void gather_burnt_ground_decals(const Point3 &eye_pos,
  float visibility_range,
  const eastl::fixed_function<32, void(const BurntGroundDecalInfo &decal)> &decal_processor);