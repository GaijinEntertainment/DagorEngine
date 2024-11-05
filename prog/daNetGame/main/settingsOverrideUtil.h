// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_oaHashNameMap.h>

FastNameMap find_difference_in_config_blk(const DataBlock &game, const DataBlock &overridden);
DataBlock convert_settings_object_to_blk(const ecs::Object &object, const DataBlock *game_settings = nullptr);
