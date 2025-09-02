// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_nvFeatures.h>
#include <drv/3d/dag_commands.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_debug.h>
#include <nvapi.h>
#include <3d/gpuLatency.h>
#include <dag/dag_vector.h>

template <int id, typename... Args>
static bool CheckOnce(bool cond, const char *msg, const Args &...args)
{
  if (cond)
    return true;

  static bool firstError = true;
  if (firstError)
  {
    logerr(msg, args...);
    firstError = false;
  }
  return false;
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

class StreamlineGpuLatency : public GpuLatency
{
public:
  StreamlineGpuLatency()
  {
    if (auto sl = getStreamline())
    {
      modes = {Mode::Off};
      if (sl->isReflexSupported() == nv::SupportState::Supported)
        reflex = sl->createReflexFeature();

      if (reflex)
        if (auto state = reflex->getState(); state.has_value() && state->lowLatencyAvailable)
          modes = {Mode::Off, Mode::On, Mode::OnPlusBoost};

      latencyIndicatorEnabled = false;
    }
  }

  ~StreamlineGpuLatency()
  {
    if (reflex)
    {
      setOptions(Mode::Off, 0);

      if (auto sl = getStreamline())
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

    if (marker_type == lowlatency::LatencyMarkerType::TRIGGER_FLASH && !latencyIndicatorEnabled)
      return;

    CheckOnce<__LINE__>(reflex->setMarker(frame_id, marker_type), "Reflex: setMarker has failed with frame_id: %d, and marker: %d",
      frame_id, uint32_t(marker_type));
  }

  void setOptions(Mode mode, uint32_t frame_limit_us) override
  {
    if (reflex)
    {
      debug("[Reflex] Set latency mode: latency mode: %d, interval: %dus", eastl::to_underlying(mode), frame_limit_us);

      bool success = false;
      switch (mode)
      {
        case Mode::Off: success = reflex->setOptions(nv::Reflex::ReflexMode::Off, frame_limit_us); break;
        case Mode::On: success = reflex->setOptions(nv::Reflex::ReflexMode::On, frame_limit_us); break;
        case Mode::OnPlusBoost: success = reflex->setOptions(nv::Reflex::ReflexMode::OnPlusBoost, frame_limit_us); break;
      }

      CheckOnce<__LINE__>(success, "[Reflex] setReflexOptions has failed");
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

    return ret;
  }

  void *getHandle() const override { return nullptr; }

  bool isEnabled() const override
  {
    if (!reflex)
      return false;
    return reflex->getCurrentMode() != nv::Reflex::ReflexMode::Off;
  }

  GpuVendor getVendor() const override { return GpuVendor::NVIDIA; }

  bool isVsyncAllowed() const override { return !reflex || reflex->getCurrentMode() == nv::Reflex::ReflexMode::Off; }

private:
  static nv::Streamline *getStreamline()
  {
    nv::Streamline *sl = nullptr;
    d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &sl);
    return sl;
  }

  nv::Reflex *reflex = nullptr;
  dag::Vector<Mode> modes = {Mode::Off};
  bool latencyIndicatorEnabled = false;
};

GpuLatency *create_gpu_latency_nvidia() { return new StreamlineGpuLatency(); }
