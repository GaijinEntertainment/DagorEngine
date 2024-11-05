// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/props/physMatDamageModelProps.h>

#include <propsRegistry/commonPropsRegistry.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathBase.h>

void PhysMatDamageModelProps::load(const DataBlock *blk)
{
  armorThickness = blk->getBlockByNameEx("damageModel")->getReal("armorThickness", armorThickness);
  ricochetAngleMult = blk->getBlockByNameEx("damageModel")->getReal("ricochetAngleMult", ricochetAngleMult);
  bulletBrokenThreshold = blk->getBlockByNameEx("damageModel")->getReal("bulletBrokenThreshold", bulletBrokenThreshold);
}

bool PhysMatDamageModelProps::can_load(const DataBlock *blk) { return blk->getBlockByNameEx("damageModel", nullptr) != nullptr; }

PROPS_REGISTRY_IMPL_EX(PhysMatDamageModelProps, phys_mat_dm_props, "physmat");
