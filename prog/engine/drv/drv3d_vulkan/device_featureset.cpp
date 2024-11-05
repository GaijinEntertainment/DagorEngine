// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device_featureset.h"

#include <util/dag_string.h>
#include <debug/dag_debug.h>

bool drv3d_vulkan::check_and_update_device_features(VkPhysicalDeviceFeatures &set)
{
  String dump("vulkan: disable features [");
  const size_t elemsPerLine = 4;
  size_t cnt = 0;
#define VULKAN_FEATURE_REQUIRE(name) \
  if (set.name == VK_FALSE)          \
    return false;
#define VULKAN_FEATURE_DISABLE(name)         \
  dump += #name;                             \
  ++cnt;                                     \
  if (cnt % elemsPerLine)                    \
    dump += ", ";                            \
  else                                       \
    dump += "]\nvulkan: disable features ["; \
  set.name = VK_FALSE;
#define VULKAN_FEATURE_OPTIONAL(name)
  VULKAN_FEATURESET_REQUIRED_LIST
  VULKAN_FEATURESET_OPTIONAL_LIST
  VULKAN_FEATURESET_DISABLED_LIST
#undef VULKAN_FEATURE_REQUIRE
#undef VULKAN_FEATURE_DISABLE
#undef VULKAN_FEATURE_OPTIONAL

  dump.chop(2);
  dump += "]";
  debug(dump);
  return true;
}
