// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/walker/humanWeapState.h>
#include <ioSys/dag_dataBlock.h>
#include <daNet/bitStream.h>

void HumanWeaponParams::loadFromBlk(const DataBlock *blk)
{
  walkMoveMagnitude = blk->getReal("walkMoveMagnitude", 0.004f);

  holsterTime = blk->getReal("holsterTime", 1.f);
  equipTime = blk->getReal("equipTime", 0.5f);

  breathShakeMagnitude = blk->getReal("breathShakeMagnitude", 0.2f);
  crouchBreathShakeMagnitude = blk->getReal("crouchBreathShakeMagnitude", breathShakeMagnitude);
  crawlBreathShakeMagnitude = blk->getReal("crawlBreathShakeMagnitude", crouchBreathShakeMagnitude);

  predictTime = blk->getReal("predictTime", 0.1f);
  targetSpdVisc = blk->getReal("targetSpdVisc", 0.05f);
  gunSpdVisc = blk->getReal("gunSpdVisc", 0.45f);
  gunSpdDeltaMult = blk->getReal("gunSpdDeltaMult", 1.2f);
  maxGunSpd = blk->getReal("maxGunSpd", HALFPI);
  moveToSpd = blk->getReal("moveToSpd", TWOPI);
  vertOffsetRestoreVisc = blk->getReal("vertOffsetRestoreVisc", 0.2f);
  vertOffsetMoveDownVisc = blk->getReal("vertOffsetMoveDownVisc", 0.5f);
  equipSpeedMult = blk->getReal("equipSpeedMult", 1.f);
  holsterSwapSpeedMult = blk->getReal("holsterSwapSpeedMult", 1.f);

  offsAimNode = blk->getPoint3("offsAimNode", Point3(0.f, 0.15f, 0.f));
  offsCheckLeftNode = blk->getPoint3("offsCheckLeftNode", Point3(0.f, 0.075f, 0.075f));
  offsCheckRightNode = blk->getPoint3("offsCheckRightNode", Point3(0.f, 0.075f, -0.075f));
  gunLen = blk->getReal("gunLen", 0.1f);

  exists = true;
}
