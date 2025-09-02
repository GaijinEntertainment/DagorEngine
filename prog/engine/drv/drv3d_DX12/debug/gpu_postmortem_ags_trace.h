// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include "configuration.h"
#include <driver.h>

#include "amd_ags_init.h"
#include <EASTL/span.h>


namespace drv3d_dx12
{
class BasePipeline;
class ComputePipeline;
class PipelineVariant;
struct BufferResourceReferenceAndOffset;
struct Direct3D12Enviroment;
struct PipelineStageStateBase;
namespace debug
{
union Configuration;
namespace gpu_postmortem::ags
{
class AgsTrace
{
public:
  // Have to delete move constructor, otherwise compiler / templated stuff of variant tries to be smart and results in compile errors.
  AgsTrace(AgsTrace &&) = delete;
  AgsTrace &operator=(AgsTrace &&) = delete;
  AgsTrace(AGSContext *ags_context) : agsContext{ags_context} {}
  ~AgsTrace();

  void configure() { logdbg("DX12: ...no GPU postmortem tracer active..."); }
  constexpr void beginCommandBuffer(D3DDevice *, D3DGraphicsCommandList *) {}
  constexpr void endCommandBuffer(D3DGraphicsCommandList *) {}
  void beginEvent(D3DGraphicsCommandList *, eastl::span<const char>, const eastl::string &);
  void endEvent(D3DGraphicsCommandList *, const eastl::string &);
  constexpr void marker(D3DGraphicsCommandList *, eastl::span<const char>) {}
  void onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &);
  constexpr bool sendGPUCrashDump(const char *, const void *, uintptr_t) { return false; }
  constexpr void onDeviceShutdown() {}
  template <typename... Ts>
  constexpr bool onDeviceSetup(Ts &&...)
  {
    return true;
  }
  template <typename... Ts>
  auto onDeviceCreated(Ts &&...ts)
  {
    return onDeviceSetup(eastl::forward<Ts>(ts)...);
  }

  template <typename T>
  static bool load(const Configuration &config, const Direct3D12Enviroment &, T &&target)
  {
    if (!config.enableAgsTrace)
    {
      logdbg("DX12: Ags Trace is disabled by configuration...");
      return false;
    }

    AGSContext *agsContext = debug::ags::get_context();
    if (DAGOR_UNLIKELY(!agsContext))
      return false;

    target.template emplace<AgsTrace>(agsContext);
    logdbg("DX12: ...using AMD GPU Services API for GPU postmortem trace");
    return true;
  }

  template <typename T, typename K>
  static bool load(const Configuration &config, const Direct3D12Enviroment &e, const K &, T &&target)
  {
    return load(config, e, target);
  }

  bool tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr,
    HLSLVendorExtensions &extensions);

  void nameResource(ID3D12Resource *, eastl::string_view) {}
  void nameResource(ID3D12Resource *, eastl::wstring_view) {}

private:
  AGSContext *agsContext = nullptr;
  bool deviceInitialized = false;
};
} // namespace gpu_postmortem::ags
} // namespace debug
} // namespace drv3d_dx12
