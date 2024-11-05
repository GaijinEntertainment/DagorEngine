// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "base_pipeline.h"
#include "globals.h"
#include "debug_naming.h"

using namespace drv3d_vulkan;

void drv3d_vulkan::setDebugNameForDescriptorSet(VulkanDescriptorSetHandle ds_handle, const char *name)
{
  Globals::Dbg::naming.setDescriptorName(ds_handle, name);
}
