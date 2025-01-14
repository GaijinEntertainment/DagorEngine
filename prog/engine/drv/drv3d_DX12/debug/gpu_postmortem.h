// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include <pipeline.h>

#include <EASTL/unordered_map.h>
#include <EASTL/variant.h>

#if HAS_GF_AFTERMATH
#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#endif

#include "gpu_postmortem_null_trace.h"
#if HAS_GF_AFTERMATH
#include "gpu_postmortem_nvidia_aftermath.h"
#endif
#include "gpu_postmortem_dagor_trace.h"
#include "gpu_postmortem_microsoft_dred.h"
#if HAS_AMD_GPU_SERVICES
#include "gpu_postmortem_ags_trace.h"
#endif


namespace drv3d_dx12::debug
{
namespace detail
{
template <typename NoT, typename... Ts>
class TracerSet
{
public:
  using StorageType = eastl::variant<NoT, Ts...>;
  using DefaultType = NoT;

  template <typename... Is>
  static bool load(StorageType &store, Is &&...is)
  {
    for (auto loader : {try_load<Ts, Is...>...})
    {
      if (loader(is..., store))
      {
        return true;
      }
    }

    store = DefaultType{};
    return false;
  }

  template <typename... Is>
  static bool on_device_setup(StorageType &store, ID3D12Device *device, Is &&...is)
  {
    if (eastl::visit([&](auto &tracer) { return tracer.onDeviceSetup(device, is...); }, store))
    {
      return true;
    }

    using LoaderEntry = bool (*)(Is && ..., StorageType &);
    // Note that entry 0 in the variant i NoT and so we have to adjust for that.
    static LoaderEntry loaderTable[sizeof...(Ts)] = {try_load<Ts, Is...>...};
    auto index = store.index();
    // We can imply start with index adjusted as it will index to loader of the next one in the list.
    for (; index < sizeof...(Ts); ++index)
    {
      auto loader = loaderTable[index];
      if (loader(is..., store))
      {
        if (eastl::visit([&](auto &tracer) { return tracer.onDeviceSetup(device, is...); }, store))
        {
          return true;
        }
      }
    }

    store = DefaultType{};
    return false;
  }

private:
  template <typename T, typename... Is>
  static bool try_load(Is &&...is, StorageType &store)
  {
    return T::load(is..., store);
  }
};
} // namespace detail

class GpuPostmortem
{
  using TracerTableType = detail::TracerSet< //
    gpu_postmortem::NullTrace,
#if HAS_GF_AFTERMATH
    gpu_postmortem::nvidia::Aftermath,
#endif
#if HAS_AMD_GPU_SERVICES
    gpu_postmortem::ags::AgsTrace,
#endif
    gpu_postmortem::microsoft::DeviceRemovedExtendedData, //
    gpu_postmortem::dagor::Trace>;

  TracerTableType::StorageType tracer = TracerTableType::DefaultType{};

public:
  void setup(const Configuration &config, const Direct3D12Enviroment &d3d_env) { TracerTableType::load(tracer, config, d3d_env); }
  void setupDevice(ID3D12Device *device, const Configuration &config, const Direct3D12Enviroment &d3d_env)
  {
    TracerTableType::on_device_setup(tracer, device, config, d3d_env);
  }
  void teardown() { tracer = gpu_postmortem::NullTrace{}; }
  void onDeviceShutdown()
  {
    eastl::visit([=](auto &tracer) { tracer.onDeviceShutdown(); }, tracer);
  }
  void beginCommandBuffer(ID3D12Device3 *device, ID3D12GraphicsCommandList *cmd)
  {
    // This if isAnyActive makes this a bit cheaper to execute when no trace instance is active.
    if (isAnyActive())
    {
      eastl::visit([=](auto &tracer) { tracer.beginCommandBuffer(device, cmd); }, tracer);
    }
  }
  void endCommandBuffer(ID3D12GraphicsCommandList *cmd)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tracer) { tracer.endCommandBuffer(cmd); }, tracer);
    }
  }
  void beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text, const eastl::string &full_path)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tracer) { tracer.beginEvent(cmd, text, full_path); }, tracer);
    }
  }
  void endEvent(ID3D12GraphicsCommandList *cmd, const eastl::string &full_path)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tracer) { tracer.endEvent(cmd, full_path); }, tracer);
    }
  }
  void marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tracer) { tracer.marker(cmd, text); }, tracer);
    }
  }
  void draw(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
  {
    if (isAnyActive())
    {
      eastl::visit(
        [&](auto &tracer) {
          tracer.draw(debug_info, cmd, vs, ps, pipeline_base, pipeline, count, instance_count, start, first_instance, topology);
        },
        tracer);
    }
  }
  void drawIndexed(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t index_start, int32_t vertex_base, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
  {
    if (isAnyActive())
    {
      eastl::visit(
        [&](auto &tracer) {
          tracer.drawIndexed(debug_info, cmd, vs, ps, pipeline_base, pipeline, count, instance_count, index_start, vertex_base,
            first_instance, topology);
        },
        tracer);
    }
  }
  void drawIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer)
  {
    if (isAnyActive())
    {
      eastl::visit([&](auto &tracer) { tracer.drawIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, buffer); }, tracer);
    }
  }
  void drawIndexedIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer)
  {
    if (isAnyActive())
    {
      eastl::visit([&](auto &tracer) { tracer.drawIndexedIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, buffer); },
        tracer);
    }
  }
  void dispatchIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &state,
    ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
  {
    if (isAnyActive())
    {
      eastl::visit([&](auto &tracer) { tracer.dispatchIndirect(debug_info, cmd, state, pipeline, buffer); }, tracer);
    }
  }
  void dispatch(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &stage,
    ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
  {
    if (isAnyActive())
    {
      eastl::visit([&](auto &tracer) { tracer.dispatch(debug_info, cmd, stage, pipeline, x, y, z); }, tracer);
    }
  }
  void dispatchMesh(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t x, uint32_t y, uint32_t z)
  {
    if (isAnyActive())
    {
      eastl::visit([&](auto &tracer) { tracer.dispatchMesh(debug_info, cmd, vs, ps, pipeline_base, pipeline, x, y, z); }, tracer);
    }
  }
  void dispatchMeshIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
  {
    if (isAnyActive())
    {
      eastl::visit(
        [&](auto &tracer) { tracer.dispatchMeshIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, args, count, max_count); },
        tracer);
    }
  }
  void blit(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd)
  {
    if (isAnyActive())
    {
      eastl::visit([&](auto &tracer) { tracer.blit(debug_info, cmd); }, tracer);
    }
  }

#if D3D_HAS_RAY_TRACING
  void dispatchRays(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
  {
    if (isAnyActive())
    {
      eastl::visit([&](auto &tracer) { tracer.dispatchRays(debug_info, cmd, dispatch_parameters, rbt, rdp); }, tracer);
    }
  }

  void dispatchRaysIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
  {
    if (isAnyActive())
    {
      eastl::visit([&](auto &tracer) { tracer.dispatchRaysIndirect(debug_info, cmd, dispatch_parameters, rbt, rdip); }, tracer);
    }
  }
#endif

  void onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &reporter)
  {
    eastl::visit([&](auto &tracer) { tracer.onDeviceRemoved(device, reason, reporter); }, tracer);
  }
  bool isAnyActive() const { return !eastl::holds_alternative<gpu_postmortem::NullTrace>(tracer); }
  template <typename T>
  bool isActive() const
  {
    return eastl::holds_alternative<T>(tracer);
  }
  bool tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr)
  {
    return eastl::visit([=](auto &tracer) { return tracer.tryCreateDevice(adapter, uuid, minimum_feature_level, ptr); }, tracer);
  }
  bool sendGPUCrashDump(const char *type, const void *data, uintptr_t size)
  {
    return eastl::visit([=](auto &tracer) { return tracer.sendGPUCrashDump(type, data, size); }, tracer);
  }
};
} // namespace drv3d_dx12::debug
