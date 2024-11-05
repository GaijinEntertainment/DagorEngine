// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_instance.h"
#include <drv/3d/dag_driver.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <EASTL/string.h>
#include "vulkan_device.h"

#if _TARGET_C3

#elif _TARGET_PC_WIN
#include "os_win32.h"
#elif _TARGET_PC_LINUX
#include "os_linux.h"
#elif _TARGET_ANDROID
#include "os_android.h"
#else
#error unsupported platform
#endif

namespace drv3d_vulkan
{
void get_video_modes_list(Tab<String> &list);
bool validate_vulkan_signature(void *file);

VulkanSurfaceKHRHandle init_window_surface(VulkanInstance &instance);

void os_restore_display_mode();
void os_set_display_mode(int res_x, int res_y);
eastl::string os_get_additional_ext_requirements(VulkanPhysicalDeviceHandle dev,
  const eastl::vector<VkExtensionProperties> &extensions);

} // namespace drv3d_vulkan
