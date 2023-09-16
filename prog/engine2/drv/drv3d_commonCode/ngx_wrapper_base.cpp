#include "ngx_wrapper_base.h"

#include <EASTL/algorithm.h>
#include <3d/dag_drv3d.h>
#include <osApiWrappers/dag_unicode.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

// for getcwd()
#if _TARGET_PC_WIN
#include <direct.h>
#elif defined(__GNUC__)
#include <unistd.h>
#else
G_STATIC_ASSERT(false && "Make getcwd available for this platform!");
#endif
#include <stdio.h> // for _snprintf

bool NgxWrapperBase::init(void *device, const char *log_dir)
{
  G_ASSERT_RETURN(device != nullptr, false);

  // NVIDIA Application IDs:
  // Enlisted:         100200011
  // Cuisine Royale:   100223411
  // War Thunder:      10839111
  // DLSS Sample app:  231313132
  int appId = ::dgs_get_settings()->getInt("nvidia_app_id", 0);
  if (appId == 0)
  {
    dlssState = DlssState::NGX_INIT_ERROR_NO_APP_ID;
    return false;
  }

  wchar_t logDirW[DAGOR_MAX_PATH];
  utf8_to_wcs(log_dir, logDirW, DAGOR_MAX_PATH);

  NVSDK_NGX_FeatureCommonInfo info{};
  const char *dllFolder = ::dgs_get_settings()->getStr("nvidia_dlss_dll_folder", nullptr);
  wchar_t dllFolderAbsW[DAGOR_MAX_PATH];
  wchar_t *dllFolderAbsWPtr = dllFolderAbsW;
  if (dllFolder != nullptr && strlen(dllFolder) > 0)
  {
    char dllFolderAbs[DAGOR_MAX_PATH];
    getcwd(dllFolderAbs, DAGOR_MAX_PATH);
    eastl::replace(eastl::begin(dllFolderAbs), eastl::end(dllFolderAbs), '\\', '/');
    int sb = i_strlen(dllFolderAbs);
    _snprintf(dllFolderAbs + sb, DAGOR_MAX_PATH - sb, "/%s", dllFolder);
    utf8_to_wcs(dllFolderAbs, dllFolderAbsW, DAGOR_MAX_PATH);
    info.PathListInfo.Path = &dllFolderAbsWPtr;
    info.PathListInfo.Length = 1;
  }

  NVSDK_NGX_Result result = ngxInit(appId, logDirW, device, &info);
  debug("NGX: ngxInit result: %ls", GetNGXResultAsString(result));
  if (NVSDK_NGX_FAILED(result))
  {
    if (result == NVSDK_NGX_Result_FAIL_FeatureNotSupported)
      dlssState = DlssState::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE;
    else if (result == NVSDK_NGX_Result_FAIL_OutOfDate)
      dlssState = DlssState::NOT_SUPPORTED_OUTDATED_VGA_DRIVER;
    else
    {
      dlssState = DlssState::NGX_INIT_ERROR_UNKNOWN;
      logerr("Failed to initialize NGX with unexpected error: %ls", GetNGXResultAsString(result));
    }
    return false;
  }

  eastl::tie(capabilityParams, result) = ngxGetCapabilityParameters();
  if (result == NVSDK_NGX_Result_FAIL_OutOfDate)
  {
    eastl::tie(capabilityParams, result) = ngxGetParameters(); // use the deprecated version for older drivers
  }

  if (NVSDK_NGX_FAILED(result))
  {
    logerr("NGX: ngxGetParameters: Failed to get parameters: %ls", GetNGXResultAsString(result));
    dlssState = DlssState::NGX_INIT_ERROR_UNKNOWN;
    capabilityParams = NVSDK_NGX_Parameter_Ptr(nullptr, [](auto) {});
    return false;
  }

  return true;
}

bool NgxWrapperBase::shutdown(void *device)
{
  capabilityParams = NVSDK_NGX_Parameter_Ptr(nullptr, [](auto) {});
  releaseDlssFeature();
  NVSDK_NGX_Result result = ngxShutdown(device);
  debug("NGX: ngxShutdown result: %ls", GetNGXResultAsString(result));
  return NVSDK_NGX_SUCCEED(result);
}

bool NgxWrapperBase::checkDlssSupport()
{
  int needsUpdatedDriver = 0;
  uint32_t minDriverVersionMajor = 0;
  uint32_t minDriverVersionMinor = 0;
  NVSDK_NGX_Result resultUpdatedDriver =
    capabilityParams->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);
  NVSDK_NGX_Result resultMinDriverVersionMajor =
    capabilityParams->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minDriverVersionMajor);
  NVSDK_NGX_Result resultMinDriverVersionMinor =
    capabilityParams->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &minDriverVersionMinor);

  if (NVSDK_NGX_SUCCEED(resultUpdatedDriver) && NVSDK_NGX_SUCCEED(resultMinDriverVersionMajor) &&
      NVSDK_NGX_SUCCEED(resultMinDriverVersionMinor))
  {
    if (needsUpdatedDriver)
    {
      debug("NGX: NVIDIA DLSS cannot be loaded due to outdated driver. Min Driver Version required: %u.%u", minDriverVersionMajor,
        minDriverVersionMinor);
      dlssState = DlssState::NOT_SUPPORTED_OUTDATED_VGA_DRIVER;
      return false;
    }
    debug("NGX: NVIDIA DLSS Minimum driver version was reported as %u.%u. No driver update required.", minDriverVersionMajor,
      minDriverVersionMinor);
  }
  else
  {
    debug("NGX: NVIDIA DLSS Minimum driver version was not reported.\n"
          "  resultUpdatedDriver: %ls\n"
          "  resultMinDriverVersionMajor: %ls\n"
          "  resultMinDriverVersionMinor: %ls",
      GetNGXResultAsString(resultUpdatedDriver), GetNGXResultAsString(resultMinDriverVersionMajor),
      GetNGXResultAsString(resultMinDriverVersionMinor));
    dlssState = DlssState::NOT_SUPPORTED_OUTDATED_VGA_DRIVER;
    return false;
  }

  int dlssSupported = 0;
  NVSDK_NGX_Result resultDlssSupported = capabilityParams->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssSupported);
  if (NVSDK_NGX_FAILED(resultDlssSupported))
  {
    debug("NGX: Failed to get NVIDIA DLSS availability: %ls", GetNGXResultAsString(resultDlssSupported));
    dlssState = DlssState::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE;
    return false;
  }
  else if (!dlssSupported)
  {
    debug("NGX: NVIDIA DLSS not available on this hardward/platform.");
    dlssState = DlssState::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE;
    return false;
  }

  debug("NGX: NVIDIA DLSS is supported!");
  dlssState = DlssState::SUPPORTED;
  return true;
}

bool NgxWrapperBase::isDlssQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int dlss_quality) const
{
  G_ASSERT_RETURN(dlssState == DlssState::SUPPORTED || dlssState == DlssState::READY, false);

  uint32_t renderOptimalWidth = 0, renderOptimalHeight = 0;
  uint32_t renderMaxWidth = 0, renderMaxHeight = 0;
  uint32_t renderMinWidth = 0, renderMinHeight = 0;
  float sharpness = 0.0f;

  NVSDK_NGX_Result result =
    NGX_DLSS_GET_OPTIMAL_SETTINGS(capabilityParams.get(), target_width, target_height, (NVSDK_NGX_PerfQuality_Value)dlss_quality,
      &renderOptimalWidth, &renderOptimalHeight, &renderMaxWidth, &renderMaxHeight, &renderMinWidth, &renderMinHeight, &sharpness);

  // Ultra quality mode with version v2.1-rev1 returns with success, but with 0 optimal rendering width and height -.-
  return NVSDK_NGX_SUCCEED(result) && renderOptimalWidth != 0 && renderOptimalHeight != 0;
}

bool NgxWrapperBase::createOptimalDlssFeature(void *context, uint32_t target_width, uint32_t target_height, int dlss_quality,
  bool stereo_render, uint32_t creation_node_mask, uint32_t visibility_node_mask)
{
  G_ASSERT_RETURN(dlssState == DlssState::SUPPORTED || dlssState == DlssState::READY, false);
  G_ASSERT_RETURN(dlss_quality >= int(NVSDK_NGX_PerfQuality_Value_MaxPerf), false);
  G_ASSERT_RETURN(dlss_quality <= int(NVSDK_NGX_PerfQuality_Value_UltraQuality), false);

  const int origDlssQuality = dlss_quality;
  if (!isDlssQualityAvailableAtResolution(target_width, target_height, dlss_quality))
  {
    const int sortedQualityValues[] = {
      (int)NVSDK_NGX_PerfQuality_Value_UltraQuality,
      (int)NVSDK_NGX_PerfQuality_Value_MaxQuality,
      (int)NVSDK_NGX_PerfQuality_Value_Balanced,
      (int)NVSDK_NGX_PerfQuality_Value_MaxPerf,
      (int)NVSDK_NGX_PerfQuality_Value_UltraPerformance,
      -1,
    };
    dlss_quality = *eastl::find_if(eastl::begin(sortedQualityValues), eastl::end(sortedQualityValues),
      [&](int q) { return q == -1 || isDlssQualityAvailableAtResolution(target_width, target_height, q); });
    if (dlss_quality >= 0)
    {
      logerr("NGX: DLSS quality was changed from %d to %d based on availability at current resolution: %dx%d", origDlssQuality,
        dlss_quality, target_width, target_height);
    }
    else
    {
      logerr("NGX: Couldn't find any DLSS quality option available at this resolution: %dx%d", target_width, target_height);
      return false;
    }
  }

  releaseDlssFeature();

  uint32_t renderOptimalWidth = 0, renderOptimalHeight = 0;
  uint32_t renderMaxWidth = 0, renderMaxHeight = 0;
  uint32_t renderMinWidth = 0, renderMinHeight = 0;
  float sharpness = 0.0f;

  NVSDK_NGX_Result result =
    NGX_DLSS_GET_OPTIMAL_SETTINGS(capabilityParams.get(), target_width, target_height, (NVSDK_NGX_PerfQuality_Value)dlss_quality,
      &renderOptimalWidth, &renderOptimalHeight, &renderMaxWidth, &renderMaxHeight, &renderMinWidth, &renderMinHeight, &sharpness);

  // This shouldn't fail as we made sure this quality is available with this resoltution
  G_ASSERT_RETURN(NVSDK_NGX_SUCCEED(result), false);

  const int flags = NVSDK_NGX_DLSS_Feature_Flags_MVLowRes | NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
                    NVSDK_NGX_DLSS_Feature_Flags_DepthInverted | NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;

  NVSDK_NGX_DLSS_Create_Params dlssCreateParams = {};
  dlssCreateParams.Feature.InWidth = renderOptimalWidth;
  dlssCreateParams.Feature.InHeight = renderOptimalHeight;
  dlssCreateParams.Feature.InTargetWidth = target_width;
  dlssCreateParams.Feature.InTargetHeight = target_height;
  dlssCreateParams.Feature.InPerfQualityValue = (NVSDK_NGX_PerfQuality_Value)dlss_quality;
  dlssCreateParams.InFeatureCreateFlags = flags;

  for (int viewIx = 0; viewIx < (stereo_render ? 2 : 1); ++viewIx)
  {
    result = ngxCreateDlssFeature(context, &dlssFeatures[viewIx], capabilityParams.get(), &dlssCreateParams, creation_node_mask,
      visibility_node_mask);
    if (NVSDK_NGX_FAILED(result))
    {
      logerr("NGX: ngxCreateDlssFeature failed: %ls", GetNGXResultAsString(result));
      releaseDlssFeature();
      return false;
    }
  }

  renderResolutionW = renderOptimalWidth;
  renderResolutionH = renderOptimalHeight;

  debug("NGX: Optimal DLSS feature created:\n"
        "              Render resolution: %ux%u\n"
        "              Target resulution: %ux%u\n"
        "              Quality: %d",
    renderOptimalWidth, renderOptimalHeight, target_width, target_height, dlss_quality);

  dlssState = DlssState::READY;
  return true;
}

bool NgxWrapperBase::releaseDlssFeature()
{
  renderResolutionW = renderResolutionH = 0;
  if (dlssState == DlssState::READY)
    dlssState = DlssState::SUPPORTED;

  bool releasedAll = true;
  for (NVSDK_NGX_Handle *&dlssFeature : dlssFeatures)
  {
    if (!dlssFeature)
      continue;
    NVSDK_NGX_Result result = ngxReleaseDlssFeature(dlssFeature);
    if (NVSDK_NGX_FAILED(result))
      logerr("NGX: Failed to ngxReleaseDlssFeature: %ls", GetNGXResultAsString(result));
    dlssFeature = nullptr;
    releasedAll &= NVSDK_NGX_SUCCEED(result);
  }
  return releasedAll;
}

DlssState NgxWrapperBase::getDlssState() const { return dlssState; }

void NgxWrapperBase::getDlssRenderResolution(int &w, int &h) const
{
  if (dlssState == DlssState::READY)
  {
    w = renderResolutionW;
    h = renderResolutionH;
  }
  else
  {
    // for debug_use_dlss_path_without_dlss
    d3d::get_screen_size(w, h);
    w /= 2;
    h /= 2;
  }
}

bool NgxWrapperBase::evaluateDlss(void *context, const void *dlss_params, int view_index)
{
  G_ASSERT_RETURN(dlssState == DlssState::READY, false);
  G_ASSERT_RETURN(dlssFeatures[view_index] != nullptr, false);
  G_ASSERT_RETURN(renderResolutionW > 0 && renderResolutionH > 0, false);

  NVSDK_NGX_Dimensions render_dimensions{renderResolutionW, renderResolutionH};
  NVSDK_NGX_Result result = ngxEvaluateDlss(context, dlssFeatures[view_index], capabilityParams.get(), dlss_params, render_dimensions);
  if (NVSDK_NGX_FAILED(result))
    logerr("NGX: Failed to evaluate DLSS: %ls", GetNGXResultAsString(result));

  return NVSDK_NGX_SUCCEED(result);
}

NgxWrapperBase::NVSDK_NGX_Parameter_Result NgxWrapperBase::ngxGetParameters()
{
  return {nullptr, NVSDK_NGX_Result_FAIL_FeatureNotSupported};
}