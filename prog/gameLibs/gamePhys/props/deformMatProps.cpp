#include <gamePhys/props/deformMatProps.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/commonPropsRegistry.h>

void physmat::DeformMatProps::load(const DataBlock *blk)
{
  const DataBlock *deformBlk = blk->getBlockByNameEx("deformProps");
  coverNoiseMult = deformBlk->getReal("coverNoiseMult", 1.f);
  coverNoiseAdd = deformBlk->getReal("coverNoiseAdd", 0.f);
  period = deformBlk->getPoint2("period", Point2(1.f, 0.5f));
  mult = deformBlk->getReal("mult", 0.f);
}

PROPS_REGISTRY_IMPL(physmat::DeformMatProps, deform_props, "physmat");
