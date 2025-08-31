// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "command_list_storage.h"
#include "command_list_trace.h"
#include "command_list_trace_recorder.h"

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

public:
  // Have to delete move constructor, otherwise compiler / templated stuff of variant tries to be smart and results in compile errors.
  DeviceRemovedExtendedData(DeviceRemovedExtendedData &&) = delete;
  DeviceRemovedExtendedData &operator=(DeviceRemovedExtendedData &&) = delete;
  DeviceRemovedExtendedData() = default;
  ~DeviceRemovedExtendedData() { logdbg("DX12: Shutting down DRED"); }
  constexpr void configure() {}
  void beginCommandBuffer(D3DDevice *device, D3DGraphicsCommandList *cmd);
  void endCommandBuffer(D3DGraphicsCommandList *);
  void beginEvent(D3DGraphicsCommandList *, eastl::span<const char>, const eastl::string &);
  void endEvent(D3DGraphicsCommandList *, const eastl::string &);
  void marker(D3DGraphicsCommandList *, eastl::span<const char>);
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
  template <typename... Ts>
  constexpr bool onDeviceSetup(Ts &&...)
  {
    return true;
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
    if (!try_load(config, d3d_env))
    {
      return false;
    }
    target.template emplace<DeviceRemovedExtendedData>();
    return false;
  }

  template <typename T, typename K>
  static bool load(const Configuration &config, const Direct3D12Enviroment &e, const K &, T &&target)
  {
    return load(config, e, target);
  }

  void nameResource(ID3D12Resource *resource, eastl::string_view name);
  void nameResource(ID3D12Resource *resource, eastl::wstring_view name);
};
} // namespace gpu_postmortem::microsoft
} // namespace debug
} // namespace drv3d_dx12
