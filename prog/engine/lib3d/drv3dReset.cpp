// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_renderTarget.h>
#include <3d/dag_gpuConfig.h>
#include <startup/dag_restart.h>
#include <debug/dag_debug.h>
#include "texMgrData.h"
#include <drv_log_defs.h>

bool dagor_d3d_force_driver_reset = false;
bool dagor_d3d_force_driver_mode_reset = false;
bool dagor_d3d_notify_fullscreen_state_restored = false;

static IDrv3DResetCB *ext_drv3d_reset_handler = NULL;
IDrv3DDeviceLostCB *ext_drv3d_device_lost_handler = NULL;

static bool window_resizing_by_mouse = false;
static bool window_resize_handling_in_progress = false;
static int window_width = 0, window_height = 0;

bool is_window_resizing_by_mouse() { return window_resizing_by_mouse || window_resize_handling_in_progress; }
void set_window_resizing_by_mouse(bool value) { window_resizing_by_mouse = value; }
void notify_window_resized(int w, int h)
{
  window_width = w;
  window_height = h;
}

void
#if _MSC_VER >= 1300
  __declspec(noinline)
#endif
  d3derr_in_device_reset(const char *msg)
{
  D3D_ERROR("%s:\n%s (device reset)", msg, d3d::get_last_error());
  G_UNUSED(msg);
}


void before_reset_3d_device(bool full_reset)
{
  if (texmgr_internal::stop_bkg_tex_loading)
  {
    d3d::driver_command(Drv3dCommand::RELEASE_LOADING, (void *)1); // lockWrite
    texmgr_internal::stop_bkg_tex_loading(1);
    d3d::driver_command(Drv3dCommand::ACQUIRE_LOADING, (void *)1);
  }
  if (full_reset)
    discard_unused_managed_textures();

  D3dResetQueue::perform_before_reset(full_reset);
  if (ext_drv3d_reset_handler)
    ext_drv3d_reset_handler->beforeReset(full_reset);
}

void after_reset_3d_device(bool full_reset)
{
  D3dResetQueue::perform_after_reset(full_reset);
  if (ext_drv3d_reset_handler)
    ext_drv3d_reset_handler->afterReset(full_reset);
}

void zero_reset_3d_device_counter()
{
  if (ext_drv3d_reset_handler)
    ext_drv3d_reset_handler->resetCounter();
}

bool check_and_handle_window_resize()
{
  if (dgs_get_window_mode() != WindowMode::WINDOWED_RESIZABLE)
    return false;

  int scrW, scrH;
  d3d::get_screen_size(scrW, scrH);
  if (DAGOR_LIKELY(scrW == window_width && scrH == window_height))
    return false;

  window_resize_handling_in_progress = true;

  if (ext_drv3d_reset_handler)
    ext_drv3d_reset_handler->windowResized();
  bool applyAfterResetDevice = false;
  change_driver_reset_request(applyAfterResetDevice, true);
  return true;
}

void fullscreen_state_restored()
{
  if (ext_drv3d_reset_handler)
    ext_drv3d_reset_handler->fullscreenStateRestored();
}


void change_driver_reset_request(bool &out_apply_after_reset_device, bool mode_reset)
{
#if _TARGET_PC_WIN | _TARGET_ANDROID
  out_apply_after_reset_device = true;
  if (mode_reset)
    dagor_d3d_force_driver_mode_reset = true;
  else
    dagor_d3d_force_driver_reset = true;
#else
  out_apply_after_reset_device = false;
  G_UNUSED(mode_reset);
#endif
}

void set_3d_device_reset_callback(IDrv3DResetCB *handler) { ext_drv3d_reset_handler = handler; }

void set_3d_device_lost_callback(IDrv3DDeviceLostCB *handler) { ext_drv3d_device_lost_handler = handler; }

#if _TARGET_PC | _TARGET_ANDROID
static int restoring_3d_device = 0;
bool is_restoring_3d_device() { return interlocked_acquire_load(restoring_3d_device) != 0; }
bool check_and_restore_3d_device()
{
  static int reset_failed_count = 0;
  static bool d3dd_requires_reset = false;

  if (DAGOR_UNLIKELY(dagor_d3d_notify_fullscreen_state_restored))
  {
    dagor_d3d_notify_fullscreen_state_restored = false;
    fullscreen_state_restored();
  }

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
  if (bool can_reset_now = true; DAGOR_LIKELY(!d3d::device_lost(&can_reset_now) && !d3dd_requires_reset))
  {
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
    return true;
  }
  else if (!can_reset_now)
  {
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
    return false;
  }

  interlocked_release_store(restoring_3d_device, 1);
  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);          // Reset CS is outside of the GPU CS in loading threads.
  d3d::driver_command(Drv3dCommand::ACQUIRE_LOADING, (void *)1); // lockWrite
  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);          // Re-acquire the GPU from non-loading, non-main threads, no
                                                                 // need to re-check device_lost, if it was lost, only the
                                                                 // main thread will reset it.

  bool full_reset = !dagor_d3d_force_driver_mode_reset && d3d::get_driver_code().is(d3d::dx11 || d3d::dx12 || d3d::vulkan);
  bool mode_reset = dagor_d3d_force_driver_mode_reset;

  before_reset_3d_device(full_reset);
  if (full_reset)
    shutdown_game(RESTART_VIDEODRV);
  else if (mode_reset)
    shutdown_game(RESTART_DRIVER_VIDEO_MODE);

  DEBUG_CTX("==== resetting 3d device ====");
  if (!d3d::reset_device())
  {
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
    d3d::driver_command(Drv3dCommand::RELEASE_LOADING, (void *)1);
    debug("d3d::reset_device(%d) failed with '%s'", reset_failed_count, d3d::get_last_error());
    if (++reset_failed_count > 16)
      d3derr(0, "can't reset 3d device (possible solution: update or reinstall the gpu driver)");
    d3dd_requires_reset = true;
    return false;
  }

  d3dd_requires_reset = false;
  d3d::driver_command(Drv3dCommand::RELEASE_LOADING, (void *)1);

  if (full_reset)
    startup_game(RESTART_VIDEODRV);
  else if (mode_reset)
    startup_game(RESTART_DRIVER_VIDEO_MODE);

  after_reset_3d_device(full_reset);

  if (d3d::device_lost(NULL))
    debug("Device lost again during afterReset"); // It is OK for device to be lost again at this point.

  reset_failed_count = 0;
  window_resize_handling_in_progress = false;
  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
  interlocked_release_store(restoring_3d_device, 0);
  return true;
}
#endif

static uint32_t d3d_reset_counter = 1, d3d_full_reset_counter = 1;
unsigned int get_d3d_reset_counter() { return d3d_reset_counter; }
unsigned int get_d3d_full_reset_counter() { return d3d_full_reset_counter; }

D3dResetQueue *D3dResetQueue::tailBeforeReset = nullptr, *D3dResetQueue::tailAfterReset = nullptr;
void D3dResetQueue::perform_reset_queue(const D3dResetQueue *q, bool full_reset)
{
  if (++d3d_reset_counter == 0)
    ++d3d_reset_counter;
  if (full_reset && ++d3d_full_reset_counter == 0)
    ++d3d_full_reset_counter;
  for (; q; q = q->next)
    if (q->func)
    {
      debug("perform reset func: %s", q->name);
      q->func(full_reset);
    }
}
