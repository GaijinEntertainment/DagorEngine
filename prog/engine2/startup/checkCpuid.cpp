#include <supp/_platform.h>
#include <startup/dag_globalSettings.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> //memcpy

#if _TARGET_SIMD_SSE
#if !_MSC_VER || defined(__clang__)
#include <cpuid.h>
static inline void get_cpuid(int regs[4], uint32_t l)
{
  __get_cpuid(l, (uint32_t *)&regs[0], (uint32_t *)&regs[1], (uint32_t *)&regs[2], (uint32_t *)&regs[3]);
}
#else

static inline void get_cpuid(int regs[4], uint32_t l) { __cpuid(regs, l); }
#endif

#endif

void check_cpuid()
{
#if _TARGET_SIMD_SSE
  int cpuInfo[4] = {0};
  get_cpuid(cpuInfo, 1);
  const char *sse = "SSE2";
  uint32_t features = (int)cpuInfo[2];
  if (features & (1 << 9))
    sse = "SSE3";
  if (features & (1 << 19))
    sse = "SSE41";
  if (features & (1 << 20))
    sse = "SSE42";
  const char *popcnt = (features & (1 << 23)) ? " popcnt" : "";

  get_cpuid(cpuInfo, 0x80000000);
  unsigned nExIds = cpuInfo[0];
  char cpuBrandString[0x40] = {0};
  for (unsigned i = 0x80000000; i <= nExIds; ++i)
  {
    get_cpuid(cpuInfo, i);
    if (i == 0x80000002)
      memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
    else if (i == 0x80000003)
      memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
    else if (i == 0x80000004)
      memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
  }
  snprintf(::dgs_cpu_name, sizeof(::dgs_cpu_name), "<%.64s> %s%s", cpuBrandString, sse, popcnt);
#elif _TARGET_C3

#else
  snprintf(::dgs_cpu_name, sizeof(::dgs_cpu_name), "n/a");
#endif
}
