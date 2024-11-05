// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpu_postmortem_ags_trace.h"

namespace drv3d_dx12::debug::gpu_postmortem::ags
{
AgsTrace::~AgsTrace()
{
  logdbg("DX12: Shutting down AMD GPU Services API");
  if (agsDeInit(agsContext) != AGS_SUCCESS)
  {
    printf("Failed to cleanup AGS Library\n");
  }
}

void AgsTrace::beginEvent(ID3D12GraphicsCommandList *command_list, eastl::span<const char>, const eastl::string &full_path)
{
  const auto agsResult = agsDriverExtensionsDX12_PushMarker(agsContext, command_list, full_path.c_str());
  if (agsResult == AGS_SUCCESS)
    return;
  logwarn("DX12: Failed to push marker to AGS (%d)", agsResult);
}

void AgsTrace::endEvent(ID3D12GraphicsCommandList *command_list, const eastl::string &)
{
  const auto agsResult = agsDriverExtensionsDX12_PopMarker(agsContext, command_list);
  if (agsResult == AGS_SUCCESS)
    return;
  logwarn("DX12: Failed to pop marker to AGS (%d)", agsResult);
}

void AgsTrace::onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &)
{
  logdbg("DX12: Use Radeon GPU Detective to gather information...");
}

bool AgsTrace::tryCreateDevice(IUnknown *adapter, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr)
{
  logdbg("DX12: Ags device creation...");
  AGSDX12DeviceCreationParams params = {};
  params.pAdapter = static_cast<IDXGIAdapter *>(adapter);
  params.FeatureLevel = minimum_feature_level;
  params.iid = __uuidof(ID3D12Device);
  AGSDX12ExtensionParams extensionParams = {};
  AGSDX12ReturnedParams returnedParams = {};
  const auto agsResult = agsDriverExtensionsDX12_CreateDevice(agsContext, &params, &extensionParams, &returnedParams);
  if (agsResult != AGS_SUCCESS)
  {
    logwarn("DX12: ...failed (%d)", agsResult);
    return false;
  }
  logdbg("DX12: ...success: device has been created, supported extensions: %x", returnedParams.extensionsSupported);
  *ptr = returnedParams.pDevice;
  G_ASSERT(*ptr);
  return true;
}
} // namespace drv3d_dx12::debug::gpu_postmortem::ags
