// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_string.h>
#if HAS_NVAPI
#include <nvapi.h>
#include <NvApiDriverSettings.h>
#endif


namespace gpu
{
//------------------------
// NVIDIA
//------------------------

#if HAS_NVAPI
bool init_nvapi();
NvPhysicalGpuHandle get_nv_physical_gpu();
void get_nv_gpu_memory(uint32_t &out_dedicated_kb, uint32_t &out_shared_kb, uint32_t &out_dedicated_free_kb);
void get_video_nvidia_str(String &out_str);
#else
typedef void *NvPhysicalGpuHandle;
inline void get_nv_gpu_memory(uint32_t &out_m1, uint32_t &out_m2, uint32_t &out_m3) { out_m1 = out_m2 = out_m3 = 0; }
inline void get_video_nvidia_str(String &out_str) { out_str = ""; }
#endif
} // namespace gpu