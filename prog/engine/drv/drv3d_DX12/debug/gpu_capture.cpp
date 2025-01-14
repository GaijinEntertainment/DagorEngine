// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpu_capture.h"
#include "global_state.h"
#include <platform.h>

#include <RenderDoc/renderdoc_app.h>

#include "amd_ags_init.h"

#if USE_PIX
// PROFILE_BUILD will enable USE_PIX in pix3.h if architecture is supported
#define PROFILE_BUILD
#if !defined(__d3d12_h__)
#define __d3d12_h__
#include <WinPixEventRuntime/pix3.h>
#undef __d3d12_h__
#else
#include <WinPixEventRuntime/pix3.h>
#endif
#endif

#if !defined(PIX_EVENT_UNICODE_VERSION)
#define PIX_EVENT_UNICODE_VERSION 0
#endif

#if !defined(PIX_EVENT_ANSI_VERSION)
#define PIX_EVENT_ANSI_VERSION 1
#endif

#if !defined(PIX_EVENT_PIX3BLOB_VERSION)
#define PIX_EVENT_PIX3BLOB_VERSION 2
#endif

namespace drv3d_dx12::debug::gpu_capture
{
RENDERDOC_API_1_5_0 *RenderDoc::try_connect_interface()
{
  auto module = GetModuleHandleW(L"renderdoc.dll");
  if (!module)
  {
    return nullptr;
  }

  logdbg("DX12: ...found, acquiring interface...");
  pRENDERDOC_GetAPI RENDERDOC_GetAPI;
  reinterpret_cast<FARPROC &>(RENDERDOC_GetAPI) = GetProcAddress(module, "RENDERDOC_GetAPI");
  RENDERDOC_API_1_5_0 *api = nullptr;
  if (RENDERDOC_GetAPI)
  {
    logdbg("DX12: ...requesting API table for version %u...", (unsigned)eRENDERDOC_API_Version_1_5_0);
    RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void **)&api);
  }
  return api;
}

void RenderDoc::configure() { api->UnloadCrashHandler(); }

void RenderDoc::beginCapture(const wchar_t *) { api->StartFrameCapture(nullptr, nullptr); }

void RenderDoc::endCapture() { api->EndFrameCapture(nullptr, nullptr); }

void RenderDoc::onPresent() {}

void RenderDoc::captureFrames(const wchar_t *, int count) { api->TriggerMultiFrameCapture(count); }

void RenderDoc::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->BeginEvent(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

void RenderDoc::endEvent(ID3D12GraphicsCommandList *cmd) { cmd->EndEvent(); }

void RenderDoc::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->SetMarker(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

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

void LegacyPIX::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->BeginEvent(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

void LegacyPIX::endEvent(ID3D12GraphicsCommandList *cmd) { cmd->EndEvent(); }

void LegacyPIX::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->SetMarker(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

LibPointer PIX::try_load_runtime_interface() { return {LoadLibraryW(L"WinPixEventRuntime.dll"), {}}; }

LibPointer PIX::try_connect_capture_interface()
{
  HMODULE module = nullptr;
  if (GetModuleHandleExW(0, L"WinPixGpuCapturer.dll", &module))
  {
    return {module, {}};
  }
  return {};
}

#if USE_PIX
LibPointer PIX::try_load_capture_interface() { return {PIXLoadLatestWinPixGpuCapturerLibrary(), {}}; }

void PIX::configure() { PIXSetHUDOptions(PIXHUDOptions::PIX_HUD_SHOW_ON_NO_WINDOWS); }

void PIX::beginCapture(const wchar_t *name)
{
  PIXCaptureParameters params = {};
  params.GpuCaptureParameters.FileName = name;
  PIXBeginCapture2(PIX_CAPTURE_GPU, name ? &params : nullptr);
}

void PIX::endCapture() { PIXEndCapture(false); }

void PIX::onPresent() {}

void PIX::captureFrames(const wchar_t *file_name, int count)
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

void PIX::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
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

void PIX::endEvent(ID3D12GraphicsCommandList *cmd)
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

void PIX::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
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
#else
LibPointer PIX::try_load_capture_interface() { return {}; }
void PIX::configure() {}
void PIX::beginCapture(const wchar_t *) {}
void PIX::endCapture() {}
void PIX::onPresent() {}
void PIX::captureFrames(const wchar_t *, int) {}
void PIX::beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
void PIX::endEvent(ID3D12GraphicsCommandList *) {}
void PIX::marker(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
#endif

bool nvidia::NSight::try_connect_interface()
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
  return false;
}

void nvidia::NSight::configure() {}
void nvidia::NSight::beginCapture(const wchar_t *) {}
void nvidia::NSight::endCapture() {}
void nvidia::NSight::onPresent() {}
void nvidia::NSight::captureFrames(const wchar_t *, int) {}
void nvidia::NSight::beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->BeginEvent(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}
void nvidia::NSight::endEvent(ID3D12GraphicsCommandList *cmd) { cmd->EndEvent(); }
void nvidia::NSight::marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->SetMarker(PIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

#if HAS_AMD_GPU_SERVICES
void amd::RadeonGPUProfiler::configure() {}
void amd::RadeonGPUProfiler::beginCapture(const wchar_t *) {}
void amd::RadeonGPUProfiler::endCapture() {}
void amd::RadeonGPUProfiler::onPresent() {}
void amd::RadeonGPUProfiler::captureFrames(const wchar_t *, int) {}
void amd::RadeonGPUProfiler::beginEvent(ID3D12GraphicsCommandList *command_list, eastl::span<const char> text)
{
  G_ASSERT_RETURN(command_list, );
  G_ASSERT_RETURN(agsContext, );

  if (!deviceInitialized)
    return;

  const auto agsResult = agsDriverExtensionsDX12_PushMarker(agsContext, command_list, eastl::string(text.cbegin()).c_str());
  if (EASTL_LIKELY(agsResult == AGS_SUCCESS))
    return;
  logwarn("DX12: Failed to push AGS marker %s. Result = %d", eastl::string(text.data(), text.size()).c_str(), agsResult);
}
void amd::RadeonGPUProfiler::endEvent(ID3D12GraphicsCommandList *command_list)
{
  G_ASSERT_RETURN(command_list, );
  G_ASSERT_RETURN(agsContext, );

  if (!deviceInitialized)
    return;

  const auto agsResult = agsDriverExtensionsDX12_PopMarker(agsContext, command_list);
  if (EASTL_LIKELY(agsResult == AGS_SUCCESS))
    return;
  logwarn("DX12: Failed to pop AGS marker. Result = %d", agsResult);
}
void amd::RadeonGPUProfiler::marker(ID3D12GraphicsCommandList *command_list, eastl::span<const char> text)
{
  G_ASSERT_RETURN(command_list, );
  G_ASSERT_RETURN(agsContext, );

  if (!deviceInitialized)
    return;

  const auto agsResult = agsDriverExtensionsDX12_SetMarker(agsContext, command_list, eastl::string(text.cbegin()).c_str());
  if (EASTL_LIKELY(agsResult == AGS_SUCCESS))
    return;
  logwarn("DX12: Failed to set AGS marker %s. Result = %d", eastl::string(text.data(), text.size()).c_str(), agsResult);
}

bool amd::RadeonGPUProfiler::try_connect_interface()
{
#if _TARGET_64BIT
  logdbg("DX12: Looking for amd_ags_x64.dll...");
  if (GetModuleHandleW(L"amd_ags_x64.dll"))
  {
    logdbg("DX12: ...found...");
    return true;
  }
#else
  logdbg("DX12: Looking for amd_ags_x86.dll...");
  if (GetModuleHandleW(L"amd_ags_x86.dll"))
  {
    logdbg("DX12: ...found...");
    return true;
  }
#endif
  return false;
}

bool amd::RadeonGPUProfiler::tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr)
{
  deviceInitialized = debug::ags::create_device_with_user_markers(agsContext, adapter, uuid, minimum_feature_level, ptr);
  return deviceInitialized;
}
#endif

} // namespace drv3d_dx12::debug::gpu_capture
