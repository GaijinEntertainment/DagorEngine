#include "device.h"

// PROFILE_BUILD will enable USE_PIX in pix3.h if architecture is supported
#define PROFILE_BUILD
#if !defined(__d3d12_h__)
#define __d3d12_h__
#include "WinPixEventRuntime/pix3.h"
#undef __d3d12_h__
#else
#include "WinPixEventRuntime/pix3.h"
#endif
#include <RenderDoc/renderdoc_app.h>

using namespace drv3d_dx12;

#if !defined(PIX_EVENT_UNICODE_VERSION)
#define PIX_EVENT_UNICODE_VERSION 0
#endif

#if !defined(PIX_EVENT_ANSI_VERSION)
#define PIX_EVENT_ANSI_VERSION 1
#endif

#if !defined(PIX_EVENT_PIX3BLOB_VERSION)
#define PIX_EVENT_PIX3BLOB_VERSION 2
#endif

RENDERDOC_API_1_5_0 *debug::gpu_capture::RenderDoc::try_connect_interface()
{
  auto module = GetModuleHandleW(L"renderdoc.dll");
  if (!module)
  {
    return nullptr;
  }

  debug("DX12: ...found, acquiring interface...");
  pRENDERDOC_GetAPI RENDERDOC_GetAPI;
  reinterpret_cast<FARPROC &>(RENDERDOC_GetAPI) = GetProcAddress(module, "RENDERDOC_GetAPI");
  RENDERDOC_API_1_5_0 *api = nullptr;
  if (RENDERDOC_GetAPI)
  {
    debug("DX12: ...requesting API table for version %u...", (unsigned)eRENDERDOC_API_Version_1_5_0);
    RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void **)&api);
  }
  return api;
}

void debug::gpu_capture::RenderDoc::configure() { api->UnloadCrashHandler(); }

void debug::gpu_capture::RenderDoc::beginCapture() { api->StartFrameCapture(nullptr, nullptr); }

void debug::gpu_capture::RenderDoc::endCapture() { api->EndFrameCapture(nullptr, nullptr); }

void debug::gpu_capture::RenderDoc::onPresent() {}

void debug::gpu_capture::RenderDoc::captureFrames(const wchar_t *, int count) { api->TriggerMultiFrameCapture(count); }

void debug::gpu_capture::RenderDoc::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->BeginEvent(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

void debug::gpu_capture::RenderDoc::endEvent(ID3D12GraphicsCommandList *cmd) { cmd->EndEvent(); }

void debug::gpu_capture::RenderDoc::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->SetMarker(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

ComPtr<IDXGraphicsAnalysis> debug::gpu_capture::LegacyPIX::try_connect_interface(Direct3D12Enviroment &d3d_env)
{
  debug("DX12: ...GPA for 'DXGIGetDebugInterface1'...");
  ComPtr<IDXGraphicsAnalysis> result;
  PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
  reinterpret_cast<FARPROC &>(DXGIGetDebugInterface1) = d3d_env.getDXGIProcAddress("DXGIGetDebugInterface1");

  if (DXGIGetDebugInterface1)
  {
    debug("DX12: ...calling DXGIGetDebugInterface1 for IDXGraphicsAnalysis...");
    HRESULT hr = DXGIGetDebugInterface1(0, COM_ARGS(&result));
    debug("DX12: ...DXGIGetDebugInterface1 returned %p - 0x%08x...", result.Get(), hr);
  }
  else
  {
    debug("DX12: ...failed...");
  }
  return result;
}

void debug::gpu_capture::LegacyPIX::configure() {}

void debug::gpu_capture::LegacyPIX::beginCapture()
{
  api->BeginCapture();
  isCapturing = true;
}

void debug::gpu_capture::LegacyPIX::endCapture()
{
  api->EndCapture();
  isCapturing = false;
}

void debug::gpu_capture::LegacyPIX::onPresent()
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
      beginCapture();
    }
  }
}

void debug::gpu_capture::LegacyPIX::captureFrames(const wchar_t *, int count) { framesToCapture = count; }

void debug::gpu_capture::LegacyPIX::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->BeginEvent(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

void debug::gpu_capture::LegacyPIX::endEvent(ID3D12GraphicsCommandList *cmd) { cmd->EndEvent(); }

void debug::gpu_capture::LegacyPIX::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->SetMarker(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

LibPointer debug::gpu_capture::PIX::try_load_runtime_interface() { return {LoadLibraryW(L"WinPixEventRuntime.dll"), {}}; }

LibPointer debug::gpu_capture::PIX::try_connect_capture_interface()
{
  HMODULE module = nullptr;
  if (GetModuleHandleExW(0, L"WinPixGpuCapturer.dll", &module))
  {
    return {module, {}};
  }
  return {};
}

LibPointer debug::gpu_capture::PIX::try_load_capture_interface() { return {PIXLoadLatestWinPixGpuCapturerLibrary(), {}}; }

void debug::gpu_capture::PIX::configure() { PIXSetHUDOptions(PIXHUDOptions::PIX_HUD_SHOW_ON_NO_WINDOWS); }

void debug::gpu_capture::PIX::beginCapture() { PIXBeginCapture2(PIX_CAPTURE_GPU, nullptr); }

void debug::gpu_capture::PIX::endCapture() { PIXEndCapture(false); }

void debug::gpu_capture::PIX::onPresent() {}

void debug::gpu_capture::PIX::captureFrames(const wchar_t *file_name, int count)
{
  wchar_t filePath[1024];
  GetCurrentDirectoryW(ARRAYSIZE(filePath), filePath);
  wcscat_s(filePath, L"\\GpuCaptures");
  CreateDirectoryW(filePath, nullptr);
  wcscat_s(filePath, L"\\");
  wcscat_s(filePath, file_name);
  wcscat(filePath, L".wpix");

  PIXGpuCaptureNextFrames(filePath, count);
}

void debug::gpu_capture::PIX::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  if (runtimeLib)
  {
    PIXBeginEvent(cmd, 0, text.data());
  }
  else
  {
    cmd->BeginEvent(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
  }
}

void debug::gpu_capture::PIX::endEvent(ID3D12GraphicsCommandList *cmd)
{
  if (runtimeLib)
  {
    PIXEndEvent(cmd);
  }
  else
  {
    cmd->EndEvent();
  }
}

void debug::gpu_capture::PIX::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  if (runtimeLib)
  {
    PIXSetMarker(cmd, 0, text.data());
  }
  else
  {
    cmd->SetMarker(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
  }
}

bool debug::gpu_capture::nvidia::NSight::try_connect_interface()
{
  debug("DX12: ...looking for Nomad.Injection.dll...");
  if (GetModuleHandleW(L"Nomad.Injection.dll"))
  {
    debug("DX12: ...found...");
    return true;
  }
  debug("DX12: ...nothing, looking for Nvda.Graphics.Interception.dll...");
  if (GetModuleHandleW(L"Nvda.Graphics.Interception.dll"))
  {
    debug("DX12: ...found...");
    return true;
  }
  debug("DX12: ...nothing, looking for NvLog.dll...");
  if (GetModuleHandleW(L"NvLog.dll"))
  {
    debug("DX12: ...found...");
    return true;
  }
  return false;
}

void debug::gpu_capture::nvidia::NSight::configure() {}
void debug::gpu_capture::nvidia::NSight::beginCapture() {}
void debug::gpu_capture::nvidia::NSight::endCapture() {}
void debug::gpu_capture::nvidia::NSight::onPresent() {}
void debug::gpu_capture::nvidia::NSight::captureFrames(const wchar_t *, int) {}
void debug::gpu_capture::nvidia::NSight::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->BeginEvent(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}
void debug::gpu_capture::nvidia::NSight::endEvent(ID3D12GraphicsCommandList *cmd) { cmd->EndEvent(); }
void debug::gpu_capture::nvidia::NSight::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->SetMarker(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}
