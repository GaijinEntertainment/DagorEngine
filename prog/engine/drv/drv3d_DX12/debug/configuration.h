// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string_view.h>
#include <ioSys/dag_dataBlock.h>


namespace drv3d_dx12::debug
{
struct Configuration
{
  bool enableGPUCapturers : 1 = false;
  bool loadPIXCapturer : 1 = false;
  bool enableAftermath : 1 = false;
  bool enableShaderErrorReporting : 1 = false;
  bool enableGPUDumps : 1 = false;
  bool enableDRED : 1 = false;
  bool ignoreDREDAvailability : 1 = false;
  bool trackPageFaults : 1 = false;
  bool enableDagorGPUTrace : 1 = false;
  bool enableCPUValidation : 1 = false;
  bool enableGPUValidation : 1 = false;
  bool enableNVRTValidation : 1 = false;
  bool enableAgsTrace : 1 = false;
  bool enableAgsProfile : 1 = false;
  bool loadRenderDoc : 1 = false;

  bool anyValidation() const { return enableGPUValidation || enableCPUValidation; }

  void applyDefaults()
  {
#if DAGOR_DBGLEVEL < 1
    applyReleaseDefaults();
#elif DAGOR_DBGLEVEL > 1
    applyDebugDefaults();
#else
    applyDevDefaults();
#endif
  }

  void applyReleaseDefaults()
  {
    enableAftermath = true;
    enableGPUDumps = true;
  }

  void applyDevDefaults()
  {
    enableGPUCapturers = true;
    enableAftermath = true;
    enableGPUDumps = true;
    enableDRED = true;
    trackPageFaults = true;
    enableDagorGPUTrace = true;
  }

  void applyDebugDefaults()
  {
    enableGPUCapturers = true;
    enableAftermath = true;
    enableShaderErrorReporting = true;
    enableGPUDumps = true;
    enableDRED = true;
    ignoreDREDAvailability = false;
    trackPageFaults = true;
    enableDagorGPUTrace = true;
    enableCPUValidation = true;
  }

  void applyPIXProfile() { enableGPUCapturers = loadPIXCapturer = true; }

  void applyRenderDocProfile() { loadRenderDoc = true; }

  void applyGenericCaptureToolProfile() { enableGPUCapturers = loadPIXCapturer = true; }

  void applyShaderDebugProfile()
  {
    enableAftermath = true;
    enableShaderErrorReporting = true;
    enableGPUDumps = true;
    enableDRED = true;
    trackPageFaults = true;
    enableDagorGPUTrace = true;
    enableGPUValidation = true;
    enableNVRTValidation = true;
  }

  void applyTraceProfile()
  {
    enableAftermath = true;
    enableShaderErrorReporting = true;
    enableGPUDumps = true;
    enableDRED = true;
    trackPageFaults = true;
    enableDagorGPUTrace = true;
    enableAgsTrace = true;
  }

  void applyAftermathProfile()
  {
    enableAftermath = true;
    enableShaderErrorReporting = true;
    enableGPUDumps = true;
    trackPageFaults = true;
  }

  void applyDREDProfile()
  {
    enableDRED = true;
    ignoreDREDAvailability = true;
    trackPageFaults = true;
  }

  void applyDagorTraceProfile()
  {
    trackPageFaults = true;
    enableDagorGPUTrace = true;
  }

  void applyAgsProfile()
  {
    trackPageFaults = true;
    enableAgsProfile = true;
  }

  void applyDebugProfile(eastl::string_view profile)
  {
    if (profile.empty())
    {
      return;
    }
    if (0 == profile.compare("release"))
    {
      // *Defaults methods assume all are default initialized
      *this = {};
      applyReleaseDefaults();
    }
    else if (0 == profile.compare("dev"))
    {
      // *Defaults methods assume all are default initialized
      *this = {};
      applyDevDefaults();
    }
    else if (0 == profile.compare("debug"))
    {
      // *Defaults methods assume all are default initialized
      *this = {};
      applyDebugDefaults();
    }
    else if (0 == profile.compare("pix"))
    {
      applyPIXProfile();
    }
    else if (0 == profile.compare("renderdoc"))
    {
      applyRenderDocProfile();
    }
    else if (0 == profile.compare("capture"))
    {
      applyGenericCaptureToolProfile();
    }
    else if ((0 == profile.compare("hung")) || (0 == profile.compare("hang")))
    {
      applyTraceProfile();
    }
    else if (0 == profile.compare("shaders"))
    {
      applyShaderDebugProfile();
    }
    else if (0 == profile.compare("trace"))
    {
      applyTraceProfile();
    }
    else if (0 == profile.compare("ags"))
    {
      applyAgsProfile();
    }
    else if (0 == profile.compare("postmortem"))
    {
      applyTraceProfile();
    }
    else if (0 == profile.compare("aftermath"))
    {
      applyAftermathProfile();
    }
    else if (0 == profile.compare("dred"))
    {
      applyDREDProfile();
    }
    else if (0 == profile.compare("dagor"))
    {
      applyDagorTraceProfile();
    }
  }

  void applyLegacySettings(const DataBlock *settings)
  {
    int debugLevel = settings->getInt("debugLevel", 0);

    enableAftermath = settings->getBool("enableAftermath", enableAftermath);
    trackPageFaults = settings->getBool("trackPageFaults", trackPageFaults || debugLevel > 0 || enableAftermath);
    enableDRED = settings->getBool("allowDRED", enableDRED);
    enableGPUCapturers = settings->getBool("enableGPUCapturers", enableGPUCapturers);
    loadPIXCapturer = settings->getBool("loadPixCapturer", loadPIXCapturer);
    enableDagorGPUTrace = settings->getBool("enableDagorGPUTrace", enableDagorGPUTrace);
    enableShaderErrorReporting = settings->getBool("enableShaderErrorReporting", enableShaderErrorReporting);
    ignoreDREDAvailability = settings->getBool("ignoreDREDAvailability", ignoreDREDAvailability);
    enableGPUDumps = settings->getBool("gpuDumps", enableAftermath);
    enableCPUValidation = settings->getBool("cpuValidation", enableCPUValidation || debugLevel > 1);
    enableGPUValidation = settings->getBool("gpuValidation", enableGPUValidation || debugLevel > 2);

    applyDebugProfile(settings->getStr("debugProfile", ""));
  }

  void applyModernSettings(const DataBlock *settings)
  {
    // modern settings are all in debug sub-block
    auto modernSettings = settings->getBlockByName("debug");
    if (!modernSettings)
    {
      return;
    }

    int debugLevel = modernSettings->getInt("level", 0);
    enableAftermath = modernSettings->getBool("aftermath", enableAftermath);
    enableAgsTrace = modernSettings->getBool("agsTrace", enableAgsTrace);
    enableAgsProfile = modernSettings->getBool("agsProfile", enableAgsProfile);
    trackPageFaults = modernSettings->getBool("pageFaults", trackPageFaults || debugLevel > 0 || enableAftermath);
    enableDRED = modernSettings->getBool("DRED", enableDRED);
    enableGPUCapturers = modernSettings->getBool("gpuCapturers", enableGPUCapturers);
    loadPIXCapturer = modernSettings->getBool("loadPixCapturer", loadPIXCapturer);
    enableDagorGPUTrace = modernSettings->getBool("dagorGPUTrace", enableDagorGPUTrace);
    enableShaderErrorReporting = modernSettings->getBool("shaderErrorReporting", enableShaderErrorReporting);
    ignoreDREDAvailability = modernSettings->getBool("ignoreDREDAvailability", ignoreDREDAvailability);
    enableGPUDumps = modernSettings->getBool("gpuDumps", enableAftermath);
    enableCPUValidation = settings->getBool("cpuValidation", enableCPUValidation || debugLevel > 1);
    enableGPUValidation = settings->getBool("gpuValidation", enableGPUValidation || debugLevel > 2);
    enableNVRTValidation = modernSettings->getBool("nvRTValidation", enableNVRTValidation || debugLevel > 1);
    loadRenderDoc = modernSettings->getBool("loadRenderDoc", loadRenderDoc);

    applyDebugProfile(modernSettings->getStr("profile", ""));
  }

  void applySettings(const DataBlock *settings)
  {
    applyLegacySettings(settings);
    applyModernSettings(settings);
  }
};

} // namespace drv3d_dx12::debug
