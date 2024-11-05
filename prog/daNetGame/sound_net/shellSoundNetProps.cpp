// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shellSoundNetProps.h"
#include <propsRegistry/commonPropsRegistry.h>
#include <ioSys/dag_dataBlock.h>

void ShellSoundNetProps::load(const DataBlock *blk)
{
  const DataBlock *shellSoundBlk = blk->getBlockByNameEx("shellSoundNet");
  throwPhrase = shellSoundBlk->getStr("throwPhrase", "");
}

PROPS_REGISTRY_IMPL_EX(ShellSoundNetProps, shell_sound_net, "shell");

/*static*/ const ShellSoundNetProps *ShellSoundNetProps::try_get_props(int prop_id)
{
  return shell_sound_net_registry.tryGetProps(prop_id);
}

/*static*/ bool ShellSoundNetProps::can_load(const DataBlock *blk)
{
  return blk->getBlockByNameEx("shellSoundNet", nullptr) != nullptr;
}
