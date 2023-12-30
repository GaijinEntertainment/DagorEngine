#pragma once

#include "call_stack.h"

#include <EASTL/variant.h>
#include <EASTL/span.h>
#include <EASTL/unordered_map.h>

#include "pipeline.h"


namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug
{
union Configuration;
}
} // namespace drv3d_dx12

namespace drv3d_dx12::debug::gpu_postmortem
{
class NullTrace
{
public:
  void configure() { logdbg("DX12: ...no GPU postmortem tracer active..."); }
  void beginCommandBuffer(ID3D12Device3 *, ID3D12GraphicsCommandList *) {}
  void endCommandBuffer(ID3D12GraphicsCommandList *) {}
  void beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>, eastl::span<const char>) {}
  void endEvent(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
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
  void onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &)
  {
    logdbg("DX12: Device reset detected, no postmortem data available...");
  }
  bool sendGPUCrashDump(const char *, const void *, uintptr_t) { return false; }
  void onDeviceShutdown() {}
  bool onDeviceSetup(ID3D12Device *, const Configuration &, const Direct3D12Enviroment &) { return true; }
};
} // namespace drv3d_dx12::debug::gpu_postmortem