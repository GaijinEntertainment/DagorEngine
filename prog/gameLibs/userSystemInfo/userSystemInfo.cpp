// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <drv/3d/dag_info.h>
#include <userSystemInfo/systemInfo.h>
#include <rendInst/rendInstGen.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/hid/dag_hiComposite.h>
#include <3d/dag_gpuConfig.h>
#include <util/dag_localization.h>

#ifdef USE_STEAMWORKS
#include <steam/steam.h>
#endif

#ifdef USE_EPIC_GAME_STORE
#include <epic/epic.h>
#endif

#if _TARGET_PC_WIN
#include <Windows.h>
#endif

namespace systeminfo
{
void (*fill_additional_info)(DataBlock &cachedSysInfo) = nullptr;
}

#ifdef DEDICATED

void get_user_system_info(DataBlock *blk, const char *gfx_preset) { blk->reset(); }

#else

static void copyRootParam(DataBlock *to, const char *name) { to->addStr(name, dgs_get_settings()->getStr(name, "<unknown>")); }

static bool is_remote_controller_in_use()
{
  const HumanInput::CompositeJoystickClassDriver *joysticks = ::global_cls_composite_drv_joy;
  int totalJoysticks = joysticks ? joysticks->getDeviceCount() : 0;
  for (int i = 0; i < totalJoysticks; ++i)
    if (joysticks->getDevice(i)->isRemoteController())
      return true;

  return false;
}

void get_user_system_info(DataBlock *blk, const char *gfx_preset)
{
  G_ASSERT(blk);

  // The system info must be collected only once.
  // If it is collected more than once, the results may differ
  static DataBlock cachedSysInfo;
  if (!cachedSysInfo.isEmpty())
  {
    blk->setFrom(&cachedSysInfo);
    return;
  }

  // Attention! order is important
  cachedSysInfo.addStr("platform", ::get_platform_string_id());

#if _TARGET_64BIT
  const bool is64bitClient = true;
#else
  const bool is64bitClient = false;
#endif
  cachedSysInfo.addBool("is64bitClient", is64bitClient);

#ifdef USE_STEAMWORKS
  if (steamapi::is_running())
    cachedSysInfo.addBool("isSteamRunning", true);
#endif

#ifdef USE_EPIC_GAME_STORE
  if (epicapi::is_running())
    cachedSysInfo.addBool("isEpicRunning", true);
#endif

  const char *const releaseChannel = dgs_get_settings()->getStr("releaseChannel", NULL);
  if (releaseChannel != NULL && releaseChannel[0] != 0)
    cachedSysInfo.addStr("releaseChannel", releaseChannel);

  const DataBlock *videoBlk = ::dgs_get_settings()->getBlockByNameEx("video");
  const DataBlock *renderBlk = ::dgs_get_settings()->getBlockByNameEx("render");
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");

  if (gfx_preset)
    cachedSysInfo.addStr("graphicsQuality", gfx_preset);
  else
    copyRootParam(&cachedSysInfo, "graphicsQuality");

  bool renderCompatibility = videoBlk->getBool("compatibilityMode", false);

  bool shouldRenderShadows = renderBlk->getBool("shadows", true);

  const bool renderShadows = !renderCompatibility && shouldRenderShadows;

  const DataBlock &blkVideo = *dgs_get_settings()->getBlockByNameEx("video");

  int w, h;
  d3d::get_screen_size(w, h);
  const String resolution(100, "%d x %d", w, h);

  cachedSysInfo.addStr("gameResolution", resolution.str());
  cachedSysInfo.addStr("windowMode", blkVideo.getStr("mode", "<unknown>"));
  cachedSysInfo.addBool("compatibilityMode", renderCompatibility);
  cachedSysInfo.addBool("renderShadows", renderShadows);
  cachedSysInfo.addBool("vsync", blkVideo.getBool("vsync", false));

  bool compatibilityMsaaEnabled = false;
  if (renderCompatibility && d3d::get_driver_desc().caps.hasReadMultisampledDepth && d3d::get_driver_code().is(d3d::dx11))
  {
    compatibilityMsaaEnabled = ::dgs_get_settings()->getBlockByNameEx("directx")->getInt("msaa", 0) > 1;
  }

  cachedSysInfo.addBool("msaaEnabled", compatibilityMsaaEnabled);
  cachedSysInfo.addStr("waterEffectsQuality", graphicsBlk->getStr("waterEffectsQuality", "unknown"));

  if (renderShadows)
  {
    const DataBlock &blkGraphics = *dgs_get_settings()->getBlockByNameEx("graphics");
    cachedSysInfo.addStr("shadowQuality", blkGraphics.getStr("shadowQuality", "unknown"));
  }

#if _TARGET_PC_WIN
  if (auto blkDirectX = dgs_get_settings()->getBlockByName("directx"); blkDirectX && blkDirectX->paramExists("msaa"))
  {
    cachedSysInfo.addBool("msaa", blkDirectX->getInt("msaa", 0) > 1);
  }
#endif

  cachedSysInfo.addStr("driver", blkVideo.getStr("driver", "auto"));
  cachedSysInfo.addStr("renderDriver", d3d::get_driver_name());
  if (d3d::get_driver_code().is(d3d::dx11)) // only valid after initialization has been called
  {
    cachedSysInfo.addStr("dx11_caps",
      d3d::get_driver_desc()
        .shaderModel //
        .map(5.0_sm, "hw_dx11")
        .map(4.1_sm, "hw_dx10_1")
        .map(4.0_sm, d3d::get_driver_desc().caps.hasResourceCopyConversion ? "hw_dx10plus" : "hw_dx10")
        .map(d3d::smAny, "hw_dx9"));
  }

  cachedSysInfo.addBool("isNvApi", d3d::get_driver_desc().caps.hasNVApi);

  cachedSysInfo.addBool("isAtiApi", d3d::get_driver_desc().caps.hasATIApi);

  const GpuUserConfig &gpuCfg = d3d_get_gpu_cfg();

  cachedSysInfo.addStr("videoCardDriverVer",
    String(32, "%u.%u.%u.%u", gpuCfg.driverVersion[0], gpuCfg.driverVersion[1], gpuCfg.driverVersion[2], gpuCfg.driverVersion[3]));

  if (gpuCfg.hardwareDx10)
    cachedSysInfo.addBool("hardwareDx10", true);

  cachedSysInfo.addBool("is64bitOS", systeminfo::is_64bit_os());

  String desktopResolution, videoCard, videoVendor;
  if (systeminfo::get_video_info(desktopResolution, videoCard, videoVendor))
  {
    cachedSysInfo.addStr("desktopResolution", desktopResolution.str());
    if (!videoCard.empty())
      cachedSysInfo.addStr("videoCard", videoCard.str());
    if (!videoVendor.empty())
      cachedSysInfo.addStr("videoVendor", videoVendor.str());
  }

  cachedSysInfo.addInt("videoMemory", d3d::get_dedicated_gpu_memory_size_kb() >> 10);

  String s;
  int val;

  if (systeminfo::get_os(s))
    cachedSysInfo.addStr("OS", s.str());

  char osRealName[64] = {0};
  if (detect_os_compatibility_mode(osRealName, sizeof(osRealName)))
  {
    cachedSysInfo.addBool("osCompatibility", true);
    if (osRealName[0] != 0)
      cachedSysInfo.addStr("osReal", osRealName);
  }

  String cpu, cpuFreq, cpuVendor;
  if (systeminfo::get_cpu_info(cpu, cpuFreq, cpuVendor, s, val))
  {
    cpu = systeminfo::get_model_by_cpu(cpu.c_str());
    cachedSysInfo.addStr("cpu", cpu);
    cachedSysInfo.addStr("cpuFreq", cpuFreq);
    cachedSysInfo.addStr("cpuVendor", cpuVendor);
    cachedSysInfo.addStr("cpuSeriesCores", s.str());
    cachedSysInfo.addInt("numCores", val);
  }

  String cpuArch, cpuUarch;
  Tab<String> cpuFeatures;
  if (systeminfo::get_cpu_features(cpuArch, cpuUarch, cpuFeatures))
  {
    if (!cpuArch.empty())
      cachedSysInfo.addStr("cpuArch", cpuArch);
    if (!cpuUarch.empty())
      cachedSysInfo.addStr("cpuUarch", cpuUarch);
    if (!cpuFeatures.empty())
    {
      String cpuFeaturesStr;
      cpuFeaturesStr.reserve(cpuFeatures.size() * 8);
      cpuFeaturesStr += ";";
      for (const String &featureName : cpuFeatures)
      {
        cpuFeaturesStr += featureName;
        cpuFeaturesStr += ";";
      }
      cachedSysInfo.addStr("cpuFeatures", cpuFeaturesStr);
    }
  }

  if (systeminfo::get_physical_memory(val))
    cachedSysInfo.addInt("physicalMemory", val);

  if (systeminfo::get_virtual_memory(val))
    cachedSysInfo.addInt("virtualMemory", val);

  String adapter, mac;
  if (systeminfo::get_mac(adapter, mac))
  {
    cachedSysInfo.addStr("MAC", mac);
    if (!adapter.empty())
      cachedSysInfo.addStr("adapter", adapter);
  }

  if (systeminfo::fill_additional_info)
    systeminfo::fill_additional_info(cachedSysInfo);

  const char *const consoleRevision = get_console_model_revision(get_console_model());
  if (consoleRevision != nullptr && consoleRevision[0] != 0)
    cachedSysInfo.addStr("consoleRevision", consoleRevision);

  if (is_remote_controller_in_use())
    cachedSysInfo.addBool("isUsingRemoteController", true);

  String location;
  if (systeminfo::get_system_location_2char_code(location))
    cachedSysInfo.addStr("location", location);

  cachedSysInfo.addStr("gameLanguage", get_current_language());
  cachedSysInfo.addStr("systemLanguage", get_default_lang());

  long timezone;
  if (get_timezone_minutes(timezone))
    cachedSysInfo.addInt("timezoneOffset", timezone);

  cachedSysInfo.addReal("pixelRatio", d3d::get_display_scale());

#if _TARGET_PC_WIN
  int colorDepth = 0;
  HDC hdc = GetDC(0);
  if (hdc)
  {
    colorDepth = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);
  }

  cachedSysInfo.addInt("colorDepth", colorDepth);
#endif

  cachedSysInfo.addBool("inlineRaytracingAvailable", systeminfo::get_inline_raytracing_available());

  cachedSysInfo.addInt("isTablet", systeminfo::is_tablet());

  blk->setFrom(&cachedSysInfo);
}

#endif
