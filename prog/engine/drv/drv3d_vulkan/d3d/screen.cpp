// Copyright (C) Gaijin Games KFT.  All rights reserved.

// screen/display related methods implementation
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform.h>
#include "globals.h"
#include "frontend.h"
#include "driver_config.h"
#include "predicted_latency_waiter.h"
#include "device_context.h"

using namespace drv3d_vulkan;

bool d3d::update_screen(bool /*app_active*/)
{
  Globals::ctx.present();

  // restore BB
  d3d::set_render_target();

  if (::dgs_on_swap_callback)
    ::dgs_on_swap_callback();

  return true;
}

void d3d::wait_for_async_present(bool) {}

void d3d::gpu_latency_wait()
{
  if (!Globals::cfg.bits.autoPredictedLatencyWaitApp && Globals::cfg.bits.allowPredictedLatencyWaitApp)
    Frontend::latencyWaiter.asyncWait();
}

bool d3d::get_vsync_enabled() { return Globals::swapchain.getMode().isVsyncOn(); }

bool d3d::enable_vsync(bool enable)
{
  auto videoCfg = ::dgs_get_settings()->getBlockByNameEx("video");
  {
    SwapchainMode newMode(Globals::swapchain.getMode());
    newMode.presentMode = get_requested_present_mode(enable, videoCfg->getBool("adaptive_vsync", false));
    Globals::swapchain.setMode(newMode);
  }
  return true;
}

#if _TARGET_PC
void d3d::get_video_modes_list(Tab<String> &list) { drv3d_vulkan::get_video_modes_list(list); }
#endif

float d3d::get_screen_aspect_ratio() { return Globals::window.settings.aspect; }

void d3d::get_screen_size(int &w, int &h)
{
  VkExtent2D extent = Globals::swapchain.getMode().extent;

  w = extent.width;
  h = extent.height;
}
