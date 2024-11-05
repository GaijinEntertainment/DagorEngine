// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
class DataBlock;

ECS_BROADCAST_EVENT_TYPE(OnRenderSettingsReady)

class DataBlock;
bool update_settings_entity(const DataBlock *level_override);

void prepare_ri_united_vdata_setup(const DataBlock *level_blk = nullptr);
void prepare_dynm_united_vdata_setup(const DataBlock *level_blk = nullptr);
inline void prepare_united_vdata_setup(const DataBlock *level_blk = nullptr)
{
  prepare_ri_united_vdata_setup(level_blk);
  prepare_dynm_united_vdata_setup(level_blk);
}
void apply_united_vdata_settings(const DataBlock *scene_blk);
