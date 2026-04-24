// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "radeon_gpu_profiler.h"

#include <EASTL/string.h>


namespace drv3d_dx12::debug::gpu_capture::amd
{
void RadeonGPUProfiler::configure() {}
void RadeonGPUProfiler::beginCapture(const wchar_t *) {}
void RadeonGPUProfiler::endCapture() {}
void RadeonGPUProfiler::onPresent() {}
void RadeonGPUProfiler::captureFrames(const wchar_t *, int) {}
void RadeonGPUProfiler::beginEvent(D3DGraphicsCommandList *command_list, eastl::span<const char> text)
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
void RadeonGPUProfiler::endEvent(D3DGraphicsCommandList *command_list)
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
void RadeonGPUProfiler::marker(D3DGraphicsCommandList *command_list, eastl::span<const char> text)
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

bool RadeonGPUProfiler::try_connect_interface()
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

bool RadeonGPUProfiler::tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr,
  HLSLVendorExtensions &extensions)
{
  deviceInitialized = debug::ags::create_device_with_user_markers(agsContext, adapter, uuid, minimum_feature_level, ptr, extensions);
  return deviceInitialized;
}

void RadeonGPUProfiler::nameResource(ID3D12Resource *, eastl::string_view) {}
void RadeonGPUProfiler::nameResource(ID3D12Resource *, eastl::wstring_view) {}
void RadeonGPUProfiler::nameObject(ID3D12Object *, eastl::string_view) {}
void RadeonGPUProfiler::nameObject(ID3D12Object *, eastl::wstring_view) {}
} // namespace drv3d_dx12::debug::gpu_capture::amd
