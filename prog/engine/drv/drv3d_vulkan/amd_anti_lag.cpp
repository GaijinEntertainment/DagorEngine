// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amd_anti_lag.h"
#include "globals.h"
#include "vulkan_device.h"
#include "physical_device_set.h"

using namespace drv3d_vulkan;

class AMDAntiLagGpuLatency : public GpuLatency
{
public:
  AMDAntiLagGpuLatency()
  {
    if (Globals::VK::phy.hasAMDAntiLag)
      modes = {Mode::Off, Mode::On};
    else
    {
      debug("vulkan: No AMD AntiLag support detected.");
      modes = {Mode::Off};
    }
  }

  ~AMDAntiLagGpuLatency() {}

  dag::ConstSpan<Mode> getAvailableModes() const override { return make_span_const(modes); }

  void startFrame(uint32_t) override {}

  void setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) override
  {
    if (modes.size() == 1)
      return;

    if (marker_type != lowlatency::LatencyMarkerType::PRESENT_START)
      return;

#if VK_AMD_anti_lag
    TIME_D3D_PROFILE(amd_anti_lag_present);
    VkAntiLagPresentationInfoAMD presentData{
      VK_STRUCTURE_TYPE_ANTI_LAG_PRESENTATION_INFO_AMD, nullptr, VK_ANTI_LAG_STAGE_PRESENT_AMD, frame_id};
    VkAntiLagDataAMD updData{VK_STRUCTURE_TYPE_ANTI_LAG_DATA_AMD, nullptr, VK_ANTI_LAG_MODE_ON_AMD, maxFps, &presentData};
    Globals::VK::dev.vkAntiLagUpdateAMD(Globals::VK::dev.get(), &updData);
#else
    G_UNUSED(frame_id);
#endif
  }

  void setOptions(Mode mode, uint32_t frame_limit_us) override
  {
    if (modes.size() == 1)
      return;

    maxFps = frame_limit_us ? round(1.0f / frame_limit_us) : 0;

    switch (mode)
    {
      case Mode::Off: isActive = false; break;
      case Mode::On: isActive = true; break;
      default: debug("vulkan: Unknown AMD latency mode"); break;
    }
  }

  void sleep(uint32_t frame_id) override
  {
    if (modes.size() == 1)
      return;

#if VK_AMD_anti_lag
    TIME_D3D_PROFILE(amd_anti_lag_input);
    VkAntiLagPresentationInfoAMD presentData{
      VK_STRUCTURE_TYPE_ANTI_LAG_PRESENTATION_INFO_AMD, nullptr, VK_ANTI_LAG_STAGE_INPUT_AMD, frame_id};
    VkAntiLagDataAMD updData{VK_STRUCTURE_TYPE_ANTI_LAG_DATA_AMD, nullptr, VK_ANTI_LAG_MODE_ON_AMD, maxFps, &presentData};
    Globals::VK::dev.vkAntiLagUpdateAMD(Globals::VK::dev.get(), &updData);
#else
    G_UNUSED(frame_id);
#endif
  }

  lowlatency::LatencyData getStatisticsSince(uint32_t frame_id, uint32_t max_count = 0) override
  {
    // no support for this
    G_UNUSED(frame_id);
    G_UNUSED(max_count);
    lowlatency::LatencyData ret;
    return ret;
  }

  void *getHandle() const override { return nullptr; }

  bool isEnabled() const override { return isActive; }

  GpuVendor getVendor() const override { return GpuVendor::AMD; }

  bool isVsyncAllowed() const override { return true; }

private:
  dag::Vector<Mode> modes = {Mode::Off};
  bool isActive = false;
  uint32_t maxFps = 0;
};

GpuLatency *drv3d_vulkan::create_gpu_latency_amd_anti_lag() { return new AMDAntiLagGpuLatency(); }
