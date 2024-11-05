// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>

#include "call_stack.h"
#include "configuration.h"
#include "pipeline.h"

#include <ags_sdk/include/amd_ags.h>


namespace drv3d_dx12
{
struct Direct3D12Enviroment;
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
  void beginCommandBuffer(ID3D12Device3 *, ID3D12GraphicsCommandList *) {}
  void endCommandBuffer(ID3D12GraphicsCommandList *) {}
  void beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>, const eastl::string &);
  void endEvent(ID3D12GraphicsCommandList *, const eastl::string &);
  void marker(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
  void draw(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}
  void drawIndexed(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, int32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}
  void drawIndirect(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, BufferResourceReferenceAndOffset)
  {}
  void drawIndexedIndirect(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, BufferResourceReferenceAndOffset)
  {}
  void dispatchIndirect(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    ComputePipeline &, BufferResourceReferenceAndOffset)
  {}
  void dispatch(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &, ComputePipeline &,
    uint32_t, uint32_t, uint32_t)
  {}
  void dispatchMesh(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t)
  {}
  void dispatchMeshIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, BufferResourceReferenceAndOffset,
    BufferResourceReferenceAndOffset, uint32_t)
  {}
  void blit(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *) {}
  void onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &);
  bool sendGPUCrashDump(const char *, const void *, uintptr_t) { return false; }
  void onDeviceShutdown() {}
  bool onDeviceSetup(ID3D12Device *, const Configuration &, const Direct3D12Enviroment &) { return true; }

  template <typename T>
  static bool load(const Configuration &config, const Direct3D12Enviroment &, T &target)
  {
    if (!config.enableAgsTrace)
    {
      logdbg("DX12: Ags Trace is disabled by configuration...");
      return false;
    }

    AGSContext *agsContext = nullptr;
    AGSGPUInfo gpuInfo = {};
    AGSConfiguration agsConfig = {};

    logdbg("DX12: Ags initialization...");
    const auto agsResult = agsInit(&agsContext, &agsConfig, &gpuInfo);
    if (agsResult != AGS_SUCCESS)
    {
      logwarn("DX12: ...failed (%d)", agsResult);
      return false;
    }

    target.template emplace<AgsTrace>(agsContext);
    logdbg("DX12: ...using AMD GPU Services API for GPU postmortem trace");
    return true;
  }

  bool tryCreateDevice(IUnknown *adapter, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr);

private:
  AGSContext *agsContext = nullptr;
};
} // namespace gpu_postmortem::ags
} // namespace debug
} // namespace drv3d_dx12
