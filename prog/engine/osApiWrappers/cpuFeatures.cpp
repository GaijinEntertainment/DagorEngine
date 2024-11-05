// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "osApiWrappers/dag_cpuFeatures.h"
#include "osApiWrappers/dag_xstateFeatures.h"
#include <cstdint>

bool cpu_feature_sse41_checked = false;
bool cpu_feature_sse42_checked = false;
bool cpu_feature_popcnt_checked = false;
bool cpu_feature_fma_checked = false;
bool cpu_feature_avx_checked = false;
bool cpu_feature_avx2_checked = false;
bool cpu_feature_fast_256bit_avx_checked = false;

#ifdef _TARGET_SIMD_SSE

#if !_MSC_VER || defined(__clang__)
#include <cpuid.h>
#else
#include <intrin.h>

static inline void __get_cpuid(uint32_t l, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  int regs[4];
  __cpuid(regs, l);
  *eax = regs[0];
  *ebx = regs[1];
  *ecx = regs[2];
  *edx = regs[3];
}
static inline void __get_cpuid_count(uint32_t l, uint32_t sl, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  int regs[4];
  __cpuidex(regs, l, sl);
  *eax = regs[0];
  *ebx = regs[1];
  *ecx = regs[2];
  *edx = regs[3];
}
#endif // !_MSC_VER || defined(__clang__)

static bool dag_check_cpu_features()
{
  enum CpuFeatures1Ecx : uint32_t // eax=1
  {
    FMA3_BIT = 1 << 12,
    SSE41_BIT = 1 << 19,
    SSE42_BIT = 1 << 20,
    POPCNT_BIT = 1 << 23,
    OSXSAVE_BIT = 1 << 27,
    AVX_BIT = 1 << 28
  };
  enum CpuFeatures70Ebx : uint32_t // eax=7, ecx=0
  {
    AVX2_BIT = 1 << 5
  };
  uint32_t eax, ebx, ecx, edx;

#if defined(_TARGET_64BIT)
  bool isX64 = true;
#else
  bool isX64 = false;
#endif

  __get_cpuid(0, &eax, &ebx, &ecx, &edx);
  bool isAmdCpu = ebx == 0x68747541 && edx == 0x69746E65 && ecx == 0x444D4163; // AuthenticAMD

  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  cpu_feature_sse41_checked = ecx & CpuFeatures1Ecx::SSE41_BIT;
  cpu_feature_sse42_checked = ecx & CpuFeatures1Ecx::SSE42_BIT;
  cpu_feature_popcnt_checked = ecx & CpuFeatures1Ecx::POPCNT_BIT;
  cpu_feature_fma_checked = ecx & CpuFeatures1Ecx::FMA3_BIT;
  bool osxsave = ecx & CpuFeatures1Ecx::OSXSAVE_BIT;
#if _TARGET_PC_WIN
  osxsave &= isAvxXStateFeatureEnabled();
#endif
  cpu_feature_avx_checked = (ecx & CpuFeatures1Ecx::AVX_BIT) && osxsave;

  uint32_t amdJaguarFamily = 0x16;
  uint32_t amdCpuFamily = (eax >> 8) & 0xf; // base family
  if (amdCpuFamily == 0xf)
    amdCpuFamily += (eax >> 20) & 0xff; // ext family
  cpu_feature_fast_256bit_avx_checked = cpu_feature_avx_checked && (!isAmdCpu || amdCpuFamily > amdJaguarFamily) && isX64;

  __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx);
  cpu_feature_avx2_checked = (ebx & CpuFeatures70Ebx::AVX2_BIT) && osxsave;
  return true;
}
static bool initialized = dag_check_cpu_features();

#endif // _TARGET_SIMD_SSE

#define EXPORT_PULL dll_pull_osapiwrappers_cpuFeatures
#include <supp/exportPull.h>
