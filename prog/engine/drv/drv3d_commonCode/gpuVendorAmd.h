// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <util/dag_stdint.h>
#if _TARGET_PC_WIN
#include <d3d11.h>
#endif
#if HAS_ADL
#include <adl_sdk.h>
#endif
#if HAS_AMD_DX_EXT
#include <AmdDxExtDepthBoundsApi.h>
#endif


namespace gpu
{
//------------------------
// AMD
//------------------------

#if HAS_ADL
using ADL_MAIN_CONTROL_CREATE = int (*)(ADL_MAIN_MALLOC_CALLBACK, int);
using ADL_MAIN_CONTROL_DESTROY = int (*)();
using ADL_APPLICATIONPROFILES_PROFILEOFANAPPLICATION_SEARCH = int (*)(const char *, const char *, const char *, const char *,
  ADLApplicationProfile **);
using ADL_APPLICATIONPROFILES_HITLISTS_GET = int (*)(int, int *, ADLApplicationData **);
using ADL_APPLICATIONPROFILES_SYSTEM_RELOAD = int (*)();
using ADL_APPLICATIONPROFILES_USER_LOAD = int (*)();
using ADL_ADAPTER_CROSSFIRE_GET = int (*)();
using ADL_GRAPHICS_PLATFORM_GET = int (*)(int *);
using ADL_ADAPTER_NUMBEROFADAPTERS_GET = int (*)(int *);
using ADL_ADAPTER_ADAPTERINFO_GET = int (*)(LPAdapterInfo, int);
using ADL_ADAPTER_ACTIVE_GET = int (*)(int, int *);
using ADL_ADAPTER_MEMORYINFO_GET = int (*)(int, ADLMemoryInfo *);
using ADL_ADAPTER_VIDEOBIOSINFO_GET = int (*)(int, ADLBiosInfo *);
using ADL_OVERDRIVE5_CURRENTACTIVITY_GET = int (*)(int, ADLPMActivity *);
using ADL_DISPLAY_MVPUSTATUS_GET = int (*)(int, ADLMVPUStatus *);
using ADL_DISPLAY_MVPUCAPS_GET = int (*)(int, ADLMVPUCaps *);

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

bool init_adl();
void close_adl();
void get_video_amd_str(String &out_str);
#else
inline void get_video_amd_str(String &out_str) { out_str = ""; }
#endif

#if HAS_AMD_DX_EXT
#if defined _WIN64
inline static constexpr auto amd_lib_name = "atidxx64.dll";
#else
inline static constexpr auto amd_lib_name = "atidxx32.dll";
#endif
#endif

} // namespace gpu