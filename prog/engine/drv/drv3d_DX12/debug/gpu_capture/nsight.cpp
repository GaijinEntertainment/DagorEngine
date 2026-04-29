// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nsight.h"


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
void NSight::beginCapture(const wchar_t *) {}
void NSight::endCapture() {}
void NSight::onPresent() {}
void NSight::captureFrames(const wchar_t *, int) {}
} // namespace drv3d_dx12::debug::gpu_capture::nvidia
