#pragma once
#include "vulkan_instance.h"
#include <3d/dag_drv3d.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>

#if _TARGET_C3

#endif

namespace drv3d_vulkan
{
void get_video_modes_list(Tab<String> &list);
bool validate_vulkan_signature(void *file);

// Target specific structure, constructor abstracts away the different kinds of inputs
struct SurfaceCreateParams
{
#if _TARGET_PC_WIN
  HINSTANCE hInstance;
  HWND hWnd;
  SurfaceCreateParams(HINSTANCE hi, HWND hw) : hInstance(hi), hWnd(hw) {}
#elif _TARGET_PC_LINUX
  Display *display;
  Window window;
  SurfaceCreateParams(Display *d, Window w) : display(d), window(w) {}
#elif _TARGET_C3


#elif _TARGET_PC_MACOSX
  void *view;
  SurfaceCreateParams(void *v) : view(v) {}
#elif _TARGET_ANDROID
  void *window = nullptr;
  SurfaceCreateParams(void *w) : window(w) {}
#endif
  SurfaceCreateParams() = default;
  ~SurfaceCreateParams() = default;

  SurfaceCreateParams(const SurfaceCreateParams &) = default;
  SurfaceCreateParams &operator=(const SurfaceCreateParams &) = default;

  SurfaceCreateParams(SurfaceCreateParams &&) = default;
  SurfaceCreateParams &operator=(SurfaceCreateParams &&) = default;
};
VulkanSurfaceKHRHandle init_window_surface(VulkanInstance &instance, SurfaceCreateParams parms);


struct ScopedGPUPowerState
{
  ScopedGPUPowerState(bool forceHighPower);
  ~ScopedGPUPowerState();
  ScopedGPUPowerState(const ScopedGPUPowerState &) = delete;
  ScopedGPUPowerState &operator=(const ScopedGPUPowerState &) = delete;

#if _TARGET_C3

#endif
};
} // namespace drv3d_vulkan
