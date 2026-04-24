// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "command_list_storage.h"
#include "command_list_trace.h"
#include "command_list_trace_recorder.h"
#include "trace_checkpoint.h"

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
struct Configuration;
namespace gpu_postmortem::dagor
{
class Trace
{
  struct TraceRecording
  {
    CommandListTrace traceList;
    CommandListTraceRecorderPool::RecorderID recorder;

    TraceRecording() = default;
    TraceRecording(CommandListTraceRecorderPool::RecorderID r) : recorder{r} {}

    ~TraceRecording() = default;
    TraceRecording(TraceRecording &&) = default;
    TraceRecording &operator=(TraceRecording &&) = default;
  };
  CommandListStorage<CommandListIdentifier, TraceRecording> commandListTable;
  CommandListTraceRecorderPool traceRecoderingPool;
  TraceCheckpoint lastCheckpoint = {};
  // have to keep the last valid checkpoint around to properly handle device resets and the execution of trace capture command
  // SET_GPU_POSTMORTEM_DATA_TRACE_ENABLED
  TraceCheckpoint lastValidCheckpoint = {};
  bool isContinuousTraceMode = false;
  bool traceEnabled = false;

  class RangeReporter
  {
    static constexpr uint32_t MAX_RANGES_COUNT = 16;

    carray<eastl::pair<TraceCheckpoint, TraceCheckpoint>, MAX_RANGES_COUNT> traceRangesRing;
    int insertIndex = 0;

  public:
    RangeReporter();

    void markCmdListDataExpired(CommandListIdentifier cmdListId);
    void beginRange(const TraceCheckpoint &checkpoint);
    void endRange(const TraceCheckpoint &checkpoint);

    void report(Trace &trace, call_stack::Reporter &reporter);
  };
  RangeReporter rangeReporter;

  void walkBreadcumbs(call_stack::Reporter &reporter);
  static bool checkForContinuousTraceModeFromConfig(const Configuration &config);

  const Trace::TraceRecording *getOptionalListForRange(const TraceCheckpoint &from, const TraceCheckpoint &to) const;

  void updateCheckpoint(const TraceCheckpoint &checkpoint)
  {
    if (lastCheckpoint.isValid())
    {
      lastValidCheckpoint = lastCheckpoint;
    }
    lastCheckpoint = checkpoint;
  }

public:
  // Have to delete move constructor, otherwise compiler / templated stuff of variant tries to be smart and results in compile errors.
  Trace(Trace &&) = delete;
  Trace &operator=(Trace &&) = delete;
  Trace() = default;
  ~Trace() { logdbg("DX12: Shutting down DAGOR GPU Trace"); }
  void configure() {}
  void beginCommandBuffer(D3DDevice *device, CommandListIdentifier cmd_id);
  void endCommandBuffer(CommandListIdentifier cmd_id);
  void beginEvent(CommandListIdentifier cmd_id, eastl::span<const char>, const eastl::string &);
  void endEvent(CommandListIdentifier cmd_id, const eastl::string &);
  void marker(CommandListIdentifier cmd_id, eastl::span<const char>);
  void draw(const call_stack::CommandData &, CommandListIdentifier cmd_id, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY);
  void drawIndexed(const call_stack::CommandData &, CommandListIdentifier cmd_id, D3DGraphicsCommandList *,
    const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t,
    int32_t, uint32_t, D3D12_PRIMITIVE_TOPOLOGY);
  void drawIndirect(const call_stack::CommandData &, CommandListIdentifier cmd_id, D3DGraphicsCommandList *,
    const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &,
    const BufferResourceReferenceAndOffset &);
  void drawIndexedIndirect(const call_stack::CommandData &, CommandListIdentifier cmd_id, D3DGraphicsCommandList *,
    const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &,
    const BufferResourceReferenceAndOffset &);
  void dispatchIndirect(const call_stack::CommandData &, CommandListIdentifier cmd_id, D3DGraphicsCommandList *,
    const PipelineStageStateBase &, ComputePipeline &, const BufferResourceReferenceAndOffset &);
  void dispatch(const call_stack::CommandData &, CommandListIdentifier cmd_id, D3DGraphicsCommandList *,
    const PipelineStageStateBase &, ComputePipeline &, uint32_t, uint32_t, uint32_t);
  void dispatchMesh(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
    const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    uint32_t x, uint32_t y, uint32_t z);
  void dispatchMeshIndirect(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
    const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count);
  void blit(const call_stack::CommandData &, CommandListIdentifier cmd_id, D3DGraphicsCommandList *);
#if D3D_HAS_RAY_TRACING
  void dispatchRays(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp);
  void dispatchRaysIndirect(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip);
#endif
  void onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &reporter);
  bool sendGPUCrashDump(const char *, const void *, uintptr_t);
  void onDeviceShutdown();
  bool onDeviceSetup(D3DDevice *, const Configuration &, const Direct3D12Enviroment &);
  template <typename K>
  bool onDeviceSetup(D3DDevice *device, const Configuration &config, const Direct3D12Enviroment &env, const K &)
  {
    return onDeviceSetup(device, config, env);
  }
  template <typename... Ts>
  auto onDeviceCreated(Ts &&...ts)
  {
    return onDeviceSetup(eastl::forward<Ts>(ts)...);
  }

  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **, HLSLVendorExtensions &) { return false; }

  template <typename T>
  static bool load(const Configuration &config, const Direct3D12Enviroment &d3d_env, T &&target)
  {
    G_UNUSED(d3d_env);

    auto isContinuousTraceMode = checkForContinuousTraceModeFromConfig(config);
    logdbg("DX12: Dagor %s GPU trace is enabled...", isContinuousTraceMode ? "" : "production");

    target.template emplace<Trace>().isContinuousTraceMode = isContinuousTraceMode;

    return true;
  }

  template <typename T, typename K>
  static bool load(const Configuration &config, const Direct3D12Enviroment &e, const K &, T &&target)
  {
    return load(config, e, target);
  }

  TraceCheckpoint getCheckpoint() { return lastCheckpoint; }
  TraceCheckpoint getLastValidCheckpoint()
  {
    if (lastCheckpoint.isValid())
    {
      return lastCheckpoint;
    }
    return lastValidCheckpoint;
  }

  TraceRunStatus getTraceRunStatusFor(const TraceCheckpoint &cp)
  {
    auto list = commandListTable.getOptionalList(cp.commandList);
    if (!list)
    {
      logdbg("DX12: getDeviceProgressFor(%p) -> NoTraceData", cp.commandList);
      return TraceRunStatus::NoTraceData;
    }

    return traceRecoderingPool.getTraceRunStatus(list->recorder);
  }

  TraceStatus getTraceStatusFor(const TraceCheckpoint &cp)
  {
    auto list = commandListTable.getOptionalList(cp.commandList);
    if (!list)
    {
      return TraceStatus::NotLaunched;
    }

    return traceRecoderingPool.readTraceStatus(list->recorder, cp.progress);
  }

  void reportTraceDataForRange(const TraceCheckpoint &from, const TraceCheckpoint &to, call_stack::Reporter &reporter);
  bool reportOnlyLaunchedTrace(const TraceCheckpoint &from, const TraceCheckpoint &to, call_stack::Reporter &reporter);

  bool isTraceEnabled() const { return isContinuousTraceMode || traceEnabled; }
  void setTraceEnabled(CommandListIdentifier cmd_id, bool is_enabled);
};

} // namespace gpu_postmortem::dagor
} // namespace debug
} // namespace drv3d_dx12
