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

void set_name(ID3D12Resource *resource, eastl::string_view name)
{
  // lazy way of converting to wchar, this assumes name is not multi byte encoding
  wchar_t wcharName[1024];
  *eastl::copy(name.data(), min(name.data() + name.size(), name.data() + 1023), wcharName) = L'\0';
  resource->SetName(wcharName);
}

const char *to_string(D3D12_DRED_ALLOCATION_TYPE type)
{
  switch (type)
  {
    default: return "<invalid>";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE: return "COMMAND_QUEUE";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR: return "COMMAND_ALLOCATOR";
    case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE: return "PIPELINE_STATE";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST: return "COMMAND_LIST";
    case D3D12_DRED_ALLOCATION_TYPE_FENCE: return "FENCE";
    case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP: return "DESCRIPTOR_HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_HEAP: return "HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP: return "QUERY_HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE: return "COMMAND_SIGNATURE";
    case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY: return "PIPELINE_LIBRARY";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER: return "VIDEO_DECODER";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR: return "VIDEO_PROCESSOR";
    case D3D12_DRED_ALLOCATION_TYPE_RESOURCE: return "RESOURCE";
    case D3D12_DRED_ALLOCATION_TYPE_PASS: return "PASS";
    case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION: return "CRYPTOSESSION";
    case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY: return "CRYPTOSESSIONPOLICY";
    case D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION: return "PROTECTEDRESOURCESESSION";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP: return "VIDEO_DECODER_HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL: return "COMMAND_POOL";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER: return "COMMAND_RECORDER";
    case D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT: return "STATE_OBJECT";
    case D3D12_DRED_ALLOCATION_TYPE_METACOMMAND: return "METACOMMAND";
    case D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP: return "SCHEDULINGGROUP";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR: return "VIDEO_MOTION_ESTIMATOR";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP: return "VIDEO_MOTION_VECTOR_HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND: return "VIDEO_EXTENSION_COMMAND";
  }
}

void report_alternate_name(const char *fmt, const char *a_str, const wchar_t *w_str)
{
  if (a_str)
  {
    logdbg(fmt, a_str);
  }
  else if (w_str)
  {
    char buf[1024];
    uint32_t i;
    for (i = 0; i < 1023 && w_str[i] != L'\0'; ++i)
      buf[i] = static_cast<char>(w_str[i]);
    buf[i] = '\0';
    logdbg(fmt, buf);
  }
  else
  {
    logdbg(fmt, "NULL");
  }
}

void report_allocation_info(const D3D12_DRED_ALLOCATION_NODE *node)
{
  if (!node)
  {
    logdbg("DX12: No allocation nodes found");
  }
  else
  {
    for (; node; node = node->pNext)
    {
      if (node->ObjectNameA)
      {
        logdbg("NODE: %s '%s'", to_string(node->AllocationType), node->ObjectNameA);
      }
      else if (node->ObjectNameW)
      {
        // TODO replace with proper wide to multi byte string conversion (not really needed?)
        // simple squash wchar to char
        char buf[1024];
        uint32_t i;
        for (i = 0; i < sizeof(buf) - 1 && node->ObjectNameW[i] != L'\0'; ++i)
          buf[i] = node->ObjectNameW[i] > 127 ? '.' : node->ObjectNameW[i];
        buf[i] = '\0';
        logdbg("NODE: %s '%s'", to_string(node->AllocationType), buf);
      }
      else
      {
        logdbg("NODE: %s with no name", to_string(node->AllocationType));
      }
    }
  }
}

void report_page_fault(ID3D12DeviceRemovedExtendedData *dred)
{
  logdbg("DX12: Acquiring page fault information from DRED...");
  D3D12_DRED_PAGE_FAULT_OUTPUT pagefaultInfo = {};
  if (FAILED(dred->GetPageFaultAllocationOutput(&pagefaultInfo)))
  {
    logdbg("DX12: ...failed, no page fault data available");
    return;
  }

  logdbg("DX12: Page fault info (DRED):");
  logdbg("DX12: GPU page fault address %016X", pagefaultInfo.PageFaultVA);
  logdbg("DX12: Heap existing allocations:");
  report_allocation_info(pagefaultInfo.pHeadExistingAllocationNode);
  logdbg("DX12: Heap recently freed allocations:");
  report_allocation_info(pagefaultInfo.pHeadRecentFreedAllocationNode);
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

void DeviceRemovedExtendedData::beginCommandBuffer(D3DDevice *, D3DGraphicsCommandList *) {}

void DeviceRemovedExtendedData::endCommandBuffer(D3DGraphicsCommandList *) {}

void DeviceRemovedExtendedData::beginEvent(D3DGraphicsCommandList *, eastl::span<const char>, const eastl::string &) {}

void DeviceRemovedExtendedData::endEvent(D3DGraphicsCommandList *, const eastl::string &) {}

void DeviceRemovedExtendedData::marker(D3DGraphicsCommandList *, eastl::span<const char>) {}

void DeviceRemovedExtendedData::onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &)
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

  report_page_fault(dred.Get());
}

void DeviceRemovedExtendedData::onDeviceShutdown() {}

void DeviceRemovedExtendedData::nameResource(ID3D12Resource *resource, eastl::string_view name) { set_name(resource, name); }

void DeviceRemovedExtendedData::nameResource(ID3D12Resource *resource, eastl::wstring_view name)
{
  // technically not correct, when name is a sub-string...
  resource->SetName(name.data());
}

} // namespace drv3d_dx12::debug::gpu_postmortem::microsoft
