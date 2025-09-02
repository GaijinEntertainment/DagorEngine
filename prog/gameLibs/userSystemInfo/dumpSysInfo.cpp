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
  int physicalMemory = 0;
  if (systeminfo::get_physical_memory(physicalMemory))
    debug("SYSINFO_PHYSICAL_MEM: %lldMb", physicalMemory);
  String soc;
  if (systeminfo::get_soc_info(soc))
    debug("SYSINFO_SOC: \"%s\"", soc);
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
