#ifndef __DRV3D_DX11_NGX_WRAPPER_BASE_H
#define __DRV3D_DX11_NGX_WRAPPER_BASE_H
#pragma once

#include <3d/dag_drvDecl.h>
#include <3d/dag_drv3dConsts.h>
#if HAS_NVSDK_NGX
#include <nvsdk_ngx_helpers.h>
#endif

#include <EASTL/tuple.h>
#include <EASTL/unique_ptr.h>

class NgxWrapperBase
{
public:
  NgxWrapperBase() = default;
  virtual ~NgxWrapperBase() = default;
  bool init(void *device, const char *log_dir);
  bool shutdown(void *device);
  bool checkDlssSupport();
  bool isDlssQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int dlss_quality) const;
  bool createOptimalDlssFeature(void *context, uint32_t target_width, uint32_t target_height, int dlss_quality, bool stereo_render,
    uint32_t creation_node_mask = 0, uint32_t visibility_node_mask = 0);
  bool releaseDlssFeature();
  DlssState getDlssState() const;
  void getDlssRenderResolution(int &w, int &h) const;
  bool evaluateDlss(void *context, const void *dlss_params, int view_index);

#if HAS_NVSDK_NGX
protected:
  using NVSDK_NGX_Parameter_Ptr = eastl::unique_ptr<NVSDK_NGX_Parameter, eastl::function<void(NVSDK_NGX_Parameter *)>>;
  using NVSDK_NGX_Parameter_Result = eastl::tuple<NVSDK_NGX_Parameter_Ptr, NVSDK_NGX_Result>;

  virtual NVSDK_NGX_Result ngxInit(int app_id, const wchar_t *log_dir, void *device, const NVSDK_NGX_FeatureCommonInfo *info) = 0;
  virtual NVSDK_NGX_Result ngxShutdown(void *device) = 0;
  virtual NVSDK_NGX_Parameter_Result ngxGetParameters(); // deprecated
  virtual NVSDK_NGX_Parameter_Result ngxGetCapabilityParameters() = 0;
  virtual NVSDK_NGX_Parameter_Result ngxAllocateParameters() = 0;
  virtual NVSDK_NGX_Result ngxDestroyParameters(NVSDK_NGX_Parameter *param) = 0;
  virtual NVSDK_NGX_Result ngxCreateDlssFeature(void *context, NVSDK_NGX_Handle **out_handle, NVSDK_NGX_Parameter *in_params,
    NVSDK_NGX_DLSS_Create_Params *dlss_create_params, uint32_t creation_node_mask, uint32_t visibility_node_mask) = 0;
  virtual NVSDK_NGX_Result ngxReleaseDlssFeature(NVSDK_NGX_Handle *feature_handle) = 0;
  virtual NVSDK_NGX_Result ngxEvaluateDlss(void *context, NVSDK_NGX_Handle *feature_handle, NVSDK_NGX_Parameter *in_params,
    const void *dlss_params, const NVSDK_NGX_Dimensions &render_dimensions) = 0;

private:
  NVSDK_NGX_Parameter_Ptr capabilityParams = {nullptr, [](auto) {}};
  NVSDK_NGX_Handle *dlssFeatures[2] = {nullptr, nullptr};
  DlssState dlssState = DlssState::NOT_CHECKED;
  uint32_t renderResolutionW = 0, renderResolutionH = 0;
#endif
};

#endif