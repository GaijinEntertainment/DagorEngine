#include "os.h"
#include <osApiWrappers/dag_messageBox.h>
#include <macosx/macWnd.h>

using namespace drv3d_vulkan;

void drv3d_vulkan::get_video_modes_list(Tab<String> &list)
{
  mac_wnd::get_video_modes_list(list);
}

bool drv3d_vulkan::validate_vulkan_signature(void* file)
{
  // on linux no way to verify driver signature?!?
  return true;
}

VulkanSurfaceKHRHandle drv3d_vulkan::init_window_surface(VulkanInstance &instance, SurfaceCreateParams parms)
{
  VulkanSurfaceKHRHandle result;
  VkMacOSSurfaceCreateInfoMVK createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.pView = parms.view;
  if (VULKAN_CHECK_FAIL(instance.vkCreateMacOSSurfaceMVK(instance.get(), &createInfo, NULL, ptr(result))))
    result = VulkanNullHandle();
  return result;
}

drv3d_vulkan::ScopedGPUPowerState::ScopedGPUPowerState(bool)
{

}

drv3d_vulkan::ScopedGPUPowerState::~ScopedGPUPowerState()
{

}