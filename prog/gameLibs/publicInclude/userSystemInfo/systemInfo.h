//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_tabFwd.h>

class String;


namespace systeminfo
{
enum class ThermalStatus
{
  NotSupported = -1,
  Normal,
  Light,
  Moderate,
  Severe,
  Critical,
  Emergency,
  Shutdown,
};

void init();

bool is_64bit_os();

bool get_physical_memory(int &res);
bool get_physical_memory_free(int &res);
bool get_virtual_memory(int &res);
bool get_virtual_memory_free(int &res);


bool get_os(String &osName);
bool get_os_common_name(String &osCommonName);

bool get_cpu_info(String &cpu, String &cpuFreq, String &cpuVendor, String &cpu_series, int &cores_count);
bool get_cpu_features(String &cpu_arch, String &cpu_uarch, Tab<String> &cpu_features);
bool get_soc_info(String &soc);

bool get_mac(String &adapter, String &mac);


bool get_video_info(String &desktopResolution, String &videoCard, String &videoVendor);

struct ProcMemInfo
{
  size_t residentSizeBytes;
};
bool get_proc_mem_info(ProcMemInfo &mem_info);

struct CpuTimes
{
  struct
  {
    uint64_t kernelTime;
    uint64_t userTime;
  } process, system;
};

bool get_cpu_times(CpuTimes &cpu_times);

bool get_system_guid(String &res);

bool get_system_location_2char_code(String &location); //"RU", "US", etc.

void dump_sysinfo();

void dump_dll_names();

ThermalStatus get_thermal_state(); // From normal (0) to the worse
const char *to_string(ThermalStatus status);
int is_tablet();
int get_battery_capacity_mah();
float get_battery();
int get_is_charging();
int get_gpu_freq();
bool get_inline_raytracing_available();
String get_model_by_cpu(const char *cpu);
int get_network_connection_type();
} // namespace systeminfo
