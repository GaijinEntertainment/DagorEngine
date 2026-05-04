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
#include "backend/cmd/misc.h"
#include "backend/cmd/screen.h"

using namespace drv3d_vulkan;

bool d3d::update_screen(uint32_t frame_id, bool /*app_active*/)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  Globals::ctx.present(frame_id);

  {
    // may call wait and disturb front state, depends at least on driver global lock
    ScopedTimerTicks watch(Frontend::timings.acquireBackBufferDuration);
    Frontend::currentSwapchainToPresent->nextFrame();
  }

  // restore BB
  d3d::set_render_target();

  if (::dgs_on_swap_callback)
    ::dgs_on_swap_callback();

  return true;
}

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

void d3d::mark_simulation_start(uint32_t frame_id)
{
  if (auto *lowLatencyModule = GpuLatency::getInstance())
    lowLatencyModule->setMarker(frame_id, lowlatency::LatencyMarkerType::SIMULATION_START);
}

void d3d::mark_simulation_end(uint32_t frame_id)
{
  if (auto *lowLatencyModule = GpuLatency::getInstance())
    lowLatencyModule->setMarker(frame_id, lowlatency::LatencyMarkerType::SIMULATION_END);
}

void d3d::mark_render_start(uint32_t frame_id)
{
  Globals::ctx.dispatchCmd<CmdSetLatencyMarker>({frame_id, lowlatency::LatencyMarkerType::RENDERSUBMIT_START});
}

void d3d::mark_render_end(uint32_t frame_id)
{
  Globals::ctx.dispatchCmd<CmdSetLatencyMarker>({frame_id, lowlatency::LatencyMarkerType::RENDERSUBMIT_END});
}

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
  // weak point, pointer and content (size + image) is not stable, yet used without locks
  return Frontend::currentSwapchainToPresent->getCurrentTargetTex();
}

#if _TARGET_PC_WIN | _TARGET_PC_MACOSX | _TARGET_PC_LINUX
namespace
{
Swapchain *ensure_swapchain_for_window(void *hwnd)
{
  G_ASSERT(hwnd);
  if (hwnd == Globals::window.getMainWindow())
    return &Frontend::swapchain;

  auto swapchain = Frontend::secondarySwapchains.find(hwnd);
  if (swapchain == nullptr)
    return Frontend::secondarySwapchains.allocate(hwnd);

  const auto extent = get_window_client_rect_extent(hwnd);
  auto mode = swapchain->getMode();
  if (mode.extent.width != extent.width || mode.extent.height != extent.height)
  {
    mode.extent = extent;
    swapchain->setMode(mode);
  }
  return swapchain;
}
} // namespace

void d3d::pcwin::set_present_wnd(void *hwnd)
{
  D3D_CONTRACT_ASSERT_RETURN(hwnd, );
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  auto swapchain = ensure_swapchain_for_window(hwnd);
  Frontend::currentSwapchainToPresent = swapchain ? swapchain : &Frontend::swapchain;
}

bool d3d::pcwin::can_render_to_window() { return true; }

BaseTexture *d3d::pcwin::get_swapchain_for_window(void *hwnd)
{
  D3D_CONTRACT_ASSERT_RETURN(hwnd, nullptr);
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  auto swapchain = ensure_swapchain_for_window(hwnd);
  return swapchain ? swapchain->getCurrentTargetTex() : nullptr;
}

void d3d::pcwin::present_to_window(void *hwnd)
{
  D3D_CONTRACT_ASSERT_RETURN(hwnd, );
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  auto swapchain = Frontend::secondarySwapchains.find(hwnd);
  D3D_CONTRACT_ASSERTF_RETURN(swapchain, ,
    "vulkan: swapchain is not found, consider using `get_swapchain_for_window` or `set_present_wnd` before `present_to_window`");
  swapchain->prePresent();
  CmdPresent cmd = swapchain->present();
  cmd.enginePresentFrameId = 0;
  {
    OSSpinlockScopedLock lock{Globals::ctx.getFrontLock()};
    Frontend::replay->secondaryPresents.push_back(cmd);
  }
  swapchain->nextFrame();
}
#endif
