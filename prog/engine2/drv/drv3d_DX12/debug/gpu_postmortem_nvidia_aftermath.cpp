#include "device.h"

#include <debug/dag_logSys.h>
#include <json/json.h>
#include <eventLog/eventLog.h>
#include <breakpad/binder.h>
#include <util/dag_watchdog.h>

using namespace drv3d_dx12;

namespace
{
const char *to_string(GFSDK_Aftermath_Result result)
{
  switch (result)
  {
#define TS(name) \
  case GFSDK_Aftermath_Result_##name: return #name
    TS(Success);
    TS(NotAvailable);
    TS(Fail);
    TS(FAIL_VersionMismatch);
    TS(FAIL_NotInitialized);
    TS(FAIL_InvalidAdapter);
    TS(FAIL_InvalidParameter);
    TS(FAIL_Unknown);
    TS(FAIL_ApiError);
    TS(FAIL_NvApiIncompatible);
    TS(FAIL_GettingContextDataWithNewCommandList);
    TS(FAIL_AlreadyInitialized);
    TS(FAIL_D3DDebugLayerNotCompatible);
    TS(FAIL_DriverInitFailed);
    TS(FAIL_DriverVersionNotSupported);
    TS(FAIL_OutOfMemory);
    TS(FAIL_GetDataOnBundle);
    TS(FAIL_GetDataOnDeferredContext);
    TS(FAIL_FeatureNotEnabled);
    TS(FAIL_NoResourcesRegistered);
    TS(FAIL_ThisResourceNeverRegistered);
    TS(FAIL_NotSupportedInUWP);
    TS(FAIL_D3dDllNotSupported);
    TS(FAIL_D3dDllInterceptionNotSupported);
    TS(FAIL_Disabled);
#undef TS
  }
  return "<unknown>";
}

const char *to_string(GFSDK_Aftermath_Device_Status status)
{
  switch (status)
  {
    default: return "<unknown>";
    case GFSDK_Aftermath_Device_Status_Active: return "Active";
    case GFSDK_Aftermath_Device_Status_Timeout: return "Timeout";
    case GFSDK_Aftermath_Device_Status_OutOfMemory: return "OutOfMemory";
    case GFSDK_Aftermath_Device_Status_PageFault: return "PageFault";
    case GFSDK_Aftermath_Device_Status_Stopped: return "Stopped";
    case GFSDK_Aftermath_Device_Status_Reset: return "Reset";
    case GFSDK_Aftermath_Device_Status_Unknown: return "Unknown";
    case GFSDK_Aftermath_Device_Status_DmaFault: return "DMAFault";
  }
}

const char *to_string(GFSDK_Aftermath_Context_Status status)
{
  switch (status)
  {
    default: return "<unknown>";
    case GFSDK_Aftermath_Context_Status_NotStarted: return "NotStarted";
    case GFSDK_Aftermath_Context_Status_Executing: return "Executing";
    case GFSDK_Aftermath_Context_Status_Finished: return "Finished";
    case GFSDK_Aftermath_Context_Status_Invalid: return "Invalid";
  }
}

const char *to_string(GFSDK_Aftermath_Context_Type type)
{
  switch (type)
  {
    default: return "<unknown>";
    case GFSDK_Aftermath_Context_Type_Invalid: return "Invalid";
    case GFSDK_Aftermath_Context_Type_Immediate: return "Immediate";
    case GFSDK_Aftermath_Context_Type_CommandList: return "CommandList";
    case GFSDK_Aftermath_Context_Type_Bundle: return "Bundle";
    case GFSDK_Aftermath_Context_Type_CommandQueue: return "CommandQueue";
  }
}

const char *to_string(GFSDK_Aftermath_Engine engine)
{
  switch (engine)
  {
    default: return "<unknown>";
    case GFSDK_Aftermath_Engine_Unknown: return "Unknown";
    case GFSDK_Aftermath_Engine_Graphics: return "Graphics";
    case GFSDK_Aftermath_Engine_Display: return "Display";
    case GFSDK_Aftermath_Engine_CopyEngine: return "CopyEngine";
    case GFSDK_Aftermath_Engine_VideoDecoder: return "VideoDecoder";
    case GFSDK_Aftermath_Engine_VideoEncoder: return "VideoEncoder";
    case GFSDK_Aftermath_Engine_Other: return "Other";
  }
}

const char *to_string(GFSDK_Aftermath_FaultType fault_type)
{
  switch (fault_type)
  {
    default: return "<unknown>";
    case GFSDK_Aftermath_FaultType_Unknown: return "Unknown";
    case GFSDK_Aftermath_FaultType_AddressTranslationError: return "AddressTranslationError";
    case GFSDK_Aftermath_FaultType_IllegalAccessError: return "IllegalAccessError";
  }
}

const char *to_string(GFSDK_Aftermath_AccessType access_type)
{
  switch (access_type)
  {
    default: return "<unknown>";
    case GFSDK_Aftermath_AccessType_Unknown: return "Unknown";
    case GFSDK_Aftermath_AccessType_Read: return "Read";
    case GFSDK_Aftermath_AccessType_Write: return "Write";
    case GFSDK_Aftermath_AccessType_Atomic: return "Atomic";
  }
}

const char *to_string(GFSDK_Aftermath_Client client)
{
  switch (client)
  {
    default: return "<unknown>";
    case GFSDK_Aftermath_Client_Unknown: return "Unknown";
    case GFSDK_Aftermath_Client_HostInterface: return "HostInterface";
    case GFSDK_Aftermath_Client_FrontEnd: return "FrontEnd";
    case GFSDK_Aftermath_Client_PrimitiveDistributor: return "PrimitiveDistributor";
    case GFSDK_Aftermath_Client_GraphicsProcessingCluster: return "GraphicsProcessingCluster";
    case GFSDK_Aftermath_Client_PolymorphEngine: return "PolymorphEngine";
    case GFSDK_Aftermath_Client_RasterEngine: return "RasterEngine";
    case GFSDK_Aftermath_Client_Rasterizer2D: return "Rasterizer2D";
    case GFSDK_Aftermath_Client_RenderOutputUnit: return "RenderOutputUnit";
    case GFSDK_Aftermath_Client_TextureProcessingCluster: return "TextureProcessingCluster";
    case GFSDK_Aftermath_Client_CopyEngine: return "CopyEngine";
    case GFSDK_Aftermath_Client_VideoDecoder: return "VideoDecoder";
    case GFSDK_Aftermath_Client_VideoEncoder: return "VideoEncoder";
    case GFSDK_Aftermath_Client_Other: return "Other";
  }
}

const char *to_string(GFSDK_Aftermath_ShaderType shader_type)
{
  switch (shader_type)
  {
    default: return "<unknown>";
    case GFSDK_Aftermath_ShaderType_Unknown: return "Unknown";
    case GFSDK_Aftermath_ShaderType_Vertex: return "Vertex";
    case GFSDK_Aftermath_ShaderType_Hull: return "Hull";
    case GFSDK_Aftermath_ShaderType_Domain: return "Domain";
    case GFSDK_Aftermath_ShaderType_Geometry: return "Geometry";
    case GFSDK_Aftermath_ShaderType_Pixel: return "Pixel";
    case GFSDK_Aftermath_ShaderType_Compute: return "Compute";
    case GFSDK_Aftermath_ShaderType_RayTracing_RayGeneration: return "Raytracing-RayGeneration";
    case GFSDK_Aftermath_ShaderType_RayTracing_Miss: return "Raytracing-Miss";
    case GFSDK_Aftermath_ShaderType_RayTracing_Intersection: return "Raytracing-Intersection";
    case GFSDK_Aftermath_ShaderType_RayTracing_AnyHit: return "Raytracing-AnyHit";
    case GFSDK_Aftermath_ShaderType_RayTracing_ClosestHit: return "Raytracing-ClosestHit";
    case GFSDK_Aftermath_ShaderType_RayTracing_Callable: return "Raytracing-Callable";
    case GFSDK_Aftermath_ShaderType_RayTracing_Internal: return "Raytracing-Internal";
    case GFSDK_Aftermath_ShaderType_Mesh: return "Mesh";
    case GFSDK_Aftermath_ShaderType_Task: return "Task";
  }
}

bool check_aftermath_result(GFSDK_Aftermath_Result result, const char *what)
{
  if (GFSDK_Aftermath_Result_Success == result)
  {
    return true;
  }

  logwarn("DX12: %s returned an error %s", what, to_string(result));
  return false;
}

#define AFTHERMATH_CALL(call) check_aftermath_result(call, #call)

template <typename T>
class AftermathCrashDumpDecoder
{
  T &p;
  GFSDK_Aftermath_GpuCrashDump_Decoder context = nullptr;

public:
  AftermathCrashDumpDecoder(T &ptr, const void *dump, uint32_t size) : p{ptr}
  {
    if (!AFTHERMATH_CALL(p.createDecoder(GFSDK_Aftermath_Version_API, dump, size, &context)))
    {
      context = nullptr;
    }
  }

  ~AftermathCrashDumpDecoder()
  {
    if (context)
    {
      AFTHERMATH_CALL(p.destroyDecoder(context));
    }
  }

  explicit operator bool() const { return nullptr != context; }

  eastl::optional<GFSDK_Aftermath_GpuCrashDump_BaseInfo> getBaseInfo() const
  {
    GFSDK_Aftermath_GpuCrashDump_BaseInfo info{};
    if (AFTHERMATH_CALL(p.getBaseInfo(context, &info)))
    {
      return info;
    }
    else
    {
      return {};
    }
  }

  eastl::optional<GFSDK_Aftermath_GpuCrashDump_DeviceInfo> getDeviceInfo() const
  {
    GFSDK_Aftermath_GpuCrashDump_DeviceInfo info{};
    if (AFTHERMATH_CALL(p.getDeviceInfo(context, &info)))
    {
      return info;
    }
    else
    {
      return {};
    }
  }

  eastl::optional<GFSDK_Aftermath_GpuCrashDump_SystemInfo> getSystemInfo() const
  {
    GFSDK_Aftermath_GpuCrashDump_SystemInfo info{};
    if (AFTHERMATH_CALL(p.getSystemInfo(context, &info)))
    {
      return info;
    }
    else
    {
      return {};
    }
  }

  template <typename T>
  uint32_t forEachGPUInfo(T clb) const
  {
    uint32_t count = 0;
    if (!AFTHERMATH_CALL(p.getGPUInfoCount(context, &count)))
    {
      return 0;
    }
    eastl::vector<GFSDK_Aftermath_GpuCrashDump_GpuInfo> data;
    data.resize(count);
    if (!AFTHERMATH_CALL(p.getGPUInfo(context, count, data.data())))
    {
      return 0;
    }
    for (const auto &info : data)
    {
      clb(&info - data.data(), info);
    }
    return count;
  }

  eastl::optional<GFSDK_Aftermath_GpuCrashDump_PageFaultInfo> getPageFaultInfo() const
  {
    GFSDK_Aftermath_GpuCrashDump_PageFaultInfo info{};
    if (AFTHERMATH_CALL(p.getPageFaultInfo(context, &info)))
    {
      return info;
    }
    else
    {
      return {};
    }
  }

  template <typename T>
  uint32_t forEachEventMarker(T clb) const
  {
    uint32_t count = 0;
    if (!AFTHERMATH_CALL(p.getEventMarkersInfoCount(context, &count)))
    {
      return 0;
    }
    eastl::vector<GFSDK_Aftermath_GpuCrashDump_EventMarkerInfo> data;
    data.resize(count);
    if (!AFTHERMATH_CALL(p.getEventMarkersInfo(context, count, data.data())))
    {
      return 0;
    }
    for (const auto &info : data)
    {
      clb(&info - data.data(), info);
    }
    return count;
  }

  template <typename T>
  uint32_t forEachShaderInfo(T clb) const
  {
    uint32_t count = 0;
    if (!AFTHERMATH_CALL(p.getActiveShadersInfoCount(context, &count)))
    {
      return 0;
    }
    eastl::vector<GFSDK_Aftermath_GpuCrashDump_ShaderInfo> data;
    data.resize(count);
    if (!AFTHERMATH_CALL(p.getActiveShadersInfo(context, count, data.data())))
    {
      return 0;
    }
    for (const auto &info : data)
    {
      clb(&info - data.data(), info);
    }
    return count;
  }
};

template <typename T>
void send_dump_context(T *ctx_ptr, const void *dump, const char *ext, size_t size, time_t time_stamp, bool manually_send)
{
  debug("DX12: Preparing to send GPU dump...");
  Json::Value meta;
  // we may support vulkan in the future, so reporting which api the error occurred is useful
  meta["api"] = "DX12";
  // Should any other vendor support stuff like this, we need to know which is which
  meta["vendor"] = "NVIDIA";
  // Helpful should there be different types of dumps
  meta["file-ext"] = ext;
  // Vendor independent timestamp
  meta["timestamp"] = time_stamp;
  if (manually_send)
  {
    meta["was-manually-send"] = true;
  }
  if (ctx_ptr && *ctx_ptr)
  {
    auto &ctx = *ctx_ptr;
    if (auto baseInfo = ctx.getBaseInfo())
    {
      meta["app-name"] = baseInfo->applicationName;
      meta["creation-date"] = baseInfo->creationDate;
      meta["pid"] = baseInfo->pid;
    }

    if (auto deviceInfo = ctx.getDeviceInfo())
    {
      meta["device-status-id"] = deviceInfo->status;
      meta["device-status"] = to_string(deviceInfo->status);
      meta["was-adapter-reset"] = deviceInfo->adapterReset;
      meta["was-engine-reset"] = deviceInfo->engineReset;

      debug("DX12: Device status %u - %s...", deviceInfo->status, to_string(deviceInfo->status));
      debug("DX12: Was adapter reset %s...", deviceInfo->adapterReset ? "Yes" : "No");
      debug("DX12: Was engine reset %s...", deviceInfo->engineReset ? "Yes" : "No");
    }

    if (auto systemInfo = ctx.getSystemInfo())
    {
      meta["os-version"] = systemInfo->osVersion;
      meta["driver-version-major"] = systemInfo->displayDriver.major;
      meta["driver-version-minor"] = systemInfo->displayDriver.minor;
      char vstring[64];
      _snprintf(vstring, sizeof(vstring), "%d.%02d", systemInfo->displayDriver.major, systemInfo->displayDriver.minor);
      meta["driver-version"] = vstring;

      debug("DX12: OS version %u...", systemInfo->osVersion);
      debug("DX12: Driver version %s...", vstring);
    }

    uint32_t gpuInfoCount = ctx.forEachGPUInfo([&meta](uint32_t index, auto &info) {
      char keyName[64];
      _snprintf(keyName, sizeof(keyName), "gpu-name-%u", index);
      meta[keyName] = info.adapterName;

      _snprintf(keyName, sizeof(keyName), "gpu-generation-name-%u", index);
      meta[keyName] = info.generationName;

      _snprintf(keyName, sizeof(keyName), "gpu-LUID-%u", index);
      meta[keyName] = info.adapterLUID;

      debug("DX12: GPU %u name %s...", index, info.adapterName);
      debug("DX12: GPU %u generation name %s...", index, info.generationName);
      debug("DX12: GPU %u LUID %s...", index, info.adapterLUID);
    });
    meta["gpu-count"] = gpuInfoCount;

    if (auto pageFaultInfo = ctx.getPageFaultInfo())
    {
      meta["page-fault-virtual-gpu-address"] = pageFaultInfo->faultingGpuVA;
      meta["page-fault-type"] = to_string(pageFaultInfo->faultType);
      meta["page-fault-access-type"] = to_string(pageFaultInfo->accessType);
      meta["page-fault-engine"] = to_string(pageFaultInfo->engine);
      meta["page-fault-client"] = to_string(pageFaultInfo->client);

      debug("DX12: GPU page fault address %016X", pageFaultInfo->faultingGpuVA);
      debug("DX12: GPU page fault type %s", to_string(pageFaultInfo->faultType));
      debug("DX12: GPU page fault access type %s", to_string(pageFaultInfo->accessType));
      debug("DX12: GPU page engine %s", to_string(pageFaultInfo->engine));
      debug("DX12: GPU page client %s", to_string(pageFaultInfo->client));
      if (pageFaultInfo->bHasResourceInfo)
      {
        meta["page-fault-resource-virtual-gpu-address"] = pageFaultInfo->resourceInfo.gpuVa;
        meta["page-fault-resource-size"] = pageFaultInfo->resourceInfo.size;
        meta["page-fault-resource-width"] = pageFaultInfo->resourceInfo.width;
        meta["page-fault-resource-height"] = pageFaultInfo->resourceInfo.height;
        meta["page-fault-resource-depth"] = pageFaultInfo->resourceInfo.depth;
        meta["page-fault-resource-mip-levels"] = pageFaultInfo->resourceInfo.mipLevels;
        meta["page-fault-resource-format-id"] = pageFaultInfo->resourceInfo.format;
        meta["page-fault-resource-format"] = dxgi_format_name(static_cast<DXGI_FORMAT>(pageFaultInfo->resourceInfo.format));
        meta["page-fault-resource-is-buffer-heap"] = pageFaultInfo->resourceInfo.bIsBufferHeap;
        meta["page-fault-resource-is-static-texture"] = pageFaultInfo->resourceInfo.bIsStaticTextureHeap;
        meta["page-fault-resource-is-render-target-or-depth-stencil-view"] =
          pageFaultInfo->resourceInfo.bIsRenderTargetOrDepthStencilViewHeap;
        meta["page-fault-resource-is-placed-resource"] = pageFaultInfo->resourceInfo.bPlacedResource;
        meta["page-fault-resource-was-destroyed"] = pageFaultInfo->resourceInfo.bWasDestroyed;
        meta["page-fault-resource-create-destroy-tick-count"] = pageFaultInfo->resourceInfo.createDestroyTickCount;

        debug("DX12: Resource address %016X", pageFaultInfo->resourceInfo.gpuVa);
        debug("DX12: Resource size %u", pageFaultInfo->resourceInfo.size);
        debug("DX12: Resource width %u", pageFaultInfo->resourceInfo.width);
        debug("DX12: Resource height %u", pageFaultInfo->resourceInfo.height);
        debug("DX12: Resource depth %u", pageFaultInfo->resourceInfo.depth);
        debug("DX12: Resource mip levels %u", pageFaultInfo->resourceInfo.mipLevels);
        debug("DX12: Resource format id %u", pageFaultInfo->resourceInfo.format);
        debug("DX12: Resource DXGI format %s", dxgi_format_name(static_cast<DXGI_FORMAT>(pageFaultInfo->resourceInfo.format)));
        debug("DX12: Resource is BufferHeap %s", pageFaultInfo->resourceInfo.bIsBufferHeap ? "Yes" : "NO");
        debug("DX12: Resource is StaticTextureHeap", pageFaultInfo->resourceInfo.bIsStaticTextureHeap ? "Yes" : "No");
        debug("DX12: Resource is RenderTargetViewHeap or DepthStencilViewHeap",
          pageFaultInfo->resourceInfo.bIsRenderTargetOrDepthStencilViewHeap ? "Yes" : "No");
        debug("DX12: Resource is PlacedResource %s", pageFaultInfo->resourceInfo.bPlacedResource ? "Yes" : "No");
        debug("DX12: Resource wasDestroyed %s", pageFaultInfo->resourceInfo.bWasDestroyed ? "Yes" : "No");
        debug("DX12: Resource create destroy tick count", pageFaultInfo->resourceInfo.createDestroyTickCount);
      }
      else
      {
        debug("DX12: No resource info available...");
      }
    }

    uint32_t shaderCount = ctx.forEachShaderInfo([&meta](uint32_t index, auto &info) {
      char keyName[64];
      _snprintf(keyName, sizeof(keyName), "shader-hash-%u", index);
      meta[keyName] = info.shaderHash;

      _snprintf(keyName, sizeof(keyName), "shader-instance-%u", index);
      meta[keyName] = info.shaderInstance;

      _snprintf(keyName, sizeof(keyName), "shader-is-internal-%u", index);
      meta[keyName] = info.isInternal;

      _snprintf(keyName, sizeof(keyName), "shader-type-id-%u", index);
      meta[keyName] = info.shaderType;

      _snprintf(keyName, sizeof(keyName), "shader-type-%u", index);
      meta[keyName] = to_string(info.shaderType);

      debug("DX12: Shader %u hash %016X", index, info.shaderHash);
      debug("DX12: Shader %u instance %u", index, info.shaderInstance);
      debug("DX12: Shader %u is internal %s", index, info.isInternal ? "Yes" : "No");
      debug("DX12: Shader %u shader type %u %u", index, info.shaderType, to_string(info.shaderType));
    });
    meta["shader-count"] = shaderCount;

    uint32_t eventMarkerCount = ctx.forEachEventMarker([&meta](uint32_t index, auto &info) {
      char keyName[64];
      _snprintf(keyName, sizeof(keyName), "event-marker-context-id-%u", index);
      meta[keyName] = info.contextId;

      _snprintf(keyName, sizeof(keyName), "event-marker-context-status-%u", index);
      meta[keyName] = to_string(info.contextStatus);

      _snprintf(keyName, sizeof(keyName), "event-marker-context-type-%u", index);
      meta[keyName] = to_string(info.contextType);

      _snprintf(keyName, sizeof(keyName), "event-marker-%u", index);
      auto cp = (const char *)info.markerData;
      meta[keyName] = {cp, cp + info.markerDataSize};

      debug("DX12: Event marker %u context id %u", index, info.contextId);
      debug("DX12: Event marker %u context status %s", index, to_string(info.contextStatus));
      debug("DX12: Event marker %u '%s'", index, eastl::string_view(cp, info.markerDataSize));
    });
    meta["event-marker-count"] = eventMarkerCount;
  }

  meta["content-type"] = "application/octet-stream";
  event_log::send_http_instant("gpu_crash_dump", dump, size, &meta);
}
} // namespace

LibPointer debug::gpu_postmortem::nvidia::Aftermath::try_load_library()
{
  static const char libName[] =
#if _TARGET_64BIT
    "GFSDK_2022.2.0.22145\\GFSDK_Aftermath_Lib.x64.dll";
#else
    "GFSDK_2022.2.0.22145\\GFSDK_Aftermath_Lib.x86.dll";
#endif
  debug("DX12: ...loading '%s'...", libName);
  return {LoadLibraryA(libName), {}};
}

debug::gpu_postmortem::nvidia::Aftermath::ApiTable debug::gpu_postmortem::nvidia::Aftermath::try_load_api(HMODULE module)
{
  ApiTable table;
#define GPA(var, name)                                                                      \
  reinterpret_cast<FARPROC &>(table.var) = GetProcAddress(module, "GFSDK_Aftermath_" name); \
  if (nullptr == table.var)                                                                 \
  {                                                                                         \
    return table;                                                                           \
  }
#define GPA_CD(var, name)                                                                                          \
  reinterpret_cast<FARPROC &>(table.crashDump.var) = GetProcAddress(module, "GFSDK_Aftermath_GpuCrashDump_" name); \
  if (nullptr == table.crashDump.var)                                                                              \
  {                                                                                                                \
    return table;                                                                                                  \
  }

  GPA(createContextHandle, "DX12_CreateContextHandle");
  GPA(releaseContextHandle, "ReleaseContextHandle");
  GPA(setEventMarker, "SetEventMarker");
  GPA(getDeviceStatus, "GetDeviceStatus");

  GPA(enableGpuCrashDumps, "EnableGpuCrashDumps");
  GPA(disableGpuCrashDumps, "DisableGpuCrashDumps");
  GPA(getShaderDebugInfoIdentifier, "GetShaderDebugInfoIdentifier");
  GPA(getCrashDumpStatus, "GetCrashDumpStatus");

  GPA_CD(createDecoder, "CreateDecoder");
  GPA_CD(destroyDecoder, "DestroyDecoder");
  GPA_CD(getBaseInfo, "GetBaseInfo");
  GPA_CD(getDeviceInfo, "GetDeviceInfo");
  GPA_CD(getSystemInfo, "GetSystemInfo");
  GPA_CD(getGPUInfoCount, "GetGpuInfoCount");
  GPA_CD(getGPUInfo, "GetGpuInfo");
  GPA_CD(getPageFaultInfo, "GetPageFaultInfo");
  GPA_CD(getActiveShadersInfoCount, "GetActiveShadersInfoCount");
  GPA_CD(getActiveShadersInfo, "GetActiveShadersInfo");
  GPA_CD(getEventMarkersInfoCount, "GetEventMarkersInfoCount");
  GPA_CD(getEventMarkersInfo, "GetEventMarkersInfo");

  // Do the last so if this passes the API table is considered valid (and we can not start up the API anyways)
  GPA(initialize, "DX12_Initialize");

#undef GPA

  return table;
}

bool debug::gpu_postmortem::nvidia::Aftermath::tryEnableDumps()
{
  debug("DX12: ...enabling GPU crash dumps...");
  // let Aftermath store data that might be needed for dumps
  auto result = api.enableGpuCrashDumps(GFSDK_Aftermath_Version_API, GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
    GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks, &onCrashDumpGenerateProxy, &onShaderDebugInfoProxy,
    &onCrashDumpDescriptionProxy, &onResolveMarkerCallbackProxy, this);
  if (GFSDK_Aftermath_Result_Success != result)
  {
    debug("DX12: ...failed with %s...", to_string(result));
    // indicated dumps are not active
    api.disableGpuCrashDumps = nullptr;
    return false;
  }
  return true;
}

void debug::gpu_postmortem::nvidia::Aftermath::onCrashDumpGenerate(const void *dump, const uint32_t size, bool manually_send)
{
  auto rawTime = time(nullptr);
  auto time = localtime(&rawTime);
  char path[DAGOR_MAX_PATH];
  // same format as regular program crash dumps, just with .nv-gpudmp extension which can be opened with NVIDIA Nsight Graphics
  _snprintf(path, sizeof(path), "%scrashDump-%.02d.%.02d.%.02d.nv-gpudmp", get_log_directory(), time->tm_hour, time->tm_min,
    time->tm_sec);
  debug("DX12: Trying to write GPU crash dump to '%s'...", path);
  auto file = fopen(path, "wb");
  if (!file)
  {
    logwarn("DX12: ...failed");
    return;
  }
  fwrite(dump, 1, size, file);
  fclose(file);
  debug("DX12: ...finished");

  WinAutoLock lock{crashWriteMutex};
  debug("DX12: Added GPU crash dump to crash report file...");
  breakpad::add_file_to_report(path);

  debug("DX12: Preparing to send GPU dump...");
  AftermathCrashDumpDecoder<decltype(api.crashDump)> crashDumpDecoder{api.crashDump, dump, size};
  send_dump_context(&crashDumpDecoder, dump, "nv-gpudmp", size, rawTime, manually_send);
}

void debug::gpu_postmortem::nvidia::Aftermath::onShaderDebugInfo(const void *dump, const uint32_t size)
{
  GFSDK_Aftermath_ShaderDebugInfoIdentifier ident{};
  api.getShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, dump, size, &ident);
  auto rawTime = time(nullptr);
  char path[DAGOR_MAX_PATH];
  _snprintf(path, sizeof(path), "%sshader-%016I64x-%016I64x.nvdbg", get_log_directory(), ident.id[0], ident.id[1]);
  debug("DX12: Trying to write shader debug info to '%s'...", path);
  auto file = fopen(path, "wb");
  if (!file)
  {
    logwarn("DX12: ...failed");
    return;
  }
  fwrite(dump, 1, size, file);
  fclose(file);
  debug("DX12: ...finished");

  WinAutoLock lock{crashWriteMutex};
  debug("DX12: Added shader debug info to crash report file...");
  breakpad::add_file_to_report(path);

  debug("DX12: Preparing to send shader debug info...");
  send_dump_context<AftermathCrashDumpDecoder<decltype(api.crashDump)>>(nullptr, dump, "nvdbg", size, rawTime, false);
}

void debug::gpu_postmortem::nvidia::Aftermath::onCrashDumpDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_value)
{
  if (auto gameName = ::dgs_get_settings()->getStr("gameName", nullptr))
  {
    add_value(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, gameName);
  }
  // There is no uniform way to get either app name, app version and such ...
  /*
  GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion = 0x00000002,
  GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined = 0x00010000,
  */
}

void debug::gpu_postmortem::nvidia::Aftermath::onResolveMarkerCallback(const void *marker_id, void **resolved_marker_data,
  uint32_t *marker_size)
{
  G_UNUSED(marker_id);
  // For maximum CPU performance, use GFSDK_Aftermath_SetEventMarker() with dataSize=0.
  // This instructs Aftermath not to allocate and copy off memory internally, relying on
  // the application to resolve marker pointers in this callback.
  // Important: the pointer passed back via resolved_marker_data must remain valid after this function
  // returns
  static const char marker_data[] = "Application resolved markers not implemented";
  *resolved_marker_data = (void *)marker_data;
  *marker_size = sizeof(marker_data);
}

void GFSDK_AFTERMATH_CALL debug::gpu_postmortem::nvidia::Aftermath::onCrashDumpGenerateProxy(const void *dump, const uint32_t size,
  void *self)
{
  reinterpret_cast<debug::gpu_postmortem::nvidia::Aftermath *>(self)->onCrashDumpGenerate(dump, size, false);
}

void GFSDK_AFTERMATH_CALL debug::gpu_postmortem::nvidia::Aftermath::onShaderDebugInfoProxy(const void *dump, const uint32_t size,
  void *self)
{
  reinterpret_cast<debug::gpu_postmortem::nvidia::Aftermath *>(self)->onShaderDebugInfo(dump, size);
}

void GFSDK_AFTERMATH_CALL debug::gpu_postmortem::nvidia::Aftermath::onCrashDumpDescriptionProxy(
  PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_value, void *self)
{
  reinterpret_cast<debug::gpu_postmortem::nvidia::Aftermath *>(self)->onCrashDumpDescription(add_value);
}

void GFSDK_AFTERMATH_CALL debug::gpu_postmortem::nvidia::Aftermath::onResolveMarkerCallbackProxy(const void *marker_id, void *self,
  void **resolved_marker_data, uint32_t *marker_size)
{
  reinterpret_cast<debug::gpu_postmortem::nvidia::Aftermath *>(self)->onResolveMarkerCallback(marker_id, resolved_marker_data,
    marker_size);
}

void debug::gpu_postmortem::nvidia::Aftermath::configure() {}

void debug::gpu_postmortem::nvidia::Aftermath::beginCommandBuffer(ID3D12Device *, ID3D12GraphicsCommandList *cmd)
{
  commandListTable.beginListWithCallback(cmd, [=](auto cmd) {
    GFSDK_Aftermath_ContextHandle handle = nullptr;
    api.createContextHandle(cmd, &handle);
    return ContextPointer(handle, {api.releaseContextHandle});
  });
}

void debug::gpu_postmortem::nvidia::Aftermath::endCommandBuffer(ID3D12GraphicsCommandList *cmd) { commandListTable.endList(cmd); }

void debug::gpu_postmortem::nvidia::Aftermath::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char>,
  eastl::span<const char> full_path)
{
  auto &context = commandListTable.getList(cmd);
  api.setEventMarker(context.get(), full_path.data(), full_path.size());
}

void debug::gpu_postmortem::nvidia::Aftermath::endEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> full_path)
{
  auto &context = commandListTable.getList(cmd);
  api.setEventMarker(context.get(), full_path.data(), full_path.size());
}

void debug::gpu_postmortem::nvidia::Aftermath::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  auto &context = commandListTable.getList(cmd);
  api.setEventMarker(context.get(), text.data(), text.size());
}

void debug::gpu_postmortem::nvidia::Aftermath::draw(const call_stack::CommandData &, D3DGraphicsCommandList *,
  const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t,
  uint32_t, D3D12_PRIMITIVE_TOPOLOGY)
{}

void debug::gpu_postmortem::nvidia::Aftermath::drawIndexed(const call_stack::CommandData &, D3DGraphicsCommandList *,
  const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t,
  int32_t, uint32_t, D3D12_PRIMITIVE_TOPOLOGY)
{}

void debug::gpu_postmortem::nvidia::Aftermath::drawIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *,
  const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, BufferResourceReferenceAndOffset)
{}

void debug::gpu_postmortem::nvidia::Aftermath::drawIndexedIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *,
  const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, BufferResourceReferenceAndOffset)
{}

void debug::gpu_postmortem::nvidia::Aftermath::dispatchIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *,
  const PipelineStageStateBase &, ComputePipeline &, BufferResourceReferenceAndOffset)
{}

void debug::gpu_postmortem::nvidia::Aftermath::dispatch(const call_stack::CommandData &, D3DGraphicsCommandList *,
  const PipelineStageStateBase &, ComputePipeline &, uint32_t, uint32_t, uint32_t)
{}

void debug::gpu_postmortem::nvidia::Aftermath::dispatchMesh(const call_stack::CommandData &, D3DGraphicsCommandList *,
  const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t)
{}

void debug::gpu_postmortem::nvidia::Aftermath::dispatchMeshIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *,
  const PipelineStageStateBase &, const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, BufferResourceReferenceAndOffset,
  BufferResourceReferenceAndOffset, uint32_t)
{}

void debug::gpu_postmortem::nvidia::Aftermath::blit(const call_stack::CommandData &, D3DGraphicsCommandList *) {}

void debug::gpu_postmortem::nvidia::Aftermath::onDeviceRemoved(D3DDevice *, HRESULT reason, call_stack::Reporter &)
{
  if (DXGI_ERROR_INVALID_CALL == reason)
  {
    // Invalid call is catched by the runtime and we will not get anything from Aftermath.
    return;
  }

  debug("DX12: Checking for NVIDIA Aftermath crash dumps...");
  GFSDK_Aftermath_CrashDump_Status crashDumpStatus = GFSDK_Aftermath_CrashDump_Status_Unknown;
  AFTHERMATH_CALL(api.getCrashDumpStatus(&crashDumpStatus));
  if (GFSDK_Aftermath_CrashDump_Status_NotStarted == crashDumpStatus)
  {
    debug("DX12: ...reported no crash dump was generated");
    return;
  }

  constexpr uint32_t collectingDataWait = 50;
  constexpr uint32_t invokingCallbackWait = 50;
  constexpr uint32_t unkownWait = 100;
  // 100ms * 50 = 5 seconds, after that we abort waiting
  constexpr uint32_t maxUnkownWaitCount = 50;

  uint32_t unkownWaitCount = 0;
  for (;;)
  {
    AFTHERMATH_CALL(api.getCrashDumpStatus(&crashDumpStatus));
    if (GFSDK_Aftermath_CrashDump_Status_CollectingData == crashDumpStatus)
    {
      debug("DX12: ...collecting data, waiting for %ums...", collectingDataWait);
      watchdog_kick();
      sleep_msec(collectingDataWait);
    }
    else if (GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed == crashDumpStatus)
    {
      debug("DX12: ...collecting data failed, no dump was produced");
      break;
    }
    else if (GFSDK_Aftermath_CrashDump_Status_InvokingCallback == crashDumpStatus)
    {
      debug("DX12: ...processing crash dump, waiting for %ums...", invokingCallbackWait);
      watchdog_kick();
      sleep_msec(invokingCallbackWait);
    }
    else if (GFSDK_Aftermath_CrashDump_Status_Finished == crashDumpStatus)
    {
      debug("DX12: ...finished");
      break;
    }
    else if (GFSDK_Aftermath_CrashDump_Status_Unknown == crashDumpStatus)
    {
      // For driver that do not fully support the query the lib will return unknown and may
      // return either finished or failed.
      // To be sure we have also a max iteration count and exit when reached.
      debug("DX12: ...working, waiting for %ums...", unkownWait);
      watchdog_kick();
      sleep_msec(unkownWait);
      if (++unkownWaitCount > maxUnkownWaitCount)
      {
        debug("DX12: ...max waiting reached, aborting");
        break;
      }
    }
    else
    {
      debug("DX12: ...unexpected return code %u, aborting", crashDumpStatus);
      break;
    }
  }
}

bool debug::gpu_postmortem::nvidia::Aftermath::sendGPUCrashDump(const char *type, const void *data, uintptr_t size)
{
  if (0 != strcmp(type, "nv-gpudmp"))
  {
    return false;
  }
  onCrashDumpGenerate(data, size, true);
  return true;
}

void debug::gpu_postmortem::nvidia::Aftermath::onDeviceShutdown() { commandListTable.reset(); }

bool debug::gpu_postmortem::nvidia::Aftermath::onDeviceSetup(ID3D12Device *device, const Configuration &config,
  const Direct3D12Enviroment &)
{
  debug("DX12: Initializing NVIDA Aftermath for device %p...", device);
  uint32_t flags = GFSDK_Aftermath_FeatureFlags_Minimum | GFSDK_Aftermath_FeatureFlags_EnableMarkers |
                   GFSDK_Aftermath_FeatureFlags_EnableResourceTracking | GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo;

  // Have shader error reporting behind a extra config value that is off by default, the documentation talks about a different
  // operation mode of the GPU for better error reporting. Nothing about the impact, but its probably not cheap.
  if (config.enableShaderErrorReporting)
  {
    debug("DX12: ...enabling shader error reporting...");
    flags |= GFSDK_Aftermath_FeatureFlags_EnableShaderErrorReporting;
  }

  auto result = api.initialize(GFSDK_Aftermath_Version_API, flags, device);
  if (GFSDK_Aftermath_Result_Success != result)
  {
    debug("DX12: ...failed with %s...", to_string(result));
    return false;
  }
  debug("DX12: ...NVIDIA Aftermath GPU postmortem trace enabled for device %p", device);
  return true;
}
