#include <gamePhys/props/arcadeBoostProps.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/commonPropsRegistry.h>

void physmat::ArcadeBoostProps::load(const DataBlock *blk)
{
  const DataBlock *arcadeBlk = blk->getBlockByNameEx("arcadeProps");
  boost = arcadeBlk->getReal("boost", 1.f);
}

PROPS_REGISTRY_IMPL(physmat::ArcadeBoostProps, arcade_boost, "physmat");
