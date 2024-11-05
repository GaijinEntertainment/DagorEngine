// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_nvLowLatency.h>

#include <3d/dag_nvFeatures.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_commands.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include "nvLowLatency.h"

#include <debug/dag_debug.h>

#include <d3d11.h>
#include <nvapi.h>

static IUnknown *dx_device = nullptr;
static bool deferred_submission = false;
static bool latency_indicator_enabled = false;
static bool initialized = false;


bool nvlowlatency::feed_latency_input(uint32_t frame_id, unsigned int msg)
{
  G_UNUSED(frame_id);
  G_UNUSED(msg);
  return false;
}

void nvlowlatency::init()
{
  nv::Reflex *reflex{};
  d3d::driver_command(Drv3dCommand::GET_REFLEX, &reflex);
  if (reflex && reflex->isReflexSupported() == nv::SupportState::Supported)
  {
    initialized = true;
    reflex->initializeReflexState();
  }
  latency_indicator_enabled = false;
}

void nvlowlatency::close()
{
  set_latency_mode(lowlatency::LatencyMode::LATENCY_MODE_OFF, 0.f);
  initialized = false;
}

void nvlowlatency::start_frame(uint32_t frame_id)
{
  nv::Reflex *reflex{};
  d3d::driver_command(Drv3dCommand::GET_REFLEX, &reflex);
  if (reflex)
    reflex->startFrame(frame_id);
}

bool nvlowlatency::is_available()
{
  if (!initialized)
    return false;

  nv::Reflex *reflex{};
  d3d::driver_command(Drv3dCommand::GET_REFLEX, &reflex);
  if (!reflex)
    return false;
  auto state = reflex->getReflexState();
  if (!state)
    return false;
  return state->lowLatencyAvailable;
}

void nvlowlatency::set_latency_mode(lowlatency::LatencyMode mode, float min_interval_ms)
{
  if (!initialized)
    return;

  nv::Reflex *reflex{};
  d3d::driver_command(Drv3dCommand::GET_REFLEX, &reflex);
  if (reflex)
  {
    nv::Reflex::ReflexMode reflexMode = nv::Reflex::ReflexMode::Off;
    switch (mode)
    {
      case lowlatency::LATENCY_MODE_OFF: reflexMode = nv::Reflex::ReflexMode::Off; break;
      case lowlatency::LATENCY_MODE_NV_ON: reflexMode = nv::Reflex::ReflexMode::On; break;
      case lowlatency::LATENCY_MODE_NV_BOOST:
      case lowlatency::LATENCY_MODE_EXPERIMENTAL: reflexMode = nv::Reflex::ReflexMode::OnPlusBoost; break;
    }
    auto minimumIntervalUs = static_cast<uint32_t>(min_interval_ms * 1000.f);
    debug("[Reflex] Set latency mode: latency mode: %d, interval: %dus", eastl::to_underlying(reflexMode), minimumIntervalUs);
    bool success = reflex->setReflexOptions(reflexMode, minimumIntervalUs);
    static bool firstError = true;
    if (!success && firstError)
    {
      logerr("Reflex: setReflexOptions has failed");
      firstError = false;
    }
  }
}

void nvlowlatency::sleep()
{
  if (!initialized)
    return;

  nv::Reflex *reflex{};
  d3d::driver_command(Drv3dCommand::GET_REFLEX, &reflex);
  if (reflex)
  {
    TIME_D3D_PROFILE(nv_low_latency_sleep);
    bool success = reflex->sleep();
    G_ASSERT(success);

    static bool firstError = true;
    if (!success && firstError)
    {
      logerr("Reflex: sleep has failed");
      firstError = false;
    }
  }
}

void nvlowlatency::set_marker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type)
{
  if (!initialized)
    return;

  if (marker_type == lowlatency::LatencyMarkerType::TRIGGER_FLASH && !latency_indicator_enabled)
    return;

  nv::Reflex *reflex{};
  d3d::driver_command(Drv3dCommand::GET_REFLEX, &reflex);
  if (reflex)
  {
    bool success = reflex->setMarker(frame_id, marker_type);
    static bool firstError = true;
    if (!success && firstError)
    {
      logerr("Reflex: setMarker has failed with frame_id: %d, and marker: %d", frame_id, static_cast<uint32_t>(marker_type));
      firstError = false;
    }
  }
}

static bool is_report_valid(const nv::Reflex::ReflexStats &report)
{
  uint64_t maxDifference = 16667 * 100; // 100 frames
  const uint64_t startTime = report.simStartTime;
  if (report.simEndTime < startTime || report.simEndTime - startTime > maxDifference)
    return false;
  if (report.renderSubmitStartTime < startTime || report.renderSubmitStartTime - startTime > maxDifference)
    return false;
  if (report.renderSubmitEndTime < startTime || report.renderSubmitEndTime - startTime > maxDifference)
    return false;
  if (report.driverStartTime < startTime || report.driverStartTime - startTime > maxDifference)
    return false;
  if (report.driverEndTime < startTime || report.driverEndTime - startTime > maxDifference)
    return false;
  if (report.osRenderQueueStartTime < startTime || report.osRenderQueueStartTime - startTime > maxDifference)
    return false;
  if (report.osRenderQueueEndTime < startTime || report.osRenderQueueEndTime - startTime > maxDifference)
    return false;
  if (report.gpuRenderStartTime < startTime || report.gpuRenderStartTime - startTime > maxDifference)
    return false;
  if (report.gpuRenderEndTime < startTime || report.gpuRenderEndTime - startTime > maxDifference)
    return false;
  return true;
}

lowlatency::LatencyData nvlowlatency::get_statistics_since(uint32_t frame_id, uint32_t max_count)
{
  lowlatency::LatencyData ret;
  if (!initialized)
    return ret;

  nv::Reflex *reflex{};
  d3d::driver_command(Drv3dCommand::GET_REFLEX, &reflex);
  if (reflex)
  {
    auto state = reflex->getReflexState();
    if (!state)
      return ret; // frameCount = 0
    if (max_count == 0 || max_count > nv::Reflex::ReflexState::FRAME_COUNT)
      max_count = nv::Reflex::ReflexState::FRAME_COUNT;

    for (uint32_t i = nv::Reflex::ReflexState::FRAME_COUNT;
         i > nv::Reflex::ReflexState::FRAME_COUNT - max_count && state->stats[i - 1].frameID > frame_id; --i)
    {
      const nv::Reflex::ReflexStats &report = state->stats[i - 1];
      if (!is_report_valid(report))
        continue;
      const NvU64 startTime = report.simStartTime;
      const float gpuRenderEndTime = (report.gpuRenderEndTime - startTime) / 1000.f;
      const float renderSubmitEndTime = (report.renderSubmitEndTime - startTime) / 1000.f;
      const float osRenderQueueStartTime = (report.osRenderQueueStartTime - startTime) / 1000.f;

      ret.gameToRenderLatency.apply(gpuRenderEndTime);
      ret.gameLatency.apply(renderSubmitEndTime);
      ret.renderLatency.apply(gpuRenderEndTime - osRenderQueueStartTime);

      ret.inputSampleTime.apply((report.inputSampleTime - startTime) / 1000.f);
      ret.simEndTime.apply((report.simEndTime - startTime) / 1000.f);
      ret.renderSubmitStartTime.apply((report.renderSubmitStartTime - startTime) / 1000.f);
      ret.renderSubmitEndTime.apply(renderSubmitEndTime);
      ret.presentStartTime.apply((report.presentStartTime - startTime) / 1000.f);
      ret.presentEndTime.apply((report.presentEndTime - startTime) / 1000.f);
      ret.driverStartTime.apply((report.driverStartTime - startTime) / 1000.f);
      ret.driverEndTime.apply((report.driverEndTime - startTime) / 1000.f);
      ret.osRenderQueueStartTime.apply(osRenderQueueStartTime);
      ret.osRenderQueueEndTime.apply((report.osRenderQueueEndTime - startTime) / 1000.f);
      ret.gpuRenderStartTime.apply((report.gpuRenderStartTime - startTime) / 1000.f);
      ret.gpuRenderEndTime.apply(gpuRenderEndTime);
      ret.frameCount++;
    }
    ret.gameToRenderLatency.finalize(ret.frameCount);
    ret.gameLatency.finalize(ret.frameCount);
    ret.renderLatency.finalize(ret.frameCount);
    ret.inputSampleTime.finalize(ret.frameCount);
    ret.simEndTime.finalize(ret.frameCount);
    ret.renderSubmitStartTime.finalize(ret.frameCount);
    ret.renderSubmitEndTime.finalize(ret.frameCount);
    ret.presentStartTime.finalize(ret.frameCount);
    ret.presentEndTime.finalize(ret.frameCount);
    ret.driverStartTime.finalize(ret.frameCount);
    ret.driverEndTime.finalize(ret.frameCount);
    ret.osRenderQueueStartTime.finalize(ret.frameCount);
    ret.osRenderQueueEndTime.finalize(ret.frameCount);
    ret.gpuRenderStartTime.finalize(ret.frameCount);
    ret.gpuRenderEndTime.finalize(ret.frameCount);
  }

  return ret;
}
