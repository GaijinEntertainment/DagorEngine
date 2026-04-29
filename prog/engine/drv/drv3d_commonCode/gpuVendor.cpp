// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpuVendor.h"
#include "gpuVendorAmd.h"
#include "gpuVendorNvidia.h"

#include <cstdio>
#include <dag/dag_vector.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_driverDesc.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/span.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>
#include <generic/dag_carray.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_finally.h>
#include <startup/dag_globalSettings.h>

#if _TARGET_PC_WIN
#include <devguid.h>
#include <SetupAPI.h>
#include "winapi_helpers.h"

#pragma comment(lib, "setupapi.lib")
#endif


#if HAS_ADL
// clang-format off
namespace gpu
{
ADL_MAIN_CONTROL_CREATE                               ADL_Main_Control_Create                               = nullptr;
ADL_MAIN_CONTROL_DESTROY                              ADL_Main_Control_Destroy                              = nullptr;
ADL_APPLICATIONPROFILES_PROFILEOFANAPPLICATION_SEARCH ADL_ApplicationProfiles_ProfileOfAnApplication_Search = nullptr;
ADL_APPLICATIONPROFILES_HITLISTS_GET                  ADL_ApplicationProfiles_HitLists_Get                  = nullptr;
ADL_APPLICATIONPROFILES_SYSTEM_RELOAD                 ADL_ApplicationProfiles_System_Reload                 = nullptr;
ADL_APPLICATIONPROFILES_USER_LOAD                     ADL_ApplicationProfiles_User_Load                     = nullptr;
ADL_ADAPTER_CROSSFIRE_GET                             ADL_Adapter_Crossfire_Get                             = nullptr;
ADL_GRAPHICS_PLATFORM_GET                             ADL_Graphics_Platform_Get                             = nullptr;
ADL_ADAPTER_NUMBEROFADAPTERS_GET                      ADL_Adapter_NumberOfAdapters_Get                      = nullptr;
ADL_ADAPTER_ADAPTERINFO_GET                           ADL_Adapter_AdapterInfo_Get                           = nullptr;
ADL_ADAPTER_ACTIVE_GET                                ADL_Adapter_Active_Get                                = nullptr;
ADL_ADAPTER_MEMORYINFO_GET                            ADL_Adapter_MemoryInfo_Get                            = nullptr;
ADL_ADAPTER_VIDEOBIOSINFO_GET                         ADL_Adapter_VideoBiosInfo_Get                         = nullptr;
ADL_OVERDRIVE5_CURRENTACTIVITY_GET                    ADL_Overdrive5_CurrentActivity_Get                    = nullptr;
ADL_DISPLAY_MVPUSTATUS_GET                            ADL_Display_MVPUStatus_Get                            = nullptr;
ADL_DISPLAY_MVPUCAPS_GET                              ADL_Display_MVPUCaps_Get                              = nullptr;
}
// clang-format on
#endif

using namespace gpu;

#if HAS_NVAPI

static NvAPI_Status nvapi_init_status = NVAPI_API_NOT_INITIALIZED;

bool gpu::init_nvapi()
{
  if (nvapi_init_status == NVAPI_API_NOT_INITIALIZED)
    nvapi_init_status = NvAPI_Initialize();
  return nvapi_init_status == NVAPI_OK;
}

NvPhysicalGpuHandle gpu::get_nv_physical_gpu()
{
  static NvPhysicalGpuHandle nvPhysicalGpuHandle = NULL;

  if (!init_nvapi())
    return nvPhysicalGpuHandle;

  if (nvPhysicalGpuHandle == NULL)
  {
    NvPhysicalGpuHandle nvPhysicalGPUHandles[NVAPI_MAX_PHYSICAL_GPUS] = {0};
    NvU32 physicalGPUCount = 0;
    NvAPI_EnumPhysicalGPUs(nvPhysicalGPUHandles, &physicalGPUCount);
    if (physicalGPUCount > 0)
      nvPhysicalGpuHandle = nvPhysicalGPUHandles[0];
  }

  return nvPhysicalGpuHandle;
}

void gpu::get_nv_gpu_memory(uint32_t &out_dedicated_kb, uint32_t &out_shared_kb, uint32_t &out_dedicated_free_kb)
{
  out_dedicated_kb = out_shared_kb = out_dedicated_free_kb = 0;

  if (!get_nv_physical_gpu())
    return;

  NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = {.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER};
  NvAPI_Status status = NvAPI_GPU_GetMemoryInfo(get_nv_physical_gpu(), &memoryInfo);
  if (status != NVAPI_OK)
    return;

  out_dedicated_kb = memoryInfo.dedicatedVideoMemory;
  out_shared_kb = memoryInfo.sharedSystemMemory;
  out_dedicated_free_kb = memoryInfo.curAvailableDedicatedVideoMemory;
}

void gpu::get_video_nvidia_str(String &out_str)
{
  if (!init_nvapi())
    return;

  NV_DISPLAY_DRIVER_VERSION version = {.version = NV_DISPLAY_DRIVER_VERSION_VER};
  NvAPI_Status status = NvAPI_GetDisplayDriverVersion(NVAPI_DEFAULT_HANDLE, &version);
  if (status != NVAPI_OK)
    return;

  NvLogicalGpuHandle nvLogicalGPUHandle[NVAPI_MAX_LOGICAL_GPUS] = {0};
  NvPhysicalGpuHandle nvPhysicalGPUHandles[NVAPI_MAX_PHYSICAL_GPUS] = {0};
  NvU32 logicalGPUCount = 0, physicalGPUCount = 0;
  NvAPI_EnumLogicalGPUs(nvLogicalGPUHandle, &logicalGPUCount);
  NvAPI_EnumPhysicalGPUs(nvPhysicalGPUHandles, &physicalGPUCount);
  bool sli = logicalGPUCount < physicalGPUCount;

  NV_SYSTEM_TYPE systemType = NV_SYSTEM_TYPE_UNKNOWN;
  if (get_nv_physical_gpu())
    NvAPI_GPU_GetSystemType(get_nv_physical_gpu(), &systemType);

  NvU32 physicalFrameBufferSize = 0;
  if (get_nv_physical_gpu())
    NvAPI_GPU_GetPhysicalFrameBufferSize(get_nv_physical_gpu(), &physicalFrameBufferSize);

  NvU32 virtualFrameBufferSize = 0;
  if (get_nv_physical_gpu())
    NvAPI_GPU_GetVirtualFrameBufferSize(get_nv_physical_gpu(), &virtualFrameBufferSize);

  NV_GPU_PERF_PSTATES20_INFO pstates = {.version = NV_GPU_PERF_PSTATES20_INFO_VER1};
  if (get_nv_physical_gpu())
    NvAPI_GPU_GetPstates20(get_nv_physical_gpu(), &pstates);

  unsigned int memSize = (unsigned int)(physicalFrameBufferSize / 1024);

  out_str.printf(128, "%s, ver: %d, %d MB (%d MB), %d MHz / %d MHz / %d MHz, logical GPUs: %d, physical GPUs: %d%s%s",
    version.szAdapterString, version.drvVersion, memSize, virtualFrameBufferSize / 1024,
    pstates.pstates[0].clocks[0].data.single.freq_kHz / 1000, pstates.pstates[0].clocks[1].data.single.freq_kHz / 1000,
    pstates.pstates[0].clocks[2].data.single.freq_kHz / 1000, logicalGPUCount, physicalGPUCount, sli ? ", SLI" : "",
    systemType == NV_SYSTEM_TYPE_LAPTOP ? ", notebook" : "");
}
#endif

#if HAS_ADL

#define MAX_REG_STR 2048

void *__stdcall gpu::ADL_Main_Memory_Alloc(int iSize)
{
  void *lpBuffer = malloc(iSize);
  return lpBuffer;
}

static LibPointer adl_dll;

// clang-format off
bool gpu::init_adl()
{
  if (adl_dll)
    return true;

  adl_dll.reset(LoadLibrary("atiadlxx.dll"));
  if (!adl_dll)
    adl_dll.reset(LoadLibrary("atiadlxy.dll"));
  if (!adl_dll)
    return false;

  ADL_Main_Control_Create                               = (ADL_MAIN_CONTROL_CREATE)                              GetProcAddress(adl_dll.get(), "ADL_Main_Control_Create");
  ADL_Main_Control_Destroy                              = (ADL_MAIN_CONTROL_DESTROY)                             GetProcAddress(adl_dll.get(), "ADL_Main_Control_Destroy");
  ADL_ApplicationProfiles_ProfileOfAnApplication_Search = (ADL_APPLICATIONPROFILES_PROFILEOFANAPPLICATION_SEARCH)GetProcAddress(adl_dll.get(), "ADL_ApplicationProfiles_ProfileOfAnApplication_Search");
  ADL_ApplicationProfiles_HitLists_Get                  = (ADL_APPLICATIONPROFILES_HITLISTS_GET)                 GetProcAddress(adl_dll.get(), "ADL_ApplicationProfiles_HitLists_Get");
  ADL_ApplicationProfiles_System_Reload                 = (ADL_APPLICATIONPROFILES_SYSTEM_RELOAD)                GetProcAddress(adl_dll.get(), "ADL_ApplicationProfiles_System_Reload");
  ADL_ApplicationProfiles_User_Load                     = (ADL_APPLICATIONPROFILES_USER_LOAD)                    GetProcAddress(adl_dll.get(), "ADL_ApplicationProfiles_User_Load");
  ADL_Adapter_Crossfire_Get                             = (ADL_ADAPTER_CROSSFIRE_GET)                            GetProcAddress(adl_dll.get(), "ADL_Adapter_Crossfire_Get");
  ADL_Graphics_Platform_Get                             = (ADL_GRAPHICS_PLATFORM_GET)                            GetProcAddress(adl_dll.get(), "ADL_Graphics_Platform_Get");
  ADL_Adapter_NumberOfAdapters_Get                      = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)                     GetProcAddress(adl_dll.get(), "ADL_Adapter_NumberOfAdapters_Get");
  ADL_Adapter_AdapterInfo_Get                           = (ADL_ADAPTER_ADAPTERINFO_GET)                          GetProcAddress(adl_dll.get(), "ADL_Adapter_AdapterInfo_Get");
  ADL_Adapter_Active_Get                                = (ADL_ADAPTER_ACTIVE_GET)                               GetProcAddress(adl_dll.get(), "ADL_Adapter_Active_Get");
  ADL_Adapter_MemoryInfo_Get                            = (ADL_ADAPTER_MEMORYINFO_GET)                           GetProcAddress(adl_dll.get(), "ADL_Adapter_MemoryInfo_Get");
  ADL_Adapter_VideoBiosInfo_Get                         = (ADL_ADAPTER_VIDEOBIOSINFO_GET)                        GetProcAddress(adl_dll.get(), "ADL_Adapter_VideoBiosInfo_Get");
  ADL_Overdrive5_CurrentActivity_Get                    = (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)                   GetProcAddress(adl_dll.get(), "ADL_Overdrive5_CurrentActivity_Get");
  ADL_Display_MVPUStatus_Get                            = (ADL_DISPLAY_MVPUSTATUS_GET)                           GetProcAddress(adl_dll.get(), "ADL_Display_MVPUStatus_Get");
  ADL_Display_MVPUCaps_Get                              = (ADL_DISPLAY_MVPUCAPS_GET)                             GetProcAddress(adl_dll.get(), "ADL_Display_MVPUCaps_Get");

  if (!ADL_Main_Control_Create ||
      !ADL_Main_Control_Destroy ||
      !ADL_ApplicationProfiles_ProfileOfAnApplication_Search ||
      !ADL_ApplicationProfiles_HitLists_Get ||
      !ADL_ApplicationProfiles_System_Reload ||
      !ADL_ApplicationProfiles_User_Load ||
      !ADL_Adapter_Crossfire_Get ||
      !ADL_Graphics_Platform_Get ||
      !ADL_Adapter_NumberOfAdapters_Get ||
      !ADL_Adapter_AdapterInfo_Get ||
      !ADL_Adapter_Active_Get ||
      !ADL_Adapter_MemoryInfo_Get ||
      !ADL_Adapter_VideoBiosInfo_Get ||
      !ADL_Overdrive5_CurrentActivity_Get ||
      !ADL_Display_MVPUStatus_Get ||
      !ADL_Display_MVPUCaps_Get)
  {
    close_adl();
    return false;
  }

  return true;
}

void gpu::close_adl()
{
  ADL_Main_Control_Create                               = nullptr;
  ADL_Main_Control_Destroy                              = nullptr;
  ADL_ApplicationProfiles_ProfileOfAnApplication_Search = nullptr;
  ADL_ApplicationProfiles_HitLists_Get                  = nullptr;
  ADL_ApplicationProfiles_System_Reload                 = nullptr;
  ADL_ApplicationProfiles_User_Load                     = nullptr;
  ADL_Adapter_Crossfire_Get                             = nullptr;
  ADL_Graphics_Platform_Get                             = nullptr;
  ADL_Adapter_NumberOfAdapters_Get                      = nullptr;
  ADL_Adapter_AdapterInfo_Get                           = nullptr;
  ADL_Adapter_Active_Get                                = nullptr;
  ADL_Adapter_MemoryInfo_Get                            = nullptr;
  ADL_Adapter_VideoBiosInfo_Get                         = nullptr;
  ADL_Overdrive5_CurrentActivity_Get                    = nullptr;
  ADL_Display_MVPUStatus_Get                            = nullptr;
  ADL_Display_MVPUCaps_Get                              = nullptr;
  adl_dll.reset();
}
// clang-format on

void gpu::get_video_amd_str(String &out_str)
{
  if (!init_adl())
    return;

  Finally closeAdl{[] { close_adl(); }};

  if (ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 0) != ADL_OK)
  {
    return;
  }

  Finally destroyControl{[] { ADL_Main_Control_Destroy(); }};

  int platform = 0;
  ADL_Graphics_Platform_Get(&platform);

  int numAdapters = 0;
  ADL_Adapter_NumberOfAdapters_Get(&numAdapters);
  if (numAdapters <= 0)
  {
    return;
  }

  dag::Vector<AdapterInfo> adapterInfo(numAdapters);
  ADL_Adapter_AdapterInfo_Get(adapterInfo.data(), sizeof(AdapterInfo) * numAdapters);

  unsigned int adapterNo = 0;
  for (;; adapterNo++)
  {
    if (adapterNo == numAdapters)
    {
      return;
    }

    int active = 0;
    if (ADL_Adapter_Active_Get(adapterInfo[adapterNo].iAdapterIndex, &active) == ADL_OK && active)
      break;
  }

  HKEY hKey;
  String keyPath(1000, "SYSTEM\\CurrentControlSet\\Control\\Class\\%s", adapterInfo[adapterNo].strDriverPathExt);
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
  {
    debug("Failed to open '%s'", keyPath);
    return;
  }
  FINALLY([=] { RegCloseKey(hKey); });

  ADLMemoryInfo memoryInfo{};
  ADL_Adapter_MemoryInfo_Get(adapterInfo[adapterNo].iAdapterIndex, &memoryInfo);

  ADLPMActivity activity{};
  ADL_Overdrive5_CurrentActivity_Get(adapterInfo[adapterNo].iAdapterIndex, &activity);

  char catalystVersion[MAX_REG_STR];
  catalystVersion[0] = 0;
  DWORD catalystVersionSize = MAX_REG_STR;
  RegQueryValueEx(hKey, "Catalyst_Version", nullptr, nullptr, reinterpret_cast<LPBYTE>(catalystVersion), &catalystVersionSize);

  char releaseVersion[MAX_REG_STR];
  releaseVersion[0] = 0;
  DWORD releaseVersionSize = MAX_REG_STR;
  RegQueryValueEx(hKey, "ReleaseVersion", nullptr, nullptr, reinterpret_cast<LPBYTE>(releaseVersion), &releaseVersionSize);

  unsigned int memSize = (unsigned int)(memoryInfo.iMemorySize / 1024 / 1024);

  out_str.printf(128, "%s, ver: %s (%s), %d MB %s (%d GB/s), %d MHz / %d MHz, %s", adapterInfo[adapterNo].strAdapterName,
    catalystVersion, releaseVersion, memSize, memoryInfo.strMemoryType, (unsigned int)memoryInfo.iMemoryBandwidth / 1000,
    activity.iEngineClock / 100, activity.iMemoryClock / 100, platform == 1 ? ", notebook" : "");
}
#endif

namespace
{
#if HAS_NVAPI
uint32_t get_nvidia_gpu_family(uint32_t device_id)
{
  if (init_nvapi())
  {
    NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = {0};
    NvU32 gpuCount = 0;
    if (NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount) == NVAPI_OK)
    {
      for (auto gpuHandle : eastl::span(gpuHandles, gpuCount))
      {
        NvU32 deviceId = 0, subSystemId = 0, revisionId = 0, extDeviceId = 0;
        if (NvAPI_GPU_GetPCIIdentifiers(gpuHandle, &deviceId, &subSystemId, &revisionId, &extDeviceId) == NVAPI_OK &&
            device_id == extDeviceId)
        {
          NV_GPU_ARCH_INFO archInfo{.version = NV_GPU_ARCH_INFO_VER};
          if (NvAPI_GPU_GetArchInfo(gpuHandle, &archInfo) == NVAPI_OK)
          {
            return archInfo.architecture_id;
          }
        }
      }
    }
  }

  return DeviceAttributes::UNKNOWN;
}
#else
constexpr uint32_t get_nvidia_gpu_family(uint32_t) { return DeviceAttributes::UNKNOWN; }
#endif

#if _TARGET_PC
// source https://github.com/intel/vpl-gpu-rt/blob/main/_studio/shared/include/mfxstructures-int.h
uint32_t get_intel_gpu_family(uint32_t device_id)
{
  using enum DeviceAttributes::IntelFamily;

  const uint32_t idhi = device_id & 0xFF00;
  const uint32_t idlo = device_id & 0x00FF;

  switch (idhi)
  {
    case 0x0100:
    {
      if ((idlo & 0xFFF0) == 0x0050 || (idlo & 0xFFF0) == 0x0060)
      {
        return IVYBRIDGE;
      }
      return SANDYBRIDGE;
    }
    case 0x0400:
    case 0x0A00:
    case 0x0C00:
    case 0x0D00: return HASWELL;
    case 0x1600: return BROADWELL;
    case 0x0900:
    case 0x1900: return SKYLAKE;
    case 0x5900: return KABYLAKE;
    case 0x3100: return GEMINILAKE;
    case 0x3E00:
    {
      switch (idlo)
      {
        case 0x00A0:
        case 0x00A1:
        case 0x00A2:
        case 0x00A3:
        case 0x00A4: return WHISKEYLAKE;
        default: return COFFEELAKE;
      }
    }
    case 0x5A00: return CANNONLAKE;
    case 0x9B00: return COMETLAKE;
    case 0x8A00: return ICELAKE_LP;
    case 0x2600: return LAKEFIELD;
    case 0x4500: return ELKHARTLAKE;
    case 0x4E00: return JASPERLAKE;
    case 0x9A00: return TIGERLAKE_LP;
    case 0x4C00: return ROCKETLAKE;
    case 0x4600:
    {
      switch (idlo)
      {
        case 0x0080:
        case 0x0081:
        case 0x0082:
        case 0x0083:
        case 0x0088:
        case 0x008A:
        case 0x008B:
        case 0x0090:
        case 0x0091:
        case 0x0092:
        case 0x0093:
        case 0x0098:
        case 0x0099: return ALDERLAKE_S;
        case 0x00A0:
        case 0x00A1:
        case 0x00A2:
        case 0x00A3:
        case 0x00A6:
        case 0x0026:
        case 0x00B0:
        case 0x00B1:
        case 0x00B2:
        case 0x00B3:
        case 0x00A8:
        case 0x0028:
        case 0x00C0:
        case 0x00C1:
        case 0x00C2:
        case 0x00C3:
        case 0x00AA:
        case 0x002A: return ALDERLAKE_P;
        case 0x00D0:
        case 0x00D1:
        case 0x00D2: return ALDERLAKE_N;
        case 0x00D3:
        case 0x00D4:
        default: return TWINLAKE;
      }
    } // -V796

    case 0xA700:
    {
      switch (idlo)
      {
        case 0x0080:
        case 0x0081:
        case 0x0082:
        case 0x0083:
        case 0x0084:
        case 0x0085:
        case 0x0086:
        case 0x0087:
        case 0x0088:
        case 0x0089:
        case 0x008A:
        case 0x008B:
        case 0x008C:
        case 0x008D:
        case 0x008E: return RAPTORLAKE_S;
        case 0x00A0:
        case 0x0020:
        case 0x00A8:
        case 0x00A1:
        case 0x0021:
        case 0x00A9: return RAPTORLAKE_P;
        case 0x00AA:
        case 0x00AB:
        case 0x00AC:
        case 0x00AD:
        default: return RAPTORLAKE;
      }
    }
    case 0x4900: return DG1;
    case 0x4F00:
    case 0x5600: return ALCHEMIST;
    case 0x0B00: return PONTEVECCHIO;
    case 0x7D00:
    {
      switch (idlo)
      {
        case 0x0067: return ARROWLAKE_S;
        case 0x0051:
        case 0x00D1:
        case 0x0041: return ARROWLAKE_H;
        case 0x0040:
        case 0x0050:
        case 0x0055:
        case 0x0057:
        case 0x0060:
        case 0x0070:
        case 0x0075:
        case 0x0079:
        case 0x0076:
        case 0x0066:
        case 0x00D5:
        case 0x00D7:
        case 0x0045:
        case 0x00E0:
        default: return METEORLAKE;
        case 0x00A0:
        case 0x00A1:
        case 0x00A2:
        case 0x00A3:
        case 0x00A4: return WHISKEYLAKE;
      }
    }
    case 0xE200: return BATTLEMAGE;
    case 0x6400: return LUNARLAKE;
    case 0xB000: return PANTHERLAKE;
    case 0xFD00: return WILDCATLAKE;
    default: return DeviceAttributes::UNKNOWN;
  }
}
#else
constexpr uint32_t get_intel_gpu_family(uint32_t) { return DeviceAttributes::UNKNOWN; }
#endif

// From https://github.com/WebKit/webkit/blob/main/Source/ThirdParty/ANGLE/src/libANGLE/renderer/driver_utils.h#L18 and
// https://gamedev.stackexchange.com/a/31626
static constexpr eastl::pair<uint32_t, GpuVendor> vendor_id_table[] = //
  {
    {0x10DE, GpuVendor::NVIDIA},

    {0x1002, GpuVendor::AMD},
    {0x1022, GpuVendor::AMD},

    {0x8086, GpuVendor::INTEL},
    {0x8087, GpuVendor::INTEL},
    {0x163C, GpuVendor::INTEL},

    {0x5143, GpuVendor::QUALCOMM},
    {0x4D4F4351, GpuVendor::QUALCOMM},

    {0x1010, GpuVendor::IMGTEC},

    {0x13B5, GpuVendor::ARM},

    {0x05AC, GpuVendor::APPLE},

    {0x10005, GpuVendor::MESA},

    {0x0003, GpuVendor::SHIM_DRIVER},

    {0x144D, GpuVendor::SAMSUNG},

    {0x19E5, GpuVendor::HUAWEI},

    {0x0000, GpuVendor::UNKNOWN},
};

static constexpr carray<const char *, GPU_VENDOR_COUNT> vendor_names = {
  "Unknown",     // GpuVendor::UNKNOWN
  "Mesa",        // GpuVendor::MESA
  "ImgTec",      // GpuVendor::IMGTEC
  "AMD",         // GpuVendor::AMD / GpuVendor::ATI
  "NVIDIA",      // GpuVendor::NVIDIA
  "Intel",       // GpuVendor::INTEL
  "Apple",       // GpuVendor::APPLE
  "Shim driver", // GpuVendor::SHIM_DRIVER
  "ARM",         // GpuVendor::ARM
  "Qualcomm",    // GpuVendor::QUALCOMM
  "Samsung",     // GpuVendor::SAMSUNG
  "Huawei",      // GpuVendor::HUAWEI
};

} // namespace

GpuVendor d3d_get_vendor(uint32_t vendor_id, const char *description)
{
  GpuVendor vendor = GpuVendor::UNKNOWN;
  auto ref =
    eastl::find_if(eastl::begin(vendor_id_table), eastl::end(vendor_id_table), [=](auto &ent) { return ent.first == vendor_id; });
  if (ref != eastl::end(vendor_id_table))
    vendor = ref->second;
  else if (description)
  {
    if (strstr(description, "ATI") || strstr(description, "AMD"))
      vendor = GpuVendor::AMD;
    else
    {
      if (char *lower = str_dup(description, tmpmem))
      {
        lower = dd_strlwr(lower);
        if (strstr(lower, "radeon"))
          vendor = GpuVendor::AMD;
        else if (strstr(lower, "geforce") || strstr(lower, "nvidia"))
          vendor = GpuVendor::NVIDIA;
        else if (strstr(lower, "intel") || strstr(lower, "rdpdd")) // Assume the worst for the Microsoft Mirroring Driver - take it as
                                                                   // it mirrors to Intel driver with broken voltex compression.
          vendor = GpuVendor::INTEL;
        memfree(lower, tmpmem);
      }
    }
  }
  return vendor;
}

const char *d3d_get_vendor_name(GpuVendor vendor)
{
  int vendorIdx = eastl::to_underlying(vendor);
  G_ASSERT(vendorIdx >= 0 && vendorIdx < vendor_names.size());
  return vendor_names[vendorIdx];
}

void gpu::update_device_attributes(uint32_t vendor_id, uint32_t device_id, DeviceAttributesBase &device_attributes)
{
  device_attributes.vendor = d3d_get_vendor(vendor_id);
  device_attributes.vendorId = vendor_id;
  device_attributes.deviceId = device_id;

  switch (device_attributes.vendor)
  {
    case GpuVendor::UNKNOWN: break;
    case GpuVendor::MESA: break;
    case GpuVendor::IMGTEC: break;
    case GpuVendor::AMD: break;
    case GpuVendor::NVIDIA: device_attributes.family = get_nvidia_gpu_family(device_id); break;
    case GpuVendor::INTEL: device_attributes.family = get_intel_gpu_family(device_id); break;
    case GpuVendor::APPLE: break;
    case GpuVendor::SHIM_DRIVER: break;
    case GpuVendor::ARM: break;
    case GpuVendor::QUALCOMM: break;
    case GpuVendor::SAMSUNG: break;
    case GpuVendor::HUAWEI: break;
    default: break;
  }
}

static bool match_device_families(const DataBlock &gpu_preferences, uint32_t vendor_id, uint32_t device_id,
  eastl::span<const uint32_t> other_discrete, eastl::span<const char *> family_keys, eastl::span<const char *> device_keys)
{
  DeviceAttributes deviceAttributes;
  gpu::update_device_attributes(vendor_id, device_id, deviceAttributes);

  for (int i = 0; i < gpu_preferences.blockCount(); i++)
  {
    const DataBlock &vendor = *gpu_preferences.getBlock(i);
    if (vendor.getInt("vendorId", -1) != vendor_id)
      continue;

    bool result = false;

    for (const char *key : family_keys)
    {
      dblk::iterate_params_by_name_and_type(vendor, key, DataBlock::TYPE_INT,
        [&](int param_idx) { result |= vendor.getInt(param_idx) == deviceAttributes.family; });
    }

    for (const char *key : device_keys)
    {
      dblk::iterate_params_by_name_and_type(vendor, key, DataBlock::TYPE_INT,
        [&](int param_idx) { result |= vendor.getInt(param_idx) == device_id; });
    }

    dblk::iterate_params_by_name_and_type(vendor, "skipIfOtherDiscrete", DataBlock::TYPE_STRING, [&](int param_idx) {
      const char *skipIfOtherDiscrete = vendor.getStr(param_idx);
      for (uint32_t otherDiscreteVendorId : other_discrete)
        result &= strcmp(skipIfOtherDiscrete, d3d_get_vendor_name(d3d_get_vendor(otherDiscreteVendorId))) != 0;
    });

    return result;
  }
  return false;
}

bool gpu::is_forced_device(const DataBlock &gpu_preferences, uint32_t vendor_id, uint32_t device_id,
  eastl::span<const uint32_t> other_discrete)
{
  const char *familyKeys[] = {"forcedFamily"};
  const char *deviceKeys[] = {"forcedDeviceIds"};
  return match_device_families(gpu_preferences, vendor_id, device_id, other_discrete, familyKeys, deviceKeys);
}

bool gpu::is_preferred_device(const DataBlock &gpu_preferences, uint32_t vendor_id, uint32_t device_id,
  eastl::span<const uint32_t> other_discrete)
{
  const char *familyKeys[] = {"forcedFamily", "preferredFamily"};
  const char *deviceKeys[] = {"forcedDeviceIds", "preferredDeviceIds"};
  return match_device_families(gpu_preferences, vendor_id, device_id, other_discrete, familyKeys, deviceKeys);
}

bool gpu::is_blacklisted_device(const DataBlock &gpu_preferences, uint32_t vendor_id, uint32_t device_id,
  eastl::span<const uint32_t> other_discrete)
{
  if (::dgs_get_settings()->getBlockByNameEx("video")->getBool("ignoreGpuPreferences", false))
    return false;

  DeviceAttributes deviceAttributes;
  gpu::update_device_attributes(vendor_id, device_id, deviceAttributes);

  for (int i = 0; i < gpu_preferences.blockCount(); i++)
  {
    const DataBlock &vendor = *gpu_preferences.getBlock(i);
    if (vendor.getInt("vendorId", -1) != vendor_id)
      continue;

    bool result = false;

    dblk::iterate_params_by_name_and_type(vendor, "blacklistedFamily", DataBlock::TYPE_INT,
      [&](int param_idx) { result |= vendor.getInt(param_idx) == deviceAttributes.family; });

    dblk::iterate_params_by_name_and_type(vendor, "blacklistedDeviceIds", DataBlock::TYPE_INT,
      [&](int param_idx) { result |= vendor.getInt(param_idx) == device_id; });

    dblk::iterate_params_by_name_and_type(vendor, "skipIfOtherDiscrete", DataBlock::TYPE_STRING, [&](int param_idx) {
      const char *skipIfOtherDiscrete = vendor.getStr(param_idx);
      for (uint32_t otherDiscreteVendorId : other_discrete)
        result &= strcmp(skipIfOtherDiscrete, d3d_get_vendor_name(d3d_get_vendor(otherDiscreteVendorId))) == 0;
    });

    return result;
  }

  return false;
}

#if _TARGET_PC_WIN
namespace
{
DagorDateTime parse_date(wchar_t *date_buf, uint32_t size)
{
  eastl::fixed_vector<uint16_t, 4> nums;
  eastl::fixed_vector<int16_t, 4> widths; // digit counts per token
  {
    uint16_t val = 0;
    int16_t digits = 0;
    for (size_t i = 0; i < size; ++i)
    {
      if (iswdigit(date_buf[i]))
      {
        val = val * 10 + (uint16_t)(date_buf[i] - L'0');
        ++digits;
        continue;
      }

      if (digits > 0)
      {
        nums.push_back(val);
        widths.push_back(digits);
        val = 0;
        digits = 0;
      }
    }
    if (digits > 0)
    {
      nums.push_back(val);
      widths.push_back(digits);
    }
  }

  // Expect at least a date (3 numbers). If more exist, we ignore time here.
  if (nums.size() < 3)
    return {};

  // Identify the year token
  int16_t yearIdx = -1;
  for (int16_t i = 0; i < (int16_t)nums.size(); ++i)
  {
    if (widths[i] >= 4 || nums[i] >= 1900)
    {
      yearIdx = i;
      break;
    }
  }
  // If no obvious year, assume last is year (Windows tends to use MDY or DMY with YYYY)
  if (yearIdx == -1)
    yearIdx = (int16_t)nums.size() - 1;

  uint16_t y = 0, m = 0, d = 0;

  if (yearIdx == 0)
  {
    // YYYY-??-?? -> Y-M-D
    y = nums[0];
    m = nums[1];
    d = nums[2];
  }
  else if (yearIdx == 2)
  {
    // ??-??-YYYY -> either M-D-Y or D-M-Y
    // Heuristic:
    // - If first > 12 -> D-M-Y (because month must be <= 12)
    // - Else if second > 12 -> M-D-Y
    // - Else default to M-D-Y (common in US locales)
    y = nums[2];
    if (nums[0] > 12 && nums[1] <= 12)
    {
      d = nums[0];
      m = nums[1];
    }
    else
    {
      m = nums[0];
      d = nums[1];
    }
  }
  else
  {
    // Fallback: try Y-M-D if year isn't first/last (rare in our context)
    // Or collapse to using first three numbers assuming Y-M-D if sensible.
    // Prefer positions around yearIdx.
    // Simple fallback: try to map sequentially Y-M-D.
    // If invalid, try M-D-Y with last as year.
    y = nums[yearIdx];
    // pick two neighbors if possible
    int left = yearIdx - 1 >= 0 ? yearIdx - 1 : yearIdx + 1;
    int right = (left == yearIdx + 1) ? yearIdx + 2 : yearIdx + 1;
    if (right < (int)nums.size())
    {
      m = nums[left];
      d = nums[right];
    }
    else if (nums.size() >= 3)
    {
      // fallback
      m = nums[1];
      d = nums[2];
    }
  }

  // Minimal day bound checks
  auto is_valid_ymd = [](uint16_t y, uint16_t m, uint16_t d) {
    if (y < 1900 || y > 3000)
      return false;
    if (m < 1 || m > 12)
      return false;
    if (d < 1 || d > 31)
      return false;
    return true;
  };

  // Validate; if clearly invalid, try swapping m/d once
  if (!is_valid_ymd(y, m, d))
  {
    uint16_t mm = d, dd = m;
    if (is_valid_ymd(y, mm, dd))
    {
      m = mm;
      d = dd;
    }
  }

  return {
    .year = y,
    .month = m,
    .day = d,
  };
}
} // namespace

DagorDateTime gpu::get_driver_date(uint32_t vendor_id, uint32_t device_id)
{
  HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
  if (devInfo == INVALID_HANDLE_VALUE)
    return {};
  FINALLY([=] { SetupDiDestroyDeviceInfoList(devInfo); });

  SP_DEVINFO_DATA devData = {.cbSize = sizeof(devData)};
  for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devData); i++)
  {
    wchar_t hwidBuf[1024];
    if (SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_HARDWAREID, nullptr, reinterpret_cast<PBYTE>(hwidBuf),
          sizeof(hwidBuf), nullptr) == FALSE)
      continue;

    for (wchar_t *p = hwidBuf; *p; p += wcslen(p) + 1)
    {
      uint32_t vendorId = 0, deviceId = 0;
      if (swscanf_s(p, L"PCI\\VEN_%4x&DEV_%4x", &vendorId, &deviceId) != 2)
        continue;

      if (vendorId != vendor_id || deviceId != device_id)
        continue;

      HKEY hKey = SetupDiOpenDevRegKey(devInfo, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
      if (hKey == INVALID_HANDLE_VALUE)
        continue;
      FINALLY([=] { RegCloseKey(hKey); });

      wchar_t dateBuf[256];
      DWORD dateBufSize = sizeof(dateBuf);
      if (RegQueryValueExW(hKey, L"DriverDate", nullptr, nullptr, reinterpret_cast<LPBYTE>(dateBuf), &dateBufSize) != ERROR_SUCCESS)
        continue;

      return parse_date(dateBuf, dateBufSize >> 1); // because of wchar_t
    }
  }

  return {};
}
#else
DagorDateTime gpu::get_driver_date(uint32_t vendor_id, uint32_t device_id) { return {}; }
#endif
