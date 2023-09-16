#include <gamePhys/props/physDestructibleProps.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/commonPropsRegistry.h>

void physmat::PhysDestructibleProps::load(const DataBlock *blk)
{
  const DataBlock *destrBlk = blk->getBlockByNameEx("physDestructible");
  climbingThrough = destrBlk->getBool("climbingThrough", climbingThrough);
  impulseSpeed = destrBlk->getReal("impulseSpeed", impulseSpeed);
}
bool physmat::PhysDestructibleProps::can_load(const DataBlock *blk) { return blk->getBlockByNameEx("physDestructible", nullptr); }

PROPS_REGISTRY_IMPL_EX(physmat::PhysDestructibleProps, phys_destr_mat_props, "physmat");
