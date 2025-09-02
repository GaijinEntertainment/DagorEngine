// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include <pipeline.h>
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
namespace gpu_postmortem
{
class NullTrace
{
public:
  void configure() { logdbg("DX12: ...no GPU postmortem tracer active..."); }
  constexpr void beginCommandBuffer(D3DDevice *, D3DGraphicsCommandList *) {}
  constexpr void endCommandBuffer(D3DGraphicsCommandList *) {}
  constexpr void beginEvent(D3DGraphicsCommandList *, eastl::span<const char>, eastl::span<const char>) {}
  constexpr void endEvent(D3DGraphicsCommandList *, eastl::span<const char>) {}
  constexpr void marker(D3DGraphicsCommandList *, eastl::span<const char>) {}
  constexpr void draw(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}
  constexpr void drawIndexed(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, int32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}
  constexpr void drawIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}
  constexpr void drawIndexedIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}
  constexpr void dispatchIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    ComputePipeline &, const BufferResourceReferenceAndOffset &)
  {}
  constexpr void dispatch(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &, ComputePipeline &,
    uint32_t, uint32_t, uint32_t)
  {}
  constexpr void dispatchMesh(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t)
  {}
  constexpr void dispatchMeshIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &,
    const BufferResourceReferenceAndOffset &, uint32_t)
  {}
  constexpr void blit(const call_stack::CommandData &, D3DGraphicsCommandList *) {}
#if D3D_HAS_RAY_TRACING
  constexpr void dispatchRays(const call_stack::CommandData &, D3DGraphicsCommandList *, const RayDispatchBasicParameters &,
    const ResourceBindingTable &, const RayDispatchParameters &)
  {}
  constexpr void dispatchRaysIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const RayDispatchBasicParameters &,
    const ResourceBindingTable &, const RayDispatchIndirectParameters &)
  {}
#endif
  void onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &)
  {
    logdbg("DX12: Device reset detected, no postmortem data available...");
  }
  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **, HLSLVendorExtensions &) { return false; }
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
  static bool load(const Configuration &, const Direct3D12Enviroment &, T &&target)
  {
    target.template emplace<NullTrace>();
    return true;
  }

  void nameResource(ID3D12Resource *, eastl::string_view) {}
  void nameResource(ID3D12Resource *, eastl::wstring_view) {}
};
} // namespace gpu_postmortem
} // namespace debug
} // namespace drv3d_dx12