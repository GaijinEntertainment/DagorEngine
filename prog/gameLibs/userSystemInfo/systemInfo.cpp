// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <userSystemInfo/systemInfo.h>
#include <generic/dag_smallTab.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <stdio.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_localConv.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_gpuConfig.h>
#include "uuidgen.h"


#if USE_CPU_FEATURES
#include <cpu_features/include/cpu_features_macros.h>
#if defined(CPU_FEATURES_ARCH_AARCH64)
#include <cpu_features/include/cpuinfo_aarch64.h>
#elif defined(CPU_FEATURES_ARCH_ARM)
#include <cpu_features/include/cpuinfo_arm.h>
#elif defined(CPU_FEATURES_ARCH_X86)
#include <cpu_features/include/cpuinfo_x86.h>
#else
#error Some cpu-specific header should be included
#endif
#endif


#if _TARGET_PC_WIN
#include <windows.h>
#include <rpc.h>
#include <iphlpapi.h>
#include <Sddl.h>
#include <Lmcons.h>

#define _WIN32_DCOM

#pragma warning(push)
#pragma warning(disable : 4986)
#include <comdef.h>
#pragma warning(pop)

#include <Wbemidl.h>
#include <OleAuto.h>

#include <psapi.h>
#include <process.h>

#include <EASTL/array.h>
#include <EASTL/vector.h>

#elif _TARGET_PC_MACOSX || _TARGET_PC_LINUX
#include <unistd.h>
#include <stdlib.h>
#endif

#if _TARGET_PC_MACOSX
#include <sys/sysctl.h>
#endif

#if _TARGET_PC_MACOSX
extern const char *macosx_get_def_mac_address(unsigned char *mac);
extern bool macosx_get_desktop_res(int &wd, int &ht);
extern const char *macosx_get_os_ver();
extern int64_t macosx_get_phys_mem();
extern int64_t macosx_get_virt_mem();
extern const char *macosx_get_location();

#include <mach/mach.h>

#include <sys/param.h>
#include <sys/mount.h>

#elif _TARGET_PC_LINUX || _TARGET_ANDROID

#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <dirent.h>
#include <ctype.h>
#include <locale.h>
#if _TARGET_ANDROID
#include <sys/system_properties.h>
#include <unistd.h>
#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <jni.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <osApiWrappers/dag_files.h>
#include <android/android_platform.h>
#include <android/and_java_helpers.h>
#endif

static bool check_token(const char *str, const char *token) { return strstr(str, token) == str; }

#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX
#include "systemInfo_xbox.h"
#elif _TARGET_IOS
#include <sys/sysctl.h>
#include <ioSys/dag_dataBlock.h>

#include <EASTL/optional.h>

extern int ios_get_thermal_state();
extern bool ios_is_ipad();
extern size_t ios_get_available_memory();
extern size_t ios_get_phys_footprint();
extern void ios_activate_battery_monitoring();
extern float ios_get_battery_level();
extern int ios_get_battery_status();
extern int ios_get_network_connection_type();
extern void ios_get_os_version(int &major, int &minor, int &patch);
#endif


namespace systeminfo
{
extern void private_init();

#if USE_CPU_FEATURES

struct CFX86
{};
struct CFAarch64
{};
struct CFArm
{};


template <typename ARCH>
struct SelectCpuFeatures
{};

#if defined(CPU_FEATURES_ARCH_AARCH64)

template <>
struct SelectCpuFeatures<CFAarch64>
{
  using Features = cpu_features::Aarch64Features;
  using FeaturesEnum = cpu_features::Aarch64FeaturesEnum;
  static constexpr FeaturesEnum LAST = cpu_features::AARCH64_LAST_;
  static constexpr int (*GetFeaturesEnumValue)(const Features *, FeaturesEnum) = &cpu_features::GetAarch64FeaturesEnumValue;
  static constexpr const char *(*GetFeaturesEnumName)(FeaturesEnum) = &cpu_features::GetAarch64FeaturesEnumName;
};

#elif defined(CPU_FEATURES_ARCH_ARM)

template <>
struct SelectCpuFeatures<CFArm>
{
  using Features = cpu_features::ArmFeatures;
  using FeaturesEnum = cpu_features::ArmFeaturesEnum;
  static constexpr FeaturesEnum LAST = cpu_features::ARM_LAST_;
  static constexpr int (*GetFeaturesEnumValue)(const Features *, FeaturesEnum) = &cpu_features::GetArmFeaturesEnumValue;
  static constexpr const char *(*GetFeaturesEnumName)(FeaturesEnum) = &cpu_features::GetArmFeaturesEnumName;
};

#elif defined(CPU_FEATURES_ARCH_X86)

template <>
struct SelectCpuFeatures<CFX86>
{
  using Features = cpu_features::X86Features;
  using FeaturesEnum = cpu_features::X86FeaturesEnum;
  static constexpr FeaturesEnum LAST = cpu_features::X86_LAST_;
  static constexpr int (*GetFeaturesEnumValue)(const Features *, FeaturesEnum) = &cpu_features::GetX86FeaturesEnumValue;
  static constexpr const char *(*GetFeaturesEnumName)(FeaturesEnum) = &cpu_features::GetX86FeaturesEnumName;
};

#else
#error Define a SelectCpuFeatures<> specialization
#endif

template <typename ARCH>
static void fill_cpu_features_internal(const typename SelectCpuFeatures<ARCH>::Features &features, Tab<String> &dst)
{
  using FeaturesEnum = typename SelectCpuFeatures<ARCH>::FeaturesEnum;
  for (int i = 0; i < int(SelectCpuFeatures<ARCH>::LAST); i++)
  {
    FeaturesEnum feature = FeaturesEnum(i);
    if (SelectCpuFeatures<ARCH>::GetFeaturesEnumValue(&features, feature))
    {
      dst.push_back(String(SelectCpuFeatures<ARCH>::GetFeaturesEnumName(feature)));
    }
  }
}


bool get_cpu_features(String &cpu_arch, String &cpu_uarch, Tab<String> &cpu_features)
{
#if defined(CPU_FEATURES_ARCH_AARCH64)
  cpu_arch = "aarch64";
  const cpu_features::Aarch64Info cpuInfo = cpu_features::GetAarch64Info();
  fill_cpu_features_internal<CFAarch64>(cpuInfo.features, cpu_features);
  return true;
#elif defined(CPU_FEATURES_ARCH_ARM)
  cpu_arch = "arm";
  const cpu_features::ArmInfo cpuInfo = cpu_features::GetArmInfo();
  fill_cpu_features_internal<CFArm>(cpuInfo.features, cpu_features);
  return true;
#elif defined(CPU_FEATURES_ARCH_X86)
#if defined(CPU_FEATURES_ARCH_X86_64)
  cpu_arch = "x86_64";
#else
  cpu_arch = "x86";
#endif
  const cpu_features::X86Info cpuInfo = cpu_features::GetX86Info();
  cpu_uarch = cpu_features::GetX86MicroarchitectureName(cpu_features::GetX86Microarchitecture(&cpuInfo));
  fill_cpu_features_internal<CFX86>(cpuInfo.features, cpu_features);
  return true;
#else
  // nothing to do here
#endif
  return false;
}

#elif __e2k__

bool get_cpu_features(String &cpu_arch, String & /*cpu_uarch*/, Tab<String> & /*cpu_features*/)
{
  cpu_arch = "e2k";
  return true;
}

#else

bool get_cpu_features(String & /*cpu_arch*/, String & /*cpu_uarch*/, Tab<String> & /*cpu_features*/) { return false; }

#endif // USE_CPU_FEATURES


#if _TARGET_PC

void get_processor_freq(const char *processorName, char *strMHz, int size)
{
  const char *p = strrchr(processorName, '@');
  if (!p)
  {
    SNPRINTF(strMHz, size, "?");
    return;
  }

  p++;
  while (*p == ' ')
    p++;
  float val = atof(p);
  if (strstr(p, "GHz"))
    val *= 1000.0f;
  else if (strstr(p, "MHz"))
    val *= 1;
  else
    val = -1;
  if (val < 0)
    SNPRINTF(strMHz, size, "%s", p);
  else
    SNPRINTF(strMHz, size, "%u MHz", int(val / 100) * 100); // round by 100MHz
}


bool get_cpu_info(String &cpu, String &cpuFreq, String &cpuVendor, String &cpu_series, int &cores_count)
{
#if _TARGET_PC_WIN
  HKEY hKey;
  int num_cores = 128;
#else
  static constexpr int MAX_PATH = 260;
  typedef size_t ULONG;
  typedef unsigned int DWORD;
  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  char processorName[MAX_PATH];
  char regPath[MAX_PATH];
  char cpuSeriesCores[MAX_PATH];
  char strMHz[64];
  ULONG nMHz;
  DWORD t;
  ULONG l;
  const char *cpuSeries = "";

  for (int i = 0; i < num_cores; i++)
  {
#if _TARGET_PC_WIN
    SNPRINTF(regPath, sizeof(regPath), "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d", i);

    if (::RegOpenKey(HKEY_LOCAL_MACHINE, regPath, &hKey) != ERROR_SUCCESS)
    {
      num_cores = i;
      break;
    }
#endif

    // get info only from #0 core
    if (!i)
    {
      // MHz
      l = sizeof(ULONG);
#if _TARGET_PC_WIN
      if (::RegQueryValueEx(hKey, "~MHz", NULL, &t, (PUCHAR)&nMHz, &l) == ERROR_SUCCESS)
      {
        SNPRINTF(strMHz, sizeof(strMHz), "%u MHz", int((nMHz / 100) * 100)); // round by 100MHz
      }
      else
#elif _TARGET_PC_MACOSX
      l = sizeof(processorName) - 1;
      if (sysctlbyname("machdep.cpu.brand_string", &processorName, &l, NULL, 0) == 0)
      {
        processorName[sizeof(processorName) - 1] = 0;
        get_processor_freq(processorName, strMHz, sizeof(strMHz));
      }
      else
#endif
      {
        SNPRINTF(strMHz, sizeof(strMHz), "?");
      }

      // Name
      l = sizeof(processorName) - 1;
#if _TARGET_PC_WIN
      if (::RegQueryValueEx(hKey, "ProcessorNameString", NULL, &t, (PUCHAR)processorName, &l) == ERROR_SUCCESS)
      {
        processorName[sizeof(processorName) - 1] = 0;
      }
      else
#elif _TARGET_PC_MACOSX
      if (sysctlbyname("machdep.cpu.brand_string", &processorName, &l, NULL, 0) == 0)
      {
        processorName[sizeof(processorName) - 1] = 0;
      }
      else
#endif
      {
        SNPRINTF(processorName, sizeof(processorName), "Unknown name");
      }


#if _TARGET_PC_LINUX
      FILE *file = fopen("/proc/cpuinfo", "r");
      if (!file)
        return false;

      char buffer[1024];
      char info[MAX_PATH];
      info[MAX_PATH - 1] = 0;

      while (fgets(buffer, sizeof(buffer), file))
      {
        const char *spl = ": ";
        const char *s = strstr(buffer, spl);
        if (!s)
          continue;

        strncpy(info, s + strlen(spl), MAX_PATH - 1);
        if (info[strlen(info) - 1] == '\n')
          info[strlen(info) - 1] = 0;

        if (check_token(buffer, "processor"))
        {
          int n = atoi(info);
          if (n > 0)
            break;
        }

        if (check_token(buffer, "model name"))
        {
          strncpy(processorName, info, sizeof(processorName) - 1);
          processorName[sizeof(processorName) - 1] = 0;
          get_processor_freq(processorName, strMHz, sizeof(strMHz));
          continue;
        }

        if (check_token(buffer, "cpu cores"))
        {
          i = atoi(info);
          continue;
        }
      }
      fclose(file);
#endif

      const char *CPU_VENDOR = "cpuVendor";
      if (strstr(processorName, "AMD"))
        cpuVendor = "AMD";
      else if (strstr(processorName, "Intel"))
        cpuVendor = "Intel";
      else
        cpuVendor = "Other";


      cpu = processorName;
      cpuFreq = strMHz;

      const char *cpuTagSeries[] = {"AMD Phenom(tm) II", "AMD Phenom(tm)", "AMD Athlon(tm) II X2", "AMD Athlon(tm) II",
        "AMD Athlon(tm) 64 X2", "AMD Athlon(tm) 64", "AMD Athlon(tm) X2", "AMD Athlon(tm)", "AMD FX(tm)", "AMD A8", "AMD A6", "AMD A4",
        "AMD Turion(tm)", "AMD Turion(tm) II", "AMD Sempron(tm)", "AMD Sempron(tm) II", "AMD E2", "AMD E", "AMD C", "AMD Z",

        "Intel(R) Core(TM) i9", "Intel(R) Core(TM) i7", "Intel(R) Core(TM) i5", "Intel(R) Core(TM) i3", "Intel(R) Core(TM)2 Duo",
        "Intel(R) Core(TM)2 Quad", "Intel(R) Core(TM)2", "Intel(R) Pentium(R) Dual", "Intel(R) Pentium(R) 4", "Intel(R) Pentium(R) D",
        "Intel(R) Pentium(R)", "Intel(R) Celeron(R)", "Intel(R) Xeon(R)", "Pentium(R) Dual-Core"};

      for (int j = 0; j < countof(cpuTagSeries); j++)
      {
        if (strstr(processorName, cpuTagSeries[j]))
        {
          cpuSeries = cpuTagSeries[j];
          break;
        }
      }

      if (!cpuSeries[0])
      {
        if (strstr(processorName, "AMD"))
        {
          cpuSeries = "AMD Other";
        }
        else if (strstr(processorName, "Intel") || strstr(processorName, "Pentium") || strstr(processorName, "Celeron") ||
                 strstr(processorName, "Xeon"))
        {
          cpuSeries = "Intel Other";
        }
        else
        {
          cpuSeries = "Other CPU";
        }
      }
    }

#if _TARGET_PC_WIN
    ::RegCloseKey(hKey);
#endif
  }

  SNPRINTF(cpuSeriesCores, sizeof(cpuSeriesCores), "%s - cores:%d", cpuSeries, num_cores);
  cpu_series = cpuSeriesCores;
  cores_count = num_cores;
  return true;
}

bool get_video_info(String &desktopResolution, String &videoCard, String &videoVendor)
{
  char buffer[128];

  desktopResolution = "";
  videoCard = d3d::get_device_name();
  videoVendor = d3d_get_vendor_name(d3d_get_gpu_cfg().primaryVendor);
#if _TARGET_PC_WIN
  DEVMODE devMode;
  ZeroMemory(&devMode, sizeof(DEVMODE));
  devMode.dmSize = sizeof(DEVMODE);

  ::EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devMode);
  SNPRINTF(buffer, sizeof(buffer), "%u x %u", (int)devMode.dmPelsWidth, (int)devMode.dmPelsHeight);
  desktopResolution = buffer;
#else
  int desktop_w = 0, desktop_h = 0;
#if _TARGET_PC_MACOSX
  if (macosx_get_desktop_res(desktop_w, desktop_h))
#else
  if (d3d::is_inited() && d3d::get_render_target_size(desktop_w, desktop_h, 0, 0))
#endif
  {
    SNPRINTF(buffer, sizeof(buffer), "%u x %u", desktop_w, desktop_h);
    desktopResolution = buffer;
  }
#endif

  return true;
}


#if _TARGET_PC_MACOSX

typedef struct task_basic_info TaskBasicInfo;

bool get_physical_memory(int &res)
{
  res = (macosx_get_phys_mem() / (1024 * 1024 * 1000)) * 1024;
  return true;
}

bool get_physical_memory_free(int &res)
{
  // TO DO
  return false;
}

bool get_virtual_memory(int &res)
{
  res = (macosx_get_virt_mem() / (1024 * 1024 * 1000)) * 1024;
  return true;
}

bool get_virtual_memory_free(int &res)
{
  // TO DO
  return false;
}

bool get_os(String &res)
{
  res = "Mac OS X ";
  res += macosx_get_os_ver();
  return true;
}

bool get_os_common_name(String &res) { return get_os(res); }

bool get_mac(String &adapter, String &mac_out)
{
  unsigned char mac[8] = {0};
  const char *adapterModel = macosx_get_def_mac_address(mac);

  char macStr[32];
  SNPRINTF(macStr, sizeof(macStr), "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  mac_out = macStr;
  adapter = adapterModel ? adapterModel : "";
  return true;
}

bool get_proc_mem_info(ProcMemInfo &mem_info)
{
  TaskBasicInfo t_info;
  mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

  if (KERN_SUCCESS == task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count))
  {
    mem_info.residentSizeBytes = t_info.resident_size * getpagesize();
    return true;
  }

  return false;
}

bool get_cpu_times(CpuTimes & /*cpu_times*/) { return false; }

bool get_system_location_2char_code(String &location)
{
  const char *loc = ::macosx_get_location();
  if (loc && *loc)
  {
    location = loc;
    return true;
  }

  return false;
}

ThermalStatus get_thermal_state() { return ThermalStatus::Normal; }
int get_battery_capacity_mah() { return -1; }

float get_battery() { return -1.f; }
int get_is_charging() { return -1; };
int get_gpu_freq() { return 0; }

#elif _TARGET_PC_WIN

bool get_physical_memory(int &res)
{
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  ::GlobalMemoryStatusEx(&statex);
  res = (statex.ullTotalPhys / (1024 * 1024 * 1000)) * 1024;
  return true;
}

bool get_physical_memory_free(int &res)
{
  // TO DO
  return false;
}

bool get_virtual_memory(int &res)
{
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  ::GlobalMemoryStatusEx(&statex);
  res = (statex.ullTotalVirtual / (1024 * 1024 * 1000)) * 1024;
  return true;
}

bool get_virtual_memory_free(int &res)
{
  MEMORYSTATUSEX ms = {sizeof(MEMORYSTATUSEX)};
  GlobalMemoryStatusEx(&ms);
  res = ms.ullAvailVirtual >> 20;
  return true;
}

static void *wine_get_version_proc()
{
  const HMODULE handle = ::GetModuleHandle("ntdll.dll");
  return (handle != 0) ? ::GetProcAddress(handle, "wine_get_version") : NULL;
}

static bool get_wine_version(String &res)
{
  typedef const char *(CDECL * wine_get_version_t)();
  const wine_get_version_t pwine_get_version = wine_get_version_t(wine_get_version_proc());

  if (pwine_get_version == NULL)
    return false;

  char buffer[128];
  SNPRINTF(buffer, sizeof(buffer), "Wine_%s", pwine_get_version());
  res = buffer;

  return true;
}

namespace build_number
{
constexpr DWORD WINDOWS_11 = 22000;
constexpr DWORD WINDOWS_SERVER_2019 = 17763;
constexpr DWORD WINDOWS_SERVER_2022 = 20348;
} // namespace build_number

static bool get_windows_version(bool &isWorkstation, DWORD &versionMajor, DWORD &versionMinor, DWORD &buildNumber)
{
  OSVERSIONINFOEXW osvi;
  ZeroMemory(&osvi, sizeof(osvi));
  osvi.dwOSVersionInfoSize = sizeof(osvi);

  if (!get_version_ex(&osvi))
    return false;

  isWorkstation = (osvi.wProductType == VER_NT_WORKSTATION);
  versionMajor = osvi.dwMajorVersion;
  versionMinor = osvi.dwMinorVersion;
  buildNumber = osvi.dwBuildNumber;

  if (versionMajor == 10 && buildNumber >= build_number::WINDOWS_11)
    versionMajor = 11;

  return true;
}


static void get_windows_name(bool isWorkstation, DWORD versionMajor, DWORD versionMinor, String &osName)
{
  char buf[128];
  SNPRINTF(buf, sizeof(buf), "Windows %s v%d.%d %s", isWorkstation ? "Workstation" : "Server", (int)versionMajor, (int)versionMinor,
    is_64bit_os() ? "64-bit" : "32-bit");
  osName = buf;
}


bool get_os(String &osName)
{
  if (get_wine_version(osName))
    return true;

  bool isWorkstation;
  DWORD versionMajor, versionMinor, buildNumber;
  if (get_windows_version(isWorkstation, versionMajor, versionMinor, buildNumber))
    get_windows_name(isWorkstation, versionMajor, versionMinor, osName);
  else
    osName = "Unknown Windows version";

  return true;
}


static const char *get_windows_10_server_name(DWORD buildNumber)
{
  if (buildNumber >= build_number::WINDOWS_SERVER_2022)
    return "Server 2022";
  else if (buildNumber >= build_number::WINDOWS_SERVER_2019)
    return "Server 2019";
  return "Server 2016";
}


bool get_os_common_name(String &osCommonName)
{
  if (get_wine_version(osCommonName))
    return true;

  SYSTEM_INFO si;
  ZeroMemory(&si, sizeof(si));
  GetNativeSystemInfo(&si);
  int bits = 32;
  if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
    bits = 64;

  bool isWorkstation = true;
  DWORD versionMajor = 0, versionMinor = 0, buildNumber = 0;
  get_windows_version(isWorkstation, versionMajor, versionMinor, buildNumber);
  // Don't check result, switch default does it

  char buffer[128];

  switch (versionMajor)
  {
    case 5:
      switch (versionMinor)
      {
        case 0: _snprintf(buffer, sizeof(buffer), "Windows 2000"); break;
        case 1: _snprintf(buffer, sizeof(buffer), "Windows XP %dbit", bits); break;
        case 2: _snprintf(buffer, sizeof(buffer), "Windows %s %dbit", isWorkstation ? "XP" : "Server 2003", bits); break;
      }
      break;
    case 6:
      switch (versionMinor)
      {
        case 0: _snprintf(buffer, sizeof(buffer), "Windows %s %dbit", isWorkstation ? "Vista" : "Server 2008", bits); break;
        case 1: _snprintf(buffer, sizeof(buffer), "Windows %s %dbit", isWorkstation ? "7" : "Server 2008 R2", bits); break;
        case 2: _snprintf(buffer, sizeof(buffer), "Windows %s %dbit", isWorkstation ? "8" : "Server 2012", bits); break;
        case 3: _snprintf(buffer, sizeof(buffer), "Windows %s %dbit", isWorkstation ? "8.1" : "Server 2012 R2", bits); break;
        case 4: _snprintf(buffer, sizeof(buffer), "Windows 10 Preview %dbit", bits); break;
      }
      break;
    case 10:
      _snprintf(buffer, sizeof(buffer), "Windows %s %dbit", isWorkstation ? "10" : get_windows_10_server_name(buildNumber), bits);
      break;
    case 11:
      _snprintf(buffer, sizeof(buffer), "Windows %s %dbit", isWorkstation ? "11" : get_windows_10_server_name(buildNumber), bits);
      break;
    default:
      _snprintf(buffer, sizeof(buffer), "Unknown Windows %s v%d.%d %dbit", isWorkstation ? "Workstation" : "Server", versionMajor,
        versionMinor, bits);
  }

  buffer[sizeof(buffer) - 1] = '\0';
  osCommonName = buffer;
  return true;
}


bool get_proc_mem_info(ProcMemInfo &mem_info)
{
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
  {
    mem_info.residentSizeBytes = pmc.WorkingSetSize;
    return true;
  }

  return false;
}

bool get_cpu_times(CpuTimes &cpu_times)
{
#define FILETIME_TO_INT64(ft) (int64_t(ft.dwHighDateTime) << 32) | (int64_t(ft.dwLowDateTime))

  FILETIME sUserT, sKernelT, sIdleT;
  if (GetSystemTimes(&sIdleT, &sKernelT, &sUserT))
  {
    cpu_times.system.kernelTime = FILETIME_TO_INT64(sKernelT);
    cpu_times.system.userTime = FILETIME_TO_INT64(sUserT);
  }
  else
    return false;

  FILETIME pStartT, pExitT, pUserT, pKernelT;
  if (GetProcessTimes(GetCurrentProcess(), &pStartT, &pExitT, &pKernelT, &pUserT))
  {
    cpu_times.process.kernelTime = FILETIME_TO_INT64(pKernelT);
    cpu_times.process.userTime = FILETIME_TO_INT64(pUserT);
  }
  else
    return false;

#undef FILETIME_TO_INT64

  return true;
}

bool get_mac(String &adapter, String &mac_str)
{
  adapter = "";
  // get default MAC address
  UUID uuid;
  if (::UuidCreateSequential(&uuid) != RPC_S_OK)
  {
    // cannot get ethernet/token-ring hardware address
    return false;
  }

  BYTE mac[6];
  for (int i = 0; i < 6; ++i)
  {
    // RFC 4122:
    // "For UUID version 1, the node field consists of an IEEE 802 MAC
    // address, usually the host address. For systems with multiple IEEE
    // 802 addresses, any available one can be used."

    mac[i] = uuid.Data4[2 + i]; // bytes 2-7 are MAC address
  }

  char macStr[32];
  SNPRINTF(macStr, sizeof(macStr), "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  mac_str = macStr;

  // get default adapter name
  ULONG bufferLength = 0;
  if (::GetAdaptersInfo(NULL, &bufferLength) != ERROR_BUFFER_OVERFLOW)
    return false;

  SmallTab<BYTE> buffer;
  buffer.resize(bufferLength);
  mem_set_0(buffer);

  IP_ADAPTER_INFO *ptrAdapterInfo = (IP_ADAPTER_INFO *)buffer.data();

  if (::GetAdaptersInfo(ptrAdapterInfo, &bufferLength) != NO_ERROR)
    return false;

  // find default adapter
  for (; ptrAdapterInfo != NULL; ptrAdapterInfo = ptrAdapterInfo->Next)
  {
    if (ptrAdapterInfo->AddressLength == 6 && ::memcmp(mac, ptrAdapterInfo->Address, 6) == 0)
    {
      if (ptrAdapterInfo->Description[0] != 0)
      {
        adapter = ptrAdapterInfo->Description;
      }

      break;
    }
  }

  return true;
}

bool get_system_location_2char_code(String &location)
{
  location.resize(3);

  if (::GetGeoInfoA(::GetUserGeoID(GEOCLASS_NATION), GEO_ISO2, location.data(), location.size(), 0))
  {
    location[0] = toupper(location[0]);
    location[1] = toupper(location[1]);
    location[2] = 0;
    return true;
  }

  return false;
}

ThermalStatus get_thermal_state() { return ThermalStatus::Normal; }
int get_battery_capacity_mah() { return -1; }

float get_battery() { return -1.f; }
int get_is_charging() { return -1; };
int get_gpu_freq() { return 0; }

#elif _TARGET_PC_LINUX

typedef struct sysinfo Sysinfo;
typedef struct ifreq Ifreq;
typedef struct ifconf Ifconf;

bool get_physical_memory(int &res)
{
  Sysinfo info;
  sysinfo(&info);
  res = info.totalram * info.mem_unit / (1024 * 1024);
  return true;
}

bool get_physical_memory_free(int &res)
{
  Sysinfo info;
  sysinfo(&info);
  res = info.freeram * info.mem_unit / (1024 * 1024);
  return true;
}


bool get_virtual_memory(int &res)
{
  Sysinfo info;
  sysinfo(&info);
  res = info.totalswap * info.mem_unit / (1024 * 1024);
  return true;
}

bool get_virtual_memory_free(int &res)
{
  Sysinfo info;
  sysinfo(&info);
  res = info.freeswap * info.mem_unit / (1024 * 1024);
  return true;
}

static bool get_strvalue(const char *buffer, const char *token, char *value, int length)
{
  const char *s = strstr(buffer, token);
  if (!s || s != buffer)
    return false;
  const char *s1 = strstr(s + strlen(token), "=");
  strncpy(value, s1 + 1, length - 1);
  value[length - 1] = 0;
  return true;
}


static void remove_quotes(char *s)
{
  char *s1 = strchr(s, '"');
  char *s2 = strrchr(s, '"');
  if (s1 == s2)
    return;
  *s2 = 0;
  memmove(s, s1 + 1, strlen(s1));
}

bool get_os(String &res)
{
  const char *filenames[] = {"/etc/lsb-release", "/etc/os-release", "/etc/system-release"};
  const char *tokens[] = {"DISTRIB_ID", "DISTRIB_RELEASE", "NAME", "VERSION", "NAME", "VERSION"};
  FILE *file;

  for (int i = 0; i < countof(filenames); i++)
  {
    file = fopen(filenames[i], "r");
    if (file)
    {
      char buffer[MAX_PATH];
      char name[MAX_PATH];
      char ver[MAX_PATH];

      name[0] = 0;
      ver[0] = 0;

      while (fgets(buffer, sizeof(buffer), file))
      {
        if (buffer[strlen(buffer) - 1] == '\n')
          buffer[strlen(buffer) - 1] = 0;

        if (get_strvalue(buffer, tokens[i * 2 + 0], name, sizeof(name)))
          continue;
        if (get_strvalue(buffer, tokens[i * 2 + 1], ver, sizeof(ver)))
          continue;
      }
      fclose(file);

      if (*name)
      {
        remove_quotes(name);
        remove_quotes(ver);

        res.printf(MAX_PATH, "%s (%s)", name, ver);
        return true;
      }
    }
  }

  utsname uname_data;
  uname(&uname_data);
  res.printf(MAX_PATH, "%s %s (%s %s)", uname_data.sysname, uname_data.version, uname_data.release, uname_data.machine);
  return true;
}

bool get_os_common_name(String &res) { return get_os(res); }

bool get_mac(String &adapter, String &mac_out)
{
  Ifreq ifr;
  Ifconf ifc;
  char buf[1024];
  bool success = false;

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock == -1)
    return false;

  memset(&ifr, 0, sizeof(ifr));
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  ioctl(sock, SIOCGIFCONF, &ifc);
  Ifreq *it = ifc.ifc_req;
  const Ifreq *const end = it + (ifc.ifc_len / sizeof(Ifreq));
  for (; it != end; ++it)
  {
    strncpy(ifr.ifr_name, it->ifr_name, sizeof(ifr.ifr_name) - 1);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
    {
      if (!(ifr.ifr_flags & IFF_LOOPBACK))
      {
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
        {
          success = true;
          break;
        }
      }
    }
  }

  close(sock);
  if (!success)
    return false;

  char macStr[32];
  unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
  SNPRINTF(macStr, sizeof(macStr), "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  mac_out = macStr;
  adapter = "";
  return true;
}

bool get_proc_mem_info(ProcMemInfo &mem_info)
{
  FILE *fp = fopen("/proc/self/status", "r");
  if (!fp)
    return 0;

  char buf[128];
  char *p = NULL;
  const char rssKey[] = "VmRSS";
  while (fgets(buf, sizeof(buf), fp))
  {
    if (!strncmp(buf, rssKey, sizeof(rssKey)))
    {
      p = buf;
      size_t len = strlen(buf);
      buf[len - 3] = '\0'; // Trims the ' kB' part
      while (*p && !isdigit(*p))
        p++;
      break;
    }
  }

  if (p && *p)
  {
    mem_info.residentSizeBytes = atoi(p) * 1024;
    return true;
  }

  return false;
}

bool get_cpu_times(CpuTimes & /*cpu_times*/) { return false; }

bool get_system_location_2char_code(String &location)
{
  const char *loc = ::setlocale(LC_MESSAGES, NULL);

  if (loc && isalpha(loc[0]) && isalpha(loc[1]))
  {
    location.resize(3);
    location[2] = 0;

    if (loc[2] == '_' && isalpha(loc[3]) && isalpha(loc[4]))
    {
      location[0] = toupper(loc[3]);
      location[1] = toupper(loc[4]);
    }
    else
    {
      location[0] = toupper(loc[0]);
      location[1] = toupper(loc[1]);
    }

    return true;
  }

  return false;
}

ThermalStatus get_thermal_state() { return ThermalStatus::Normal; }

int get_battery_capacity_mah() { return -1; }

float get_battery() { return -1.f; }
int get_is_charging() { return -1; };
int get_gpu_freq() { return 0; }

#endif // _TARGET_PC_WIN, _TARGET_PC_MACOSX, _TARGET_PC_LINUX

#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX

#elif _TARGET_IOS

ThermalStatus get_thermal_state()
{
  switch (ios_get_thermal_state())
  {
    case 0: return ThermalStatus::Normal;
    case 1: return ThermalStatus::Light;
    case 2: return ThermalStatus::Severe;
    case 3: return ThermalStatus::Critical;
    default: return ThermalStatus::Normal;
  }
}

String ios_get_device_code_name()
{
  // The table of correspondence of code names to marketing names can be found here
  // https://gist.github.com/adamawolf/3048717 or here https://en.wikipedia.org/wiki/List_of_iOS_and_iPadOS_devices
  size_t size;
  sysctlbyname("hw.machine", NULL, &size, NULL, 0);
  char *machine = new char[size];
  sysctlbyname("hw.machine", machine, &size, NULL, 0);
  String result(machine);
  delete[] machine;
  return result;
}

bool get_cpu_info(String &cpu, String & /*cpuFreq*/, String &cpuVendor, String & /*cpu_series*/, int &cores_count)
{
  cpu = ios_get_device_code_name(); // Not exactly a CPU, but it is the best place to report the device model.
  cpuVendor = "Apple";
  cores_count = 1;
  uint32_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  if (!sysctlbyname("hw.logicalcpu_max", &sysctl_value, &length, NULL, 0))
    cores_count = static_cast<int>(sysctl_value);

  return true;
}

bool get_physical_memory_free(int &res)
{
  res = ios_get_available_memory() >> 20;
  return true;
}

bool get_virtual_memory_free(int &res) { return get_physical_memory_free(res); }

bool get_physical_memory(int &res)
{
  res = (ios_get_available_memory() + ios_get_phys_footprint()) >> 20;
  return true;
}

bool get_virtual_memory(int &res) { return get_physical_memory(res); }

bool get_os(String &res)
{
  int major, minor, patch;
  ios_get_os_version(major, minor, patch);
  res = String(0, "iOS %d.%d.%d", major, minor, patch);
  return true;
}
bool get_os_common_name(String &) { return false; }
bool get_mac(String &, String &) { return false; }
bool get_proc_mem_info(ProcMemInfo & /*mem_info*/) { return false; }
bool get_cpu_times(CpuTimes & /*cpu_times*/) { return false; }
bool get_video_info(String &desktopResolution, String &videoCard, String &videoVendor)
{
  // todo get desktop resolution
  desktopResolution = "";
  videoCard = d3d::get_device_name();
  videoVendor = "";

  return true;
}
bool get_system_location_2char_code(String &location) { return false; }

void init_battery_monitoring() { ios_activate_battery_monitoring(); };

static constexpr int UNKNOWN_CAPACITY = -1;

int get_battery_from_device_list()
{
  const String cpuModel = []() {
    String cpu, cpuFreqDummy, cpuVendorDummy, cpuSeriesInfoDummy;
    int cpuCoresDummy = 0;
    systeminfo::get_cpu_info(cpu, cpuFreqDummy, cpuVendorDummy, cpuSeriesInfoDummy, cpuCoresDummy);
    return systeminfo::get_model_by_cpu(cpu.c_str());
  }();

  const auto returnDefault = [&](const char *errMsg) {
    logerr(errMsg);
    return UNKNOWN_CAPACITY;
  };

  DataBlock modelsBlk{"ios-devices-batteries.blk"};

  if (modelsBlk.isEmpty())
    return returnDefault("Unable to load iOS battery list");

  const DataBlock *model = modelsBlk.getBlockByName(cpuModel.c_str());
  if (!model)
    return returnDefault("Unknown model");

  return model->getInt("batterySize", UNKNOWN_CAPACITY);
}

int get_battery_capacity_mah()
{
  static int batterySize = get_battery_from_device_list();
  return batterySize;
}

float get_battery() { return ios_get_battery_level(); }
int get_is_charging()
{
  int state = ios_get_battery_status();
  switch (state)
  {
    case 0: return -1; // UIDeviceBatteryStateUnknown
    case 1: return 0;  // UIDeviceBatteryStateUnplugged
    case 2: return 1;  // UIDeviceBatteryStateCharging
    case 3: return 0;  // UIDeviceBatteryStateFull
    default: return -1;
  }
  return -1;
};

int get_gpu_freq() { return 0; }

#elif _TARGET_ANDROID
bool get_proc_mem_info_by_param(const char *name, int &result)
{
  FILE *fp = fopen("/proc/meminfo", "r");
  if (!fp)
    return 0;

  char buf[128];
  char *p = NULL;

  while (fgets(buf, sizeof(buf), fp))
  {
    if (!strncmp(buf, name, strlen(name)))
    {
      p = buf;
      size_t len = strlen(buf);
      buf[len - 3] = '\0'; // Trims the ' kB' part
      while (*p && !isdigit(*p))
        p++;
      break;
    }
  }

  fclose(fp);

  if (p && *p)
  {
    result = atoi(p) / 1024; // this value in KB, convert to MB
    return true;
  }

  return false;
}

bool get_physical_memory(int &res) { return get_proc_mem_info_by_param("MemTotal", res); }
bool get_physical_memory_free(int &res) { return get_proc_mem_info_by_param("MemAvailable", res); }
bool get_virtual_memory(int &res) { return get_proc_mem_info_by_param("MemTotal", res); }
bool get_virtual_memory_free(int &res) { return get_proc_mem_info_by_param("MemAvailable", res); }

bool get_os(String &res) { return get_os_common_name(res); }

bool get_os_common_name(String &res)
{
  char sdkVerStr[PROP_VALUE_MAX] = {0};
  if (__system_property_get("ro.build.version.release", sdkVerStr))
    res = String(0, "Android %s", sdkVerStr);
  else
    res = "Android";
  return true;
}

bool get_mac(String &, String &) { return false; }
bool get_proc_mem_info(ProcMemInfo & /*mem_info*/) { return false; }
bool get_cpu_times(CpuTimes & /*cpu_times*/) { return false; }

bool get_cpu_info(String &cpu, String & /*cpuFreq*/, String &cpuVendor, String &cpu_series, int &cores_count)
{
  char manufacturer[PROP_VALUE_MAX] = {0};
  __system_property_get("ro.product.manufacturer", manufacturer);
  char model[PROP_VALUE_MAX] = {0};
  __system_property_get("ro.product.model", model);
  char chipname[PROP_VALUE_MAX] = {0};
  __system_property_get("ro.hardware.chipname", chipname);
  cpu = String(0, "%s %s %s", manufacturer, model, chipname);

  char hardware[PROP_VALUE_MAX] = {0};
  __system_property_get("ro.hardware", hardware);
  cpuVendor = hardware;

  cpu_series = model;

  cores_count = sysconf(_SC_NPROCESSORS_ONLN);

  return true;
}

bool get_video_info(String &desktopResolution, String &videoCard, String &videoVendor)
{
  // todo get desktop resolution
  desktopResolution = "";
  videoCard = d3d::get_device_name();
  videoVendor = d3d_get_vendor_name(d3d_get_gpu_cfg().primaryVendor);

  return true;
}

bool get_system_location_2char_code(String &location)
{
  char country[PROP_VALUE_MAX] = {0};
  __system_property_get("ro.csc.countryiso_code", country);
  location = country;
  return true;
}

ThermalStatus get_thermal_state()
{
  switch (android::platform::getThermalStatus())
  {
    case 0: return ThermalStatus::Normal;
    case 1: return ThermalStatus::Light;
    case 2: return ThermalStatus::Moderate;
    case 3: return ThermalStatus::Severe;
    case 4: return ThermalStatus::Critical;
    case 5: return ThermalStatus::Emergency;
    case 6: return ThermalStatus::Shutdown;
    case -1: return ThermalStatus::NotSupported;
    default: return ThermalStatus::Normal;
  }
}

JAVA_METHOD(getBattery, float, JNI_SIGNATURE(JNI_FLOAT, ));
JAVA_METHOD(getIsBatteryCharging, bool, JNI_SIGNATURE(JNI_BOOL, ));
JAVA_METHOD(mAhBatteryCapacity, long, JNI_SIGNATURE(JNI_LONG, ));

JAVA_APP_CONTEXT_CLASS(AppActivity, JAVA_CLASS_METHOD(getBattery), JAVA_CLASS_METHOD(getIsBatteryCharging),
  JAVA_CLASS_METHOD(mAhBatteryCapacity));

JAVA_THREAD_LOCAL_ENV(AppActivity);

int get_battery_capacity_mah() { return JAVA_CALL_RETURN(mAhBatteryCapacity, JAVA_APP_CONTEXT, -1L); }

float get_battery() { return JAVA_CALL_RETURN(getBattery, JAVA_APP_CONTEXT, -1.f); }

int get_is_charging() // note int
{
  if (JAVA_INIT_ENV)
    return JAVA_CALL_RETURN(getIsBatteryCharging, JAVA_APP_CONTEXT, false) ? 1 : 0;
  return -1;
};

int get_gpu_freq()
{
  static const char *clockFiles[] = // 'opendir' doesn't work without some additional permissions, so simply list the files.
    {"/sys/devices/platform/11500000.mali/clock", "/sys/devices/platform/13000000.mali/clock",
      "/sys/devices/platform/18500000.mali/clock", "/sys/class/kgsl/kgsl-3d0/clock_mhz"};

  static bool initialized = false;
  static file_ptr_t file = NULL;
  if (!initialized)
  {
    initialized = true;
    for (const char *clockFileName : clockFiles)
    {
      file = df_open(clockFileName, DF_READ | DF_REALFILE_ONLY | DF_IGNORE_MISSING);
      if (file)
      {
        debug("get_gpu_freq found '%s'", clockFileName);
        break;
      }
    }
  }

  if (file)
  {
    df_seek_to(file, 0);
    char buf[20];
    int len = df_read(file, buf, sizeof(buf) - 1);

    if (len > 0)
    {
      buf[len] = 0;
      int freq = atoi(buf);
      if (freq > 10000)
        freq /= 1000;
      return freq;
    }
  }

  return 0;
}

#else // !_TARGET_PC_MACOSX && !_TARGET_PC_WIN && !_TARGET_PC_LINUX && !_TARGET_C1 && !_TARGET_C2

bool get_physical_memory(int &res) { return false; }
bool get_physical_memory_free(int &res) { return false; }
bool get_virtual_memory(int &res) { return false; }
bool get_virtual_memory_free(int &res) { return false; }
bool get_os(String &res) { return false; }
bool get_os_common_name(String &) { return false; }
bool get_mac(String &, String &) { return false; }
bool get_proc_mem_info(ProcMemInfo & /*mem_info*/) { return false; }
bool get_cpu_times(CpuTimes & /*cpu_times*/) { return false; }
bool get_cpu_info(String &, String &, String &, String &, int &) { return false; }
bool get_video_info(String &, String &, String &) { return false; }
bool get_system_location_2char_code(String &location) { return false; }
ThermalStatus get_thermal_state() { return ThermalStatus::Normal; }
int get_battery_capacity_mah() { return -1; }

float get_battery() { return -1.f; }
int get_is_charging() { return -1; };
int get_gpu_freq() { return 0; }

#endif


#if _TARGET_64BIT
bool is_64bit_os() { return true; }
#elif _TARGET_PC_WIN
bool is_64bit_os()
{
  SYSTEM_INFO si;
  GetNativeSystemInfo(&si);
  return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64;
}
#else
bool is_64bit_os() { return false; }
#endif

void init() { private_init(); }


void dump_sysinfo()
{
  String os;
  get_os_common_name(os);

  String cpu, cpuFreq, cpuVendor, cpuSeries;
  int cpuCores = 0;
  get_cpu_info(cpu, cpuFreq, cpuVendor, cpuSeries, cpuCores);

  String cpuArch, cpuUarch;
  Tab<String> cpuFeatures;
  get_cpu_features(cpuArch, cpuUarch, cpuFeatures);

  int freeVirtualMemMb = 0;
  get_virtual_memory_free(freeVirtualMemMb);

  const GpuUserConfig &gpuCfg = d3d_get_gpu_cfg();
  DagorDateTime gpuDriverDate = d3d::get_gpu_driver_date(gpuCfg.primaryVendor);

  debug("SYSINFO: \"%s\",\"%s\",\"%s\",\"%d/%02d/%02d %d.%d.%d.%d\",\"%s\",\"%s\",%d,%lld", get_platform_string_id(), os.str(),
    d3d::get_device_name(), gpuDriverDate.year, gpuDriverDate.month, gpuDriverDate.day, gpuCfg.driverVersion[0],
    gpuCfg.driverVersion[1], gpuCfg.driverVersion[2], gpuCfg.driverVersion[3], d3d::get_driver_name(), cpu.str(), cpuCores,
    freeVirtualMemMb);
  debug("SYSINFO_PLATFORM: %s", get_platform_string_id());
  debug("SYSINFO_OS: %s", os.str());
  debug("SYSINFO_GPU_NAME: \"%s\"", d3d::get_device_name());
  debug("SYSINFO_GPU_VENDOR: %s", d3d_get_vendor_name(gpuCfg.primaryVendor));
  debug("SYSINFO_GPU_DRIVER: %d/%02d/%02d %d.%d.%d.%d", gpuDriverDate.year, gpuDriverDate.month, gpuDriverDate.day,
    gpuCfg.driverVersion[0], gpuCfg.driverVersion[1], gpuCfg.driverVersion[2], gpuCfg.driverVersion[3]);
  debug("SYSINFO_GPU_API: \"%s\"", d3d::get_driver_name());
  debug("SYSINFO_CPU_INFO: \"%s\", freq=%s", cpu.str(), cpuFreq, cpuCores);
  debug("SYSINFO_CPU_VENDOR: \"%s\"", cpuVendor);
  if (!cpuArch.empty())
  {
    debug("SYSINFO_CPU_ARCHITECTURE: \"%s\"", cpuArch);
  }
  if (!cpuUarch.empty())
  {
    debug("SYSINFO_CPU_MICROARCHITECTURE: \"%s\"", cpuUarch);
  }
  if (!cpuFeatures.empty())
  {
    String cpuFeaturesList = cpuFeatures[0];
    for (int i = 1; i < cpuFeatures.size(); i++)
    {
      cpuFeaturesList += " ";
      cpuFeaturesList += cpuFeatures[i];
    }
    debug("SYSINFO_CPU_FEATURES: \"%s\"", cpuFeaturesList);
  }
  debug("SYSINFO_CPU_CORES: %d", cpuCores);
  debug("SYSINFO_MEM_FREE: %lldMb", freeVirtualMemMb);
}

void dump_dll_names()
{
#if _TARGET_PC_WIN
  DWORD numBytesNeeded;
  HMODULE dllHandles[1024];
  HANDLE currentProcessHandle = GetCurrentProcess();
  if (!EnumProcessModules(currentProcessHandle, dllHandles, sizeof(dllHandles), &numBytesNeeded))
    return;
  for (unsigned int i = 0; i < (numBytesNeeded / sizeof(HMODULE)); i++)
  {
    TCHAR dllName[MAX_PATH];
    if (!GetModuleFileName(dllHandles[i], dllName, sizeof(dllName) / sizeof(TCHAR)))
      continue;
    MODULEINFO info;
    if (!GetModuleInformation(currentProcessHandle, dllHandles[i], &info, sizeof(info)))
      continue;

    // retrieve version
    DWORD versionHandle = 0;
    int major(0), minor(0), revision(0), build(0);
    DWORD size = 0;

    // Nvidia distributed broken detoured.dll with 372.54 drivers.
    // Version info in the dll causes buffer owerrun exception in GetFileVersionInfoSize on
    // Windows 7. The dll is used on Optimus enabled notebooks.
    if (!strstr(dllName, "detoured.dll"))
      size = GetFileVersionInfoSize(dllName, &versionHandle);

    if (size > 0)
    {
      BYTE *versionInfo = new BYTE[size];
      VS_FIXEDFILEINFO *vsfi = NULL;
      UINT len = 0;
      if (GetFileVersionInfo(dllName, versionHandle, size, versionInfo) && VerQueryValueW(versionInfo, L"\\", (void **)&vsfi, &len) &&
          vsfi)
      {
        // only for 32 bit build
        major = HIWORD(vsfi->dwProductVersionMS);
        minor = LOWORD(vsfi->dwProductVersionMS);
        revision = HIWORD(vsfi->dwProductVersionLS);
        build = LOWORD(vsfi->dwProductVersionLS);
      }

      delete[] versionInfo;
    }

    // retrieve the last-write time
    FILETIME fileTimeWrite;
    SYSTEMTIME date;
    HANDLE dllFileHandle = CreateFile(dllName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (dllFileHandle == INVALID_HANDLE_VALUE)
      continue;
    if (!GetFileTime(dllFileHandle, NULL, NULL, &fileTimeWrite))
    {
      CloseHandle(dllFileHandle);
      continue;
    }
    CloseHandle(dllFileHandle);
    // convert to display-friendly format
    FileTimeToSystemTime(&fileTimeWrite, &date);

    // print the module's load base address, creation date, version and name
    debug(" 0x%08X %d/%02d/%02d %d.%d.%d.%d %s ", info.lpBaseOfDll, date.wYear, date.wMonth, date.wDay, major, minor, revision, build,
      dllName);
  }
#endif
}

bool get_inline_raytracing_available() { return d3d::get_driver_desc().caps.hasRayQuery; }


#if _TARGET_IOS
String get_model_by_cpu(const char *cpu)
{
  static eastl::optional<String> loadedModel = eastl::nullopt;
  if (loadedModel)
    return *loadedModel;

  const auto storeAndReturn = [&](const char *val) {
    loadedModel = String(val);
    return *loadedModel;
  };

  const auto returnDefault = [&](const char *errMsg) {
    logerr(errMsg);
    return storeAndReturn(cpu);
  };

  DataBlock modelsBlk{"ios-devices.blk"};
  if (modelsBlk.isEmpty())
  {
    return returnDefault("Unable to load iOS device list");
  }

  for (int blockNo = 0; blockNo < modelsBlk.blockCount(); blockNo++)
  {
    const DataBlock *deviceBlk = modelsBlk.getBlock(blockNo);
    for (int paramNo = 0; paramNo < deviceBlk->paramCount(); paramNo++)
    {
      const char *codename = deviceBlk->getStr(paramNo);
      if (!strcmp(cpu, codename))
      {
        const char *modelName = deviceBlk->getBlockName();
        return storeAndReturn(modelName);
      }
    }
  }
  return returnDefault("Unknown iOS device");
}

int get_network_connection_type()
{
  // -1 -- Unknown
  // 0 -- Not connected
  // 1 -- Cellular
  // 2 -- Wi-Fi
  return ios_get_network_connection_type();
}

#else
String get_model_by_cpu(const char *cpu) { return String(cpu); }
int get_network_connection_type() { return -1; }

#endif

const char *to_string(ThermalStatus status)
{
  switch (status)
  {
    case ThermalStatus::NotSupported: return "NotSupported";
    case ThermalStatus::Normal: return "Normal";
    case ThermalStatus::Light: return "Light";
    case ThermalStatus::Moderate: return "Moderate";
    case ThermalStatus::Severe: return "Severe";
    case ThermalStatus::Critical: return "Critical";
    case ThermalStatus::Emergency: return "Emergency";
    case ThermalStatus::Shutdown: return "Shutdown";
    default: return "Unknown";
  }
}
} // namespace systeminfo
