// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include "event_marker_tracker.h"
#include <dag/dag_vector.h>
#include <driver.h>
#include <pipeline.h>
#include <EASTL/string_view.h>
#include <supp/dag_comPtr.h>
#include <util/dag_compilerDefs.h>


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
class GlobalState;
namespace pc
{
class DeviceState : public call_stack::Reporter, protected event_marker::Tracker
{
  GlobalState *globalState = nullptr;
  DWORD callbackCookie = 0;
  ComPtr<ID3D12InfoQueue> debugQueue;
  dag::Vector<uint8_t> debugMessageBuffer;

public:
  bool setup(GlobalState &global, ID3D12Device *device, const Direct3D12Enviroment &d3d_env);
  void teardown();
  void beginCommandBuffer(D3DDevice *device, ID3D12GraphicsCommandList *cmd);
  void endCommandBuffer(ID3D12GraphicsCommandList *cmd);
  void beginSection(ID3D12GraphicsCommandList *cmd, eastl::string_view text);
  void endSection(ID3D12GraphicsCommandList *cmd);
  void marker(ID3D12GraphicsCommandList *cmd, eastl::string_view text);
  void draw(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology);
  void drawIndexed(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t index_start, int32_t vertex_base, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology);
  void drawIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer);
  void drawIndexedIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer);
  void dispatchIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &state,
    ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer);
  void dispatch(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &state,
    ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z);
  void dispatchMesh(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t x, uint32_t y, uint32_t z);
  void dispatchMeshIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count);
  void blit(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd);
#if D3D_HAS_RAY_TRACING
  void dispatchRays(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp);
  void dispatchRaysIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip);
#endif
  void onDeviceRemoved(D3DDevice *device, HRESULT remove_reason);
  void preRecovery();
  void recover(ID3D12Device *device, const Direct3D12Enviroment &d3d_env);
  void beginCapture(const wchar_t *name);
  void endCapture();
  void captureNextFrames(const wchar_t *filename, int count);
  void handlePresentToPresentCapture();
  void sendGPUCrashDump(const char *type, const void *data, uintptr_t size);
  void processDebugLog()
  {
    if (DAGOR_UNLIKELY(debugQueue))
      processDebugLogImpl();
  }

  using event_marker::Tracker::currentEvent;
  using event_marker::Tracker::currentEventPath;
  using event_marker::Tracker::currentMarker;

private:
  void processDebugLogImpl();
};
} // namespace pc
} // namespace debug
} // namespace drv3d_dx12
