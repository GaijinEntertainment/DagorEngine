// Copyright (C) Gaijin Games KFT.  All rights reserved.

// screen/display related methods implementation
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform.h>
#include <3d/gpuLatency.h>
#include "globals.h"
#include "frontend.h"
#include "driver_config.h"
#include "predicted_latency_waiter.h"
#include "device_context.h"
#include "swapchain.h"
#include "execution_timings.h"
#include "texture.h"
#include "global_lock.h"

using namespace drv3d_vulkan;

bool d3d::update_screen(uint32_t /*frame_id*/, bool /*app_active*/)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  Globals::ctx.present();

  {
    // may call wait and disturb front state, depends at least on driver global lock
    ScopedTimerTicks watch(Frontend::timings.acquireBackBufferDuration);
    Frontend::swapchain.nextFrame();
  }

  // restore BB
  d3d::set_render_target();

  if (::dgs_on_swap_callback)
    ::dgs_on_swap_callback();

  return true;
}

void d3d::wait_for_async_present(bool) {}

void d3d::begin_frame(uint32_t frame_id, bool allow_wait)
{
  auto lowLatencyModule = GpuLatency::getInstance();

  if (lowLatencyModule)
    lowLatencyModule->startFrame(frame_id);

  if (allow_wait)
  {
    if (lowLatencyModule && lowLatencyModule->isEnabled())
      lowLatencyModule->sleep(frame_id);
    else if (!Globals::cfg.bits.autoPredictedLatencyWaitApp && Globals::cfg.bits.allowPredictedLatencyWaitApp)
      Frontend::latencyWaiter.asyncWait();
  }
}

void d3d::mark_simulation_start(uint32_t) {}
void d3d::mark_simulation_end(uint32_t) {}
void d3d::mark_render_start(uint32_t) {}
void d3d::mark_render_end(uint32_t) {}

bool d3d::get_vsync_enabled() { return Frontend::swapchain.getMode().isVsyncOn(); }

bool d3d::enable_vsync(bool enable)
{
  Globals::lock.acquire();
  auto videoCfg = ::dgs_get_settings()->getBlockByNameEx("video");
  {
    SwapchainMode newMode(Frontend::swapchain.getMode());
    newMode.presentMode = get_requested_present_mode(enable, videoCfg->getBool("adaptive_vsync", false));
    Frontend::swapchain.setMode(newMode);
  }
  Globals::lock.release();
  return true;
}

#if _TARGET_PC
void d3d::get_video_modes_list(Tab<String> &list) { drv3d_vulkan::get_video_modes_list(list); }
#endif

float d3d::get_screen_aspect_ratio() { return Globals::window.settings.aspect; }

void d3d::get_screen_size(int &w, int &h)
{
  VkExtent2D extent = Frontend::swapchain.getMode().extent;

  w = extent.width;
  h = extent.height;
}

Texture *d3d::get_backbuffer_tex()
{
  // weak point, pointer must be stable, but content (size + image) is not, yet used without locks
  return Frontend::swapchain.getCurrentTargetTex();
}
