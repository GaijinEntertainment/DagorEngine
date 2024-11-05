// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_device.h"
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <generic/dag_carray.h>
#include <util/dag_stdint.h>
#include "vk_wrapped_handles.h"

namespace drv3d_vulkan
{

struct BindlessSetLimit
{
  uint32_t req;
  uint32_t max;
  const char *configPath;
  const char *limitName;
  bool fits;
};
typedef carray<BindlessSetLimit, spirv::bindless::MAX_SETS> BindlessSetLimits;
typedef carray<VulkanDescriptorSetLayoutHandle, spirv::bindless::MAX_SETS> BindlessSetLayouts;

} // namespace drv3d_vulkan
