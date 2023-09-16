#if !_TARGET_ANDROID
#error Android specific file included in wrong target
#endif

#include <supp/dag_android_native_app_glue.h>

#include "os.h"

#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <math/integer/dag_IPoint2.h>
#include <drv_utils.h>

using namespace drv3d_vulkan;

extern float dagor_android_scr_xdpi;
extern float dagor_android_scr_ydpi;
extern const DataBlock *dagor_android_preload_settings_blk;

static const IPoint2 undefined_resolution = IPoint2(0, 0);

namespace native_android
{
// default resolution on nswitch, taken from vulkan
// sample from the nswitch dev docs.
int32_t window_width = 1280;
int32_t window_height = 720;
int32_t buffer_width = 640;
int32_t buffer_height = 360;
IPoint2 target_resolution = undefined_resolution;
} // namespace native_android

void android_d3d_reinit(void *w)
{
  using namespace native_android;

  ANativeWindow *nativeWindow = (ANativeWindow *)w;

  window_width = ANativeWindow_getWidth(nativeWindow);
  window_height = ANativeWindow_getHeight(nativeWindow);

  int32_t maxWaitMs = 5000;
  int32_t waitStep = 100;
  while (((window_width == 1) || (window_height == 1)) && (maxWaitMs > 0))
  {
    debug("vulkan: window size is 1x1, waiting for correct one");
    sleep_msec(waitStep);
    maxWaitMs -= waitStep;
    window_width = ANativeWindow_getWidth(nativeWindow);
    window_height = ANativeWindow_getHeight(nativeWindow);
  }

  const DataBlock *settings = dgs_get_settings();
  if (!settings || settings->isEmpty())
    settings = dagor_android_preload_settings_blk;

  target_resolution = undefined_resolution;

  String resStr(16, settings->getBlockByNameEx("video")->getStr("androidResolution", "Native"));

  if (resStr == "HD")
    target_resolution = IPoint2(1280, 720);
  else if (resStr == "Full HD")
    target_resolution = IPoint2(1920, 1080);
  else if (resStr == "SD")
    target_resolution = IPoint2(960, 540);
  // keep underfined
  // else if (resStr == "Native")

  if (target_resolution == undefined_resolution)
  {
    bool isAuto, isRetina;
    get_settings_resolution(target_resolution.x, target_resolution.y, isRetina, window_width, window_height, isAuto);
  }
  else
  {
    String aspectRatio(16, settings->getBlockByNameEx("video")->getStr("androidAspectRatio", "Auto"));
    if (aspectRatio != "Fixed")
    {
      target_resolution.x = (window_width * target_resolution.y) / window_height;
    }
  }

  buffer_width = target_resolution.x;
  buffer_height = target_resolution.y;

  ANativeWindow_setBuffersGeometry(nativeWindow, buffer_width, buffer_height, 0);
  debug_ctx("ScreenSizes realDPI: %dx%d win: %dx%d buf: %dx%d", dagor_android_scr_xdpi, dagor_android_scr_ydpi, window_width,
    window_height, buffer_width, buffer_height);

  // if this re-init is not first, reset device to setup proper surface
  if (d3d::is_inited())
    d3d::reset_device();
}

void android_update_window_size(void *w)
{
  using namespace native_android;
  ANativeWindow *nativeWindow = (ANativeWindow *)w;
  window_width = ANativeWindow_getWidth(nativeWindow);
  window_height = ANativeWindow_getHeight(nativeWindow);
}

VulkanSurfaceKHRHandle drv3d_vulkan::init_window_surface(VulkanInstance &instance, SurfaceCreateParams parms)
{
  VulkanSurfaceKHRHandle result;
  VkAndroidSurfaceCreateInfoKHR sci = {};
  sci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  sci.window = (ANativeWindow *)parms.window;
  if (VULKAN_CHECK_FAIL(instance.vkCreateAndroidSurfaceKHR(instance.get(), &sci, NULL, ptr(result))))
  {
    result = VulkanNullHandle();
  }
  return result;
}

drv3d_vulkan::ScopedGPUPowerState::ScopedGPUPowerState(bool) {}

drv3d_vulkan::ScopedGPUPowerState::~ScopedGPUPowerState() {}
