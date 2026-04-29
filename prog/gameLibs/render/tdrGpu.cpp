// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_info.h>
#include <util/dag_string.h>
#if _TARGET_PC_WIN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#define PSAPI_VERSION 2
#include <windows.h>
#include <Psapi.h> // GetProcessMemoryInfo
#endif
#include <supp/dag_cpuControl.h>

#include <perfMon/dag_cpuFreq.h> // for dump_periodic_gpu_info

#if _TARGET_PC_WIN

void dump_tdr_settings()
{
  HKEY graphicsDriversKey;
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\GraphicsDrivers", 0, KEY_READ, &graphicsDriversKey) ==
      ERROR_SUCCESS)
  {
    const char *paramNames[] = {"TdrLevel", "TdrDelay", "TdrDdiDelay", "TdrTestMode", "TdrDebugMode", "TdrLimitTime", "TdrLimitCount"};
    for (int paramNo = 0; paramNo < countof(paramNames); paramNo++)
    {
      DWORD data = 0;
      DWORD dataSize = sizeof(data);
      if (RegQueryValueEx(graphicsDriversKey, paramNames[paramNo], NULL, NULL, (LPBYTE)&data, &dataSize) == ERROR_SUCCESS)
        debug("%s=%d", paramNames[paramNo], data);
      else
      {
        uint64_t qdata = 0;
        dataSize = sizeof(qdata);
        if (RegQueryValueEx(graphicsDriversKey, paramNames[paramNo], NULL, NULL, (LPBYTE)&qdata, &dataSize) == ERROR_SUCCESS)
          debug("%s=%lld", paramNames[paramNo], qdata);
      }
    }

    RegCloseKey(graphicsDriversKey);
  }
}

void dump_proc_memory()
{
#define TO_MB(x) uint32_t((x) >> 20)
  MEMORYSTATUSEX ms{sizeof(MEMORYSTATUSEX)};
  GlobalMemoryStatusEx(&ms);
  PROCESS_MEMORY_COUNTERS_EX pmc = {sizeof(PROCESS_MEMORY_COUNTERS_EX)};
  HEAP_SUMMARY hs = {sizeof(hs)};
  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
  debug("Sys (MB): avail/total %u/%u (load %u%%); Proc (MB): working set/peak/commit %u/%u/%u", TO_MB(ms.ullAvailPhys),
    TO_MB(ms.ullTotalPhys), ms.dwMemoryLoad, TO_MB(pmc.WorkingSetSize), TO_MB(pmc.PeakWorkingSetSize), TO_MB(pmc.PrivateUsage));
#undef TO_MB
}

void dump_gpu_memory()
{
  uint32_t dedicatedMem;
  uint32_t dedicatedMemFree;
  d3d::get_current_gpu_memory_kb(&dedicatedMem, &dedicatedMemFree);
  if (dedicatedMem > 0)
  {
    debug("Dump GPU memory dedicated: %d MB, free dedicated: %d MB", dedicatedMem / 1024, dedicatedMemFree / 1024);
  }
}

void dump_gpu_freq()
{
  String gpuFreq;
  if (d3d::get_gpu_freq(gpuFreq))
    debug("GPU freq %s", gpuFreq.str());
}

void dump_gpu_temperature()
{
  int temperature = d3d::get_gpu_temperature();
  if (temperature > 0)
    debug("GPU temperature: %d'C", temperature);
}

#elif _TARGET_XBOX

#include "tdrGpu_xbox.h"

void dump_tdr_settings() {}
void dump_gpu_memory() {}
void dump_gpu_freq() {}
void dump_gpu_temperature() {}
#else
void dump_tdr_settings() {}
void dump_proc_memory() {}
void dump_gpu_memory() {}
void dump_gpu_freq() {}
void dump_gpu_temperature() {}
#endif

void dump_periodic_gpu_info()
{
#if (_TARGET_PC_WIN || _TARGET_XBOX)
  static constexpr int TEMPERATURE_PERIOD_MSEC = 60000;
  static int nextTemperatureTime = 0;
  if (get_time_msec() > nextTemperatureTime)
  {
    nextTemperatureTime = get_time_msec() + TEMPERATURE_PERIOD_MSEC;
    dump_gpu_temperature();
    dump_proc_memory();
    dump_gpu_memory();
  }
#endif
}
