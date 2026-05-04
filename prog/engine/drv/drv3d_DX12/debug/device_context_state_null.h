// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "break_point.h"
#include "device_state.h"
#include <driver.h>

#include <EASTL/span.h>
#include <EASTL/string_view.h>


namespace drv3d_dx12
{
class BasePipeline;
class ComputePipeline;
class PipelineVariant;
struct BufferResourceReferenceAndOffset;
struct PipelineStageStateBase;
namespace debug::null
{
class DeviceContextState : public break_point::Controller
{
public:
  constexpr void debugBeginCommandBuffer(DeviceState &, D3DDevice *, CommandListIdentifier, D3DGraphicsCommandList *) {}

  constexpr void debugEndCommandBuffer(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *) {}

  constexpr void debugFramePresent(DeviceState &) {}

  constexpr void debugEventBegin(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, eastl::string_view) {}
  constexpr void debugEventEnd(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *) {}
  constexpr void debugMarkerSet(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, eastl::string_view) {}

  constexpr void debugFrameCaptureBegin(DeviceState &, ID3D12CommandQueue *, uint32_t, eastl::span<const wchar_t>) {}

  constexpr void debugFrameCaptureEnd(DeviceState &, ID3D12CommandQueue *) {}

  constexpr void debugFrameCaptureQueueNextFrames(DeviceState &, ID3D12CommandQueue *, uint32_t, eastl::span<const wchar_t>, int) {}

  constexpr void debugRecordDraw(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}

  constexpr void debugRecordDrawIndexed(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, int32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}

  constexpr void debugDrawIndirect(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}

  constexpr void debugDrawIndexedIndirect(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *,
    const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &,
    const BufferResourceReferenceAndOffset &)
  {}

  constexpr void debugDispatchIndirect(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    ComputePipeline &, const BufferResourceReferenceAndOffset &)
  {}

  constexpr void debugDispatch(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    ComputePipeline &, uint32_t, uint32_t, uint32_t)
  {}

  constexpr void debugDispatchMesh(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t)
  {}

  constexpr void debugDispatchMeshIndirect(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *,
    const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &,
    const BufferResourceReferenceAndOffset &, const BufferResourceReferenceAndOffset &, uint32_t)
  {}

  constexpr void debugBlit(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *) {}

#if D3D_HAS_RAY_TRACING
  constexpr void debugDispatchRays(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *, const RayDispatchBasicParameters &,
    const ResourceBindingTable &, const RayDispatchParameters &)
  {}
  constexpr void debugDispatchRaysIndirect(DeviceState &, CommandListIdentifier, D3DGraphicsCommandList *,
    const RayDispatchBasicParameters &, const ResourceBindingTable &, const RayDispatchIndirectParameters &)
  {}
#endif

  constexpr void debugOnDeviceRemoved(DeviceState &, D3DDevice *, HRESULT) {}
  constexpr void nameResource(DeviceState &, ID3D12Resource *, eastl::string_view) {}
  constexpr void nameResource(DeviceState &, ID3D12Resource *, eastl::wstring_view) {}
  constexpr void debugSetPostmortemTraceEnabled(DeviceState &, CommandListIdentifier, bool) {}
};
} // namespace debug::null
} // namespace drv3d_dx12
