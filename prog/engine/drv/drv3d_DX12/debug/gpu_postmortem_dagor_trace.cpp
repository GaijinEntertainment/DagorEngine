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

bool Trace::try_load(const Configuration &config, const Direct3D12Enviroment &)
{
  if (!config.enableDagorGPUTrace)
  {
    logdbg("DX12: Dagor GPU trace is disabled by configuration");
    return false;
  }

  logdbg("DX12: Dagor GPU trace is enabled...");
  return true;
}

void Trace::beginCommandBuffer(D3DDevice *device, D3DGraphicsCommandList *cmd)
{
  auto &list =
    commandListTable.beginListWithCallback(cmd, [this, device](auto) { return traceRecoderingPool.allocateTraceRecorder(device); });
  traceRecoderingPool.beginRecording(list.recorder);
  list.traceList.beginTrace();
}

void Trace::endCommandBuffer(D3DGraphicsCommandList *cmd)
{
  commandListTable.endList(cmd);
  lastCheckpoint = {};
}

void Trace::beginEvent(D3DGraphicsCommandList *cmd, eastl::span<const char> text, const eastl::string &full_path)
{
  auto &list = commandListTable.getList(cmd);
  list.traceList.beginEvent(text, full_path);
}

void Trace::endEvent(D3DGraphicsCommandList *cmd, const eastl::string &full_path)
{
  auto &list = commandListTable.getList(cmd);
  list.traceList.endEvent(full_path);
}

void Trace::marker(D3DGraphicsCommandList *cmd, eastl::span<const char> text)
{
  auto &list = commandListTable.getList(cmd);
  list.traceList.marker(text);
}

void Trace::draw(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
  uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.draw(id, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, start, first_instance, topology);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::drawIndexed(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
  uint32_t index_start, int32_t vertex_base, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.drawIndexed(id, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, index_start, vertex_base,
    first_instance, topology);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::drawIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.drawIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, buffer);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::drawIndexedIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.drawIndexedIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, buffer);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::dispatchIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &state, ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchIndirect(id, debug_info, state, pipeline, buffer);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::dispatch(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &state,
  ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatch(id, debug_info, state, pipeline, x, y, z);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::dispatchMesh(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchMesh(id, debug_info, vs, ps, pipeline_base, pipeline, x, y, z);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::dispatchMeshIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchMeshIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, args, count, max_count);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::blit(const call_stack::CommandData &call_stack, D3DGraphicsCommandList *cmd)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.blit(id, call_stack);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

#if D3D_HAS_RAY_TRACING
void Trace::dispatchRays(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchRays(id, debug_info, dispatch_parameters, rbt, rdp);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}

void Trace::dispatchRaysIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
{
  auto &list = commandListTable.getList(cmd);
  auto id = traceRecoderingPool.recordTrace(list.recorder, cmd);
  list.traceList.dispatchRaysIndirect(id, debug_info, dispatch_parameters, rbt, rdip);
  lastCheckpoint = {.commandList = cmd, .progress = id};
}
#endif

void Trace::onDeviceRemoved(D3DDevice *, HRESULT reason, call_stack::Reporter &reporter)
{
  if (DXGI_ERROR_INVALID_CALL == reason)
  {
    // Data is not useful when the runtime detected a invalid call.
    return;
  }

  walkBreadcumbs(reporter);
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

void Trace::reportTraceDataForRange(const TraceCheckpoint &from, const TraceCheckpoint &to, call_stack::Reporter &reporter)
{
  if (from.commandList != to.commandList && from.commandList)
  {
    // the tracer can not handle command list jumps, this has to be handled by the caller, by keeping track on per command list
    // identity
    logwarn("DX12: reportTraceDataForRange with from.commandList = %p and to.commandList = %p", from.commandList, to.commandList);
    return;
  }

  auto list = commandListTable.getOptionalList(to.commandList);
  if (!list)
  {
    return;
  }

  list->traceList.reportRange(from.progress, to.progress, reporter,
    [this, list](auto trace_id) { return traceRecoderingPool.readTraceStatus(list->recorder, trace_id); });
}

} // namespace drv3d_dx12::debug::gpu_postmortem::dagor