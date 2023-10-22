#include <3d/dag_lowLatency.h>
#include <3d/dag_drv3dReset.h>
#include <3d/dag_nvLowLatency.h>
#include <gui/dag_stdGuiRender.h>
#include <3d/dag_profilerTracker.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_workCycle.h>
#include <shaders/dag_shaderBlock.h>
#include <math/integer/dag_IPoint2.h>

#include <debug/dag_debug.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("latency", enable_log, false);

static lowlatency::LatencyMode latency_mode = lowlatency::LATENCY_MODE_OFF;
static float minimum_interval_ms = 0;
static uint32_t last_queried_frame_id = 0;

constexpr uint32_t SLOP_HISTORY_SIZE = 8;
constexpr uint32_t SLOP_INCOMPLETE_FRAMES = 2;
static uint32_t slop_histroy[SLOP_HISTORY_SIZE] = {0};
static uint32_t current_frame = 0;
static uint32_t current_render_frame = 0;
static uint32_t frame_count = 0;
// max_sleep = max_sleep_params_us.x * min_slop + max_sleep_params_us.y
static Point2 max_sleep_params_us = Point2(0.9f, -1000);
static int64_t sim_start_stamp = 0;
static int64_t render_start_stamp = 0;
static int64_t present_start_stamp = 0;

static bool flash_triggered = false;
static int flash_draw_start = 0;

uint32_t lowlatency::start_frame()
{
  current_frame++;
  slop_histroy[current_frame % SLOP_HISTORY_SIZE] = 0;
  frame_count = ::min(SLOP_HISTORY_SIZE, current_frame);
  return current_frame;
}

bool lowlatency::feed_latency_input(unsigned int msg) { return nvlowlatency::feed_latency_input(get_current_frame(), msg); }

void lowlatency::start_render() { current_render_frame = current_frame; }

uint32_t lowlatency::get_current_render_frame() { return current_render_frame; }

uint32_t lowlatency::get_current_frame() { return current_frame; }

void lowlatency::init()
{
  debug("Low latency initialized");
  last_queried_frame_id = 0;
  current_frame = 0;
  current_render_frame = 0;
  frame_count = 0;
}

void lowlatency::close() { debug("Low latency closed"); }

lowlatency::LatencyMode lowlatency::get_latency_mode() { return latency_mode; }

lowlatency::LatencyMode lowlatency::get_from_blk()
{
  const DataBlock *video = ::dgs_get_settings()->getBlockByNameEx("video");
  constexpr int def = static_cast<lowlatency::LatencyModeFlag>(lowlatency::LATENCY_MODE_OFF);
  const int mode = video->getInt("latency", def);
  return static_cast<lowlatency::LatencyMode>(mode);
}

lowlatency::LatencyModeFlag lowlatency::get_supported_latency_modes()
{
  lowlatency::LatencyModeFlag ret = 0;
  if (nvlowlatency::is_available())
    ret |=
      static_cast<LatencyModeFlag>(lowlatency::LATENCY_MODE_NV_ON) | static_cast<LatencyModeFlag>(lowlatency::LATENCY_MODE_NV_BOOST);
#if DAGOR_DBGLEVEL > 0
  ret |= static_cast<LatencyModeFlag>(lowlatency::LATENCY_MODE_EXPERIMENTAL);
#endif
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
  nvlowlatency::set_latency_mode(latency_mode, minimum_interval_ms);
}

bool lowlatency::sleep()
{
  TIME_PROFILE(lowlatency_sleep);
  TRACK_SCOPE_TIME(Latency, sleep);
  int sleptTimeUsec = 0;
  int64_t timerStart = ::ref_time_ticks();
  nvlowlatency::sleep();
  sleptTimeUsec = ::get_time_usec(timerStart);
  if (latency_mode == LATENCY_MODE_EXPERIMENTAL && frame_count >= SLOP_INCOMPLETE_FRAMES)
  {
    TIME_D3D_PROFILE(low_latency_experimental_sleep);
    uint32_t possibleSleepTime = slop_histroy[(current_frame - frame_count + 1) % SLOP_HISTORY_SIZE];
    profiler_tracker::record_value("Latency", "Slop", static_cast<float>(possibleSleepTime) / 1000.f, 0);
    for (uint32_t i = current_frame - frame_count + 2; i < current_frame - SLOP_INCOMPLETE_FRAMES; ++i)
      possibleSleepTime = ::min(possibleSleepTime, slop_histroy[i % SLOP_HISTORY_SIZE]);
    possibleSleepTime = ::max(0.f, max_sleep_params_us.x * float(possibleSleepTime) + max_sleep_params_us.y);
    const int sleepDurationMsec = (int(possibleSleepTime) - sleptTimeUsec) / 1000;
    if (sleepDurationMsec > 0)
    {
      sleep_msec(sleepDurationMsec);
      sleptTimeUsec += sleepDurationMsec * 1000;
    }
  }
  register_slop(current_frame, (99 * sleptTimeUsec) / 100); // avoid gradually increasing the
                                                            // perceived latency
  return ::get_time_usec(timerStart) > 1000;                // 1ms
}

void lowlatency::mark_flash_indicator()
{
  flash_triggered = true;
  flash_draw_start = ::get_time_msec();
}

void lowlatency::set_marker(uint32_t frame_id, LatencyMarkerType marker_type)
{
  if (enable_log.get())
  {
    const char *markerTypes[] = {"SIMULATION_START", "SIMULATION_END", "RENDERRECORD_START", "RENDERRECORD_END", "RENDERSUBMIT_START",
      "RENDERSUBMIT_END", "PRESENT_START", "PRESENT_END", "INPUT_SAMPLE", "TRIGGER_FLASH"};
    if (marker_type == LatencyMarkerType::SIMULATION_START)
      debug("[latency] --------- Starte sequence -------- %d", frame_id);
    debug("[latency] marker type: <%s>,  frameId: %d", markerTypes[static_cast<unsigned int>(marker_type)], frame_id);
  }
  // A platform independent version can be implemented here if nvlowlatency is not available
  if (marker_type == LatencyMarkerType::SIMULATION_START)
  {
#if DAGOR_DBGLEVEL > 0
    lowlatency::LatencyData data = get_last_statistics();
    if (data.frameCount > 0)
      profiler_tracker::record_value("Latency", "Latency", data.gameToRenderLatency.avg, 0);
#endif
    sim_start_stamp = ::ref_time_ticks();
  }
  if (marker_type == LatencyMarkerType::RENDERSUBMIT_START)
    render_start_stamp = ::ref_time_ticks();
  if (marker_type == LatencyMarkerType::PRESENT_START)
    present_start_stamp = ::ref_time_ticks();
  if (flash_triggered && marker_type == LatencyMarkerType::INPUT_SAMPLE_FINISHED)
  {
    flash_triggered = false;
    set_marker(frame_id, LatencyMarkerType::TRIGGER_FLASH);
  }
  if (marker_type == LatencyMarkerType::SIMULATION_END)
    profiler_tracker::record_value("Latency", "Simulation", static_cast<float>(::get_time_usec(sim_start_stamp)) / 1000.f, 0);
  if (marker_type == LatencyMarkerType::RENDERSUBMIT_END)
    profiler_tracker::record_value("Latency", "Render submission", static_cast<float>(::get_time_usec(render_start_stamp)) / 1000.f,
      0);
  if (marker_type == LatencyMarkerType::PRESENT_END)
    profiler_tracker::record_value("Latency", "Present", static_cast<float>(::get_time_usec(present_start_stamp)) / 1000.f, 0);
  nvlowlatency::set_marker(frame_id, marker_type);
}

lowlatency::LatencyData lowlatency::get_statistics_since(uint32_t frame_id, uint32_t max_count)
{
  last_queried_frame_id = frame_id;
  // A platform independent version can be implemented here if nvlowlatency is not available
  return nvlowlatency::get_statistics_since(frame_id, max_count);
}

lowlatency::LatencyData lowlatency::get_last_statistics() { return get_statistics_since(0, 1); }

lowlatency::LatencyData lowlatency::get_new_statistics(uint32_t max_count)
{
  return get_statistics_since(last_queried_frame_id, max_count);
}

lowlatency::ScopedLatencyMarker::~ScopedLatencyMarker() { close(); }

void lowlatency::ScopedLatencyMarker::close()
{
  if (!closed)
  {
    closed = true;
    set_marker(frameId, endMarker);
  }
}

void lowlatency::register_slop(uint32_t frame, int duration_usec) { slop_histroy[frame % SLOP_HISTORY_SIZE] += duration_usec; }

lowlatency::Timer::Timer() : stamp(::ref_time_ticks()) {}

int lowlatency::Timer::timeUSec() const { return ::get_time_usec(stamp); }

static void low_latency_after_reset(bool) { lowlatency::set_latency_mode(latency_mode, minimum_interval_ms); }

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
          mode = lowlatency::LATENCY_MODE_NV_ON;
          console::print_d("Latency mode ON");
          break;
        case 2:
          mode = lowlatency::LATENCY_MODE_NV_BOOST;
          console::print_d("Latency mode BOOST");
          break;
        case 3:
          mode = lowlatency::LATENCY_MODE_EXPERIMENTAL;
          console::print_d("Latency mode EXPERIMENTAL");
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
    console::print("   mode: 0 -> Off, 1 -> On, 2 -> Boost, 3 -> Experimental");
    console::print("   min_frame_time_in_ms: 0 for no limit");
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(low_latency_console_handler);
