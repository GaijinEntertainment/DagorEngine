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
#include "os/win32.h"
#elif _TARGET_PC_LINUX
#include "os/linux.h"
#elif _TARGET_ANDROID
#include "os/android.h"
#else
#error unsupported platform
#endif

namespace drv3d_vulkan
{
void get_video_modes_list(Tab<String> &list);

// Fills monitors with unique, identifiable names of active monitors. Returns true if at least one was found.
bool get_monitors_list(Tab<String> &monitors);
// Returns user-friendly info about a monitor (nullptr/empty monitor_name selects the default monitor).
// friendly_name and monitor_index are optional. Returns false if the monitor was not found.
bool get_monitor_info(const char *monitor_name, String *friendly_name, int *monitor_index);
// Fills resolutions with available modes ("W x H") for a single monitor. Returns true if at least one was found.
bool get_resolutions_from_monitor(const char *monitor_name, Tab<String> &resolutions);

bool validate_vulkan_signature(void *file);

VulkanSurfaceKHRHandle init_window_surface(VulkanInstance &instance, void *window);
VkExtent2D get_window_client_rect_extent(void *window);

void os_restore_display_mode();
void os_set_display_mode(int res_x, int res_y);

// Serialize the WSI surface commit done inside vkQueuePresentKHR against application-side
// window-system requests on the same window. Real only on Wayland; no-op elsewhere and on X11.
void os_present_window_lock();
void os_present_window_unlock();
eastl::string os_get_additional_ext_requirements(VulkanPhysicalDeviceHandle dev, const dag::Vector<VkExtensionProperties> &extensions);

} // namespace drv3d_vulkan
