//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
  float bushDensity = 1000.0f;
  float bushDestrRadiusMult = 1.0f;
  float minSpeed = 5.0f;
  float maxLifeDistance = -1.0f;
  Point2 constraintLimitY = Point2(0.f, 0.f);
  float canopyLinearDamping = 0.9f;
  float canopyAngularDamping = 0.9f;
  bool useBoxAsCanopyCollision = false;

  struct BranchDestr
  {
    bool enableBranchDestruction = false;
    bool multiplierMode = false; // if false, overrides default values
    float impulseMul = 1.0f;     // multiplies the incoming impulse
    float impulseMin = 0.0f;     // if the impulse is lower than this, the falling chance will be 0%
    float impulseMax = 6.0f;     // if the impulse is higher than this, the falling chance will be 100%
    float branchSizeMin = 3.0f;  // if the branch size is lower than this, the falling chance will not be decreased
    float branchSizeMax = 20.0f; // if the branch size if higher than this, the falling chance will be 0%
    float rotateRandomSpeedMulX = 0.5f;
    float rotateRandomSpeedMulY = 1.5f;
    float rotateRandomSpeedMulZ = 0.5f;
    float rotateRandomAngleSpread = 0.7f * PI;
    float branchSizeSlowDown = 0.05f;
    float fallingSpeedMul = 0.7f;
    float fallingSpeedRnd = 0.4f;
    float horizontalSpeedMul = 1.0f;
    float maxVisibleDistance = 100.0f;

    void apply(const BranchDestr &other);
  } branchDestrFromDamage, branchDestrOther;

  InterpolateTabFloat radiusToImpulse;
  float getRadiusToImpulse(float radius) const;
};

void tree_destr_load_from_blk(const DataBlock &blk);
void branch_destr_load_from_blk(TreeDestr::BranchDestr &target, const DataBlock *blk);
const TreeDestr &get_tree_destr();
TreeDestr &get_tree_destr_mutable();

} // namespace rendinstdestr
