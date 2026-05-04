// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render_doc.h"

#include <RenderDoc/renderdoc_app.h>
#include "drv_log_defs.h"

namespace drv3d_dx12::debug::gpu_capture
{
static void set_capture_path(RENDERDOC_API_1_5_0 *api, const wchar_t *name)
{
  if (!name)
    return;

  char path[MAX_PATH];
  GetCurrentDirectoryA(sizeof(path), path);
  strcat_s(path, "\\GpuCaptures");
  CreateDirectoryA(path, nullptr);
  strcat_s(path, "\\");

  size_t pathLen = strlen(path);
  if (WideCharToMultiByte(CP_UTF8, 0, name, -1, path + pathLen, sizeof(path) - (int)pathLen, nullptr, nullptr) == 0)
  {
    D3D_ERROR("DX12: RenderDoc capture path name conversion failed");
    return;
  }

  logdbg("DX12: RenderDoc capture path template: %s", path);
  api->SetCaptureFilePathTemplate(path);
}
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

void RenderDoc::beginCapture(const wchar_t *name)
{
  set_capture_path(api, name);
  api->StartFrameCapture(nullptr, nullptr);
  isCapturing = true;
}

void RenderDoc::endCapture()
{
  uint32_t result = api->EndFrameCapture(nullptr, nullptr);
  isCapturing = false;
  logdbg("DX12: RenderDoc EndFrameCapture result: %u, total captures: %u", result, api->GetNumCaptures());
}

void RenderDoc::onPresent()
{
  if (isCapturing && framesToCapture == 0)
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

void RenderDoc::captureFrames(const wchar_t *name, int count)
{
  set_capture_path(api, name);
  framesToCapture = count;
}

} // namespace drv3d_dx12::debug::gpu_capture
