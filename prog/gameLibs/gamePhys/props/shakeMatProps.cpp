// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/props/shakeMatProps.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/commonPropsRegistry.h>

void physmat::ShakeMatProps::load(const DataBlock *blk)
{
  const DataBlock *shakeBlk = blk->getBlockByNameEx("shakeProps");
  shakePeriod = shakeBlk->getPoint2("period", Point2(0.11f, 0.22f));
  shakePow = shakeBlk->getPoint2("power", Point2(2.f, 2.f));
  shakeMult = shakeBlk->getPoint2("mult", Point2(1.f, 0.5f));
}

PROPS_REGISTRY_IMPL(physmat::ShakeMatProps, shake_props, "physmat");
