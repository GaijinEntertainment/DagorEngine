// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_ANDROID
#error Android specific file included in wrong target
#endif

#include <supp/dag_android_native_app_glue.h>

#include "os.h"

#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/integer/dag_IPoint2.h>
#include <drv_utils.h>
#include <driver.h>
#include "globals.h"
#include "swapchain.h"

using namespace drv3d_vulkan;

extern float dagor_android_scr_xdpi;
extern float dagor_android_scr_ydpi;
extern const DataBlock *dagor_android_preload_settings_blk;

static const IPoint2 undefined_resolution = IPoint2(0, 0);
static constexpr float undefined_scale = 1.f;

namespace native_android
{
// default resolution on nswitch, taken from vulkan
// sample from the nswitch dev docs.
int32_t window_width = 1280;
int32_t window_height = 720;
int32_t buffer_width = 640;
int32_t buffer_height = 360;
IPoint2 target_resolution = undefined_resolution;
float square_scale = 1.f;
} // namespace native_android

void android_d3d_reinit(void *w)
{
  using namespace native_android;

  ANativeWindow *nativeWindow = (ANativeWindow *)w;

  window_width = ANativeWindow_getWidth(nativeWindow);
  window_height = ANativeWindow_getHeight(nativeWindow);

  int32_t maxWaitMs = 1000;
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
  square_scale = undefined_scale;

  const DataBlock *videoBlk = settings->getBlockByNameEx("video");
  const String resStr(16, videoBlk->getStr("androidResolution", "Native"));

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

    // When screen is a square (tablet/foldable phone) depending on
    // the calculation mode for resolution we might get an unreasonably
    // low amount of total pixels. Here we can scale the resolution back
    // depending on how different the aspect it to the reference value
    // (for majority of the phones scale should be = 1)
    if (const DataBlock *referenceAspectBlk = videoBlk->getBlockByName("referenceAspectRatio"))
    {
      // 20/9 aspect is used for majority of low to medium end phones
      const IPoint2 referenceAspect = referenceAspectBlk->getIPoint2("aspect", {20, 9});
      const float referenceRatio = static_cast<float>(referenceAspect.x) / referenceAspect.y;
      const float realRatio = static_cast<float>(window_width) / window_height;

      const float threshold = referenceAspectBlk->getReal("deviationThreshold", 0.1f);
      const float deviation = (referenceRatio / realRatio) - 1;
      if (deviation >= threshold)
      {
        const float scaledDeviation = deviation * referenceAspectBlk->getReal("deviationMultiplier", 1.f);
        const float clampedDeviation = min(scaledDeviation, referenceAspectBlk->getReal("deviationLimit", 0.f));
        square_scale = sqrt(1 + clampedDeviation); // applied to both sides so sqrt

        debug("vulkan: Aspect based screen scale %f", square_scale);
      }
    }

    target_resolution.x *= square_scale;
    target_resolution.y *= square_scale;

    target_resolution = min(target_resolution, {window_width, window_height});
    target_resolution -= target_resolution % 8;
  }
  else
  {
    const String aspectRatio(16, videoBlk->getStr("androidAspectRatio", "Auto"));
    if (aspectRatio != "Fixed")
    {
      target_resolution.x = (window_width * target_resolution.y) / window_height;
    }
  }

  buffer_width = target_resolution.x;
  buffer_height = target_resolution.y;

  ANativeWindow_setBuffersGeometry(nativeWindow, buffer_width, buffer_height, 0);
  DEBUG_CTX("ScreenSizes realDPI: %dx%d win: %dx%d buf: %dx%d", dagor_android_scr_xdpi, dagor_android_scr_ydpi, window_width,
    window_height, buffer_width, buffer_height);

  // if this re-init is not first, reset device to setup proper surface
  if (d3d::is_inited())
  {
    Globals::swapchain.forceRecreate();
    d3d::reset_device();
  }
}

void android_update_window_size(void *w)
{
  using namespace native_android;
  ANativeWindow *nativeWindow = (ANativeWindow *)w;
  window_width = ANativeWindow_getWidth(nativeWindow);
  window_height = ANativeWindow_getHeight(nativeWindow);
}

void drv3d_vulkan::WindowState::getRenderWindowSettings()
{
  settings.resolutionX = native_android::buffer_width;
  settings.resolutionY = native_android::buffer_height;
}

void drv3d_vulkan::os_restore_display_mode() {}

void drv3d_vulkan::os_set_display_mode(int, int) {}

eastl::string drv3d_vulkan::os_get_additional_ext_requirements(VulkanPhysicalDeviceHandle dev,
  const eastl::vector<VkExtensionProperties> &extensions)
{
  G_UNUSED(dev);
  G_UNUSED(extensions);
  return "";
}

VulkanSurfaceKHRHandle drv3d_vulkan::init_window_surface(VulkanInstance &instance)
{
  VulkanSurfaceKHRHandle result;
  VkAndroidSurfaceCreateInfoKHR sci = {};
  sci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  sci.window = (ANativeWindow *)Globals::window.getMainWindow();
  if (VULKAN_CHECK_FAIL(instance.vkCreateAndroidSurfaceKHR(instance.get(), &sci, NULL, ptr(result))))
  {
    result = VulkanNullHandle();
  }
  return result;
}
