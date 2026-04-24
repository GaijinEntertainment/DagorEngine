// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/configuration.h>
#include <debug/amd_ags_init.h>
#include <driver.h>

#include <debug/dag_log.h>

#include <EASTL/span.h>
#include <EASTL/string_view.h>


namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug::gpu_capture
{
struct Issues;
namespace amd
{
class RadeonGPUProfiler
{
  RadeonGPUProfiler(AGSContext *ags_context) : agsContext(ags_context) {}

public:
  void configure();
  void beginCapture(const wchar_t *);
  void endCapture();
  void onPresent();
  void captureFrames(const wchar_t *, int);
  void beginEvent(D3DGraphicsCommandList *, eastl::span<const char>);
  void endEvent(D3DGraphicsCommandList *);
  void marker(D3DGraphicsCommandList *, eastl::span<const char>);
  bool tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr,
    HLSLVendorExtensions &extensions);
  void nameResource(ID3D12Resource *resource, eastl::string_view name);
  void nameResource(ID3D12Resource *resource, eastl::wstring_view name);
  void nameObject(ID3D12Object *object, eastl::string_view name);
  void nameObject(ID3D12Object *object, eastl::wstring_view name);

  template <typename T>
  static bool connect(const Configuration &config, Direct3D12Enviroment &, Issues &, T &target)
  {
    if (!config.enableAgsProfile)
    {
      logdbg("DX12: Ags Profiler is disabled by configuration...");
      return false;
    }

    logdbg("DX12: Looking for AGS...");
    if (!try_connect_interface())
    {
      logdbg("DX12: ...nothing found");
      return false;
    }
    logdbg("DX12: ...found");

    AGSContext *agsContext = debug::ags::get_context();
    if (EASTL_UNLIKELY(!agsContext))
      return false;

    target = RadeonGPUProfiler{agsContext};
    logdbg("DX12: Initialized profiler with AMD GPU Services API");
    return true;
  }

private:
  AGSContext *agsContext = nullptr;
  bool deviceInitialized = false;

  static bool try_connect_interface();
};
} // namespace amd
} // namespace debug::gpu_capture
} // namespace drv3d_dx12
