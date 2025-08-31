// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/treeDestr.h>

static rendinstdestr::TreeDestr tree_destr;

float rendinstdestr::TreeDestr::getRadiusToImpulse(float radius) const
{
  return radiusToImpulse.empty() ? 1.f : radiusToImpulse.interpolate(radius);
}

void rendinstdestr::tree_destr_load_from_blk(const DataBlock &blk)
{
  tree_destr.heightThreshold = blk.getReal("heightThreshold", 5.0f);
  tree_destr.radiusThreshold = blk.getReal("radiusThreshold", 1.0f);
  tree_destr.impulseThresholdMult = blk.getReal("impulseThresholdMult", 1000.0f);
  tree_destr.canopyRestitution = blk.getReal("canopyRestitution", 0.0f);
  tree_destr.canopyStiffnessMult = blk.getReal("canopyStiffnessMult", 1.0f);
  tree_destr.canopyAngularLimit = blk.getReal("canopyAngularLimit", 0.3f);
  tree_destr.canopyMaxRaduis = blk.getReal("canopyMaxRaduis", 5.f);
  tree_destr.canopyDensity = blk.getReal("canopyDensity", 60.f);
  tree_destr.canopyRollingFriction = blk.getReal("canopyRollingFriction", 0.1f);
  tree_destr.canopyInertia = blk.getPoint3("canopyInertia", Point3(1.f, 1.f, 1.f));
  tree_destr.treeDensity = blk.getReal("treeDensity", 1000.f);
  tree_destr.bushDensity = blk.getReal("bushDensity", tree_destr.treeDensity);
  tree_destr.bushDestrRadiusMult = blk.getReal("bushDestrRadiusMult", 1.0f);
  tree_destr.useBoxAsCanopyCollision = blk.getBool("useBoxAsCanopyCollision", false);
  tree_destr.minSpeed = blk.getReal("minSpeed", 5.f);
  tree_destr.maxLifeDistance = blk.getReal("maxLifeDistance", -1.f);
  tree_destr.constraintLimitY = blk.getPoint2("constraintLimitY", Point2(0.f, 0.f));
  tree_destr.canopyLinearDamping = blk.getReal("canopyLinearDamping", 0.9f);
  tree_destr.canopyAngularDamping = blk.getReal("canopyAngularDamping", 0.9f);
  tree_destr.canopyJointDamping = blk.getReal("canopyJointDamping", 0.5f);
  tree_destr.treeFriction = blk.getReal("friction", 0.7f);
  tree_destr.treeDampingLinear = blk.getReal("dampingLinear", 0.4f);
  tree_destr.treeDampingAngular = blk.getReal("dampingAngular", 0.5f);
  tree_destr.groundOffsetMul = blk.getReal("groundOffsetMul", 0.05f);
  tree_destr.groundDamping = blk.getReal("groundDamping", 0.85f);
  tree_destr.groundJointAngleLimitWhole = blk.getReal("groundJointAngleLimitWhole", tree_destr.groundJointAngleLimitWhole);
  tree_destr.groundJointAngleLimitBottom = blk.getReal("groundJointAngleLimitBottom", tree_destr.groundJointAngleLimitBottom);
  tree_destr.groundJointDamping = blk.getReal("groundJointDamping", tree_destr.groundJointDamping);
  tree_destr.collisionDistLimit = blk.getReal("collisionDistLimit", tree_destr.collisionDistLimit);
  tree_destr.collisionAngleLimit = blk.getReal("collisionAngleLimit", tree_destr.collisionAngleLimit);
  branch_destr_load_from_blk(tree_destr.branchDestrFromDamage, blk.getBlockByNameEx("branchDestr"));
  tree_destr.branchDestrOther = tree_destr.branchDestrFromDamage;
  branch_destr_load_from_blk(tree_destr.branchDestrOther, blk.getBlockByNameEx("branchDestrOther"));
  read_interpolate_tab_float_p2(tree_destr.radiusToImpulse, *blk.getBlockByNameEx("radiusToImpulse"));
}

void rendinstdestr::branch_destr_load_from_blk(rendinstdestr::BranchDestr &target, const DataBlock *blk)
{
  if (!blk)
    return;

  target.multiplierMode = blk->getBool("multiplyMode", target.multiplierMode);
  bool loadingMultipliers = target.multiplierMode;

  target.enableBranchDestruction = blk->getBool("enable", loadingMultipliers ? true : target.enableBranchDestruction);
  target.newPhysics = blk->getBool("newPhysics", loadingMultipliers ? true : target.newPhysics);
  target.cutInHalfDamageThreshold =
    blk->getReal("cutInHalfDamageThreshold", loadingMultipliers ? 1.0f : target.cutInHalfDamageThreshold);
  target.impulseMul = blk->getReal("impulseMul", loadingMultipliers ? 1.0f : target.impulseMul);
  target.impulseMin = blk->getReal("impulseMin", loadingMultipliers ? 1.0f : target.impulseMin);
  target.impulseMax = blk->getReal("impulseMax", loadingMultipliers ? 1.0f : target.impulseMax);
  target.branchSizeMin = blk->getReal("branchSizeMin", loadingMultipliers ? 1.0f : target.branchSizeMin);
  target.branchSizeMax = blk->getReal("branchSizeMax", loadingMultipliers ? 1.0f : target.branchSizeMax);
  target.rotateRandomSpeedMulX = blk->getReal("rotateSpeedMul", loadingMultipliers ? 1.0f : target.rotateRandomSpeedMulX);
  target.rotateRandomSpeedMulY = blk->getReal("rotateSpeedMulY", loadingMultipliers ? 1.0f : target.rotateRandomSpeedMulY);
  target.rotateRandomSpeedMulZ = blk->getReal("rotateSpeedMulZ", loadingMultipliers ? 1.0f : target.rotateRandomSpeedMulZ);
  target.rotateRandomAngleSpread = blk->getReal("rotateAngleSpread", loadingMultipliers ? 1.0f : target.rotateRandomAngleSpread);
  target.branchSizeSlowDown = blk->getReal("branchSizeSlowDown", loadingMultipliers ? 1.0f : target.branchSizeSlowDown);
  target.fallingSpeedMul = blk->getReal("fallingSpeedMul", loadingMultipliers ? 1.0f : target.fallingSpeedMul);
  target.fallingSpeedRnd = blk->getReal("fallingSpeedRnd", loadingMultipliers ? 1.0f : target.fallingSpeedRnd);
  target.horizontalSpeedMul = blk->getReal("horizontalSpeedMul", loadingMultipliers ? 1.0f : target.horizontalSpeedMul);
  target.maxVisibleDistance = blk->getReal("maxVisibleDistance", loadingMultipliers ? 1.0f : target.maxVisibleDistance);
  target.fallThroughGroundIfBigger =
    blk->getReal("fallThroughGroundIfBigger", loadingMultipliers ? 1.0f : target.fallThroughGroundIfBigger);
}

const rendinstdestr::TreeDestr &rendinstdestr::get_tree_destr() { return tree_destr; }
rendinstdestr::TreeDestr &rendinstdestr::get_tree_destr_mutable() { return tree_destr; }

void rendinstdestr::BranchDestr::apply(const BranchDestr &other)
{
  if (other.multiplierMode)
  {
    enableBranchDestruction = enableBranchDestruction && other.enableBranchDestruction;
    newPhysics = newPhysics && other.newPhysics;
    cutInHalfDamageThreshold *= other.cutInHalfDamageThreshold;
    impulseMul *= other.impulseMul;
    impulseMin *= other.impulseMin;
    impulseMax *= other.impulseMax;
    branchSizeMin *= other.branchSizeMin;
    branchSizeMax *= other.branchSizeMax;
    rotateRandomSpeedMulX *= other.rotateRandomSpeedMulX;
    rotateRandomSpeedMulY *= other.rotateRandomSpeedMulY;
    rotateRandomSpeedMulZ *= other.rotateRandomSpeedMulZ;
    rotateRandomAngleSpread *= other.rotateRandomAngleSpread;
    branchSizeSlowDown *= other.branchSizeSlowDown;
    fallingSpeedMul *= other.fallingSpeedMul;
    fallingSpeedRnd *= other.fallingSpeedRnd;
    horizontalSpeedMul *= other.horizontalSpeedMul;
    maxVisibleDistance *= other.maxVisibleDistance;
    fallThroughGroundIfBigger *= other.fallThroughGroundIfBigger;
  }
  else
  {
    enableBranchDestruction = other.enableBranchDestruction;
    newPhysics = other.newPhysics;
    cutInHalfDamageThreshold = other.cutInHalfDamageThreshold;
    impulseMul = other.impulseMul;
    impulseMin = other.impulseMin;
    impulseMax = other.impulseMax;
    branchSizeMin = other.branchSizeMin;
    branchSizeMax = other.branchSizeMax;
    rotateRandomSpeedMulX = other.rotateRandomSpeedMulX;
    rotateRandomSpeedMulY = other.rotateRandomSpeedMulY;
    rotateRandomSpeedMulZ = other.rotateRandomSpeedMulZ;
    rotateRandomAngleSpread = other.rotateRandomAngleSpread;
    branchSizeSlowDown = other.branchSizeSlowDown;
    fallingSpeedMul = other.fallingSpeedMul;
    fallingSpeedRnd = other.fallingSpeedRnd;
    horizontalSpeedMul = other.horizontalSpeedMul;
    maxVisibleDistance = other.maxVisibleDistance;
    fallThroughGroundIfBigger = other.fallThroughGroundIfBigger;
  }
}
