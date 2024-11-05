// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline.h"
#include "dynamic_graphics_state.h"
#include "graphics_state2.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(BackDynamicGraphicsState, dynamic, BackGraphicsStateStorage);

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void BackDynamicGraphicsStateStorage::makeDirty() {}

void BackDynamicGraphicsStateStorage::clearDirty() {}