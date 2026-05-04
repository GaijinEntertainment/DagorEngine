// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "legacy_pix.h"
#include <platform.h>
#include <debug/global_state.h>


namespace drv3d_dx12::debug::gpu_capture
{
ComPtr<IDXGraphicsAnalysis> LegacyPIX::try_connect_interface(Direct3D12Enviroment &d3d_env)
{
  logdbg("DX12: ...GPA for 'DXGIGetDebugInterface1'...");
  ComPtr<IDXGraphicsAnalysis> result;
  PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
  reinterpret_cast<FARPROC &>(DXGIGetDebugInterface1) = d3d_env.getDXGIProcAddress("DXGIGetDebugInterface1");

  if (DXGIGetDebugInterface1)
  {
    logdbg("DX12: ...calling DXGIGetDebugInterface1 for IDXGraphicsAnalysis...");
    HRESULT hr = DXGIGetDebugInterface1(0, COM_ARGS(&result));
    logdbg("DX12: ...DXGIGetDebugInterface1 returned %p - 0x%08x...", result.Get(), hr);
  }
  else
  {
    logdbg("DX12: ...failed...");
  }
  return result;
}

void LegacyPIX::configure() {}

void LegacyPIX::beginCapture(const wchar_t *)
{
  api->BeginCapture();
  isCapturing = true;
}

void LegacyPIX::endCapture()
{
  api->EndCapture();
  isCapturing = false;
}

void LegacyPIX::onPresent()
{
  if (isCapturing && (framesToCapture == 0))
  {
    endCapture();
  }

  if (framesToCapture)
  {
    --framesToCapture;
    if (!isCapturing)
    {
      beginCapture(nullptr);
    }
  }
}

void LegacyPIX::captureFrames(const wchar_t *, int count) { framesToCapture = count; }

} // namespace drv3d_dx12::debug::gpu_capture
