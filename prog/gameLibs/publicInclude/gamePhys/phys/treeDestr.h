//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <gameMath/interpolateTabUtils.h>

class DataBlock;

namespace rendinstdestr
{

struct TreeDestr
{
  float heightThreshold = 5.0f;
  float radiusThreshold = 1.0f;
  float impulseThresholdMult = 1000.0f;
  float canopyRestitution = 0.0f;
  float canopyStiffnessMult = 1.0f;
  float canopyAngularLimit = 0.3f;
  float canopyMaxRaduis = 5.0f;
  float canopyDensity = 60.0f;
  float canopyRollingFriction = 0.1f;
  Point3 canopyInertia = Point3(1.f, 1.f, 1.f);
  float treeDensity = 1000.0f;
  float minSpeed = 5.0f;
  float maxLifeDistance = -1.0f;
  Point2 constraintLimitY = Point2(0.f, 0.f);
  float canopyLinearDamping = 0.9f;
  float canopyAngularDamping = 0.9f;

  InterpolateTabFloat radiusToImpulse;
  float getRadiusToImpulse(float radius) const;
};

void tree_destr_load_from_blk(const DataBlock &blk);
const TreeDestr &get_tree_destr();

} // namespace rendinstdestr
