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
    PFN_GFSDK_Aftermath_DX12_RegisterResource registerResource = nullptr;
    PFN_GFSDK_Aftermath_DX12_UnregisterResource unregisterResource = nullptr;
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
  HRESULT lastError = S_OK;
  WinCritSec crashWriteMutex;

  static inline struct Proxy
  {
    Aftermath *object;
  } proxy{nullptr};

  static LibPointer try_load_library();
  static ApiTable try_load_api(HMODULE module);
  bool tryEnableDumps();

  void waitForDump() const;

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
  Aftermath(LibPointer &&lib, const ApiTable &table) : library{eastl::move(lib)}, api{table}
  {
    G_ASSERT(!proxy.object);
    proxy.object = this;
  }
  ~Aftermath();

  void configure();
  void beginCommandBuffer(D3DDevice *device, D3DGraphicsCommandList *cmd);
  void endCommandBuffer(D3DGraphicsCommandList *cmd);
  void beginEvent(D3DGraphicsCommandList *cmd, eastl::span<const char> text, const eastl::string &full_path);
  void endEvent(D3DGraphicsCommandList *cmd, const eastl::string &full_path);
  void marker(D3DGraphicsCommandList *cmd, eastl::span<const char> text);
  void onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &reporter);
  bool sendGPUCrashDump(const char *type, const void *data, uintptr_t size);
  void onDeviceShutdown();
  bool onDeviceSetup(D3DDevice *device, const Configuration &config, const Direct3D12Enviroment &d3d_env);
  template <typename K>
  bool onDeviceSetup(D3DDevice *device, const Configuration &config, const Direct3D12Enviroment &env, const K &)
  {
    return onDeviceSetup(device, config, env);
  }
  template <typename... Ts>
  auto onDeviceCreated(Ts &&...ts)
  {
    return onDeviceSetup(eastl::forward<Ts>(ts)...);
  }

  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **, HLSLVendorExtensions &) { return false; }

  template <typename T>
  static bool load(const Configuration &config, const Direct3D12Enviroment &, T &&target)
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

  template <typename T, typename K>
  static bool load(const Configuration &config, const Direct3D12Enviroment &e, const K &, T &&target)
  {
    return load(config, e, target);
  }

  void nameResource(ID3D12Resource *, eastl::string_view);
  void nameResource(ID3D12Resource *, eastl::wstring_view);
};
} // namespace debug::gpu_postmortem::nvidia
} // namespace drv3d_dx12