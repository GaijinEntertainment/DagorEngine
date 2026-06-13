// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/props/shakeMatProps.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/commonPropsRegistry.h>

static float frac(float v) { return v - floorf(v); }

void physmat::ShakeMatProps::load(const DataBlock *blk)
{
  const DataBlock *shakeBlk = blk->getBlockByNameEx("shakeProps");
  shakePeriod = shakeBlk->getPoint2("period", Point2(0.11f, 0.22f));
  shakeMult = shakeBlk->getPoint2("mult", Point2(1.f, 0.5f));

  const Point2 defaultPow = {2.f, 2.f};
  shakePow = shakeBlk->getPoint2("power", defaultPow);
  // TODO: delete this check, when calc_wheel_shaking will be fixed
  if (float_nonzero(frac(shakePow.x)) || float_nonzero(frac(shakePow.y)))
  {
    logerr("ERROR in %s blk, shakeProps: fractioned power is not implemented", blk->getBlockName());
    shakePow = defaultPow;
  }
}

PROPS_REGISTRY_IMPL(physmat::ShakeMatProps, shake_props, "physmat");
