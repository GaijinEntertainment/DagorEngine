// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC_WIN

#include <util/dag_string.h>
#include <util/dag_stdint.h>
#include <d3d11.h>
#if HAS_NVAPI
#include <nvapi.h>
#include <NvApiDriverSettings.h>
#endif
#include <include/adl_sdk.h>
#include <AmdDxExtDepthBoundsApi.h>
#include <atimgpud.h>

#endif

namespace gpu
{
#if _TARGET_PC_WIN

//------------------------
// NVIDIA
//------------------------

#if HAS_NVAPI
bool init_nvapi();
const NvPhysicalGpuHandle &get_nv_physical_gpu();
void get_nv_gpu_memory(uint32_t &out_dedicated_kb, uint32_t &out_shared_kb, uint32_t &out_dedicated_free_kb);
void get_video_nvidia_str(String &out_str);
uint32_t get_video_nvidia_version();
#else
typedef void *NvPhysicalGpuHandle;
inline void get_nv_gpu_memory(uint32_t &out_m1, uint32_t &out_m2, uint32_t &out_m3) { out_m1 = out_m2 = out_m3 = 0; }
inline void get_video_nvidia_str(String &out_str) { out_str = ""; }
#endif

//------------------------
// ATI
//------------------------

typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int (*ADL_MAIN_CONTROL_DESTROY)();
typedef int (*ADL_APPLICATIONPROFILES_PROFILEOFANAPPLICATION_SEARCH)(const char *, const char *, const char *, const char *,
  ADLApplicationProfile **);
typedef int (*ADL_APPLICATIONPROFILES_HITLISTS_GET)(int, int *, ADLApplicationData **);
typedef int (*ADL_APPLICATIONPROFILES_SYSTEM_RELOAD)();
typedef int (*ADL_APPLICATIONPROFILES_USER_LOAD)();
typedef int (*ADL_ADAPTER_CROSSFIRE_GET)();
typedef int (*ADL_GRAPHICS_PLATFORM_GET)(int *);
typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int *);
typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);
typedef int (*ADL_ADAPTER_ACTIVE_GET)(int, int *);
typedef int (*ADL_ADAPTER_MEMORYINFO_GET)(int, ADLMemoryInfo *);
typedef int (*ADL_ADAPTER_VIDEOBIOSINFO_GET)(int, ADLBiosInfo *);
typedef int (*ADL_OVERDRIVE5_CURRENTACTIVITY_GET)(int, ADLPMActivity *);
typedef int (*ADL_DISPLAY_MVPUSTATUS_GET)(int, ADLMVPUStatus *);
typedef int (*ADL_DISPLAY_MVPUCAPS_GET)(int, ADLMVPUCaps *);

extern ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create;
extern ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy;
extern ADL_APPLICATIONPROFILES_PROFILEOFANAPPLICATION_SEARCH ADL_ApplicationProfiles_ProfileOfAnApplication_Search;
extern ADL_APPLICATIONPROFILES_HITLISTS_GET ADL_ApplicationProfiles_HitLists_Get;
extern ADL_APPLICATIONPROFILES_SYSTEM_RELOAD ADL_ApplicationProfiles_System_Reload;
extern ADL_APPLICATIONPROFILES_USER_LOAD ADL_ApplicationProfiles_User_Load;
extern ADL_ADAPTER_CROSSFIRE_GET ADL_Adapter_Crossfire_Get;
extern ADL_GRAPHICS_PLATFORM_GET ADL_Graphics_Platform_Get;
extern ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
extern ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get;
extern ADL_ADAPTER_ACTIVE_GET ADL_Adapter_Active_Get;
extern ADL_ADAPTER_MEMORYINFO_GET ADL_Adapter_MemoryInfo_Get;
extern ADL_ADAPTER_VIDEOBIOSINFO_GET ADL_Adapter_VideoBiosInfo_Get;
extern ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get;
extern ADL_DISPLAY_MVPUSTATUS_GET ADL_Display_MVPUStatus_Get;
extern ADL_DISPLAY_MVPUCAPS_GET ADL_Display_MVPUCaps_Get;

void *__stdcall ADL_Main_Memory_Alloc(int iSize);

bool init_ati();
void close_ati();
void get_video_ati_str(String &out_str);

#endif
} // namespace gpu