// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/commonPropsRegistry.h>
#include "actionSoundProps.h"

void sound::ActionProps::load(const DataBlock *blk)
{
  const DataBlock *actBlk = blk->getBlockByName("actionSound");
  humanHitSoundName = actBlk->getStr("humanHitSoundName", "");
  humanHitSoundPath = actBlk->getStr("humanHitSoundPath", "");
}
bool sound::ActionProps::can_load(const DataBlock *blk) { return blk->getBlockByNameEx("actionSound", nullptr) != nullptr; }

PROPS_REGISTRY_IMPL_EX(sound::ActionProps, action_sound_props, "action");

const sound::ActionProps *sound::ActionProps::try_get_props(int prop_id) { return action_sound_props_registry.tryGetProps(prop_id); }
