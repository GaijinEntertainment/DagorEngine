// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "os.h"

#include <stdlib.h>
#include <debug/dag_debug.h>
#include <drv_utils.h>
#include <osApiWrappers/dag_linuxGUI.h>
#include <startup/dag_globalSettings.h>

#include "globals.h"
#include "../workCycle/workCyclePriv.h"

using namespace drv3d_vulkan;

namespace
{

bool get_window_client_rect(void *w, linux_GUI::RECT *rect)
{
  if (!w || !rect)
    return false;

  linux_GUI::RECT client, unclipped;
  linux_GUI::get_window_screen_rect(w, &client, &unclipped);
  rect->right = client.right - unclipped.left;
  rect->left = client.left - unclipped.left;
  rect->top = client.top - unclipped.top;
  rect->bottom = client.bottom - unclipped.top;
  return true;
}
} // namespace

void drv3d_vulkan::get_video_modes_list(Tab<String> &list) { linux_GUI::get_video_mode_list(list); }

bool drv3d_vulkan::validate_vulkan_signature(void *file)
{
  G_UNREFERENCED(file);
  // on linux no way to verify driver signature?!?
  return true;
}

VulkanSurfaceKHRHandle drv3d_vulkan::init_window_surface(VulkanInstance &instance, void *window)
{
  VulkanSurfaceKHRHandle result{};
  if (!window)
    return result;
  G_UNUSED(instance);
  if (linux_GUI::is_wayland())
  {
#if VK_KHR_wayland_surface
    if (instance.hasExtension<SurfaceWaylandKHR>())
    {
      VkWaylandSurfaceCreateInfoKHR wlsci;
      wlsci.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
      wlsci.pNext = NULL;
      wlsci.flags = 0;
      wlsci.display = (wl_display *)linux_GUI::get_native_display();
      wlsci.surface = (wl_surface *)linux_GUI::get_native_window(window);
      instance.vkCreateWaylandSurfaceKHR(instance.get(), &wlsci, NULL, ptr(result));
    }
#endif
  }
  else if (linux_GUI::is_x11())
  {
#if VK_KHR_xlib_surface
    if (instance.hasExtension<SurfaceXlibKHR>())
    {
      VkXlibSurfaceCreateInfoKHR xlibsci;
      xlibsci.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
      xlibsci.pNext = NULL;
      xlibsci.flags = 0;
      xlibsci.dpy = (Display *)linux_GUI::get_native_display();
      xlibsci.window = (Window)linux_GUI::get_native_window(window);
      instance.vkCreateXlibSurfaceKHR(instance.get(), &xlibsci, NULL, ptr(result));
    }
#endif
  }
  else
    D3D_ERROR("vulkan: linux windowing system failed to init");
  return result;
}

VkExtent2D drv3d_vulkan::get_window_client_rect_extent(void *window)
{
  linux_GUI::RECT rc = {};
  get_window_client_rect(window, &rc);
  return {static_cast<uint32_t>(rc.right - rc.left), static_cast<uint32_t>(rc.bottom - rc.top)};
}

void drv3d_vulkan::WindowState::set(void *, const char *, int, void *, void *, const char *title, void *wnd_proc)
{
  if (!linux_GUI::init())
    DAG_FATAL("vulkan: unable to init windowing subsystem check that X11/Wayland is avaialble");

  windowTitle = title;
  mainCallback = (main_wnd_f *)wnd_proc;
}

void drv3d_vulkan::WindowState::getRenderWindowSettings()
{
  int w, h;
  bool isAuto, isRetina;
  linux_GUI::get_display_size(w, h, dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE);
  bool windowed =
    dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE && dgs_get_window_mode() != WindowMode::WINDOWED_FULLSCREEN;
  if (windowed && linux_GUI::get_main_window_ptr_handle())
  {
    const auto extent = get_window_client_rect_extent(linux_GUI::get_main_window_ptr_handle());
    w = extent.width;
    h = extent.height;
  }
  get_settings_resolution(settings.resolutionX, settings.resolutionY, isRetina, w, h, isAuto);
}

bool drv3d_vulkan::WindowState::setRenderWindowParams()
{
  if (dgs_get_window_mode() == WindowMode::WINDOWED_RESIZABLE && linux_GUI::get_main_window_ptr_handle())
  {
    const auto extent = get_window_client_rect_extent(linux_GUI::get_main_window_ptr_handle());
    currentResolutionX = extent.width;
    currentResolutionY = extent.height;
  }

  if (windowMode == (int)dgs_get_window_mode() && currentResolutionX == settings.resolutionX &&
      currentResolutionY == settings.resolutionY && ownsWindow)
    return true;

  closeWindow();
  if (linux_GUI::init_window(windowTitle, settings.resolutionX, settings.resolutionY))
  {
    bool windowed = dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE &&
                    dgs_get_window_mode() != WindowMode::WINDOWED_FULLSCREEN &&
                    dgs_get_window_mode() != WindowMode::WINDOWED_NO_BORDER;
    linux_GUI::set_fullscreen_mode(!windowed);
    currentResolutionX = settings.resolutionX;
    currentResolutionY = settings.resolutionY;
    windowMode = (int)dgs_get_window_mode();
    settings.aspect = (float)settings.resolutionX / settings.resolutionY;
    ownsWindow = true;
    return true;
  }
  return false;
}

void *drv3d_vulkan::WindowState::getMainWindow()
{
  if (!ownsWindow)
    return nullptr;
  return linux_GUI::get_main_window_ptr_handle();
}

void drv3d_vulkan::WindowState::closeWindow()
{
  if (ownsWindow)
  {
    linux_GUI::destroy_main_window();
    ownsWindow = false;
  }
}

void drv3d_vulkan::os_restore_display_mode() {}

void drv3d_vulkan::os_set_display_mode(int, int) {}

eastl::string drv3d_vulkan::os_get_additional_ext_requirements(VulkanPhysicalDeviceHandle, const dag::Vector<VkExtensionProperties> &)
{
  return "";
}

void drv3d_vulkan::WindowState::updateRefreshRateFromCurrentDisplayMode() { refreshRate = linux_GUI::get_screen_refresh_rate(); }
