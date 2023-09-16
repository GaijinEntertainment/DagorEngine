#include <3d/dag_drv3dReset.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_gpuConfig.h>
#include <startup/dag_restart.h>
#include <debug/dag_debug.h>
#include "texMgrData.h"
#include <drv_log_defs.h>

#include <EASTL/array.h>
#include <statsd/statsd.h>

bool dagor_d3d_force_driver_reset = false;
bool dagor_d3d_force_driver_mode_reset = false;
bool dagor_d3d_notify_fullscreen_state_restored = false;

static IDrv3DResetCB *ext_drv3d_reset_handler = NULL;
IDrv3DDeviceLostCB *ext_drv3d_device_lost_handler = NULL;


void
#if _MSC_VER >= 1300
  __declspec(noinline)
#endif
    d3derr_in_device_reset(const char *msg)
{
  logerr("%s:\n%s (device reset)", msg, d3d::get_last_error());
  G_UNUSED(msg);
}


void before_reset_3d_device(bool full_reset)
{
  if (texmgr_internal::stop_bkg_tex_loading)
  {
    d3d::driver_command(DRV3D_COMMAND_RELEASE_LOADING, (void *)1, NULL, NULL); // lockWrite
    texmgr_internal::stop_bkg_tex_loading(1);
    d3d::driver_command(DRV3D_COMMAND_ACQUIRE_LOADING, (void *)1, NULL, NULL);
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

#if _TARGET_PC_WIN | _TARGET_ANDROID
bool check_and_restore_3d_device()
{
  static int reset_failed_count = 0;
  static bool d3dd_requires_reset = false;

  bool can_reset_now = true;

  if (dagor_d3d_notify_fullscreen_state_restored)
  {
    dagor_d3d_notify_fullscreen_state_restored = false;
    fullscreen_state_restored();
  }

  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);
  if (!d3d::device_lost(&can_reset_now) && !d3dd_requires_reset)
  {
    d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
    return true;
  }

  if (!can_reset_now)
  {
    d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
    return false;
  }

  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);    // Reset CS is outside of the GPU CS in loading threads.
  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_LOADING, (void *)1, NULL, NULL); // lockWrite
  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL); // Re-acquire the GPU from non-loading, non-main threads, no
                                                                          // need to re-check device_lost, if it was lost, only the
                                                                          // main thread will reset it.

  bool full_reset =
    !dagor_d3d_force_driver_mode_reset && (d3d::get_driver_code() == _MAKE4C('DX11') || d3d::get_driver_code() == _MAKE4C('DX12'));
  bool mode_reset = dagor_d3d_force_driver_mode_reset;

  before_reset_3d_device(full_reset);
  if (full_reset)
    shutdown_game(RESTART_VIDEODRV);
  else if (mode_reset)
    shutdown_game(RESTART_DRIVER_VIDEO_MODE);

  debug_ctx("==== resetting 3d device ====");
  if (!d3d::reset_device())
  {
    d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
    d3d::driver_command(DRV3D_COMMAND_RELEASE_LOADING, (void *)1, NULL, NULL);
    debug("d3d::reset_device(%d) failed with '%s'", reset_failed_count, d3d::get_last_error());
    if (++reset_failed_count > 16)
      d3derr(0, "can't reset 3d device (possible solution: update or reinstall the gpu driver)");
    d3dd_requires_reset = true;
    return false;
  }

  d3dd_requires_reset = false;
  d3d::driver_command(DRV3D_COMMAND_RELEASE_LOADING, (void *)1, NULL, NULL);

  if (full_reset)
    startup_game(RESTART_VIDEODRV);
  else if (mode_reset)
    startup_game(RESTART_DRIVER_VIDEO_MODE);

  after_reset_3d_device(full_reset);

  if (d3d::device_lost(NULL))
    debug("Device lost again during afterReset"); // It is OK for device to be lost again at this point.

  reset_failed_count = 0;
  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
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

#ifdef REQUIRE_DRIVER_RESET_REPORT
static float convert_game_time_to_reset_stat_buckets(int game_ms)
{
  const float minutes = static_cast<float>(game_ms) / 1000.f / 60.f;
  const eastl::array<float, 5> KEY_POINTS = {0.f, 1.f, 5.f, 10.f, 30.f};
  const eastl::array<float, 7> BUCKET_LIMITS = {0.f, 10.f, 25.f, 50.f, 75.f, 90.f, 100.f};
  G_STATIC_ASSERT(KEY_POINTS.size() < BUCKET_LIMITS.size());
  for (uint32_t i = 1; i < KEY_POINTS.size(); ++i)
  {
    if (minutes < KEY_POINTS[i])
    {
      return (minutes - KEY_POINTS[i - 1]) / (KEY_POINTS[i] - KEY_POINTS[i - 1]) * (BUCKET_LIMITS[i] - BUCKET_LIMITS[i - 1]) +
             BUCKET_LIMITS[i - 1];
    }
  }
  return min(minutes - KEY_POINTS.back() + BUCKET_LIMITS[KEY_POINTS.size() - 1], BUCKET_LIMITS.back());
}

void report_device_reset(int resetCount, int lastResetTime)
{
  // Store tmp objects to have valid const char * for tag values
  const String driverVersion = d3d_get_gpu_cfg().generateDriverVersionString();
  const String resetCountStr(0, "%d", resetCount);
  const String lastErrorCode(0, "%X", d3d::get_last_error_code());
  const dag::Vector<statsd::MetricTag> tags = {
    {"driver", d3d::get_driver_name()},
    {"vendor", d3d_get_vendor_name(d3d_get_gpu_cfg().primaryVendor)},
    {"driver_version", driverVersion.c_str()},
    {"idx", resetCountStr.c_str()},
    {"code", lastErrorCode.c_str()},
  };
  statsd::counter("render.device_reset", 1, tags);
  statsd::histogram("render.device_reset_time", convert_game_time_to_reset_stat_buckets(lastResetTime), tags);
}
#endif
