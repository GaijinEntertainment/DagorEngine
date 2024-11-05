// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/props/softMaterialProps.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/commonPropsRegistry.h>

void physmat::SoftMaterialProps::load(const DataBlock *blk)
{
  const DataBlock *softBlk = blk->getBlockByNameEx("softCollision");
  physViscosity = softBlk->getReal("physViscosity", physViscosity);
  collisionForce = softBlk->getReal("collisionForce", collisionForce);
}
bool physmat::SoftMaterialProps::can_load(const DataBlock *blk) { return blk->getBlockByNameEx("softCollision", nullptr); }

PROPS_REGISTRY_IMPL_EX(physmat::SoftMaterialProps, soft_mat_props, "physmat");
