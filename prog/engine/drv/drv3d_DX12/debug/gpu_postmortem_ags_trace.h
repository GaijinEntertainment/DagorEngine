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
  constexpr void beginCommandBuffer(ID3D12Device3 *, ID3D12GraphicsCommandList *) {}
  constexpr void endCommandBuffer(ID3D12GraphicsCommandList *) {}
  void beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>, const eastl::string &);
  void endEvent(ID3D12GraphicsCommandList *, const eastl::string &);
  constexpr void marker(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
  constexpr void draw(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}
  constexpr void drawIndexed(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, int32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}
  constexpr void drawIndirect(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}
  constexpr void drawIndexedIndirect(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}
  constexpr void dispatchIndirect(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    ComputePipeline &, const BufferResourceReferenceAndOffset &)
  {}
  constexpr void dispatch(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *, const PipelineStageStateBase &,
    ComputePipeline &, uint32_t, uint32_t, uint32_t)
  {}
  constexpr void dispatchMesh(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t)
  {}
  constexpr void dispatchMeshIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &,
    const BufferResourceReferenceAndOffset &, uint32_t)
  {}
  constexpr void blit(const call_stack::CommandData &, ID3D12GraphicsCommandList2 *) {}
#if D3D_HAS_RAY_TRACING
  constexpr void dispatchRays(const call_stack::CommandData &, D3DGraphicsCommandList *, const RayDispatchBasicParameters &,
    const ResourceBindingTable &, const RayDispatchParameters &)
  {}
  constexpr void dispatchRaysIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const RayDispatchBasicParameters &,
    const ResourceBindingTable &, const RayDispatchIndirectParameters &)
  {}
#endif
  void onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &);
  constexpr bool sendGPUCrashDump(const char *, const void *, uintptr_t) { return false; }
  constexpr void onDeviceShutdown() {}
  constexpr bool onDeviceSetup(ID3D12Device *, const Configuration &, const Direct3D12Enviroment &) { return true; }

  template <typename T>
  static bool load(const Configuration &config, const Direct3D12Enviroment &, T &target)
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

  bool tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr);

private:
  AGSContext *agsContext = nullptr;
  bool deviceInitialized = false;
};
} // namespace gpu_postmortem::ags
} // namespace debug
} // namespace drv3d_dx12
