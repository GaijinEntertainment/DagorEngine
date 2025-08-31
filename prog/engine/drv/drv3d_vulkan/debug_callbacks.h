// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// handles debug report/utils EXT callbacks for validation layer messages
#include "vulkan_instance.h"
#include <generic/dag_tab.h>

namespace drv3d_vulkan
{

class DebugCallbacks
{
#if VK_EXT_debug_report || VK_EXT_debug_utils
  Tab<int32_t> suppressedMessage;
#endif
#if VK_EXT_debug_report
  VulkanDebugReportCallbackEXTHandle debugHandleReport;
#endif
#if VK_EXT_debug_utils
  VulkanDebugUtilsMessengerEXTHandle debugHandleUtils;
#endif

#if VK_EXT_debug_report
  static VkBool32 VKAPI_PTR msgCallbackReport(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
    size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData);
#endif
#if VK_EXT_debug_utils
  static VkBool32 VKAPI_PTR msgCallbackUtils(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
#endif

public:
  DebugCallbacks() = default;
  DebugCallbacks(const DebugCallbacks &) = delete;
  ~DebugCallbacks() {}

  void init();
  void shutdown();
};

} // namespace drv3d_vulkan
