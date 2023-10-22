#include "ngx_wrapper.h"
#include "device.h"

using namespace drv3d_dx12;

NVSDK_NGX_Result NgxWrapper::ngxInit(int app_id, const wchar_t *log_dir, void *device, const NVSDK_NGX_FeatureCommonInfo *info)
{
  return NVSDK_NGX_D3D12_Init(app_id, log_dir, static_cast<ID3D12Device *>(device), info);
}

NVSDK_NGX_Result NgxWrapper::ngxShutdown(void *device) { return NVSDK_NGX_D3D12_Shutdown1(static_cast<ID3D12Device *>(device)); }

#ifdef NGX_ENABLE_DEPRECATED_GET_PARAMETERS
NgxWrapper::NVSDK_NGX_Parameter_Result NgxWrapper::ngxGetParameters()
{
  NVSDK_NGX_Parameter *out_params;
  NVSDK_NGX_Result result = NVSDK_NGX_D3D12_GetParameters(&out_params);
  return {NVSDK_NGX_Parameter_Ptr(out_params, [](auto) {}), result};
}
#endif

NgxWrapper::NVSDK_NGX_Parameter_Result NgxWrapper::ngxGetCapabilityParameters()
{
  NVSDK_NGX_Parameter *out_params;
  NVSDK_NGX_Result result = NVSDK_NGX_D3D12_GetCapabilityParameters(&out_params);
  return {NVSDK_NGX_Parameter_Ptr(out_params, [](auto p) { NVSDK_NGX_D3D12_DestroyParameters(p); }), result};
}

NgxWrapper::NVSDK_NGX_Parameter_Result NgxWrapper::ngxAllocateParameters()
{
  NVSDK_NGX_Parameter *out_params;
  NVSDK_NGX_Result result = NVSDK_NGX_D3D12_AllocateParameters(&out_params);
  return {NVSDK_NGX_Parameter_Ptr(out_params, [](auto p) { NVSDK_NGX_D3D12_DestroyParameters(p); }), result};
}

NVSDK_NGX_Result NgxWrapper::ngxDestroyParameters(NVSDK_NGX_Parameter *params) { return NVSDK_NGX_D3D12_DestroyParameters(params); }

NVSDK_NGX_Result NgxWrapper::ngxCreateDlssFeature(void *context, NVSDK_NGX_Handle **out_handle, NVSDK_NGX_Parameter *in_params,
  NVSDK_NGX_DLSS_Create_Params *dlss_create_params, uint32_t creation_node_mask, uint32_t visibility_node_mask)
{
  return NGX_D3D12_CREATE_DLSS_EXT(static_cast<ID3D12GraphicsCommandList *>(context), creation_node_mask, visibility_node_mask,
    out_handle, in_params, dlss_create_params);
}

NVSDK_NGX_Result NgxWrapper::ngxReleaseDlssFeature(NVSDK_NGX_Handle *feature_handle)
{
  return NVSDK_NGX_D3D12_ReleaseFeature(feature_handle);
}

NVSDK_NGX_Result NgxWrapper::ngxEvaluateDlss(void *context, NVSDK_NGX_Handle *feature_handle, NVSDK_NGX_Parameter *in_params,
  const void *dlss_params, const NVSDK_NGX_Dimensions &render_dimensions)
{
  const DlssParamsDx12 &dlssParams = *static_cast<const DlssParamsDx12 *>(dlss_params);

  G_ASSERT_RETURN(dlssParams.inColor != nullptr, NVSDK_NGX_Result_FAIL_InvalidParameter);
  G_ASSERT_RETURN(dlssParams.inDepth != nullptr, NVSDK_NGX_Result_FAIL_InvalidParameter);
  G_ASSERT_RETURN(dlssParams.inMotionVectors != nullptr, NVSDK_NGX_Result_FAIL_InvalidParameter);
  G_ASSERT_RETURN(dlssParams.outColor != nullptr, NVSDK_NGX_Result_FAIL_InvalidParameter);
  G_ASSERTF_RETURN(dlssParams.inSharpness >= 0 && dlssParams.inSharpness <= 1, NVSDK_NGX_Result_FAIL_InvalidParameter,
    "Invalid inSharpness value in dlssParams. Value should be between in range [0, 1], but value is: %f", dlssParams.inSharpness);
  G_ASSERTF_RETURN(abs(dlssParams.inJitterOffsetX) <= 0.5f && abs(dlssParams.inJitterOffsetY) <= 0.5f,
    NVSDK_NGX_Result_FAIL_InvalidParameter,
    "Invalid parameters values in dlssParams. Jitter values should be in range [-0.5, 0.5], but they are: x:%f, y:%f",
    dlssParams.inJitterOffsetX, dlssParams.inJitterOffsetY);

  NVSDK_NGX_D3D12_DLSS_Eval_Params evalParams = {};
  evalParams.Feature.pInColor = dlssParams.inColor->getHandle();
  evalParams.Feature.pInOutput = dlssParams.outColor->getHandle();
  evalParams.pInExposureTexture = dlssParams.inExposure ? dlssParams.inExposure->getHandle() : nullptr;
  evalParams.Feature.InSharpness = dlssParams.inSharpness;
  evalParams.pInDepth = dlssParams.inDepth->getHandle();
  evalParams.pInMotionVectors = dlssParams.inMotionVectors->getHandle();
  evalParams.InJitterOffsetX = dlssParams.inJitterOffsetX;
  evalParams.InJitterOffsetY = dlssParams.inJitterOffsetY;
  evalParams.InRenderSubrectDimensions = render_dimensions;
  evalParams.InMVScaleX = dlssParams.inMVScaleX;
  evalParams.InMVScaleY = dlssParams.inMVScaleY;
  evalParams.InColorSubrectBase.X = dlssParams.inColorDepthOffsetX;
  evalParams.InColorSubrectBase.Y = dlssParams.inColorDepthOffsetY;
  evalParams.InDepthSubrectBase.X = dlssParams.inColorDepthOffsetX;
  evalParams.InDepthSubrectBase.Y = dlssParams.inColorDepthOffsetY;

  return NGX_D3D12_EVALUATE_DLSS_EXT(static_cast<ID3D12GraphicsCommandList *>(context), feature_handle, in_params, &evalParams);
}

NVSDK_NGX_Result NgxWrapper::ngxDlssGetStats(unsigned long long *pVRAMAllocatedBytes)
{
  *pVRAMAllocatedBytes = 0;
  NVSDK_NGX_Parameter *out_params;
  NVSDK_NGX_Result result = NVSDK_NGX_D3D12_GetParameters(&out_params);
  if (result == NVSDK_NGX_Result_Success)
    NGX_DLSS_GET_STATS(out_params, pVRAMAllocatedBytes);

  return result;
}