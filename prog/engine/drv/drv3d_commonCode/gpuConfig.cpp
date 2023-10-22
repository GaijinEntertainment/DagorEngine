#include "gpuConfig.h"
#include "gpuVendor.h"


#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/dag_drv3d.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_localConv.h>
#if _TARGET_PC_WIN
#include <d3d11.h>
#endif

using namespace gpu;

#if _TARGET_PC_MACOSX
extern bool mac_get_model(String &out_str);
extern bool mac_is_web_gpu_driver();
#endif

static GpuDriverConfig gpu_driver_config;
static bool gpu_driver_engine_inited = false;

GpuDriverConfig::GpuDriverConfig() { memset(this, 0, sizeof(*this)); }

void (*update_gpu_driver_config)(GpuDriverConfig &) = [](GpuDriverConfig &) {};

#if _TARGET_PC_WIN
#if HAS_NVAPI
static bool verify_nvidia_settings(int active_vendor, const GpuVideoSettings &video, GpuDriverConfig &out_cfg)
{
  if (!init_nvapi())
    return false;

  if (active_vendor != D3D_VENDOR_NVIDIA)
    return true;

  NvLogicalGpuHandle nvLogicalGPUHandle[NVAPI_MAX_LOGICAL_GPUS] = {0};
  NvPhysicalGpuHandle nvPhysicalGPUHandles[NVAPI_MAX_PHYSICAL_GPUS] = {0};
  NvU32 logicalGPUCount = 0, physicalGPUCount = 0;
  ;
  NvAPI_EnumLogicalGPUs(nvLogicalGPUHandle, &logicalGPUCount);
  NvAPI_EnumPhysicalGPUs(nvPhysicalGPUHandles, &physicalGPUCount);

  NvU32 physicalFrameBufferSize = 0;
  if (get_nv_physical_gpu())
    NvAPI_GPU_GetPhysicalFrameBufferSize(get_nv_physical_gpu(), &physicalFrameBufferSize);
  out_cfg.physicalFrameBufferSize = (unsigned int)(physicalFrameBufferSize / 1024);

  if (video.disableNvTweaks)
    return true;

  NvDRSSessionHandle hSession = 0;
  NvAPI_Status status = NvAPI_DRS_CreateSession(&hSession);
  if (status != NVAPI_OK)
    return true;

  status = NvAPI_DRS_LoadSettings(hSession);
  if (status != NVAPI_OK)
  {
    NvAPI_DRS_DestroySession(hSession);
    return true;
  }

  NvDRSProfileHandle hProfile[3] = {NULL};

  status = NvAPI_DRS_GetBaseProfile(hSession, &hProfile[0]);
  if (status != NVAPI_OK)
    debug("NvAPI_DRS_GetBaseProfile failed (%d)", status);

  status = NvAPI_DRS_GetCurrentGlobalProfile(hSession, &hProfile[1]);
  if (status != NVAPI_OK)
    debug("NvAPI_DRS_GetCurrentGlobalProfile failed (%d)", status);

  wchar_t processPath[MAX_PATH];
  GetModuleFileNameW(NULL, processPath, sizeof(processPath) / sizeof(wchar_t));
  processPath[sizeof(processPath) / sizeof(wchar_t) - 1] = 0;
  NvAPI_UnicodeString appName;
  memcpy(appName, processPath, (wcslen(processPath) + 1) * sizeof(wchar_t));
  NVDRS_APPLICATION appl = {0};
  appl.version = NVDRS_APPLICATION_VER;
  status = NvAPI_DRS_FindApplicationByName(hSession, appName, &hProfile[2], &appl);

  bool changed = false;

  for (unsigned int profileNo = 0; profileNo < sizeof(hProfile) / sizeof(hProfile[0]); profileNo++)
  {
    if (!hProfile[profileNo])
      continue;

    NVDRS_SETTING drsSetting = {0};
    drsSetting.version = NVDRS_SETTING_VER;

    status = NvAPI_DRS_GetSetting(hSession, hProfile[profileNo], FXAA_ENABLE_ID, &drsSetting);
    if (status == NVAPI_OK && drsSetting.u32CurrentValue != FXAA_ENABLE_OFF)
    {
      debug("FXAA: %d changed to FXAA_ENABLE_OFF", drsSetting.u32CurrentValue);
      changed = true;
      drsSetting.u32CurrentValue = FXAA_ENABLE_OFF;
      status = NvAPI_DRS_SetSetting(hSession, hProfile[profileNo], &drsSetting);
      if (status != NVAPI_OK)
        debug("NvAPI_DRS_SetSetting failed");
    }

    status = NvAPI_DRS_GetSetting(hSession, hProfile[profileNo], AA_MODE_SELECTOR_ID, &drsSetting);
    if (status == NVAPI_OK && drsSetting.u32CurrentValue != AA_MODE_SELECTOR_APP_CONTROL)
    {
      debug("AA: %d changed to AA_MODE_SELECTOR_APP_CONTROL", drsSetting.u32CurrentValue);
      changed = true;
      drsSetting.u32CurrentValue = AA_MODE_SELECTOR_APP_CONTROL;
      status = NvAPI_DRS_SetSetting(hSession, hProfile[profileNo], &drsSetting);
      if (status != NVAPI_OK)
        debug("NvAPI_DRS_SetSetting failed");
    }
  }

  if (changed)
  {
    status = NvAPI_DRS_SaveSettings(hSession);
    if (status != NVAPI_OK)
      debug("NvAPI_DRS_SaveSettings failed");
  }

  NvAPI_DRS_DestroySession(hSession);
  return true;
}
#endif

#pragma warning(push)
#pragma warning(disable : 4191)

static bool verify_ati_settings(int active_vendor, const GpuVideoSettings &video, GpuDriverConfig &out_cfg)
{
  if (!init_ati())
    return false;

  if (ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 0) != ADL_OK)
  {
    close_ati();
    return false;
  }

  int mgpuCount = AtiMultiGPUAdapters();
  if (mgpuCount < 1)
  {
    ADL_Main_Control_Destroy();
    close_ati();
    return false;
  }

  int numAdapters = 0;
  ADL_Adapter_NumberOfAdapters_Get(&numAdapters);
  if (numAdapters <= 0)
  {
    ADL_Main_Control_Destroy();
    close_ati();
    return false;
  }

  if (active_vendor != D3D_VENDOR_ATI)
    return true;

  AdapterInfo *adapterInfo = new AdapterInfo[numAdapters];
  memset(adapterInfo, 0, sizeof(AdapterInfo) * numAdapters);
  ADL_Adapter_AdapterInfo_Get(adapterInfo, sizeof(AdapterInfo) * numAdapters);

  unsigned int adapterNo = 0;
  for (; adapterNo < numAdapters; adapterNo++)
  {
    int active = 0;
    if (ADL_Adapter_Active_Get(adapterInfo[adapterNo].iAdapterIndex, &active) == ADL_OK && active)
      break;
  }

  if (adapterNo == numAdapters)
  {
    delete[] adapterInfo;
    ADL_Main_Control_Destroy();
    close_ati();
    return true;
  }

  HKEY hUmdKey;
  String keyPathUmd(1000, "SYSTEM\\CurrentControlSet\\Control\\Class\\%s\\UMD", adapterInfo[adapterNo].strDriverPathExt);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPathUmd, 0, KEY_READ, &hUmdKey) != ERROR_SUCCESS)
  {
    delete[] adapterInfo;
    ADL_Main_Control_Destroy();
    close_ati();
    return true;
  }

  ADLMemoryInfo memoryInfo;
  memset(&memoryInfo, 0, sizeof(memoryInfo));
  ADL_Adapter_MemoryInfo_Get(adapterInfo[adapterNo].iAdapterIndex, &memoryInfo);

  ADLMVPUStatus mvpuStatus;
  memset(&mvpuStatus, 0, sizeof(mvpuStatus));
  mvpuStatus.iSize = sizeof(mvpuStatus);
  ADL_Display_MVPUStatus_Get(adapterInfo[adapterNo].iAdapterIndex, &mvpuStatus);
  debug("ADL_Display_MVPUStatus_Get: %d, %d", mvpuStatus.iActiveAdapterCount, mvpuStatus.iStatus);

  ADLMVPUCaps mvpuCaps;
  memset(&mvpuCaps, 0, sizeof(mvpuCaps));
  mvpuCaps.iSize = sizeof(mvpuCaps);
  ADL_Display_MVPUCaps_Get(adapterInfo[adapterNo].iAdapterIndex, &mvpuCaps);
  debug("ADL_Display_MVPUCaps_Get: %d, 0x%02X, 0x%02X", mvpuCaps.iAdapterCount, mvpuCaps.iPossibleMVPUMasters,
    mvpuCaps.iPossibleMVPUSlaves);

  out_cfg.physicalFrameBufferSize = (unsigned int)(memoryInfo.iMemorySize / 1024 / 1024);

  if (video.disableAtiTweaks)
  {
    RegCloseKey(hUmdKey);
    delete[] adapterInfo;
    ADL_Main_Control_Destroy();
    close_ati();
    return true;
  }

  DWORD AntiAlias = 0x0030;
  DWORD AntiAliasSize = sizeof(DWORD);
  RegQueryValueEx(hUmdKey, "AntiAlias", NULL, NULL, (LPBYTE)&AntiAlias, &AntiAliasSize);

  DWORD EQAA = 0x0030;
  DWORD EQAASize = sizeof(DWORD);
  RegQueryValueEx(hUmdKey, "EQAA", NULL, NULL, (LPBYTE)&EQAA, &EQAASize);

  RegCloseKey(hUmdKey);

  if ((AntiAlias >= 0x0032 || EQAA >= 0x0031))
    out_cfg.vendorAAisOn = true;

  if (mgpuCount > 1)
  {
    debug("Crossfire support disabled"); // On 2016/04/04 8.17.10.1452 RT lock returns data from GPU that is not current and has
                                         // outdated RT.
    out_cfg.forceFullscreenToWindowed = true;
  }

  delete[] adapterInfo;

  ADL_Main_Control_Destroy();
  close_ati();
  return true;
}

#pragma warning(pop)

void d3d::disable_sli()
{
#if HAS_NVAPI
  if (init_nvapi())
  {
    NvAPI_Status status = NvAPI_D3D_ImplicitSLIControl(DISABLE_IMPLICIT_SLI);
    if (status != NVAPI_OK)
      debug("NvAPI_D3D_ImplicitSLIControl failed (%d)", status);
  }
#endif
}

#else

void d3d::disable_sli() {}

#endif

static void check_intel_driver(const GpuVideoSettings &video, const char *gpu_desc, const uint32_t driver_version[],
  const DagorDateTime &driver_date, GpuDriverConfig &out_cfg)
{
  G_UNREFERENCED(driver_date);
  G_UNREFERENCED(gpu_desc);
  G_UNREFERENCED(video);

  // sbuffers (on effects specifically) broken right now on most intel drivers (https://youtrack.gaijin.ru/issue/1-89481)
  // falling back to non-sbuffer (tbuffer for example)
  out_cfg.disableSbuffers = true;
  const uint32_t version = driver_version[2] * 10000u + driver_version[3];
  if (version == 0)
    return;
  // 9.17.10.2884 (2012/11/12), 9.17.10.2817 (2012/7/20) random false E_OUTOFMEMORY.
  // 10.18.0010.3345 (2013/11/7) crash on shaders loading (igdusc32!USC::CShaderInstruction::SupportsPredicate).
  // 27.20.100.8280 (5/29/2020) crashed in the CreatePixelShader function with the deferred_shadow_to_buffer shader
  if (version <= 10u * 10000u + 3345u || version == 100u * 10000u + 8280u)
  {
    out_cfg.outdatedDriver = out_cfg.fallbackToCompatibilty = out_cfg.forceDx10 = true;
    debug("Fallback to compatibility on outdated Intel driver.");
  }

#if _TARGET_PC_WIN
  if (!out_cfg.fallbackToCompatibilty || !out_cfg.forceDx10 || !out_cfg.disableSbuffers) //-V560
  {
    HMODULE d3d11dll = LoadLibraryA("d3d11.dll");
    if (d3d11dll)
    {
      PFN_D3D11_CREATE_DEVICE createDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11dll, "D3D11CreateDevice");
      if (createDevice)
      {
        D3D_FEATURE_LEVEL featureLevelsRequested[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
        D3D_FEATURE_LEVEL supportedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
        HRESULT hr = createDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevelsRequested, countof(featureLevelsRequested),
          D3D11_SDK_VERSION, NULL, &supportedFeatureLevel, NULL);
        if (FAILED(hr) || supportedFeatureLevel <= D3D_FEATURE_LEVEL_10_1) // Intel HD 3000 (Sandy Bridge, last 10.1 level GPU) has
                                                                           // somewhat broken DX11 driver but Intel claims there will
                                                                           // be no updates.
        {                                                                  // Workaround tested only on compatibility.
          out_cfg.fallbackToCompatibilty = out_cfg.forceDx10 = true;
          debug("Fallback to compatibilty and force DX10 on D3D_FEATURE_LEVEL_10_1 Intel (hr=0x%08x, supportedFeatureLevel=0x%08x).",
            hr, supportedFeatureLevel);
        }

        if (!out_cfg.fallbackToCompatibilty || !out_cfg.forceDx10 || !out_cfg.disableSbuffers) //-V560
        {
          D3D_FEATURE_LEVEL featureLevelRequested12[] = {(D3D_FEATURE_LEVEL)0xc000}; // D3D_FEATURE_LEVEL_12_0 (feature level 11.2)
          hr = createDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevelRequested12, countof(featureLevelRequested12),
            D3D11_SDK_VERSION, NULL, &supportedFeatureLevel, NULL);
          if (FAILED(hr)) // SBuffers are broken on DX11 Intels, tested on 20.19.15.5166
          {               // All Intels on Windows 7 will be affected by this workaround, perhaps it is too strict.
            out_cfg.fallbackToCompatibilty = out_cfg.forceDx10 = out_cfg.disableSbuffers = true; //-V1048
            debug("Fallback to compatibilty and force DX10 on D3D_FEATURE_LEVEL_11 or Win7 Intel (hr=0x%08x)", hr);
          }
        }
      }
      FreeLibrary(d3d11dll);
    }
  }

  // Access violation in igd10umd64.dll on HD3000 with 2015/05/27 9.17.10.4229 driver and silent exit on 32-bit exe.
  // 4229 is the latest driver for HD3000 and older on Windows 7.
  // 9.17.10.4459 on Windows 10 is not fixed, but don't fallback basing on that number so as not to affect newer GPUs.
  if (!out_cfg.fallbackToCompatibilty && version <= 10u * 10000u + 4229u) // It is a newer Intel which should have a fixed driver.
  {
    out_cfg.outdatedDriver = out_cfg.fallbackToCompatibilty = true; // Show outdated driver message box.
    debug("Fallback to compatibility on outdated Intel driver.");
  }

  OSVERSIONINFOEXW osvi;
  get_version_ex(&osvi);

  if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1) // Windows 7
  {
    out_cfg.disableTexArrayCompression = true; // bugged even with latest (early 2017) win7 drivers
    debug("Disable texarray compression");
  }

#endif

  if (video.configCompatibilityMode)                    // Make compatibility more safer, avoid hungs on DX11 driver.
    out_cfg.forceDx10 = out_cfg.disableSbuffers = true; //-V1048

  if (out_cfg.forceDx10 && !video.allowDx10Fallback)
  {
    out_cfg.forceDx10 = false;
    debug("DX10 is not supported by the game, disable the DX10 fallback");
  }
}

static void check_ati_driver(const GpuVideoSettings &video, const char *gpu_desc, const uint32_t driver_version[],
  const DagorDateTime &driver_date, GpuDriverConfig &out_cfg)
{
  G_UNREFERENCED(video);
  G_UNREFERENCED(driver_date);

  // False negative survey results. Tested on R9 380 with 2016/10/25 8.17.10.1484 <aticfx32.dll>. Not reproduced with
  // 2019/08/26 8.17.10.1669 driver.
  if (driver_version[0] == 8 && driver_version[3] < 1669)
  {
    debug("flushBeforeSurvey enabled on outdated ATI driver");
    out_cfg.flushBeforeSurvey = true;
  }

  // 2009/07/14 8.15.10.163 crash in atidxx32.dll (In 2012 drivers numbering started from 1000).
  if (driver_version[0] == 8 && driver_version[3] < 1000)
  {
    out_cfg.outdatedDriver = out_cfg.fallbackToCompatibilty = true;
    debug("Fallback to compatibility on outdated ATI driver.");
  }

  // on radeon 6000 series on latest driver versions many problems with mesh streaming issued
  // needs actual fix, it looks like mesh streaming is pretty bugged
  if ((driver_version[0] == 8) && (driver_version[3] >= 1698))
  {
    out_cfg.disableMeshStreaming = true;
    debug("Disable mesh streaming in bugged driver version.");
  }

  out_cfg.gradientWorkaroud = strstr(gpu_desc, "radeon hd 3");
  if (out_cfg.gradientWorkaroud)
    debug("'Radeon HD 3' detected, gradientWorkaroud enabled");
}

static void check_nvidia_driver(const GpuVideoSettings &video, const char *gpu_desc, const uint32_t driver_version[],
  const DagorDateTime &driver_date, GpuDriverConfig &out_cfg)
{
  G_UNREFERENCED(driver_date);
  G_UNREFERENCED(gpu_desc);
  int nvidiaDriverVersion = driver_version[2] % 10 * 10000 + driver_version[3];

#if _TARGET_PC_WIN
  OSVERSIONINFOEXW osvi;
  ZeroMemory(&osvi, sizeof(osvi));
  osvi.dwOSVersionInfoSize = sizeof(osvi);
  get_version_ex(&osvi);
  // 341.81 on Windows 10 - CreateTexture2D E_OUTOFMEMORY, DEVICE_REMOVED-DXGI_ERROR_DEVICE_RESET.
  if (osvi.dwMajorVersion >= 10 && nvidiaDriverVersion > 0 && nvidiaDriverVersion < 34192)
  {
    out_cfg.outdatedDriver = true;
    debug("Old Nvidia GPU with outdated driver on Windows 10.");
  }
#endif

  // 347.88 and older - UAV causes VB Map to return random invalid pointers.
  if (nvidiaDriverVersion > 0 && nvidiaDriverVersion <= 34788)
  {
    out_cfg.disableUav = true;
    out_cfg.fallbackToCompatibilty = true; // UAV is required for normal DX11 shaders.
    debug("Disable UAV on outdated Nvidia driver.");
  }

#if _TARGET_PC_MACOSX
  String macModel;
  bool macWebDriver = mac_is_web_gpu_driver();
  mac_get_model(macModel);
  debug("Mac nvidia check: model=%s web driver=%d code=%c%c%c%c", macModel.str(), macWebDriver, _DUMP4C(video.drvCode.asFourCC()));
#endif
}

static void check_gpu_driver(const GpuVideoSettings &video, int active_vendor, const String &gpu_desc, const uint32_t driver_version[],
  const DagorDateTime &driver_date, GpuDriverConfig &out_cfg)
{
  if (video.ignoreOutdatedDriver)
    return;

  switch (active_vendor)
  {
    case D3D_VENDOR_INTEL: check_intel_driver(video, gpu_desc.str(), driver_version, driver_date, out_cfg); break;
    case D3D_VENDOR_ATI: check_ati_driver(video, gpu_desc.str(), driver_version, driver_date, out_cfg); break;
    case D3D_VENDOR_NVIDIA: check_nvidia_driver(video, gpu_desc.str(), driver_version, driver_date, out_cfg); break;
    case D3D_VENDOR_SHIM_DRIVER: // Actual driver is hidden behind the shim driver, assume the worst and apply basic Intel workarounds.
    {
      uint32_t noDriverVersion[4] = {0};
      DagorDateTime noDriverDate = {0};
      check_intel_driver(video, gpu_desc.str(), noDriverVersion, noDriverDate, out_cfg);
    }
    break;
  }

    // sbuffers (on effects specifically) broken right now on some metal hardware, was found on various vendors and drivers
    // (https://youtrack.gaijin.team/issue/1-99630) falling back to non-sbuffer (tbuffer for example)
#if _TARGET_PC_MACOSX
  out_cfg.disableSbuffers = true;
#endif

  // Some drivers (ATI on XP) are not updated since late 2013.
  // Do not fallback to compatibilty if there are no bugs reported on specific driver version.
  if (driver_date.year > 0 && driver_date.year < 2013)
  {
    out_cfg.outdatedDriver = true;
    debug("Outdated driver.");
  }
}

static bool should_fallback_to_compatibility()
{
  // autotests must work as close to players as possible, but compatibility mode disables some important code paths
  if (!d3d::is_inited() || d3d::is_stub_driver())
    return false;

  if (d3d::get_driver_desc().shaderModel < 5.0_sm)
  {
    debug("not supporting SM 5.0 - fallback to compatibility");
    return true;
  }

  unsigned int workingFlags = d3d::USAGE_FILTER | d3d::USAGE_BLEND | d3d::USAGE_RTARGET;
  if ((d3d::get_texformat_usage(TEXFMT_A2R10G10B10) & workingFlags) != workingFlags)
  {
    debug("not supporting 10bit render target - sets ultralow, usage=%x", d3d::get_texformat_usage(TEXFMT_A2R10G10B10));
    return true;
  }

  if (!(d3d::get_texformat_usage(TEXFMT_DEPTH24) & d3d::USAGE_DEPTH))
  {
    debug("not supporting depth24bit  render target - sets ultralow, usage=%x", d3d::get_texformat_usage(TEXFMT_DEPTH24));
    return true;
  }

  if (!(d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_VERTEXTEXTURE) ||
      !(d3d::get_texformat_usage(TEXFMT_A32B32G32R32F) & d3d::USAGE_VERTEXTEXTURE))
  {
    debug("not supporting vertex textures - sets ultralow, usage=%x (FP32x4), %x (FP32)",
      d3d::get_texformat_usage(TEXFMT_A32B32G32R32F), d3d::get_texformat_usage(TEXFMT_R32F));
    return true;
  }
  return false;
}

static void check_mem(const GpuVideoSettings &video, GpuDriverConfig &out_cfg)
{
  out_cfg.videoMemMb = d3d::get_dedicated_gpu_memory_size_kb() >> 10;
  debug("d3d::get_dedicated_gpu_memory_size_kb(): %dMB", out_cfg.videoMemMb);

  const DataBlock *debugBlk = ::dgs_get_settings()->getBlockByNameEx("debug");
  if (!debugBlk->getBool("adjustVideoSettings", true) || d3d::is_stub_driver())
    return;

  bool lowMem = false;
  bool ultraLowMem = false;

#if _TARGET_PC_WIN // Do not rely on memory detection on less popular platforms, it may be inaccurate.
  if (out_cfg.videoMemMb > 0)
  {
    if (out_cfg.videoMemMb <= video.lowVideoMemMb)
      lowMem = true;
    if (out_cfg.videoMemMb <= video.ultraLowVideoMemMb)
      ultraLowMem = true;
  }
#endif

#if _TARGET_PC_WIN
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  if (GlobalMemoryStatusEx(&statex))
  {
    // Reprot free memory to scare the user, but make decisions based on total memory for consistency from run to run.
    out_cfg.freePhysMemMb = statex.ullAvailPhys >> 20;
    out_cfg.freeVirtualMemMb = statex.ullAvailVirtual >> 20;
    out_cfg.totalVirtualMemMb = statex.ullTotalVirtual >> 20;

    if (statex.ullTotalVirtual >> 20 <= video.lowSystemMemAtMb)
      lowMem = true;
    if (statex.ullTotalVirtual >> 20 <= video.ultralowSystemMemAtMb)
      ultraLowMem = true;

    if (statex.ullTotalPhys >> 20 <= video.lowSystemMemAtMb)
      lowMem = true;
    if (statex.ullTotalPhys >> 20 <= video.ultralowSystemMemAtMb)
      ultraLowMem = true;
  }
#endif
  lowMem |= ultraLowMem;

  out_cfg.lowMem = lowMem;
  out_cfg.ultraLowMem = ultraLowMem;
}

static void update_gpu_settings()
{
  if (should_fallback_to_compatibility())
  {
    debug("should_fallback_to_compatibility");
    gpu_driver_config.fallbackToCompatibilty = true;
  }

  if (gpu_driver_config.primaryVendor == D3D_VENDOR_NONE)
  {
    update_gpu_driver_config(gpu_driver_config);
  }
}

void d3d_apply_gpu_settings(const GpuVideoSettings &video)
{
  if (video.drvCode.is(d3d::windows && (d3d::dx12 || d3d::vulkan)))
    return;

  String gpuDescription;
  DagorDateTime gpuDriverDate = {0};
  int activeVendor =
    d3d::guess_gpu_vendor(&gpuDescription, gpu_driver_config.driverVersion, &gpuDriverDate, &gpu_driver_config.deviceId);
  gpuDescription.toLower();

  // Verify vendor user settings
  gpu_driver_config.primaryVendor = D3D_VENDOR_NONE;
#if _TARGET_PC_WIN
#if HAS_NVAPI
  if (verify_nvidia_settings(activeVendor, video, gpu_driver_config))
    gpu_driver_config.primaryVendor = D3D_VENDOR_NVIDIA;
#endif
  if (gpu_driver_config.primaryVendor == D3D_VENDOR_NONE && verify_ati_settings(activeVendor, video, gpu_driver_config))
    gpu_driver_config.primaryVendor = D3D_VENDOR_ATI;
#endif
  if (gpu_driver_config.primaryVendor == D3D_VENDOR_NONE)
    gpu_driver_config.primaryVendor = activeVendor;

#if _TARGET_PC
  check_mem(video, gpu_driver_config);
#endif

  // Test outdated driver
  check_gpu_driver(video, activeVendor, gpuDescription, gpu_driver_config.driverVersion, gpuDriverDate, gpu_driver_config);

  // Test vendors on bugs
  if (activeVendor == D3D_VENDOR_INTEL && gpu_driver_config.primaryVendor != activeVendor)
  {
    debug("Integrated GPU selected in switchable configuration (activeGpuVendor=%s, haveNvidia=%d)", d3d_get_vendor_name(activeVendor),
      (int)(gpu_driver_config.primaryVendor == D3D_VENDOR_NVIDIA));
    gpu_driver_config.usedSlowIntegrated = true;
    gpu_driver_config.usedSlowIntegratedSwitchableGpu = true;
  }
  else if (gpu_driver_config.primaryVendor == D3D_VENDOR_INTEL)
    // In dx11 the gpu type (integrated/dedicated) cannot be detected. Newer dedicated intel gpus
    // all use dx12, so here if the vendor is intel, we can assume that the gpu is an integrated one.
    gpu_driver_config.usedSlowIntegrated = true;

  if (gpu_driver_config.integrated && activeVendor == D3D_VENDOR_INTEL && video.drvCode.is(d3d::dx11))
  {
    // On some intels we have problems with z testing if the texture with a depth format was written manually or updated
    // from some other texture
    gpu_driver_config.disableDepthCopyResource = true;
  }

  const DataBlock *debugBlk = ::dgs_get_settings()->getBlockByNameEx("debug");
  if (debugBlk->getBool("isOldHardware", false))
  {
    gpu_driver_config.oldHardware = true;
    debug("forced oldHardware mode from config.blk");
  }

  for (int oldHardwareNo = 0; oldHardwareNo < video.oldHardwareList.size() && !gpu_driver_config.oldHardware; ++oldHardwareNo)
  {
    const String &oldHardware = video.oldHardwareList[oldHardwareNo];
    const String gpuDesc = gpuDescription.replace("(tm)", "").replace("(r)", "");
    int hwLen = oldHardware.length();
    if (!hwLen)
      continue;
    bool strictTerm = oldHardware[hwLen - 1] != '$';
    if (!strictTerm)
    {
      const char *first = strstr(gpuDesc.str(), String(oldHardware.str(), hwLen - 1).str());
      if (!first)
        continue;
    }
    else if (!gpuDesc.suffix(oldHardware.str()))
      continue;
    gpu_driver_config.oldHardware = true;
    debug("oldHardware found for %s from %s", oldHardware.str(), gpuDescription.str());
  }

  // Count systems with only DX10 support for statistics.
#if _TARGET_PC_WIN
  if (gpu_driver_config.usedSlowIntegrated)
    gpu_driver_config.hardwareDx10 = false; // Switchable GPU support has been added to DX11 GPUs, so even if an integrated DX10 GPU is
                                            // selected, the system has a DX11 GPU.
  else
  {
    gpu_driver_config.hardwareDx10 = true;
    HMODULE d3d11dll = LoadLibraryA("d3d11.dll");
    if (d3d11dll)
    {
      PFN_D3D11_CREATE_DEVICE createDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11dll, "D3D11CreateDevice");
      if (createDevice)
      {
        D3D_FEATURE_LEVEL featureLevelsRequested[] = {D3D_FEATURE_LEVEL_11_0};
        D3D_FEATURE_LEVEL supportedFeatureLevel;
        HRESULT hr = createDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevelsRequested, countof(featureLevelsRequested),
          D3D11_SDK_VERSION, NULL, &supportedFeatureLevel, NULL);
        if (SUCCEEDED(hr) && supportedFeatureLevel >= D3D_FEATURE_LEVEL_11_0)
          gpu_driver_config.hardwareDx10 = false;
      }
      FreeLibrary(d3d11dll);
    }
  }
  debug("hardwareDx10=%d", (int)gpu_driver_config.hardwareDx10);
#endif
}

void d3d_read_gpu_video_settings(const DataBlock &blk, GpuVideoSettings &out_video)
{
  const DataBlock *graphicsBlk = blk.getBlockByNameEx("graphics");
  const DataBlock *videoBlk = blk.getBlockByNameEx("video");
  const DataBlock *debugBlk = blk.getBlockByNameEx("debug");

  out_video.drvCode = d3d::get_driver_code();
  out_video.disableNvTweaks = debugBlk->getBool("disableNvTweaks", false);
  out_video.disableAtiTweaks = debugBlk->getBool("disableAtiTweaks", false);
  out_video.ignoreOutdatedDriver = debugBlk->getBool("ignoreOutdatedDriver", false);
  out_video.configCompatibilityMode = videoBlk->getBool("compatibilityMode", false);
  out_video.allowDx10Fallback = videoBlk->getBool("allowDx10Fallback", false);
  const DataBlock *oldHardwareBlk = blk.getBlockByNameEx("oldHardware");
  out_video.oldHardwareList.resize(oldHardwareBlk->paramCount());
  for (int oldHardwareNo = 0; oldHardwareNo < oldHardwareBlk->paramCount(); oldHardwareNo++)
  {
    out_video.oldHardwareList[oldHardwareNo] = oldHardwareBlk->getStr(oldHardwareNo);
    out_video.oldHardwareList[oldHardwareNo].toLower();
  }
  out_video.adjustVideoSettings = debugBlk->getBool("adjustVideoSettings", true);
  out_video.lowVideoMemMb = graphicsBlk->getInt("lowVideoMemMb", 0);
  out_video.ultraLowVideoMemMb = graphicsBlk->getInt("ultraLowVideoMemMb", 1024);
  out_video.lowSystemMemAtMb = graphicsBlk->getInt("lowSystemMemAtMb", 3072);
  out_video.ultralowSystemMemAtMb = graphicsBlk->getInt("ultralowSystemMemAtMb", 2048);
}

void d3d_apply_gpu_settings(const DataBlock &blk)
{
  GpuVideoSettings video;
  d3d_read_gpu_video_settings(blk, video);
  d3d_apply_gpu_settings(video);
}

const GpuUserConfig &d3d_get_gpu_cfg()
{
  if (!gpu_driver_engine_inited && d3d::is_inited())
  {
    gpu_driver_engine_inited = true;
    update_gpu_settings();
  }
  return gpu_driver_config;
}

const GpuDriverConfig &get_gpu_driver_cfg() { return gpu_driver_config; }

String GpuUserConfig::generateDriverVersionString() const
{
  return String(0, "%d.%d.%d.%d", driverVersion[0], driverVersion[1], driverVersion[2], driverVersion[3]);
}
