// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_nvFeatures.h>
#include <3d/gpuLatency.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_info.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_statDrv.h>
#include <dag/dag_vector.h>

#include <d3d11.h>
#include <d3d12.h>

#include <ffx_antilag2_dx11.h>
#include <ffx_antilag2_dx12.h>

class AMDGpuLatency : public GpuLatency
{
public:
  AMDGpuLatency()
  {
    G_ASSERT(d3d::get_driver_code().is(d3d::dx11 || d3d::dx12));

    auto hr = d3d::get_driver_code().is(d3d::dx11)
                ? AMD::AntiLag2DX11::Initialize(&context11)
                : AMD::AntiLag2DX12::Initialize(&context12, static_cast<ID3D12Device *>(d3d::get_device()));
    if (SUCCEEDED(hr))
      modes = {Mode::Off, Mode::On};
    else
    {
      logdbg("AMD AntiLag2 initialization failed: %l", hr);
      modes = {Mode::Off};
    }
  }

  ~AMDGpuLatency()
  {
    if (modes.size() == 1)
      return;

    if (d3d::get_driver_code().is(d3d::dx11))
    {
      G_VERIFY(AMD::AntiLag2DX11::DeInitialize(&context11) == 0);
      context11 = AMD::AntiLag2DX11::Context{};
    }
    else
    {
      G_VERIFY(AMD::AntiLag2DX12::DeInitialize(&context12) == 0);
      context12 = AMD::AntiLag2DX12::Context{};
    }
  }

  dag::ConstSpan<Mode> getAvailableModes() const override { return make_span_const(modes); }

  void startFrame(uint32_t) override {}

  void setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) override
  {
    if (modes.size() == 1)
      return;

    if (d3d::get_driver_code().is(d3d::dx12) && marker_type == lowlatency::LatencyMarkerType::RENDERSUBMIT_END)
      G_VERIFY(SUCCEEDED(AMD::AntiLag2DX12::MarkEndOfFrameRendering(&context12)));
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
      default: logwarn("Unknown AMD latency mode"); break;
    }
  }

  void sleep(uint32_t) override
  {
    if (modes.size() == 1)
      return;

    TIME_D3D_PROFILE(nv_low_latency_sleep);
    if (d3d::get_driver_code().is(d3d::dx11))
      G_VERIFY(SUCCEEDED(AMD::AntiLag2DX11::Update(&context11, isActive, maxFps)));
    else
      G_VERIFY(SUCCEEDED(AMD::AntiLag2DX12::Update(&context12, isActive, maxFps)));
  }

  lowlatency::LatencyData getStatisticsSince(uint32_t frame_id, uint32_t max_count = 0) override
  {
    // Anti Lag 2 has no support for this
    G_UNUSED(frame_id);
    G_UNUSED(max_count);
    lowlatency::LatencyData ret;
    return ret;
  }

  void *getHandle() const override { return d3d::get_driver_code().is(d3d::dx11) ? (void *)&context11 : (void *)&context12; }

  bool isEnabled() const override { return isActive; }

  GpuVendor getVendor() const override { return GpuVendor::AMD; }

  bool isVsyncAllowed() const override { return true; }

private:
  dag::Vector<Mode> modes = {Mode::Off};
  AMD::AntiLag2DX11::Context context11 = {};
  AMD::AntiLag2DX12::Context context12 = {};
  bool isActive = false;
  int maxFps = 0;
};

GpuLatency *create_gpu_latency_amd() { return new AMDGpuLatency(); }
