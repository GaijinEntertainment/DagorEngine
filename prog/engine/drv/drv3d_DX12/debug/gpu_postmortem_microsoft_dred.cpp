// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpu_postmortem_microsoft_dred.h"
#include "configuration.h"
#include <platform.h>

namespace
{
bool is_dred_avilable()
{
  auto enablement = drv3d_dx12::get_application_DRED_enablement_from_registry();
  return D3D12_DRED_ENABLEMENT_FORCED_ON == enablement;
}
} // namespace

namespace drv3d_dx12::debug::gpu_postmortem::microsoft
{

bool DeviceRemovedExtendedData::try_load(const Configuration &config, const Direct3D12Enviroment &d3d_env)
{
  if (!config.enableDRED)
  {
    logdbg("DX12: ...DRED is not allowed per configuration...");
    return false;
  }

  if (!config.ignoreDREDAvailability)
  {
    if (!is_dred_avilable())
    {
      logdbg("DX12: ...after registry inspection, it seems DRED is not available, DRED is ignored...");
      return false;
    }
  }
  else
  {
    logdbg("DX12: ...skipped DRED availability inspection of the registry, assuming it is available...");
  }

  logdbg("DX12: ...loading debug interface query...");
  PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
  reinterpret_cast<FARPROC &>(D3D12GetDebugInterface) = d3d_env.getD3DProcAddress("D3D12GetDebugInterface");
  if (!D3D12GetDebugInterface)
  {
    logdbg("DX12: ...D3D12GetDebugInterface not found in direct dx runtime library...");
    return false;
  }

  ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredConfig;
  if (FAILED(D3D12GetDebugInterface(COM_ARGS(&dredConfig))))
  {
    logdbg("DX12: ...unable to query DRED settings interface, assuming it is unavailable...");
    return false;
  }

  dredConfig->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

  if (config.trackPageFaults)
  {
    dredConfig->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  }
  return true;
}

void DeviceRemovedExtendedData::walkBreadcumbs(ID3D12DeviceRemovedExtendedData *dred, call_stack::Reporter &reporter)
{
  logdbg("DX12: Acquiring breadcrumb information from DRED...");
  D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breacrumbInfo = {};
  if (FAILED(dred->GetAutoBreadcrumbsOutput(&breacrumbInfo)))
  {
    logdbg("DX12: ...failed, no breadcrumb data available");
    return;
  }
  walkBreadcumbs(breacrumbInfo.pHeadAutoBreadcrumbNode, reporter);
}

void DeviceRemovedExtendedData::walkBreadcumbs(const D3D12_AUTO_BREADCRUMB_NODE *node, call_stack::Reporter &reporter)
{
  if (!node)
  {
    logdbg("No breadcrumb nodes found");
  }

  CommandListTraceBase::printLegend();

  for (; node; node = node->pNext)
  {
    // simple check, if count is the same with last breadcrumb
    // value, than the cmd buffer was executed completely
    if (node->BreadcrumbCount == *node->pLastBreadcrumbValue)
    {
      logdbg("Command Buffer was executed without error");
      logdbg("Command Buffer: %p", node->pCommandList);
      report_alternate_name("Name: %s", node->pCommandListDebugNameA, node->pCommandListDebugNameW);
      logdbg("Command Queue: %p", node->pCommandQueue);
      report_alternate_name("Name: %s", node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW);
    }
    // if last breadcrumb value is 0, then this command buffer was probably never submitted
    // we report the first command to be sure
    else if (*node->pLastBreadcrumbValue == 0)
    {
      logdbg("Command Buffer execution was likely not started yet");
      logdbg("Command Buffer: %p", node->pCommandList);
      report_alternate_name("Name: %s", node->pCommandListDebugNameA, node->pCommandListDebugNameW);
      logdbg("Command Queue: %p", node->pCommandQueue);
      report_alternate_name("Name: %s", node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW);
      logdbg("Breadcrumb count: %u", node->BreadcrumbCount);
      logdbg("Last breadcrumb value: %u", *node->pLastBreadcrumbValue);
      auto &listInfo = commandListTable.getList(node->pCommandList);
      auto visitorContext = listInfo.beginVisitation();
      listInfo.reportAsCompleted(visitorContext, node->pCommandHistory[0], reporter);
    }
    else
    {
      auto &listInfo = commandListTable.getList(node->pCommandList);
      auto visitorContext = listInfo.beginVisitation();
      G_UNUSED(listInfo);

      logdbg("Command Buffer execution incomplete");
      logdbg("Command Buffer: %p", node->pCommandList);
      report_alternate_name("Name: %s", node->pCommandListDebugNameA, node->pCommandListDebugNameW);
      logdbg("Command Queue: %p", node->pCommandQueue);
      report_alternate_name("Name: %s", node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW);
      logdbg("Breadcrumb count: %u", node->BreadcrumbCount);
      logdbg("Last breadcrumb value: %u", *node->pLastBreadcrumbValue);

      auto lastValue = *node->pLastBreadcrumbValue;
      for (uint32_t i = 0; i < lastValue; ++i)
      {
        listInfo.reportAsCompleted(visitorContext, node->pCommandHistory[i], reporter);
      }

      logdbg("~Last known good command~~");
      listInfo.reportAsLastCompleted(visitorContext, node->pCommandHistory[lastValue], reporter);
      logdbg("~First may be bad command~");

      for (uint32_t i = lastValue + 1; i < node->BreadcrumbCount; ++i)
      {
        // Any of those commands could be the reason for the crash
        listInfo.reportAsNotCompleted(visitorContext, node->pCommandHistory[i], reporter);
      }
    }
  }
}

void DeviceRemovedExtendedData::beginCommandBuffer(ID3D12Device *, ID3D12GraphicsCommandList *cmd)
{
  commandListTable.beginList(cmd).beginTrace();
}

void DeviceRemovedExtendedData::endCommandBuffer(ID3D12GraphicsCommandList *cmd)
{
  commandListTable.getList(cmd).endTrace();
  commandListTable.endList(cmd);
}

void DeviceRemovedExtendedData::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text,
  const eastl::string &full_path)
{
  commandListTable.getList(cmd).beginEvent({}, text, full_path);
}

void DeviceRemovedExtendedData::endEvent(ID3D12GraphicsCommandList *cmd, const eastl::string &full_path)
{
  commandListTable.getList(cmd).endEvent({}, full_path);
}

void DeviceRemovedExtendedData::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  commandListTable.getList(cmd).marker({}, text);
}

void DeviceRemovedExtendedData::draw(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  commandListTable.getList(cmd).draw({}, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, start, first_instance,
    topology);
}

void DeviceRemovedExtendedData::drawIndexed(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base, uint32_t first_instance,
  D3D12_PRIMITIVE_TOPOLOGY topology)
{
  commandListTable.getList(cmd).drawIndexed({}, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, index_start,
    vertex_base, first_instance, topology);
}

void DeviceRemovedExtendedData::drawIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  commandListTable.getList(cmd).drawIndirect({}, debug_info, vs, ps, pipeline_base, pipeline, buffer);
}

void DeviceRemovedExtendedData::drawIndexedIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  commandListTable.getList(cmd).drawIndexedIndirect({}, debug_info, vs, ps, pipeline_base, pipeline, buffer);
}

void DeviceRemovedExtendedData::dispatchIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &state, ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
{
  commandListTable.getList(cmd).dispatchIndirect({}, debug_info, state, pipeline, buffer);
}

void DeviceRemovedExtendedData::dispatch(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &stage, ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  commandListTable.getList(cmd).dispatch({}, debug_info, stage, pipeline, x, y, z);
}

void DeviceRemovedExtendedData::dispatchMesh(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  uint32_t x, uint32_t y, uint32_t z)
{
  commandListTable.getList(cmd).dispatchMesh({}, debug_info, vs, ps, pipeline_base, pipeline, x, y, z);
}

void DeviceRemovedExtendedData::dispatchMeshIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
{
  commandListTable.getList(cmd).dispatchMeshIndirect({}, debug_info, vs, ps, pipeline_base, pipeline, args, count, max_count);
}

void DeviceRemovedExtendedData::blit(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd)
{
  commandListTable.getList(cmd).blit({}, debug_info);
}

#if D3D_HAS_RAY_TRACING
void DeviceRemovedExtendedData::dispatchRays(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
{
  commandListTable.getList(cmd).dispatchRays({}, debug_info, dispatch_parameters, rbt, rdp);
}

void DeviceRemovedExtendedData::dispatchRaysIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
{
  commandListTable.getList(cmd).dispatchRaysIndirect({}, debug_info, dispatch_parameters, rbt, rdip);
}
#endif

void DeviceRemovedExtendedData::onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &reporter)
{
  if (DXGI_ERROR_INVALID_CALL == reason)
  {
    // Data from DRED is not useful when the runtime detected a invalid call.
    return;
  }
  logdbg("DX12: Acquiring DRED interface...");
  ComPtr<ID3D12DeviceRemovedExtendedData> dred;
  if (FAILED(device->QueryInterface(COM_ARGS(&dred))))
  {
    logdbg("DX12: ...failed, no DRED information available");
    return;
  }

  walkBreadcumbs(dred.Get(), reporter);
  report_page_fault(dred.Get());
}

void DeviceRemovedExtendedData::onDeviceShutdown() { commandListTable.reset(); }

} // namespace drv3d_dx12::debug::gpu_postmortem::microsoft
