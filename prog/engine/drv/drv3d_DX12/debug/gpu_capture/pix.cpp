// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pix.h"

#include <osApiWrappers/dag_versionQuery.h>

#include <pix3.h> // not self-contained, has to be iuncluded after driver.h


namespace
{
void win_pix_gpu_capturer_issue_check(HMODULE module, ::drv3d_dx12::debug::gpu_capture::Issues &issues)
{
  auto version = get_library_version(module);
  if (version)
  {
    // This version of pix GPU capture tool causes nullptr exceptions during command list submission as soon as a resource is created
    // using a external heap. After the dll is loaded in, the process is tainted and unloading will not revert this issue.
    issues.brokenExistingHeaps = *version == LibraryVersion{.major = 1, .minor = 0, .build = 30001, .revision = 2501};

    if (issues.brokenExistingHeaps)
    {
      logdbg("DX12: Detected broken <WinPixGpuCapturer.dll> version %u.%u.%u.%u, can not use existing heaps feature or we risk "
             "nullptr execption on command list submission",
        version->major, version->minor, version->revision, version->build);
    }
  }
  else
  {
    logwarn("DX12: Unable to get DLL version of <WinPixGpuCapturer.dll>, this may cause crashes with broken versions");
  }
}
} // namespace

namespace drv3d_dx12::debug::gpu_capture
{

LibPointer PIX::try_load_runtime_interface() { return {LoadLibraryW(L"WinPixEventRuntime.dll"), {}}; }

LibPointer PIX::try_connect_capture_interface(Issues &issues)
{
  HMODULE module = nullptr;
  if (!GetModuleHandleExW(0, L"WinPixGpuCapturer.dll", &module))
  {
    return {};
  }
  win_pix_gpu_capturer_issue_check(module, issues);
  return {module, {}};
}

LibPointer PIX::try_load_capture_interface(Issues &issues)
{
  LibPointer lib{PIXLoadLatestWinPixGpuCapturerLibrary(), {}};
  if (lib)
  {
    win_pix_gpu_capturer_issue_check(lib.get(), issues);
  }

  return lib;
}

void PIX::configure() { PIXSetHUDOptions(PIXHUDOptions::PIX_HUD_SHOW_ON_NO_WINDOWS); }

static void prepare_file_path(wchar_t *file_path, size_t file_path_size, const wchar_t *file_name)
{
  GetCurrentDirectoryW(file_path_size, file_path);
  wcscat_s(file_path, file_path_size, L"\\GpuCaptures");
  CreateDirectoryW(file_path, nullptr);
  wcscat_s(file_path, file_path_size, L"\\");
  wcscat_s(file_path, file_path_size, file_name);
  wcscat_s(file_path, file_path_size, L".wpix");
}

void PIX::beginCapture(const wchar_t *name)
{
  eastl::basic_string<wchar_t> filePath;
  filePath.resize(1024);
  prepare_file_path(filePath.data(), filePath.size(), name ? name : L"capture");
  PIXCaptureParameters params = {};
  params.GpuCaptureParameters.FileName = filePath.data();
  PIXBeginCapture2(PIX_CAPTURE_GPU, &params);
}

void PIX::endCapture() { PIXEndCapture(false); }

void PIX::onPresent() {}

void PIX::captureFrames(const wchar_t *file_name, int count)
{
  eastl::basic_string<wchar_t> filePath;
  filePath.resize(1024);
  prepare_file_path(filePath.data(), filePath.size(), file_name);

  PIXGpuCaptureNextFrames(filePath.data(), count);
}

void PIX::beginEvent(D3DGraphicsCommandList *cmd, eastl::span<const char> text)
{
  if (runtimeLib)
  {
    PIXBeginEvent(cmd, 0, text.data());
  }
  else
  {
    cmd->BeginEvent(WINPIX_EVENT_ANSI_VERSION, text.data(), text.size());
  }
}

void PIX::endEvent(D3DGraphicsCommandList *cmd)
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

void PIX::marker(D3DGraphicsCommandList *cmd, eastl::span<const char> text)
{
  if (runtimeLib)
  {
    PIXSetMarker(cmd, 0, text.data());
  }
  else
  {
    cmd->SetMarker(WINPIX_EVENT_ANSI_VERSION, text.data(), text.size());
  }
}
} // namespace drv3d_dx12::debug::gpu_capture
