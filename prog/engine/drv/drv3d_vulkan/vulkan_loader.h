// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "vk_entry_points.h"
#include "extension_utils.h"
#include "streamline_adapter.h"

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
  bool isValid() const { /*can only be not null if load did not fail*/ return libHandle != nullptr; }
#endif
  dag::Vector<VkLayerProperties> getLayers();
  dag::Vector<VkExtensionProperties> getExtensions();

  bool load(const char *name, bool validate);
  void unload();

#if USE_STREAMLINE_FOR_DLSS
  bool initStreamlineAdapter();
  StreamlineAdapter::InterposerHandleType streamlineInterposer = {nullptr, nullptr};
  eastl::optional<StreamlineAdapter> streamlineAdapter;
#endif
};
} // namespace drv3d_vulkan