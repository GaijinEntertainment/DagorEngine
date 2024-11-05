// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "vk_entry_points.h"
#include "extension_utils.h"

namespace drv3d_vulkan
{
// represents the loader library (eg. vulkan-*.dll)
// minimal set of entry points to get global properties
// of the vulkan installation and the creation of a
// vulkan instance.
class VulkanLoader
{
#if !_TARGET_C3
  void *libHandle = nullptr;
#endif

public:
#define VK_DEFINE_ENTRY_POINT(name) VULKAN_MAKE_CORE_FUNCTION_POINTER_MEMBER(name);
  VK_LOADER_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT

public:
  VulkanLoader(const VulkanLoader &) = delete;
  VulkanLoader &operator=(const VulkanLoader &) = delete;

  VulkanLoader() = default;

#if _TARGET_C3

#else
  bool isValid() const
  { /*can only be not null if load did not fail*/
    return libHandle != nullptr;
  }
#endif
  eastl::vector<VkLayerProperties> getLayers();
  eastl::vector<VkExtensionProperties> getExtensions();

  bool load(const char *name, bool validate);
  void unload();
};
} // namespace drv3d_vulkan