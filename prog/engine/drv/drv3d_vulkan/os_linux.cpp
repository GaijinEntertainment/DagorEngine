// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "os.h"

#include <debug/dag_debug.h>
#include <linux/x11.h>
#include "globals.h"

using namespace drv3d_vulkan;

// code borrowed from opengl driver
void drv3d_vulkan::get_video_modes_list(Tab<String> &list) { x11.getVideoModeList(list); }

bool drv3d_vulkan::validate_vulkan_signature(void *file)
{
  G_UNREFERENCED(file);
  // on linux no way to verify driver signature?!?
  return true;
}

VulkanSurfaceKHRHandle drv3d_vulkan::init_window_surface(VulkanInstance &instance)
{
  VulkanSurfaceKHRHandle result;
#if VK_KHR_xlib_surface
  // If we would support multiple display servers on linux we would need to check here
  // if (instance.hasExtension<SurfaceXlibKHR>())
  {
    VkXlibSurfaceCreateInfoKHR xlibsci;
    xlibsci.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    xlibsci.pNext = NULL;
    xlibsci.flags = 0;
    xlibsci.dpy = x11.rootDisplay;
    xlibsci.window = x11.mainWindow;
    if (VULKAN_CHECK_FAIL(instance.vkCreateXlibSurfaceKHR(instance.get(), &xlibsci, NULL, ptr(result))))
    {
      result = VulkanNullHandle();
    }
  }
#else
  G_UNUSED(instance);
  G_UNUSED(parms);
#endif
  return result;
}

void drv3d_vulkan::os_restore_display_mode() {}

void drv3d_vulkan::os_set_display_mode(int res_x, int res_y)
{
  int w = res_x;
  int h = res_y;
  if (!x11.setScreenResolution(w, h, res_x, res_y))
    D3D_ERROR("vulkan: error setting display mode %dx%d", w, h);
  if ((w != res_x) || (h != res_y))
    debug("vulkan: ignoring modified viewport area: original %dx%d, reported %dx%d", w, h, res_x, res_y);
  Globals::window.updateRefreshRateFromCurrentDisplayMode();
}

eastl::string drv3d_vulkan::os_get_additional_ext_requirements(VulkanPhysicalDeviceHandle, const dag::Vector<VkExtensionProperties> &)
{
  return "";
}

void drv3d_vulkan::WindowState::updateRefreshRateFromCurrentDisplayMode() { refreshRate = x11.getScreenRefreshRate(); }

#if !defined(_TARGET_WAS_MULTI)
#include <linux/x11.cpp>
#endif
