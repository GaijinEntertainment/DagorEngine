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

unsigned dump_proc_memory()
{
#define TO_MB(x) uint32_t((x) >> 20)
  MEMORYSTATUSEX ms{sizeof(MEMORYSTATUSEX)};
  GlobalMemoryStatusEx(&ms);
  PROCESS_MEMORY_COUNTERS_EX pmc = {sizeof(PROCESS_MEMORY_COUNTERS_EX)};
  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
  debug("Sys (MB): avail/total %u/%u (load %u%%); Commit avail/limit: %u/%u; "
        "Proc (MB): working set/peak/commit %u/%u/%u",
    TO_MB(ms.ullAvailPhys), TO_MB(ms.ullTotalPhys), ms.dwMemoryLoad, TO_MB(ms.ullAvailPageFile), TO_MB(ms.ullTotalPageFile),
    TO_MB(pmc.WorkingSetSize), TO_MB(pmc.PeakWorkingSetSize), TO_MB(pmc.PrivateUsage));
  return TO_MB(ms.ullAvailPageFile);
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
unsigned dump_proc_memory() { return ~0u; }
void dump_gpu_memory() {}
void dump_gpu_freq() {}
void dump_gpu_temperature() {}
#endif

void dump_periodic_gpu_info()
{
#if (_TARGET_PC_WIN || _TARGET_XBOX)
  static int nextProcMemDumpTime = 0, nextTempDumpTime = 0;
  int timeNow = get_time_msec();
  if (timeNow >= nextTempDumpTime)
  {
    nextTempDumpTime = timeNow + 60000;
    dump_gpu_temperature();
    dump_gpu_memory();
  }
  if (timeNow >= nextProcMemDumpTime)
  {
    if (unsigned commitAvailMB = dump_proc_memory(); commitAvailMB > 3072) // To consider: tune these consts for XB1
      nextProcMemDumpTime = timeNow + 60000;
    else if (commitAvailMB > 1024) // (1024, 3072]
      nextProcMemDumpTime = timeNow + 15000;
    else // <= 1024
      nextProcMemDumpTime = timeNow + 5000;
  }
#endif
}
