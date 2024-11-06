// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "break_point.h"
#include "device_state.h"

namespace drv3d_dx12::debug::null
{
class DeviceContextState : public break_point::Controller
{
public:
  void debugBeginCommandBuffer(DeviceState &, D3DDevice *, ID3D12GraphicsCommandList *) {}

  void debugEndCommandBuffer(DeviceState &, ID3D12GraphicsCommandList *) {}

  void debugFramePresent(DeviceState &) {}

  void debugEventBegin(DeviceState &, ID3D12GraphicsCommandList *, eastl::string_view) {}
  void debugEventEnd(DeviceState &, ID3D12GraphicsCommandList *) {}
  void debugMarkerSet(DeviceState &, ID3D12GraphicsCommandList *, eastl::string_view) {}

  void debugFrameCaptureBegin(DeviceState &, ID3D12CommandQueue *, uint32_t, eastl::span<const wchar_t>) {}

  void debugFrameCaptureEnd(DeviceState &, ID3D12CommandQueue *) {}

  void debugFrameCaptureQueueNextFrames(DeviceState &, ID3D12CommandQueue *, uint32_t, eastl::span<const wchar_t>, int) {}

  void debugRecordDraw(DeviceState &, D3DGraphicsCommandList *, const PipelineStageStateBase &, const PipelineStageStateBase &,
    BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, uint32_t, D3D12_PRIMITIVE_TOPOLOGY)
  {}

  void debugRecordDrawIndexed(DeviceState &, D3DGraphicsCommandList *, const PipelineStageStateBase &, const PipelineStageStateBase &,
    BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, int32_t, uint32_t, D3D12_PRIMITIVE_TOPOLOGY)
  {}

  void debugDrawIndirect(DeviceState &, D3DGraphicsCommandList *, const PipelineStageStateBase &, const PipelineStageStateBase &,
    BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}

  void debugDrawIndexedIndirect(DeviceState &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}

  void debugDispatchIndirect(DeviceState &, D3DGraphicsCommandList *, const PipelineStageStateBase &, ComputePipeline &,
    const BufferResourceReferenceAndOffset &)
  {}

  void debugDispatch(DeviceState &, D3DGraphicsCommandList *, const PipelineStageStateBase &, ComputePipeline &, uint32_t, uint32_t,
    uint32_t)
  {}

  void debugDispatchMesh(DeviceState &, D3DGraphicsCommandList *, const PipelineStageStateBase &, const PipelineStageStateBase &,
    BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t)
  {}

  void debugDispatchMeshIndirect(DeviceState &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &,
    const BufferResourceReferenceAndOffset &, uint32_t)
  {}

  void debugBlit(DeviceState &, D3DGraphicsCommandList *) {}

  void debugOnDeviceRemoved(DeviceState &, D3DDevice *, HRESULT) {}
};
} // namespace drv3d_dx12::debug::null
