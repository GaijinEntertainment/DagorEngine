// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_lowLatency.h>
#include <drv/3d/dag_resetDevice.h>
#include <gui/dag_stdGuiRender.h>
#include <3d/dag_profilerTracker.h>
#include <3d/gpuLatency.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_workCycle.h>
#include <shaders/dag_shaderBlock.h>
#include <math/integer/dag_IPoint2.h>
#include <drv/3d/dag_info.h>

#include <debug/dag_debug.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("latency", enable_log, false);

static lowlatency::LatencyMode latency_mode = lowlatency::LATENCY_MODE_OFF;
static float minimum_interval_ms = 0;
static uint32_t last_queried_frame_id = 0;

bool lowlatency::is_available()
{
  if (auto latency = GpuLatency::getInstance())
    return latency->getAvailableModes().size() > 1;
  return false;
}

void lowlatency::start_frame(uint32_t frame_id)
{
  if (auto latency = GpuLatency::getInstance())
    latency->startFrame(frame_id);
}

lowlatency::LatencyMode lowlatency::get_latency_mode() { return latency_mode; }

lowlatency::LatencyMode lowlatency::get_from_blk()
{
  const DataBlock *video = ::dgs_get_settings()->getBlockByNameEx("video");
  int def = static_cast<lowlatency::LatencyModeFlag>(lowlatency::LATENCY_MODE_OFF);
  if (get_supported_latency_modes() == def)
    return static_cast<lowlatency::LatencyMode>(def);

  def = video->getInt("latency", def);
  const auto vendor = d3d::get_driver_desc().info.vendor;
  int mode = lowlatency::LatencyMode::LATENCY_MODE_OFF;
  switch (vendor)
  {
    case GpuVendor::NVIDIA: mode = video->getInt("nvidia_latency", def); break;
    case GpuVendor::AMD: mode = video->getInt("amd_latency", def); break;
    case GpuVendor::INTEL: mode = video->getInt("intel_latency", def); break;
    default: mode = lowlatency::LatencyMode::LATENCY_MODE_OFF;
  }
  if (vendor == GpuVendor::NVIDIA)
  {
    const bool frameGenerationEnabled =
      video->getInt("dlssFrameGenerationCount", 0) > 0 ||
      (stricmp(video->getStr("antialiasing_mode", "off"), "dlss") == 0 && video->getInt("antialiasing_fgc", 0) > 0);
    if (frameGenerationEnabled)
      return lowlatency::LatencyMode::LATENCY_MODE_NV_BOOST; // forced by frame generation
  }
  return static_cast<lowlatency::LatencyMode>(mode);
}

lowlatency::LatencyModeFlag lowlatency::get_supported_latency_modes()
{
  lowlatency::LatencyModeFlag ret = 0;
  if (auto latency = GpuLatency::getInstance())
  {
    for (auto mode : latency->getAvailableModes())
    {
      switch (mode)
      {
        case GpuLatency::Mode::Off: ret |= static_cast<LatencyModeFlag>(lowlatency::LATENCY_MODE_OFF); break;
        case GpuLatency::Mode::On: ret |= static_cast<LatencyModeFlag>(lowlatency::LATENCY_MODE_ON); break;
        case GpuLatency::Mode::OnPlusBoost: ret |= static_cast<LatencyModeFlag>(lowlatency::LATENCY_MODE_NV_BOOST); break;
        default: break;
      }
    }
  }

  return ret;
}

void lowlatency::set_latency_mode(LatencyMode mode) { set_latency_mode(mode, minimum_interval_ms); }

void lowlatency::set_latency_mode(LatencyMode mode, float min_interval_ms)
{
  if (mode != LATENCY_MODE_OFF && (get_supported_latency_modes() & static_cast<LatencyModeFlag>(mode)) == 0)
  {
    // This should not happen in normal cases
    logerr("Unsupported latency mode requested (requested mode: %d, supported flags: %d)", static_cast<LatencyModeFlag>(mode),
      get_supported_latency_modes());
    return;
  }
  minimum_interval_ms = min_interval_ms;
  latency_mode = mode;

  if (auto latency = GpuLatency::getInstance())
    switch (mode)
    {
      case LATENCY_MODE_OFF: latency->setOptions(GpuLatency::Mode::Off, min_interval_ms * 1000); break;
      case LATENCY_MODE_ON: latency->setOptions(GpuLatency::Mode::On, min_interval_ms * 1000); break;
      case LATENCY_MODE_NV_BOOST: latency->setOptions(GpuLatency::Mode::OnPlusBoost, min_interval_ms * 1000); break;
    }
}

void lowlatency::mark_flash_indicator()
{
  if (auto latency = GpuLatency::getInstance())
    latency->setMarker(dagor_get_latest_frame_started(), lowlatency::LatencyMarkerType::TRIGGER_FLASH);
}

void lowlatency::sleep(uint32_t frame_id)
{
  TIME_PROFILE(lowlatency_sleep);
  TRACK_SCOPE_TIME(Latency, sleep);
  if (auto latency = GpuLatency::getInstance())
    latency->sleep(frame_id);
}

lowlatency::LatencyData lowlatency::get_statistics_since(uint32_t frame_id, uint32_t max_count)
{
  LatencyData ret;
  // A platform independent version can be implemented here if nvlowlatency is not available
  if (auto latency = GpuLatency::getInstance())
    ret = latency->getStatisticsSince(frame_id, max_count);
  last_queried_frame_id = ret.latestFrameId;
  return ret;
}

lowlatency::LatencyData lowlatency::get_last_statistics() { return get_statistics_since(0, 1); }

lowlatency::LatencyData lowlatency::get_new_statistics(uint32_t max_count)
{
  return get_statistics_since(last_queried_frame_id, max_count);
}

static void low_latency_after_reset(bool) { lowlatency::set_latency_mode(lowlatency::get_from_blk(), minimum_interval_ms); }

REGISTER_D3D_AFTER_RESET_FUNC(low_latency_after_reset);

static bool low_latency_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("latency", "avg_latency", 1, 2)
  {
    const int frameCount = argc >= 2 ? console::to_int(argv[1]) : 64;
    const lowlatency::LatencyData latency = lowlatency::get_statistics_since(0, frameCount);
    if (latency.frameCount > 0)
    {
      console::print_d("Latency of %d frames:", latency.frameCount);
      console::print_d(" Game latency: %fms", latency.gameLatency.avg);
      console::print_d(" Game to render latency: %fms", latency.gameToRenderLatency.avg);
      console::print_d(" Render latency: %fms", latency.renderLatency.avg);
    }
    else
      console::print_d("No latency information");
  }
  CONSOLE_CHECK_NAME("latency", "set_latency_mode", 1, 3)
  {
    lowlatency::LatencyMode mode = lowlatency::LATENCY_MODE_OFF;
    if (argc >= 2)
    {
      int m = console::to_int(argv[1]);
      switch (m)
      {
        case 1:
          mode = lowlatency::LATENCY_MODE_ON;
          console::print_d("Latency mode ON");
          break;
        case 2:
          mode = lowlatency::LATENCY_MODE_NV_BOOST;
          console::print_d("Latency mode BOOST");
          break;
        default: console::print_d("Latency mode OFF"); break;
      }
    }
    const float minimumIntervalMs = argc >= 3 ? console::to_real(argv[2]) : 0.0f;
    lowlatency::set_latency_mode(mode, minimumIntervalMs);
  }
  CONSOLE_CHECK_NAME("latency", "help", 1, 1)
  {
    console::print(" - For debug overlay, type latency.debug_latency");
    console::print(" - To toggle debug log of latency markers, type latency.enable_debug (floods "
                   "the debug log)");
    console::print(" - To enable the low latency features, type latency.set_latency_mode "
                   "<mode> <min_frame_time_in_ms>");
    console::print("   mode: 0 -> Off, 1 -> On, 2 -> Boost");
    console::print("   min_frame_time_in_ms: 0 for no limit");
  }
  return found;
}

bool lowlatency::is_vsync_allowed()
{
  if (auto latency = GpuLatency::getInstance())
    return latency->isVsyncAllowed();
  return true;
}

REGISTER_CONSOLE_HANDLER(low_latency_console_handler);
