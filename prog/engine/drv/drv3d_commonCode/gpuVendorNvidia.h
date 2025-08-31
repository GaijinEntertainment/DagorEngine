// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC_WIN

#include <util/dag_string.h>
#include <util/dag_stdint.h>
#if HAS_NVAPI
#include <nvapi.h>
#include <NvApiDriverSettings.h>
#endif
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

#endif
} // namespace gpu