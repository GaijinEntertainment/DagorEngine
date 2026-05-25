// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "streamline_adapter.h"
#include "drv_log_defs.h"
#include <3d/gpuLatency.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_info.h>
#include <generic/dag_staticTab.h>
#include <perfMon/dag_statDrv.h>


template <int id, typename... Args>
static bool CheckOnce(bool cond, const char *msg, const Args &...args)
{
  if (cond) [[likely]]
    return true;

  static bool firstError = true;
  if (firstError)
  {
    D3D_ERROR(msg, args...);
    firstError = false;
  }
  return false;
}

static bool is_report_valid(const Reflex::Stats &report)
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

class StreamlineGpuLatency : public GpuLatency
{
public:
  StreamlineGpuLatency()
  {
    if (StreamlineAdapter * sl; d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &sl))
    {
      modes = {Mode::Off};
      if (sl->isReflexSupported() == nv::SupportState::Supported)
        reflex = sl->createReflexFeature();

      if (reflex)
        if (auto state = reflex->getState(); state.has_value() && state->lowLatencyAvailable)
          modes = {Mode::Off, Mode::On, Mode::OnPlusBoost};
    }
  }

  ~StreamlineGpuLatency()
  {
    if (reflex)
    {
      setOptions(Mode::Off, 0);

      if (StreamlineAdapter * sl; d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &sl))
      {
        reflex = nullptr;
        sl->releaseReflexFeature();
        modes = {Mode::Off};
      }
    }
  }

  dag::ConstSpan<Mode> getAvailableModes() const override { return make_span_const(modes); }

  void startFrame(uint32_t frame_id) override
  {
    if (reflex)
      reflex->startFrame(frame_id);
  }

  void setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) override
  {
    if (!reflex)
      return;

    CheckOnce<__LINE__>(reflex->setMarker(frame_id, marker_type), "Reflex: setMarker has failed with frame_id: %d, and marker: %d",
      frame_id, uint32_t(marker_type));
  }

  void setOptions(Mode mode, uint32_t frame_limit_us) override
  {
    if (reflex)
    {
      debug("[Reflex] Set latency mode: latency mode: %d, interval: %dus", eastl::to_underlying(mode), frame_limit_us);
      CheckOnce<__LINE__>(reflex->setOptions(mode, frame_limit_us), "[Reflex] setReflexOptions has failed");
    }
  }

  void sleep(uint32_t frame_id) override
  {
    if (!reflex)
      return;

    TIME_PROFILE(nv_low_latency_sleep);
    CheckOnce<__LINE__>(reflex->sleep(frame_id), "[Reflex] sleep has failed");
  }

  lowlatency::LatencyData getStatisticsSince(uint32_t frame_id, uint32_t max_count = 0) override
  {
    lowlatency::LatencyData ret;

    if (!reflex)
      return ret;

    auto state = reflex->getState();
    if (!state)
      return ret; // frameCount = 0

    if (max_count == 0 || max_count > Reflex::State::FRAME_COUNT)
      max_count = Reflex::State::FRAME_COUNT;

    // Compute a frame index offset to correct Streamline's GPU work misattribution.
    // Find it once using the latest valid report, then apply to all frames.
    int32_t gpuFrameOffset = 0;
    if (d3d::get_driver_code().is(d3d::vulkan))
    {
      const uint32_t probeIdx = Reflex::State::FRAME_COUNT - 1;
      const Reflex::Stats &probe = state->stats[probeIdx];
      if (is_report_valid(probe) && probe.renderSubmitEndTime > 0)
      {
        uint64_t bestDist = UINT64_MAX;
        for (int32_t j = 0; j < (int32_t)Reflex::State::FRAME_COUNT; ++j)
        {
          const auto &candidate = state->stats[j];
          if (candidate.gpuRenderStartTime == 0 || candidate.simStartTime == 0)
            continue;
          if (probe.renderSubmitEndTime > candidate.gpuRenderStartTime)
            continue;
          uint64_t dist = candidate.gpuRenderStartTime - probe.renderSubmitEndTime;
          if (bestDist > dist)
          {
            bestDist = dist;
            gpuFrameOffset = j - (int32_t)probeIdx;
          }
        }
      }
    }

    for (uint32_t i = Reflex::State::FRAME_COUNT; i > Reflex::State::FRAME_COUNT - max_count && state->stats[i - 1].frameID > frame_id;
         --i)
    {
      const Reflex::Stats &report = state->stats[i - 1];
      if (!is_report_valid(report))
        continue;
      const uint64_t startTime = report.simStartTime;

      // Use GPU timings from the report offset by the cached amount
      int32_t gpuIdx = (int32_t)(i - 1) + gpuFrameOffset;
      const Reflex::Stats *gpuSource = &report;
      if (gpuIdx >= 0 && gpuIdx < (int32_t)Reflex::State::FRAME_COUNT && state->stats[gpuIdx].gpuRenderEndTime != 0)
        gpuSource = &state->stats[gpuIdx];

      const float gpuRenderEndTime = (gpuSource->gpuRenderEndTime - startTime) / 1000.f;
      const float renderSubmitEndTime = (report.renderSubmitEndTime - startTime) / 1000.f;
      const float osRenderQueueStartTime = (gpuSource->osRenderQueueStartTime - startTime) / 1000.f;

      ret.latestFrameId = eastl::max(ret.latestFrameId, static_cast<uint32_t>(report.frameID));

      ret.gameToRenderLatency.apply(gpuRenderEndTime);
      ret.gameLatency.apply(renderSubmitEndTime);
      ret.renderLatency.apply(gpuRenderEndTime - osRenderQueueStartTime);

      ret.inputSampleTime.apply((report.inputSampleTime - startTime) / 1000.f);
      ret.simEndTime.apply((report.simEndTime - startTime) / 1000.f);
      ret.renderSubmitStartTime.apply((report.renderSubmitStartTime - startTime) / 1000.f);
      ret.renderSubmitEndTime.apply(renderSubmitEndTime);
      ret.presentStartTime.apply((report.presentStartTime - startTime) / 1000.f);
      ret.presentEndTime.apply((report.presentEndTime - startTime) / 1000.f);
      ret.driverStartTime.apply((gpuSource->driverStartTime - startTime) / 1000.f);
      ret.driverEndTime.apply((gpuSource->driverEndTime - startTime) / 1000.f);
      ret.osRenderQueueStartTime.apply(osRenderQueueStartTime);
      ret.osRenderQueueEndTime.apply((gpuSource->osRenderQueueEndTime - startTime) / 1000.f);
      ret.gpuRenderStartTime.apply((gpuSource->gpuRenderStartTime - startTime) / 1000.f);
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

    return ret;
  }

  void *getHandle() const override { return nullptr; }

  bool isEnabled() const override
  {
    if (!reflex)
      return false;
    return reflex->getMode() != Mode::Off;
  }

  GpuVendor getVendor() const override { return GpuVendor::NVIDIA; }

  bool isVsyncAllowed() const override { return !reflex || reflex->getMode() == Mode::Off; }

private:
  Reflex *reflex = nullptr;
  StaticTab<Mode, 3> modes = {Mode::Off};
};

GpuLatency *create_gpu_latency_nvidia() { return new StreamlineGpuLatency(); }
