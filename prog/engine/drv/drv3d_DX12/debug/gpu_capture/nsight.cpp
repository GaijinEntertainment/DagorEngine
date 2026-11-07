// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nsight.h"

#if HAS_NGFX
#include <string.h>
#include <NGFX_GraphicsCapture_D3D12.h>
#include <EASTL/algorithm.h>
#include <util/dag_globDef.h>
#endif


namespace drv3d_dx12::debug::gpu_capture::nvidia
{
bool NSight::try_connect_interface()
{
  logdbg("DX12: ...looking for Nomad.Injection.dll...");
  if (GetModuleHandleW(L"Nomad.Injection.dll"))
  {
    logdbg("DX12: ...found...");
    return true;
  }
  logdbg("DX12: ...nothing, looking for Nvda.Graphics.Interception.dll...");
  if (GetModuleHandleW(L"Nvda.Graphics.Interception.dll"))
  {
    logdbg("DX12: ...found...");
    return true;
  }
  logdbg("DX12: ...nothing, looking for NvLog.dll...");
  if (GetModuleHandleW(L"NvLog.dll"))
  {
    logdbg("DX12: ...found...");
    return true;
  }
  logdbg("DX12: ...nothing, looking for WarpViz.Injection.dll...");
  if (GetModuleHandleW(L"WarpViz.Injection.dll"))
  {
    logdbg("DX12: ...found...");
    return true;
  }
  logdbg("DX12: ...nothing, looking for ngfx-capture-injection.dll...");
  if (GetModuleHandleW(L"ngfx-capture-injection.dll"))
  {
    logdbg("DX12: ...found...");
    return true;
  }
  logdbg("DX12: ...nothing, looking for ToolsInjection64.dll...");
  if (GetModuleHandleW(L"ToolsInjection64.dll"))
  {
    logdbg("DX12: ...found...");
    return true;
  }
  return false;
}

void NSight::configure() {}
void NSight::onPresent() {}

#if HAS_NGFX
static bool ngfx_succeeded(NGFX_Result result, const char *what)
{
  if (NGFX_Result_Success == result)
  {
    return true;
  }
  logwarn("DX12: Nsight %s failed with NGFX_Result %d", what, static_cast<int>(result));
  return false;
}

bool NSight::inject(const Configuration &config)
{
  if (!config.loadNsightCapture)
  {
    return false;
  }

  NGFX_SetLibraryLoadFn(NGFX_LoadLib_NoVerification);

  NGFX_InstallationInfo installations[16] = {};
  uint32_t installationCount = 0;
  NGFX_EnumerateInstallations(installations, countof(installations), &installationCount);
  if (0 == installationCount)
  {
    logwarn("DX12: Nsight loading requested, but no Nsight Graphics installation is found");
    return false;
  }

  const NGFX_InstallationInfo &installation = installations[0];
  logdbg("DX12: Loading Nsight Graphics %u.%u.%u from <%s>...", installation.versionMajor, installation.versionMinor,
    installation.versionPatch, installation.installationPath);

  char outputDir[MAX_PATH];
  GetCurrentDirectoryA(sizeof(outputDir), outputDir);
  strcat_s(outputDir, "\\GpuCaptures");
  CreateDirectoryA(outputDir, nullptr);

  NGFX_GraphicsCapture_InjectionSettings settings;
  NGFX_GraphicsCapture_InjectionSettings_SetDefaults(&settings);
  settings.outputDir = outputDir;

  NGFX_GraphicsCapture_Inject_D3D12_Params params{
    NGFX_GraphicsCapture_Inject_D3D12_Params_VER, installation.installationPath, &settings};
  bool injected = ngfx_succeeded(NGFX_GraphicsCapture_Inject_D3D12(&params), "graphics capture injection");

  NGFX_FreeInstallations(installations, installationCount);
  if (injected)
  {
    logdbg("DX12: ...Nsight Graphics is loaded");
  }
  return injected;
}

NSight::Activity NSight::try_initialize_activity()
{
  NGFX_GraphicsCapture_InitializeActivity_D3D12_Params captureParams{NGFX_GraphicsCapture_InitializeActivity_D3D12_Params_VER};
  if (NGFX_Result_Success == NGFX_GraphicsCapture_InitializeActivity_D3D12(&captureParams))
  {
    logdbg("DX12: ...found, using Nsight graphics capture interface for frame capturing");
    return Activity::GRAPHICS_CAPTURE;
  }
  return Activity::NONE;
}

void NSight::beginCapture(const wchar_t *)
{
  if (Activity::GRAPHICS_CAPTURE == activity)
  {
    NGFX_GraphicsCapture_StartCapture_D3D12_Params params{NGFX_GraphicsCapture_StartCapture_D3D12_Params_VER};
    ngfx_succeeded(NGFX_GraphicsCapture_StartCapture_D3D12(&params), "StartCapture");
  }
}

void NSight::endCapture()
{
  if (Activity::GRAPHICS_CAPTURE == activity)
  {
    NGFX_GraphicsCapture_StopCapture_D3D12_Params params{NGFX_GraphicsCapture_StopCapture_D3D12_Params_VER};
    ngfx_succeeded(NGFX_GraphicsCapture_StopCapture_D3D12(&params), "StopCapture");
  }
}

void NSight::captureFrames(const wchar_t *, int count)
{
  if (Activity::GRAPHICS_CAPTURE == activity)
  {
    NGFX_GraphicsCapture_RequestCapture_D3D12_Params params{NGFX_GraphicsCapture_RequestCapture_D3D12_Params_VER,
      NGFX_GraphicsCapture_Delimiter_Present, 0, static_cast<uint32_t>(eastl::clamp(count, 1, 60))};
    ngfx_succeeded(NGFX_GraphicsCapture_RequestCapture_D3D12(&params), "RequestCapture");
  }
}
#else
bool NSight::inject(const Configuration &config)
{
  if (config.loadNsightCapture)
  {
    logwarn("DX12: Nsight loading requested, but the driver is built without the Nsight Graphics SDK");
  }
  return false;
}

NSight::Activity NSight::try_initialize_activity() { return Activity::NONE; }

void NSight::beginCapture(const wchar_t *) {}
void NSight::endCapture() {}
void NSight::captureFrames(const wchar_t *, int) {}
#endif
} // namespace drv3d_dx12::debug::gpu_capture::nvidia
