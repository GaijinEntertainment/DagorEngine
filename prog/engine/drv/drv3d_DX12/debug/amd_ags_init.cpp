// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amd_ags_init.h"

#include <drv/shadersMetaData/dxil/compiled_shader_header.h>

#include <debug/dag_log.h>

namespace drv3d_dx12::debug::ags
{

bool create_device_with_user_markers(AGSContext *ags_context, DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level,
  void **ptr, HLSLVendorExtensions &extensions)
{
  logdbg("DX12: AGS device creation with user markers...");
  AGSDX12DeviceCreationParams params = {};
  params.pAdapter = adapter;
  params.FeatureLevel = minimum_feature_level;
  params.iid = uuid;
  AGSDX12ExtensionParams extensionParams = {};
  AGSDX12ReturnedParams returnedParams = {};
  const auto agsResult = agsDriverExtensionsDX12_CreateDevice(ags_context, &params, &extensionParams, &returnedParams);
  if (DAGOR_UNLIKELY(agsResult != AGS_SUCCESS))
  {
    logdbg("DX12: ...failed (%d)", agsResult);
    return false;
  }
  logdbg("DX12: ...success: device has been created, supported extensions:");
  logdbg("DX12: intrinsics16    : %s", returnedParams.extensionsSupported.intrinsics16 ? "yes" : "no");
  logdbg("DX12: intrinsics17    : %s", returnedParams.extensionsSupported.intrinsics17 ? "yes" : "no");
  logdbg("DX12: userMarkers     : %s", returnedParams.extensionsSupported.userMarkers ? "yes" : "no");
  logdbg("DX12: appRegistration : %s", returnedParams.extensionsSupported.appRegistration ? "yes" : "no");
  logdbg("DX12: UAVBindSlot     : %s", returnedParams.extensionsSupported.UAVBindSlot ? "yes" : "no");
  logdbg("DX12: intrinsics19    : %s", returnedParams.extensionsSupported.intrinsics19 ? "yes" : "no");
  logdbg("DX12: baseVertex      : %s", returnedParams.extensionsSupported.baseVertex ? "yes" : "no");
  logdbg("DX12: baseInstance    : %s", returnedParams.extensionsSupported.baseInstance ? "yes" : "no");
  logdbg("DX12: getWaveSize     : %s", returnedParams.extensionsSupported.getWaveSize ? "yes" : "no");
  logdbg("DX12: floatConversion : %s", returnedParams.extensionsSupported.floatConversion ? "yes" : "no");
  logdbg("DX12: readLaneAt      : %s", returnedParams.extensionsSupported.readLaneAt ? "yes" : "no");
  logdbg("DX12: rayHitToken     : %s", returnedParams.extensionsSupported.rayHitToken ? "yes" : "no");
  logdbg("DX12: shaderClock     : %s", returnedParams.extensionsSupported.shaderClock ? "yes" : "no");

  if (!returnedParams.extensionsSupported.userMarkers)
  {
    logdbg("DX12: ...failed. User markers are not supported by the AGS device.");
    return false;
  }

  extensions.vendor = dxil::ExtensionVendor::AMD;
  extensions.vendorExtensionMask |= returnedParams.extensionsSupported.intrinsics16 ? dxil::VendorExtensionBits::AMD_INTRISICS_16 : 0;
  extensions.vendorExtensionMask |= returnedParams.extensionsSupported.intrinsics17 ? dxil::VendorExtensionBits::AMD_INTRISICS_17 : 0;
  extensions.vendorExtensionMask |= returnedParams.extensionsSupported.intrinsics19 ? dxil::VendorExtensionBits::AMD_INTRISICS_19 : 0;
  extensions.vendorExtensionMask |= returnedParams.extensionsSupported.baseVertex ? dxil::VendorExtensionBits::AMD_GET_BASE_VERTEX : 0;
  extensions.vendorExtensionMask |=
    returnedParams.extensionsSupported.baseInstance ? dxil::VendorExtensionBits::AMD_GET_BASE_INSTANCE : 0;
  extensions.vendorExtensionMask |= returnedParams.extensionsSupported.getWaveSize ? dxil::VendorExtensionBits::AMD_GET_WAVE_SIZE : 0;
  extensions.vendorExtensionMask |=
    returnedParams.extensionsSupported.floatConversion ? dxil::VendorExtensionBits::AMD_FLOAT_CONVERSION : 0;
  extensions.vendorExtensionMask |= returnedParams.extensionsSupported.readLaneAt ? dxil::VendorExtensionBits::AMD_READ_LINE_AT : 0;
  extensions.vendorExtensionMask |=
    returnedParams.extensionsSupported.rayHitToken ? dxil::VendorExtensionBits::AMD_DXR_RAY_HIT_TOKEN : 0;
  extensions.vendorExtensionMask |= returnedParams.extensionsSupported.shaderClock ? dxil::VendorExtensionBits::AMD_SHADER_CLOCK : 0;

  *ptr = returnedParams.pDevice;
  G_ASSERT(*ptr);
  return true;
}

static AGSContext *initialize_context()
{
  AGSContext *agsContext = nullptr;
  AGSGPUInfo gpuInfo = {};
  AGSConfiguration agsConfig = {};

  logdbg("DX12: AMD GPU Services API initialization...");
  const auto agsResult = agsInitialize(AGS_MAKE_VERSION(AMD_AGS_VERSION_MAJOR, AMD_AGS_VERSION_MINOR, AMD_AGS_VERSION_PATCH),
    &agsConfig, &agsContext, &gpuInfo);
  if (DAGOR_UNLIKELY(agsResult != AGS_SUCCESS))
  {
    logdbg("DX12: ...failed (%d)", agsResult);
    return nullptr;
  }
  logdbg("DX12: ...success");
  return agsContext;
}

class AgsContextHandle
{
public:
  AgsContextHandle() : agsContext{initialize_context()} {}
  ~AgsContextHandle()
  {
    if (agsContext)
    {
      logdbg("DX12: Shutting down AMD GPU Services API");
      if (agsDeInitialize(agsContext) != AGS_SUCCESS)
        logdbg("Failed to cleanup AGS Library");
    }
  }

  AGSContext *get() const { return agsContext; }

private:
  AGSContext *agsContext;
};

AGSContext *get_context()
{
  static AgsContextHandle agsContextHandle;
  return agsContextHandle.get();
}

} // namespace drv3d_dx12::debug::ags
