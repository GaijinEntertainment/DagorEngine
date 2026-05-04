// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpu_postmortem_dagor_trace.h"
#include "configuration.h"
#include "gpu_postmortem_microsoft_dred.h"
#include <platform.h>


namespace drv3d_dx12::debug::gpu_postmortem::dagor
{

void Trace::walkBreadcumbs(call_stack::Reporter &reporter)
{
  CommandListTraceBase::printLegend();

  commandListTable.visitAll([&reporter, this](auto cmd, auto &cmdList) {
    if (traceRecoderingPool.isCompleted(cmdList.recorder))
    {
      logdbg("DX12: Command Buffer was executed without error");
      logdbg("DX12: Command Buffer: %p", cmd);
    }
    else if (!traceRecoderingPool.hasAnyTracesRecorded(cmdList.recorder))
    {
      logdbg("DX12: Command Buffer execution was likely not started yet");
      logdbg("DX12: Command Buffer: %p", cmd);
    }
    else
    {
      logdbg("DX12: Command Buffer execution incomplete");
      logdbg("DX12: Command Buffer: %p", cmd);
      logdbg("DX12: Breadcrumb count: %u", traceRecoderingPool.getTraceRecordIssued(cmdList.recorder));
      cmdList.traceList.report(reporter,
        [this, &cmdList](auto trace_id) { return traceRecoderingPool.readTraceStatus(cmdList.recorder, trace_id); });
    }
  });
}

bool Trace::checkForContinuousTraceModeFromConfig(const Configuration &config) { return config.enableDagorGPUTrace; }

void Trace::beginCommandBuffer(D3DDevice *device, CommandListIdentifier cmd_id)
{
  rangeReporter.markCmdListDataExpired(cmd_id);
  auto &list =
    commandListTable.beginListWithCallback(cmd_id, [this, device](auto) { return traceRecoderingPool.allocateTraceRecorder(device); });
  traceRecoderingPool.beginRecording(list.recorder);
  list.traceList.beginTrace();
}

void Trace::endCommandBuffer(CommandListIdentifier cmd_id)
{
  commandListTable.endList(cmd_id);
  updateCheckpoint({});
}

void Trace::beginEvent(CommandListIdentifier cmd_id, eastl::span<const char> text, const eastl::string &full_path)
{
  auto &list = commandListTable.getList(cmd_id);
  list.traceList.beginEvent(text, full_path);
}

void Trace::endEvent(CommandListIdentifier cmd_id, const eastl::string &full_path)
{
  auto &list = commandListTable.getList(cmd_id);
  list.traceList.endEvent(full_path);
}

void Trace::marker(CommandListIdentifier cmd_id, eastl::span<const char> text)
{
  auto &list = commandListTable.getList(cmd_id);
  list.traceList.marker(text);
}

void Trace::draw(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.draw(id, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, start, first_instance, topology);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::drawIndexed(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base, uint32_t first_instance,
  D3D12_PRIMITIVE_TOPOLOGY topology)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.drawIndexed(id, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, index_start, vertex_base,
    first_instance, topology);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::drawIndirect(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.drawIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, buffer);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::drawIndexedIndirect(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.drawIndexedIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, buffer);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::dispatchIndirect(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &state, ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchIndirect(id, debug_info, state, pipeline, buffer);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::dispatch(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &state, ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatch(id, debug_info, state, pipeline, x, y, z);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::dispatchMesh(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  uint32_t x, uint32_t y, uint32_t z)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchMesh(id, debug_info, vs, ps, pipeline_base, pipeline, x, y, z);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::dispatchMeshIndirect(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchMeshIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, args, count, max_count);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::blit(const call_stack::CommandData &call_stack, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.blit(id, call_stack);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

#if D3D_HAS_RAY_TRACING
void Trace::dispatchRays(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchRays(id, debug_info, dispatch_parameters, rbt, rdp);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}

void Trace::dispatchRaysIndirect(const call_stack::CommandData &debug_info, CommandListIdentifier cmd_id, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
{
  auto &list = commandListTable.getList(cmd_id);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchRaysIndirect(id, debug_info, dispatch_parameters, rbt, rdip);
  updateCheckpoint({.commandList = cmd_id, .progress = id});
}
#endif

void Trace::onDeviceRemoved(D3DDevice *, HRESULT reason, call_stack::Reporter &reporter)
{
  if (DXGI_ERROR_INVALID_CALL == reason)
  {
    // Data is not useful when the runtime detected a invalid call.
    return;
  }

  // for now we are not reporting those as they are not useful on it own
  // walkBreadcumbs(reporter);

  rangeReporter.report(*this, reporter);
}

bool Trace::sendGPUCrashDump(const char *, const void *, uintptr_t) { return false; }

void Trace::onDeviceShutdown()
{
  commandListTable.reset();
  traceRecoderingPool.reset();
}

bool Trace::onDeviceSetup(D3DDevice *device, const Configuration &, const Direct3D12Enviroment &)
{
  // have to check if hw does support the necessary features
  D3D12_FEATURE_DATA_D3D12_OPTIONS3 level3Options = {};
  if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &level3Options, sizeof(level3Options))))
  {
    logdbg("DX12: CheckFeatureSupport for OPTIONS3 failed, assuming no WriteBufferImmediate support");
    return false;
  }
  if (0 == (level3Options.WriteBufferImmediateSupportFlags & D3D12_COMMAND_LIST_SUPPORT_FLAG_DIRECT))
  {
    logdbg("DX12: No support for WriteBufferImmediate on direct queue");
    return false;
  }

  // NOTE this is not really required as committed resource seems to work fine after crash, but is probably not entirely reliable
  // right now we require it though
  D3D12_FEATURE_DATA_EXISTING_HEAPS existingHeap = {};
  if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_EXISTING_HEAPS, &existingHeap, sizeof(existingHeap))))
  {
    logdbg("DX12: CheckFeatureSupport for EXISTING_HEAPS failed, assuming no existing heap support");
    return false;
  }
  if (FALSE == existingHeap.Supported)
  {
    logdbg("DX12: No support for existing heaps");
    return false;
  }
  return true;
}

const Trace::TraceRecording *Trace::getOptionalListForRange(const TraceCheckpoint &from, const TraceCheckpoint &to) const
{
  if (from.commandList != to.commandList && from.commandList)
  {
    // the tracer can not handle command list jumps, this has to be handled by the caller, by keeping track on per command list
    // identity
    logwarn("DX12: reportTraceDataForRange with from.commandList = %p and to.commandList = %p", from.commandList, to.commandList);
    return nullptr;
  }

  return commandListTable.getOptionalList(to.commandList);
}

void Trace::reportTraceDataForRange(const TraceCheckpoint &from, const TraceCheckpoint &to, call_stack::Reporter &reporter)
{
  if (auto list = getOptionalListForRange(from, to))
  {
    list->traceList.reportRange(from.progress, to.progress, reporter,
      [this, list](auto trace_id) { return traceRecoderingPool.readTraceStatus(list->recorder, trace_id); });
  }
}

bool Trace::reportOnlyLaunchedTrace(const TraceCheckpoint &from, const TraceCheckpoint &to, call_stack::Reporter &reporter)
{
  if (auto list = getOptionalListForRange(from, to))
  {
    return list->traceList.reportTraceIf(
      reporter,
      [this, list, &from, &to](auto trace) {
        auto traceId = trace.getTraceID();
        bool isInRange = (traceId > from.progress && traceId <= to.progress);
        bool isLaunched = (traceRecoderingPool.readTraceStatus(list->recorder, traceId) == TraceStatus::Launched);
        return isInRange && isLaunched;
      },
      [this, list](auto trace_id) { return traceRecoderingPool.readTraceStatus(list->recorder, trace_id); });
  }

  return false;
}

void Trace::setTraceEnabled(CommandListIdentifier cmd_id, bool is_enabled)
{
  if (traceEnabled == is_enabled)
    return;

  if (is_enabled)
  {
    auto &list = commandListTable.getList(cmd_id);
    auto progress = traceRecoderingPool.getRecorderProgress(list.recorder);
    rangeReporter.beginRange({.commandList = cmd_id, .progress = progress});
  }
  else
  {
    rangeReporter.endRange(getLastValidCheckpoint());
  }

  traceEnabled = is_enabled;
}

Trace::RangeReporter::RangeReporter()
{
  for (auto &range : traceRangesRing)
    range = eastl::make_pair(TraceCheckpoint::make_invalid(), TraceCheckpoint::make_invalid());
}

void Trace::RangeReporter::markCmdListDataExpired(CommandListIdentifier cmd_id)
{
  for (auto &range : traceRangesRing)
    if (range.first.commandList == cmd_id)
      range = eastl::make_pair(TraceCheckpoint::make_invalid(), TraceCheckpoint::make_invalid());
}

void Trace::RangeReporter::beginRange(const TraceCheckpoint &checkpoint)
{
  traceRangesRing[insertIndex] = eastl::make_pair(checkpoint, TraceCheckpoint::make_invalid());
}

void Trace::RangeReporter::endRange(const TraceCheckpoint &checkpoint)
{
  auto &target = traceRangesRing[insertIndex];
  if (!target.first.isValid())
  {
    // never properly started
    logwarn("DX12: Trace::RangeReporter::endRange: first was invalid");
    return;
  }
  if ((target.first.commandList == checkpoint.commandList) && (target.first.progress <= checkpoint.progress))
  {
    target.second = checkpoint;
  }
  else
  {
    // checkpoint for end got somehow lost, only option to make it a range of just the first command
    logwarn("DX12: Trace::RangeReporter::endRange: was given non matching checkpoint: %p == %p && %u <= %u", target.first.commandList,
      checkpoint.commandList, static_cast<uint32_t>(target.first.progress), static_cast<uint32_t>(checkpoint.progress));
    target.second = target.first;
  }

  insertIndex = (insertIndex + 1) % MAX_RANGES_COUNT;
}

void Trace::RangeReporter::report(Trace &trace, call_stack::Reporter &reporter)
{
  bool hasValidReport = false;
  for (int ofs = 0; ofs < MAX_RANGES_COUNT; ++ofs)
  {
    const auto &range = traceRangesRing[(ofs + insertIndex) % MAX_RANGES_COUNT];
    bool isValid = range.first.isValid() && range.second.isValid();
    if (!isValid)
      continue;

    if (trace.reportOnlyLaunchedTrace(range.first, range.second, reporter))
      hasValidReport = true;
  }

#if DAGOR_DBGLEVEL == 0
  if (hasValidReport)
    D3D_ERROR("Gpu crash postmortem dump");
#endif
};

} // namespace drv3d_dx12::debug::gpu_postmortem::dagor