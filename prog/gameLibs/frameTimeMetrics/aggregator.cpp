// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameTimeMetrics/aggregator.h"
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_lowLatency.h>
#include <util/dag_localization.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_commands.h>

#if _TARGET_XBOX
#include "aggregator_xbox.h"
#endif

CONSOLE_INT_VAL("fps", update_rate_msec, 1000, 100, 100000);

static const uint32_t MAX_FPS_TO_DISPLAY = 9999;

void FrameTimeMetricsAggregator::update(const float current_time_msec, const uint32_t frame_no, const float dt,
  PerfDisplayMode display_mode)
{
  if (update_rate_msec.pullValueChange())
    displayMode = PerfDisplayMode::OFF; // To reset counter
  minFrameTime = min(dt, minFrameTime);
  maxFrameTime = max(dt, maxFrameTime);
  if (int(current_time_msec) > (lastPeriod + 1) * update_rate_msec.get() || display_mode != displayMode)
  {
    displayMode = display_mode;
    const uint64_t newTime = current_time_msec;

    if (!isPixCapturerLoaded.has_value())
      isPixCapturerLoaded = (bool)d3d::driver_command(Drv3dCommand::IS_PIX_CAPTURE_LOADED);

    lastAverageFrameTime = (float)(newTime - lastTimeMsec) / (frame_no - lastFrameNo + 0.001f);

    lastAverageFps = 1000.f * (frame_no - lastFrameNo) / ((float)(newTime - lastTimeMsec) + 0.001f);

    if (lastAverageFps > MAX_FPS_TO_DISPLAY)
      lastAverageFps = 0;

    lastTimeMsec = newTime;
    lastPeriod = newTime / update_rate_msec.get();
    lastFrameNo = frame_no;

    lastMaxFrameTime = 1000.f * maxFrameTime;
    lastMinFrameTime = 1000.f * minFrameTime;
    maxFrameTime = 0.f;
    minFrameTime = 1000.f;

    lowlatency::LatencyData latencyData;
    if (display_mode != PerfDisplayMode::OFF && display_mode != PerfDisplayMode::FPS)
    {
      latencyData = lowlatency::get_statistics_since(lastLatencyFrame);
      lastLatencyFrame = lowlatency::get_current_render_frame();
      lastAverageLatency = latencyData.gameToRenderLatency.avg;
      lastAverageLatencyA = latencyData.gameLatency.avg;
      lastAverageLatencyR = latencyData.renderLatency.avg;
    }
    else
    {
      lastAverageLatency = 0;
      lastAverageLatencyA = 0;
      lastAverageLatencyR = 0;
    }
    if (displayMode == PerfDisplayMode::OFF)
      fpsText.clear();
    else if (displayMode == PerfDisplayMode::COMPACT)
      fpsText.sprintf("FPS:%5.1f", lastAverageFps);
    else
      fpsText.sprintf("%s%s FPS:%5.1f (%4.1f<%4.1f %4.1f)", d3d::get_driver_name(), isPixCapturerLoaded.value() ? "(PIX)" : "",
        lastAverageFps, lastMinFrameTime, lastMaxFrameTime, lastAverageFrameTime);
    switch (display_mode)
    {
      case PerfDisplayMode::OFF:
      case PerfDisplayMode::FPS: latencyText.clear(); break;
      case PerfDisplayMode::COMPACT: latencyText.sprintf("%s: %5.1fms", "Latency", latencyData.gameToRenderLatency.avg); break;
      case PerfDisplayMode::FULL:
        latencyText.sprintf("%s:%5.1fms (A:%5.1f R:%5.1f)", "Latency", latencyData.gameToRenderLatency.avg,
          latencyData.gameLatency.avg, latencyData.renderLatency.avg);
        break;
    }

    if (renderingResolution.has_value())
      fpsText += String(-1, " %dx%d", renderingResolution->x, renderingResolution->y);

#if _TARGET_XBOX
    size_t available = 0;
    if (xbox_get_available_memory(available))
    {
      size_t availableMb = available >> 20;
      fpsText += String(-1, " %uMB", availableMb);
      achtung = availableMb < 500;
    }
#endif

    textVersion++;
  }
}

uint32_t FrameTimeMetricsAggregator::getColorForFpsInfo() const
{
  if (lastMaxFrameTime >= 100.f || achtung)
    return QualityColors::POOR;

  if (lastAverageFps >= 59.9f)
    return QualityColors::EPIC;
  else if (lastAverageFps >= 29.9f)
    return QualityColors::GOOD;
  else if (lastAverageFps >= 20.f)
    return QualityColors::OKAY;
  else
    return QualityColors::POOR;
}
