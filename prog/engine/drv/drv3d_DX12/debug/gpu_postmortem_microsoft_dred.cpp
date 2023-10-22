#include "device.h"

using namespace drv3d_dx12;

namespace
{
bool is_dred_avilable()
{
  auto enablement = get_application_DRED_enablement_from_registry();
  return D3D12_DRED_ENABLEMENT_FORCED_ON == enablement;
}
} // namespace

bool debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::try_load(const Configuration &config,
  const Direct3D12Enviroment &d3d_env)
{
  if (!config.enableDRED)
  {
    debug("DX12: ...DRED is not allowed per configuration...");
    return false;
  }

  if (!config.ignoreDREDAvailability)
  {
    if (!is_dred_avilable())
    {
      debug("DX12: ...after registry inspection, it seems DRED is not available, DRED is ignored...");
      return false;
    }
  }
  else
  {
    debug("DX12: ...skipped DRED availability inspection of the registry, assuming it is available...");
  }

  debug("DX12: ...loading debug interface query...");
  PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
  reinterpret_cast<FARPROC &>(D3D12GetDebugInterface) = d3d_env.getD3DProcAddress("D3D12GetDebugInterface");
  if (!D3D12GetDebugInterface)
  {
    debug("DX12: ...D3D12GetDebugInterface not found in direct dx runtime library...");
    return false;
  }

  ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredConfig;
  if (FAILED(D3D12GetDebugInterface(COM_ARGS(&dredConfig))))
  {
    debug("DX12: ...unable to query DRED settings interface, assuming it is unavailable...");
    return false;
  }

  dredConfig->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

  if (config.trackPageFaults)
  {
    dredConfig->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  }
  return true;
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::walkBreadcumbs(ID3D12DeviceRemovedExtendedData *dred,
  call_stack::Reporter &reporter)
{
  debug("DX12: Acquiring breadcrumb information from DRED...");
  D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breacrumbInfo = {};
  if (FAILED(dred->GetAutoBreadcrumbsOutput(&breacrumbInfo)))
  {
    debug("DX12: ...failed, no breadcrumb data available");
    return;
  }
  walkBreadcumbs(breacrumbInfo.pHeadAutoBreadcrumbNode, reporter);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::walkBreadcumbs(const D3D12_AUTO_BREADCRUMB_NODE *node,
  call_stack::Reporter &reporter)
{
  if (!node)
  {
    debug("No breadcrumb nodes found");
  }

  CommandListTraceBase::printLegend();

  for (; node; node = node->pNext)
  {
    // simple check, if count is the same with last breadcrumb
    // value, than the cmd buffer was executed completely
    if (node->BreadcrumbCount == *node->pLastBreadcrumbValue)
    {
      debug("Command Buffer was executed without error");
      debug("Command Buffer: %p", node->pCommandList);
      report_alternate_name("Name: %s", node->pCommandListDebugNameA, node->pCommandListDebugNameW);
      debug("Command Queue: %p", node->pCommandQueue);
      report_alternate_name("Name: %s", node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW);
    }
    // if last breadcrumb value is 0, then this command buffer was probably never submitted
    // we report the first command to be sure
    else if (*node->pLastBreadcrumbValue == 0)
    {
      debug("Command Buffer execution was likely not started yet");
      debug("Command Buffer: %p", node->pCommandList);
      report_alternate_name("Name: %s", node->pCommandListDebugNameA, node->pCommandListDebugNameW);
      debug("Command Queue: %p", node->pCommandQueue);
      report_alternate_name("Name: %s", node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW);
      debug("Breadcrumb count: %u", node->BreadcrumbCount);
      debug("Last breadcrumb value: %u", *node->pLastBreadcrumbValue);
      auto &listInfo = commandListTable.getList(node->pCommandList);
      auto visitorContext = listInfo.beginVisitation();
      listInfo.reportAsCompleted(visitorContext, node->pCommandHistory[0], reporter);
    }
    else
    {
      auto &listInfo = commandListTable.getList(node->pCommandList);
      auto visitorContext = listInfo.beginVisitation();
      G_UNUSED(listInfo);

      debug("Command Buffer execution incomplete");
      debug("Command Buffer: %p", node->pCommandList);
      report_alternate_name("Name: %s", node->pCommandListDebugNameA, node->pCommandListDebugNameW);
      debug("Command Queue: %p", node->pCommandQueue);
      report_alternate_name("Name: %s", node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW);
      debug("Breadcrumb count: %u", node->BreadcrumbCount);
      debug("Last breadcrumb value: %u", *node->pLastBreadcrumbValue);

      auto lastValue = *node->pLastBreadcrumbValue;
      for (uint32_t i = 0; i < lastValue; ++i)
      {
        listInfo.reportAsCompleted(visitorContext, node->pCommandHistory[i], reporter);
      }

      debug("~Last known good command~~");
      listInfo.reportAsLastCompleted(visitorContext, node->pCommandHistory[lastValue], reporter);
      debug("~First may be bad command~");

      for (uint32_t i = lastValue + 1; i < node->BreadcrumbCount; ++i)
      {
        // Any of those commands could be the reason for the crash
        listInfo.reportAsNotCompleted(visitorContext, node->pCommandHistory[i], reporter);
      }
    }
  }
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::configure() {}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::beginCommandBuffer(ID3D12Device *, ID3D12GraphicsCommandList *cmd)
{
  commandListTable.beginList(cmd).beginTrace();
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::endCommandBuffer(ID3D12GraphicsCommandList *cmd)
{
  commandListTable.getList(cmd).endTrace();
  commandListTable.endList(cmd);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::beginEvent(ID3D12GraphicsCommandList *cmd,
  eastl::span<const char> text, eastl::span<const char> full_path)
{
  commandListTable.getList(cmd).beginEvent({}, text, full_path);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::endEvent(ID3D12GraphicsCommandList *cmd,
  eastl::span<const char> full_path)
{
  commandListTable.getList(cmd).endEvent({}, full_path);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  commandListTable.getList(cmd).marker({}, text);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::draw(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base,
  PipelineVariant &pipeline, uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance,
  D3D12_PRIMITIVE_TOPOLOGY topology)
{
  commandListTable.getList(cmd).draw({}, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, start, first_instance,
    topology);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::drawIndexed(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base,
  PipelineVariant &pipeline, uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base,
  uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  commandListTable.getList(cmd).drawIndexed({}, debug_info, vs, ps, pipeline_base, pipeline, count, instance_count, index_start,
    vertex_base, first_instance, topology);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::drawIndirect(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base,
  PipelineVariant &pipeline, BufferResourceReferenceAndOffset buffer)
{
  commandListTable.getList(cmd).drawIndirect({}, debug_info, vs, ps, pipeline_base, pipeline, buffer);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::drawIndexedIndirect(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base,
  PipelineVariant &pipeline, BufferResourceReferenceAndOffset buffer)
{
  commandListTable.getList(cmd).drawIndexedIndirect({}, debug_info, vs, ps, pipeline_base, pipeline, buffer);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::dispatchIndirect(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd, const PipelineStageStateBase &state, ComputePipeline &pipeline, BufferResourceReferenceAndOffset buffer)
{
  commandListTable.getList(cmd).dispatchIndirect({}, debug_info, state, pipeline, buffer);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::dispatch(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd, const PipelineStageStateBase &stage, ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  commandListTable.getList(cmd).dispatch({}, debug_info, stage, pipeline, x, y, z);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::dispatchMesh(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base,
  PipelineVariant &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  commandListTable.getList(cmd).dispatchMesh({}, debug_info, vs, ps, pipeline_base, pipeline, x, y, z);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::dispatchMeshIndirect(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base,
  PipelineVariant &pipeline, BufferResourceReferenceAndOffset args, BufferResourceReferenceAndOffset count, uint32_t max_count)
{
  commandListTable.getList(cmd).dispatchMeshIndirect({}, debug_info, vs, ps, pipeline_base, pipeline, args, count, max_count);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::blit(const call_stack::CommandData &debug_info,
  D3DGraphicsCommandList *cmd)
{
  commandListTable.getList(cmd).blit({}, debug_info);
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::onDeviceRemoved(D3DDevice *device, HRESULT reason,
  call_stack::Reporter &reporter)
{
  if (DXGI_ERROR_INVALID_CALL == reason)
  {
    // Data from DRED is not useful when the runtime detected a invalid call.
    return;
  }
  debug("DX12: Acquiring DRED interface...");
  ComPtr<ID3D12DeviceRemovedExtendedData> dred;
  if (FAILED(device->QueryInterface(COM_ARGS(&dred))))
  {
    debug("DX12: ...failed, no DRED information available");
    return;
  }

  walkBreadcumbs(dred.Get(), reporter);
  report_page_fault(dred.Get());
}

bool debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::sendGPUCrashDump(const char *, const void *, uintptr_t)
{
  return false;
}

void debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::onDeviceShutdown() { commandListTable.reset(); }

bool debug::gpu_postmortem::microsoft::DeviceRemovedExtendedData::onDeviceSetup(ID3D12Device *, const Configuration &,
  const Direct3D12Enviroment &)
{
  return true;
}
