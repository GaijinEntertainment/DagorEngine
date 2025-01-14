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

  commandListTable.visitAll([&reporter](auto cmd, auto &cmdList) {
    if (cmdList.traceRecodring.isCompleted())
    {
      logdbg("DX12: Command Buffer was executed without error");
      logdbg("DX12: Command Buffer: %p", cmd);
    }
    else
    {
      uint32_t count = cmdList.traceRecodring.completedCount();

      if (0 == count)
      {
        logdbg("DX12: Command Buffer execution was likely not started yet");
        logdbg("DX12: Command Buffer: %p", cmd);
        logdbg("DX12: Breadcrumb count: %u", count);
        logdbg("DX12: Last breadcrumb value: %u", cmdList.traceRecodring.indexToTraceID(0));
        auto vistorContext = cmdList.traceList.beginVisitation();
        cmdList.traceList.reportEverythingAsNotCompleted(vistorContext, cmdList.traceRecodring.indexToTraceID(1), reporter);
      }
      else
      {
        auto completedTraceID = cmdList.traceRecodring.indexToTraceID(count);
        auto vistorContext = cmdList.traceList.beginVisitation();

        logdbg("DX12: Command Buffer execution incomplete");
        logdbg("DX12: Command Buffer: %p", cmd);
        logdbg("DX12: Breadcrumb count: %u", count);
        logdbg("DX12: Last breadcrumb value: %u", completedTraceID);

        // Report everything until the trace it as regular completed
        cmdList.traceList.reportEverythingUntilAsCompleted(vistorContext, completedTraceID, reporter);

        // Report everything that matches the trace as last completed
        logdbg("DX12: ~Last known good command~~");
        cmdList.traceList.reportEverythingMatchingAsLastCompleted(vistorContext, completedTraceID, reporter);

        logdbg("DX12: ~First may be bad command~");
        // Will print full dump for all remaining trace entries
        cmdList.traceList.reportEverythingAsNoCompleted(vistorContext, reporter);
      }
    }
  });
}

bool Trace::try_load(const Configuration &config, const Direct3D12Enviroment &d3d_env)
{
  if (!config.enableDagorGPUTrace)
  {
    logdbg("DX12: Dagor GPU trace is disabled by configuration");
    return false;
  }

  logdbg("DX12: Dagor GPU trace is enabled...");

  // when page fault should be tracked we try to use DRED, if it works nice, if not, no loss
  if (!config.trackPageFaults)
  {
    logdbg("DX12: ...page fault tracking is disabled by configuration");
    return true;
  }

  logdbg("DX12: Trying to capture page faults with DRED, may not work...");
  logdbg("DX12: ...loading debug interface query...");
  PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
  reinterpret_cast<FARPROC &>(D3D12GetDebugInterface) = d3d_env.getD3DProcAddress("D3D12GetDebugInterface");
  if (!D3D12GetDebugInterface)
  {
    logdbg("DX12: ...D3D12GetDebugInterface not found in direct dx runtime library");
    return true;
  }

  ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredConfig;
  if (FAILED(D3D12GetDebugInterface(COM_ARGS(&dredConfig))))
  {
    logdbg("DX12: ...unable to acquire DRED config interface");
    return false;
  }

  dredConfig->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  logdbg("DX12: ...enabled page fault tracking of DRED");
  return true;
}

void Trace::beginCommandBuffer(ID3D12Device3 *device, ID3D12GraphicsCommandList *cmd)
{
  auto &list = commandListTable.beginList(cmd, device);
  list.traceRecodring.beginRecording();
  list.traceList.beginTrace();
}

void Trace::endCommandBuffer(ID3D12GraphicsCommandList *cmd) { commandListTable.endList(cmd); }

void Trace::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text, const eastl::string &full_path)
{
  auto &list = commandListTable.getList(cmd);
  list.traceList.beginEvent({}, text, full_path);
}

void Trace::endEvent(ID3D12GraphicsCommandList *cmd, const eastl::string &full_path)
{
  auto &list = commandListTable.getList(cmd);
  list.traceList.endEvent({}, full_path);
}

void Trace::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  auto &list = commandListTable.getList(cmd);
  list.traceList.marker({}, text);
}

void Trace::draw(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
  uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.draw(id, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, start, first_instance, topology);
}

void Trace::drawIndexed(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
  uint32_t index_start, int32_t vertex_base, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.drawIndexed(id, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, index_start, vertex_base,
    first_instance, topology);
}

void Trace::drawIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.drawIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, buffer);
}

void Trace::drawIndexedIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.drawIndexedIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, buffer);
}

void Trace::dispatchIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &state, ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.dispatchIndirect(id, debug_info, state, pipeline, buffer);
}

void Trace::dispatch(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &state,
  ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.dispatch(id, debug_info, state, pipeline, x, y, z);
}

void Trace::dispatchMesh(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.dispatchMesh(id, debug_info, vs, ps, pipeline_base, pipeline, x, y, z);
}

void Trace::dispatchMeshIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.dispatchMeshIndirect(id, debug_info, vs, ps, pipeline_base, pipeline, args, count, max_count);
}

void Trace::blit(const call_stack::CommandData &call_stack, D3DGraphicsCommandList *cmd)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.blit(id, call_stack);
}

#if D3D_HAS_RAY_TRACING
void Trace::dispatchRays(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.dispatchRays(id, debug_info, dispatch_parameters, rbt, rdp);
}

void Trace::dispatchRaysIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
{
  auto &list = commandListTable.getList(cmd);
  auto id = list.traceRecodring.record(cmd);
  list.traceList.dispatchRaysIndirect(id, debug_info, dispatch_parameters, rbt, rdip);
}
#endif

void Trace::onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &reporter)
{
  if (DXGI_ERROR_INVALID_CALL == reason)
  {
    // Data is not useful when the runtime detected a invalid call.
    return;
  }

  // For possible page fault information we fall back to DRED
  logdbg("DX12: Acquiring DRED interface...");
  ComPtr<ID3D12DeviceRemovedExtendedData> dred;
  if (FAILED(device->QueryInterface(COM_ARGS(&dred))))
  {
    logdbg("DX12: ...failed, no DRED information available");
    return;
  }

  walkBreadcumbs(reporter);
  report_page_fault(dred.Get());
}

bool Trace::sendGPUCrashDump(const char *, const void *, uintptr_t) { return false; }

void Trace::onDeviceShutdown() { commandListTable.reset(); }

bool Trace::onDeviceSetup(ID3D12Device *device, const Configuration &, const Direct3D12Enviroment &)
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
} // namespace drv3d_dx12::debug::gpu_postmortem::dagor