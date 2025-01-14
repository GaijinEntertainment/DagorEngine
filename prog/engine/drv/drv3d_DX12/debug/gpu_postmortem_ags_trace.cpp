// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpu_postmortem_ags_trace.h"
#include "amd_ags_init.h"


namespace drv3d_dx12::debug::gpu_postmortem::ags
{
AgsTrace::~AgsTrace() {}

void AgsTrace::beginEvent(ID3D12GraphicsCommandList *command_list, eastl::span<const char>, const eastl::string &full_path)
{
  if (!deviceInitialized)
    return;

  const auto agsResult = agsDriverExtensionsDX12_PushMarker(agsContext, command_list, full_path.c_str());
  if (agsResult == AGS_SUCCESS)
    return;
  logwarn("DX12: Failed to push marker to AGS (%d)", agsResult);
}

void AgsTrace::endEvent(ID3D12GraphicsCommandList *command_list, const eastl::string &)
{
  if (!deviceInitialized)
    return;

  const auto agsResult = agsDriverExtensionsDX12_PopMarker(agsContext, command_list);
  if (agsResult == AGS_SUCCESS)
    return;
  logwarn("DX12: Failed to pop marker to AGS (%d)", agsResult);
}

void AgsTrace::onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &)
{
  logdbg("DX12: Use Radeon GPU Detective to gather information...");
}

bool AgsTrace::tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr)
{
  deviceInitialized = debug::ags::create_device_with_user_markers(agsContext, adapter, uuid, minimum_feature_level, ptr);
  return deviceInitialized;
}
} // namespace drv3d_dx12::debug::gpu_postmortem::ags
