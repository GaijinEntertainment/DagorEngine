// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "command_list_storage.h"
#include "command_list_trace.h"
#include "command_list_trace_recorder.h"


inline const char *to_string(D3D12_DRED_ALLOCATION_TYPE type)
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

inline void report_alternate_name(const char *fmt, const char *a_str, const wchar_t *w_str)
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

inline void report_allocation_info(const D3D12_DRED_ALLOCATION_NODE *node)
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

inline void report_page_fault(ID3D12DeviceRemovedExtendedData *dred)
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
union Configuration;
namespace gpu_postmortem::microsoft
{
class DeviceRemovedExtendedData
{
  static bool try_load(const Configuration &config, const Direct3D12Enviroment &d3d_env);
  void walkBreadcumbs(ID3D12DeviceRemovedExtendedData *dred, call_stack::Reporter &reporter);
  void walkBreadcumbs(const D3D12_AUTO_BREADCRUMB_NODE *node, call_stack::Reporter &reporter);

  // No extra stuff, we use OP ids
  struct TraceConfig
  {
    struct OperationTraceBase
    {};
    using OperationTraceData = OperationTraceBase;
    struct EventTraceBase
    {};
    using EventTraceData = EventTraceBase;
  };

  using TraceRecodingType = CommandListTrace<TraceConfig>;

  CommandListStorage<TraceRecodingType> commandListTable;

public:
  // Have to delete move constructor, otherwise compiler / templated stuff of variant tries to be smart and results in compile errors.
  DeviceRemovedExtendedData(DeviceRemovedExtendedData &&) = delete;
  DeviceRemovedExtendedData &operator=(DeviceRemovedExtendedData &&) = delete;
  DeviceRemovedExtendedData() = default;
  ~DeviceRemovedExtendedData() { logdbg("DX12: Shutting down DRED"); }
  constexpr void configure() {}
  void beginCommandBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void endCommandBuffer(ID3D12GraphicsCommandList *);
  void beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>, const eastl::string &);
  void endEvent(ID3D12GraphicsCommandList *, const eastl::string &);
  void marker(ID3D12GraphicsCommandList *, eastl::span<const char>);
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
  void dispatch(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &stage,
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
  void onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &reporter);
  constexpr bool sendGPUCrashDump(const char *, const void *, uintptr_t) { return false; }
  void onDeviceShutdown();
  constexpr bool onDeviceSetup(ID3D12Device *, const Configuration &, const Direct3D12Enviroment &) { return true; }

  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **) { return false; }

  template <typename T>
  static bool load(const Configuration &config, const Direct3D12Enviroment &d3d_env, T &target)
  {
    if (try_load(config, d3d_env))
    {
      target.template emplace<DeviceRemovedExtendedData>();
      return true;
    }
    return false;
  }
};
} // namespace gpu_postmortem::microsoft
} // namespace debug
} // namespace drv3d_dx12
