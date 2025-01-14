// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include "command_list_storage.h"
#include "configuration.h"
#include <driver.h>
#include <pipeline.h>

#include <EASTL/span.h>
#include <osApiWrappers/dag_critSec.h>
#include <winapi_helpers.h>

// These headers are not self-contained and need to be included after driver.h
#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>


namespace drv3d_dx12
{
class BasePipeline;
class ComputePipeline;
class PipelineVariant;
struct BufferResourceReferenceAndOffset;
struct Direct3D12Enviroment;
struct PipelineStageStateBase;

namespace debug::gpu_postmortem::nvidia
{
class Aftermath
{
  struct ApiTable
  {
    // Also indicates if API is available or not
    PFN_GFSDK_Aftermath_DX12_Initialize initialize = nullptr;
    PFN_GFSDK_Aftermath_DX12_CreateContextHandle createContextHandle = nullptr;
    PFN_GFSDK_Aftermath_ReleaseContextHandle releaseContextHandle = nullptr;
    PFN_GFSDK_Aftermath_SetEventMarker setEventMarker = nullptr;
    PFN_GFSDK_Aftermath_GetDeviceStatus getDeviceStatus = nullptr;

    PFN_GFSDK_Aftermath_EnableGpuCrashDumps enableGpuCrashDumps = nullptr;
    PFN_GFSDK_Aftermath_DisableGpuCrashDumps disableGpuCrashDumps = nullptr;
    PFN_GFSDK_Aftermath_GetShaderDebugInfoIdentifier getShaderDebugInfoIdentifier = nullptr;
    PFN_GFSDK_Aftermath_GetCrashDumpStatus getCrashDumpStatus = nullptr;

    struct
    {
      PFN_GFSDK_Aftermath_GpuCrashDump_CreateDecoder createDecoder = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_DestroyDecoder destroyDecoder = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetBaseInfo getBaseInfo = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetDeviceInfo getDeviceInfo = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetSystemInfo getSystemInfo = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetGpuInfoCount getGPUInfoCount = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetGpuInfo getGPUInfo = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo getPageFaultInfo = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetPageFaultResourceInfo getPageFaultResourceInfo = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfoCount getActiveShadersInfoCount = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfo getActiveShadersInfo = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfoCount getEventMarkersInfoCount = nullptr;
      PFN_GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfo getEventMarkersInfo = nullptr;
    } crashDump;

    inline explicit operator bool() const { return nullptr != initialize; }
  };
  struct ContextDeletor
  {
    using pointer = GFSDK_Aftermath_ContextHandle;
    PFN_GFSDK_Aftermath_ReleaseContextHandle releaseContextHandle = nullptr;

    void operator()(pointer handle)
    {
      if (releaseContextHandle && handle)
      {
        releaseContextHandle(handle);
      }
    }
  };
  using ContextPointer = eastl::unique_ptr<GFSDK_Aftermath_ContextHandle, ContextDeletor>;
  LibPointer library;
  ApiTable api;
  CommandListStorage<ContextPointer> commandListTable;
  WinCritSec crashWriteMutex;

  static LibPointer try_load_library();
  static ApiTable try_load_api(HMODULE module);
  bool tryEnableDumps();

  void onCrashDumpGenerate(const void *dump, const uint32_t size, bool manually_send);
  void onShaderDebugInfo(const void *dump, const uint32_t size);
  void onCrashDumpDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_value);
  void onResolveMarkerCallback(const void *marker_id, uint32_t marker_id_size, void **resolved_marker_data, uint32_t *marker_size);

  static void GFSDK_AFTERMATH_CALL onCrashDumpGenerateProxy(const void *dump, const uint32_t size, void *self);
  static void GFSDK_AFTERMATH_CALL onShaderDebugInfoProxy(const void *dump, const uint32_t size, void *self);
  static void GFSDK_AFTERMATH_CALL onCrashDumpDescriptionProxy(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_value, void *self);
  static void GFSDK_AFTERMATH_CALL onResolveMarkerCallbackProxy(const void *marker_id, uint32_t marker_id_size, void *self,
    void **resolved_marker_data, uint32_t *marker_size);

public:
  // Have to delete move constructor, otherwise compiler / templated stuff of variant tries to be smart and results in compile errors.
  Aftermath(Aftermath &&) = delete;
  Aftermath &operator=(Aftermath &&) = delete;
  Aftermath(LibPointer &&lib, const ApiTable &table) : library{eastl::move(lib)}, api{table} {}
  ~Aftermath()
  {
    if (api.disableGpuCrashDumps)
    {
      logdbg("DX12: Shutting down NVIDIA Aftermath API");
      api.disableGpuCrashDumps();
    }
  }

  void configure();
  void beginCommandBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void endCommandBuffer(ID3D12GraphicsCommandList *cmd);
  void beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text, const eastl::string &full_path);
  void endEvent(ID3D12GraphicsCommandList *cmd, const eastl::string &full_path);
  void marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text);
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
  bool sendGPUCrashDump(const char *type, const void *data, uintptr_t size);
  void onDeviceShutdown();
  bool onDeviceSetup(ID3D12Device *device, const Configuration &config, const Direct3D12Enviroment &d3d_env);

  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **) { return false; }

  template <typename T>
  static bool load(const Configuration &config, const Direct3D12Enviroment &, T &target)
  {
    if (!config.enableAftermath)
    {
      logdbg("DX12: NVIDIA Aftermath is disabled by configuration...");
      return false;
    }
    logdbg("DX12: Loading NVIDIA Aftermath library...");
    auto lib = try_load_library();
    if (!lib)
    {
      logdbg("DX12: ...failed");
      return false;
    }

    logdbg("DX12: ...loading NVIDIA Aftermath API...");
    auto api = try_load_api(lib.get());
    if (!api)
    {
      logdbg("DX12: ...failed");
      return false;
    }

    // This is a bit awkward, we preliminary create the object and indicate
    // failure when dump init failed. Follow up init will then destruct the
    // Aftermath module.
    // This is done as the dump functions take a context pointer, we could
    // either allocate a pointer from the heap or use the pointer of this.
    auto &tracer = target.template emplace<Aftermath>(eastl::move(lib), api);
    if (!tracer.tryEnableDumps())
    {
      return false;
    }
    logdbg("DX12: ...using NVIDIA Aftermath API for GPU postmortem trace");
    return true;
  }
};
} // namespace debug::gpu_postmortem::nvidia
} // namespace drv3d_dx12