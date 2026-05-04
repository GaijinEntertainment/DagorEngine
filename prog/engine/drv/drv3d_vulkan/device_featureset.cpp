// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device_featureset.h"

#include <util/dag_string.h>
#include <debug/dag_debug.h>

bool drv3d_vulkan::check_and_update_device_features(VkPhysicalDeviceFeatures &set)
{
#define VULKAN_FEATURE_REQUIRE(name)                     \
  if (set.name == VK_FALSE)                              \
  {                                                      \
    debug("vulkan: required feature " #name " missing"); \
    return false;                                        \
  }
  VULKAN_FEATURESET_REQUIRED_LIST
#undef VULKAN_FEATURE_REQUIRE
  return true;
}
