// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/props/physContactProps.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/commonPropsRegistry.h>

void physmat::PhysContactProps::load(const DataBlock *blk)
{
  const DataBlock *destrBlk = blk->getBlockByNameEx("physContactProps");
  removePhysContact = destrBlk->getBool("removePhysContact", removePhysContact);
  forcedSliding = destrBlk->getBool("forcedSliding", forcedSliding);
}
bool physmat::PhysContactProps::can_load(const DataBlock *blk) { return blk->getBlockByNameEx("physContactProps", nullptr); }

PROPS_REGISTRY_IMPL_EX(physmat::PhysContactProps, phys_contact_mat_props, "physmat");
