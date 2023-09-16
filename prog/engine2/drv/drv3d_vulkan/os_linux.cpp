#include "os.h"

#include <debug/dag_debug.h>
#include <linux/x11.h>

using namespace drv3d_vulkan;

// code borrowed from opengl driver
void drv3d_vulkan::get_video_modes_list(Tab<String> &list) { x11.getVideoModeList(list); }

bool drv3d_vulkan::validate_vulkan_signature(void *file)
{
  G_UNREFERENCED(file);
  // on linux no way to verify driver signature?!?
  return true;
}

VulkanSurfaceKHRHandle drv3d_vulkan::init_window_surface(VulkanInstance &instance, SurfaceCreateParams parms)
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
    xlibsci.dpy = parms.display;
    xlibsci.window = parms.window;
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

drv3d_vulkan::ScopedGPUPowerState::ScopedGPUPowerState(bool) {}

drv3d_vulkan::ScopedGPUPowerState::~ScopedGPUPowerState() {}

#if !defined(_TARGET_WAS_MULTI)
#include <linux/x11.cpp>
#endif
