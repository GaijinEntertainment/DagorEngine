#pragma once

#include "ngx_wrapper_base.h"

struct ID3D12Device;

namespace drv3d_dx12
{
class Image;

struct DlssParamsDx12
{
  Image *inColor;
  Image *inDepth;
  Image *inMotionVectors;
  Image *inExposure;
  float inSharpness;
  float inJitterOffsetX;
  float inJitterOffsetY;
  float inMVScaleX;
  float inMVScaleY;
  int inColorDepthOffsetX;
  int inColorDepthOffsetY;

  Image *outColor;
};

struct XessParamsDx12
{
  Image *inColor;
  Image *inDepth;
  Image *inMotionVectors;
  float inJitterOffsetX;
  float inJitterOffsetY;
  float inInputWidth;
  float inInputHeight;
  int inColorDepthOffsetX;
  int inColorDepthOffsetY;

  Image *outColor;
};

class NgxWrapper final : public NgxWrapperBase
{
#if HAS_NVSDK_NGX
protected:
  NVSDK_NGX_Result ngxInit(int app_id, const wchar_t *log_dir, void *device, const NVSDK_NGX_FeatureCommonInfo *info) override;
  NVSDK_NGX_Result ngxShutdown(void *device) override;
#ifdef NGX_ENABLE_DEPRECATED_GET_PARAMETERS
  NVSDK_NGX_Parameter_Result ngxGetParameters() override;
#endif
  NVSDK_NGX_Parameter_Result ngxGetCapabilityParameters() override;
  NVSDK_NGX_Parameter_Result ngxAllocateParameters() override;
  NVSDK_NGX_Result ngxDestroyParameters(NVSDK_NGX_Parameter *params) override;
  NVSDK_NGX_Result ngxCreateDlssFeature(void *context, NVSDK_NGX_Handle **out_handle, NVSDK_NGX_Parameter *in_params,
    NVSDK_NGX_DLSS_Create_Params *dlss_create_params, uint32_t creation_node_mask, uint32_t visibility_node_mask) override;
  NVSDK_NGX_Result ngxReleaseDlssFeature(NVSDK_NGX_Handle *feature_handle) override;
  NVSDK_NGX_Result ngxEvaluateDlss(void *context, NVSDK_NGX_Handle *feature_handle, NVSDK_NGX_Parameter *in_params,
    const void *dlss_params, const NVSDK_NGX_Dimensions &render_dimensions) override;
#endif
};
} // namespace drv3d_dx12