// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpuVendor.h"
#include <util/dag_globDef.h>

#if _TARGET_PC_WIN

namespace gpu
{
ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create;
ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy;
ADL_APPLICATIONPROFILES_PROFILEOFANAPPLICATION_SEARCH ADL_ApplicationProfiles_ProfileOfAnApplication_Search;
ADL_APPLICATIONPROFILES_HITLISTS_GET ADL_ApplicationProfiles_HitLists_Get;
ADL_APPLICATIONPROFILES_SYSTEM_RELOAD ADL_ApplicationProfiles_System_Reload;
ADL_APPLICATIONPROFILES_USER_LOAD ADL_ApplicationProfiles_User_Load;
ADL_ADAPTER_CROSSFIRE_GET ADL_Adapter_Crossfire_Get;
ADL_GRAPHICS_PLATFORM_GET ADL_Graphics_Platform_Get;
ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get;
ADL_ADAPTER_ACTIVE_GET ADL_Adapter_Active_Get;
ADL_ADAPTER_MEMORYINFO_GET ADL_Adapter_MemoryInfo_Get;
ADL_ADAPTER_VIDEOBIOSINFO_GET ADL_Adapter_VideoBiosInfo_Get;
ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get;
ADL_DISPLAY_MVPUSTATUS_GET ADL_Display_MVPUStatus_Get;
ADL_DISPLAY_MVPUCAPS_GET ADL_Display_MVPUCaps_Get;
} // namespace gpu

#define NVAPI_MAX_MEMORY_VALUES_PER_GPU 5
#define NVAPI_GPU_MEMORY_INFO_VER       (sizeof(NvMemoryInfo) | 0x20000)
#define MAX_REG_STR                     2048

static HMODULE atiDLL;

using namespace gpu;


#if HAS_NVAPI
static NvAPI_Status nvapi_init_status = NVAPI_API_NOT_INITIALIZED;
static NvU32 driver_version = 0;

bool gpu::init_nvapi()
{
  if (nvapi_init_status == NVAPI_API_NOT_INITIALIZED)
    nvapi_init_status = NvAPI_Initialize();
  return nvapi_init_status == NVAPI_OK;
}

const NvPhysicalGpuHandle &gpu::get_nv_physical_gpu()
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4191)
#endif

void gpu::get_nv_gpu_memory(uint32_t &out_dedicated_kb, uint32_t &out_shared_kb, uint32_t &out_dedicated_free_kb)
{
  out_dedicated_kb = out_shared_kb = out_dedicated_free_kb = 0;

  if (get_nv_physical_gpu())
  {
    NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo;
    memset(&memoryInfo, 0, sizeof(memoryInfo));
    memoryInfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;

    NvAPI_Status status = NvAPI_GPU_GetMemoryInfo(get_nv_physical_gpu(), &memoryInfo);
    if (status == NVAPI_OK)
    {
      out_dedicated_kb = memoryInfo.dedicatedVideoMemory;
      out_shared_kb = memoryInfo.sharedSystemMemory;
      out_dedicated_free_kb = memoryInfo.curAvailableDedicatedVideoMemory;
    }
  }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

void gpu::get_video_nvidia_str(String &out_str)
{
  if (!init_nvapi())
    return;

  NV_DISPLAY_DRIVER_VERSION version = {0};
  version.version = NV_DISPLAY_DRIVER_VERSION_VER;
  NvAPI_Status status = NvAPI_GetDisplayDriverVersion(NVAPI_DEFAULT_HANDLE, &version);
  if (status != NVAPI_OK)
    return;

  NvLogicalGpuHandle nvLogicalGPUHandle[NVAPI_MAX_LOGICAL_GPUS] = {0};
  NvPhysicalGpuHandle nvPhysicalGPUHandles[NVAPI_MAX_PHYSICAL_GPUS] = {0};
  NvU32 logicalGPUCount = 0, physicalGPUCount = 0;
  ;
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

  NV_GPU_PERF_PSTATES20_INFO pstates = {0};
  pstates.version = NV_GPU_PERF_PSTATES20_INFO_VER1;
  if (get_nv_physical_gpu())
    NvAPI_GPU_GetPstates20(get_nv_physical_gpu(), &pstates);

  unsigned int memSize = (unsigned int)(physicalFrameBufferSize / 1024);

  out_str.printf(128, "%s, ver: %d, %d MB (%d MB), %d MHz / %d MHz / %d MHz, logical GPUs: %d, physical GPUs: %d%s%s",
    version.szAdapterString, version.drvVersion, memSize, virtualFrameBufferSize / 1024,
    pstates.pstates[0].clocks[0].data.single.freq_kHz / 1000, pstates.pstates[0].clocks[1].data.single.freq_kHz / 1000,
    pstates.pstates[0].clocks[2].data.single.freq_kHz / 1000, logicalGPUCount, physicalGPUCount, sli ? ", SLI" : "",
    systemType == NV_SYSTEM_TYPE_LAPTOP ? ", notebook" : "");
}

uint32_t gpu::get_video_nvidia_version()
{
  if (!init_nvapi())
    return 0;

  if (!driver_version)
  {
    NvAPI_ShortString driverAndBranchString{};
    NvAPI_Status status = NvAPI_SYS_GetDriverAndBranchVersion(&driver_version, driverAndBranchString);
    if (status != NVAPI_OK)
      return 0;
  }

  return driver_version;
}
#endif

void *__stdcall gpu::ADL_Main_Memory_Alloc(int iSize)
{
  void *lpBuffer = malloc(iSize);
  return lpBuffer;
}

bool gpu::init_ati()
{
  if (atiDLL)
    return true;

  atiDLL = LoadLibrary("atiadlxx.dll");
  if (!atiDLL)
    atiDLL = LoadLibrary("atiadlxy.dll");

  if (!atiDLL)
    return false;

  ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(atiDLL, "ADL_Main_Control_Create");
  ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(atiDLL, "ADL_Main_Control_Destroy");
  ADL_ApplicationProfiles_ProfileOfAnApplication_Search = (ADL_APPLICATIONPROFILES_PROFILEOFANAPPLICATION_SEARCH)GetProcAddress(atiDLL,
    "ADL_ApplicationProfiles_ProfileOfAnApplication_Search");
  ADL_ApplicationProfiles_HitLists_Get =
    (ADL_APPLICATIONPROFILES_HITLISTS_GET)GetProcAddress(atiDLL, "ADL_ApplicationProfiles_HitLists_Get");
  ADL_ApplicationProfiles_System_Reload =
    (ADL_APPLICATIONPROFILES_SYSTEM_RELOAD)GetProcAddress(atiDLL, "ADL_ApplicationProfiles_System_Reload");
  ADL_ApplicationProfiles_User_Load = (ADL_APPLICATIONPROFILES_USER_LOAD)GetProcAddress(atiDLL, "ADL_ApplicationProfiles_User_Load");
  ADL_Adapter_Crossfire_Get = (ADL_ADAPTER_CROSSFIRE_GET)GetProcAddress(atiDLL, "ADL_Adapter_Crossfire_Get");
  ADL_Graphics_Platform_Get = (ADL_GRAPHICS_PLATFORM_GET)GetProcAddress(atiDLL, "ADL_Graphics_Platform_Get");
  ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(atiDLL, "ADL_Adapter_NumberOfAdapters_Get");
  ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(atiDLL, "ADL_Adapter_AdapterInfo_Get");
  ADL_Adapter_Active_Get = (ADL_ADAPTER_ACTIVE_GET)GetProcAddress(atiDLL, "ADL_Adapter_Active_Get");
  ADL_Adapter_MemoryInfo_Get = (ADL_ADAPTER_MEMORYINFO_GET)GetProcAddress(atiDLL, "ADL_Adapter_MemoryInfo_Get");
  ADL_Adapter_VideoBiosInfo_Get = (ADL_ADAPTER_VIDEOBIOSINFO_GET)GetProcAddress(atiDLL, "ADL_Adapter_VideoBiosInfo_Get");
  ADL_Overdrive5_CurrentActivity_Get =
    (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress(atiDLL, "ADL_Overdrive5_CurrentActivity_Get");
  ADL_Display_MVPUStatus_Get = (ADL_DISPLAY_MVPUSTATUS_GET)GetProcAddress(atiDLL, "ADL_Display_MVPUStatus_Get");
  ADL_Display_MVPUCaps_Get = (ADL_DISPLAY_MVPUCAPS_GET)GetProcAddress(atiDLL, "ADL_Display_MVPUCaps_Get");

  if (!ADL_Main_Control_Create || !ADL_Main_Control_Destroy || !ADL_ApplicationProfiles_ProfileOfAnApplication_Search ||
      !ADL_ApplicationProfiles_HitLists_Get || !ADL_ApplicationProfiles_System_Reload || !ADL_ApplicationProfiles_User_Load ||
      !ADL_Adapter_Crossfire_Get || !ADL_Graphics_Platform_Get || !ADL_Adapter_NumberOfAdapters_Get || !ADL_Adapter_AdapterInfo_Get ||
      !ADL_Adapter_Active_Get || !ADL_Adapter_MemoryInfo_Get || !ADL_Adapter_VideoBiosInfo_Get ||
      !ADL_Overdrive5_CurrentActivity_Get || !ADL_Display_MVPUStatus_Get || !ADL_Display_MVPUCaps_Get)
  {
    close_ati();
    return false;
  }

  return true;
}

void gpu::close_ati()
{
  FreeLibrary(atiDLL);
  atiDLL = NULL;
}

void gpu::get_video_ati_str(String &out_str)
{
  if (!init_ati())
    return;

  if (ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 0) != ADL_OK)
  {
    close_ati();
    return;
  }

  int platform = 0;
  ADL_Graphics_Platform_Get(&platform);

  int mgpuCount = AtiMultiGPUAdapters();
  if (mgpuCount < 1)
  {
    ADL_Main_Control_Destroy();
    close_ati();
    return;
  }

  int numAdapters = 0;
  ADL_Adapter_NumberOfAdapters_Get(&numAdapters);
  if (numAdapters <= 0)
  {
    ADL_Main_Control_Destroy();
    close_ati();
    return;
  }

  AdapterInfo *adapterInfo = new AdapterInfo[numAdapters];
  memset(adapterInfo, 0, sizeof(AdapterInfo) * numAdapters);
  ADL_Adapter_AdapterInfo_Get(adapterInfo, sizeof(AdapterInfo) * numAdapters);

  unsigned int adapterNo = 0;
  for (; adapterNo < numAdapters; adapterNo++)
  {
    int active = 0;
    if (ADL_Adapter_Active_Get(adapterInfo[adapterNo].iAdapterIndex, &active) == ADL_OK && active)
      break;
  }

  if (adapterNo == numAdapters)
  {
    delete[] adapterInfo;
    ADL_Main_Control_Destroy();
    close_ati();
    return;
  }

  HKEY hBaseKey, hUmdKey;
  String keyPathBase(1000, "SYSTEM\\CurrentControlSet\\Control\\Class\\%s", adapterInfo[adapterNo].strDriverPathExt);
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPathBase, 0, KEY_READ, &hBaseKey) != ERROR_SUCCESS)
  {
    debug("Failed to open '%s'", keyPathBase);
    delete[] adapterInfo;
    ADL_Main_Control_Destroy();
    close_ati();
    return;
  }

  ADLMemoryInfo memoryInfo;
  memset(&memoryInfo, 0, sizeof(memoryInfo));
  ADL_Adapter_MemoryInfo_Get(adapterInfo[adapterNo].iAdapterIndex, &memoryInfo);

  ADLPMActivity activity;
  memset(&activity, 0, sizeof(activity));
  ADL_Overdrive5_CurrentActivity_Get(adapterInfo[adapterNo].iAdapterIndex, &activity);

  char catalystVersion[MAX_REG_STR];
  catalystVersion[0] = 0;
  DWORD catalystVersionSize = MAX_REG_STR;
  RegQueryValueEx(hBaseKey, "Catalyst_Version", NULL, NULL, (LPBYTE)catalystVersion, &catalystVersionSize);

  char releaseVersion[MAX_REG_STR];
  releaseVersion[0] = 0;
  DWORD releaseVersionSize = MAX_REG_STR;
  RegQueryValueEx(hBaseKey, "ReleaseVersion", NULL, NULL, (LPBYTE)releaseVersion, &releaseVersionSize);

  unsigned int memSize = (unsigned int)(memoryInfo.iMemorySize / 1024 / 1024);

  out_str.printf(128, "%s, ver: %s (%s), %d MB %s (%d GB/s), %d MHz / %d MHz, GPUs: %d%s%s", adapterInfo[adapterNo].strAdapterName,
    catalystVersion, releaseVersion, memSize, memoryInfo.strMemoryType, (unsigned int)memoryInfo.iMemoryBandwidth / 1000,
    activity.iEngineClock / 100, activity.iMemoryClock / 100, mgpuCount, mgpuCount > 1 ? ", crossfire" : "",
    platform == 1 ? ", notebook" : "");

  RegCloseKey(hBaseKey);
  delete[] adapterInfo;
  ADL_Main_Control_Destroy();
  close_ati();
}

#endif