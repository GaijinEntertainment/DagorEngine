// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpu_postmortem_ags_trace.h"
#include "amd_ags_init.h"

#include <EASTL/fixed_string.h>


namespace drv3d_dx12::debug::gpu_postmortem::ags
{
// AGS takes a null-terminated string, a span may have no terminator
using MarkerString = eastl::fixed_string<char, 256>;

AgsTrace::~AgsTrace() {}

// The event name, not the full path: RGD builds the tree from the push and pop nesting
void AgsTrace::beginEvent(D3DGraphicsCommandList *command_list, eastl::span<const char> name, const eastl::string &)
{
  if (!deviceInitialized)
    return;

  const MarkerString markerName(name.data(), name.size());
  const auto agsResult = agsDriverExtensionsDX12_PushMarker(agsContext, command_list, markerName.c_str());
  if (agsResult == AGS_SUCCESS)
    return;
  logwarn("DX12: Failed to push marker to AGS (%d)", agsResult);
}

void AgsTrace::endEvent(D3DGraphicsCommandList *command_list, const eastl::string &)
{
  if (!deviceInitialized)
    return;

  const auto agsResult = agsDriverExtensionsDX12_PopMarker(agsContext, command_list);
  if (agsResult == AGS_SUCCESS)
    return;
  logwarn("DX12: Failed to pop marker to AGS (%d)", agsResult);
}

void AgsTrace::marker(D3DGraphicsCommandList *command_list, eastl::span<const char> text)
{
  if (!deviceInitialized)
    return;

  // RGD shows only push and pop ranges, not set marker points, so a marker is an empty range
  const MarkerString markerText(text.data(), text.size());
  auto agsResult = agsDriverExtensionsDX12_PushMarker(agsContext, command_list, markerText.c_str());
  if (agsResult == AGS_SUCCESS)
    agsResult = agsDriverExtensionsDX12_PopMarker(agsContext, command_list);
  if (agsResult == AGS_SUCCESS)
    return;
  logwarn("DX12: Failed to set marker to AGS (%d)", agsResult);
}

void AgsTrace::onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &)
{
  logdbg("DX12: Use Radeon GPU Detective to gather information...");
}

bool AgsTrace::tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr,
  HLSLVendorExtensions &extensions)
{
  deviceInitialized = debug::ags::create_device_with_user_markers(agsContext, adapter, uuid, minimum_feature_level, ptr, extensions);
  return deviceInitialized;
}
} // namespace drv3d_dx12::debug::gpu_postmortem::ags
