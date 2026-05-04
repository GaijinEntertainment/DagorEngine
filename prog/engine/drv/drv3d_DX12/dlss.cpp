// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dlss.h"

#include <d3d12.h>
#include <drv_log_defs.h>

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_helpers_dlssd.h>

namespace drv3d_dx12
{

void initialize_dlss_on_backend(int mode, int output_width, int output_height, bool use_rr, bool use_legacy_model);

#define CHECK_NVAPI(x, on_fail)                                              \
  do                                                                         \
  {                                                                          \
    auto result = (x);                                                       \
    if (result != NVSDK_NGX_Result_Success)                                  \
    {                                                                        \
      logdbg("NVAPI fail with %s on: %s", GetNGXResultAsString(result), #x); \
      on_fail;                                                               \
    }                                                                        \
  } while (false)

static NVSDK_NGX_PerfQuality_Value convert_to_dlss(nv::DLSS::Mode q)
{
  switch (q)
  {
    case nv::DLSS::Mode::DLAA: return NVSDK_NGX_PerfQuality_Value_DLAA;
    case nv::DLSS::Mode::UltraQuality: return NVSDK_NGX_PerfQuality_Value_UltraQuality;
    case nv::DLSS::Mode::MaxQuality: return NVSDK_NGX_PerfQuality_Value_MaxQuality;
    case nv::DLSS::Mode::Balanced: return NVSDK_NGX_PerfQuality_Value_Balanced;
    case nv::DLSS::Mode::MaxPerformance: return NVSDK_NGX_PerfQuality_Value_MaxPerf;
    case nv::DLSS::Mode::UltraPerformance: return NVSDK_NGX_PerfQuality_Value_UltraPerformance;
    default: G_ASSERT(false); return NVSDK_NGX_PerfQuality_Value_DLAA;
  }
}

static const NVSDK_NGX_FeatureCommonInfo commonInfo = {.PathListInfo{
                                                         .Path = nullptr,
                                                         .Length = 0,
                                                       },
  .InternalData = nullptr,
  .LoggingInfo{
    .LoggingCallback =
      [](const char *message, NVSDK_NGX_Logging_Level /*loggingLevel*/, NVSDK_NGX_Feature sourceComponent) {
        logdbg("NGX message from feature %d: %s", int(sourceComponent), message);
      },
    .MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_VERBOSE,
    .DisableOtherLoggingSinks = true,
  }};

static NVSDK_NGX_FeatureDiscoveryInfo discoveryInfo = {
  .SDKVersion = NVSDK_NGX_Version_API,
  .FeatureID = NVSDK_NGX_Feature_SuperSampling,
  .Identifier{
    .IdentifierType = NVSDK_NGX_Application_Identifier_Type_Application_Id,
  },
  .ApplicationDataPath = L"",
  .FeatureInfo = &commonInfo,
};

static NVSDK_NGX_FeatureDiscoveryInfo discoveryInfoRR = {
  .SDKVersion = NVSDK_NGX_Version_API,
  .FeatureID = NVSDK_NGX_Feature_RayReconstruction,
  .Identifier{
    .IdentifierType = NVSDK_NGX_Application_Identifier_Type_Application_Id,
  },
  .ApplicationDataPath = L"",
  .FeatureInfo = &commonInfo,
};

DLSSSuperResolutionDirect::DLSSSuperResolutionDirect() {}

DLSSSuperResolutionDirect::~DLSSSuperResolutionDirect() {}

bool DLSSSuperResolutionDirect::Initialize(ID3D12Device *d3d_device, IDXGIAdapter *dxgi_adapter)
{
  G_ASSERT(!ngxInitialized);

  discoveryInfo.Identifier.v.ApplicationId = ::dgs_get_settings()->getInt("nvidia_app_id", 0);
  discoveryInfoRR.Identifier.v.ApplicationId = ::dgs_get_settings()->getInt("nvidia_app_id", 0);

  auto onFailure = [&]() {
    Teardown(d3d_device);
    return false;
  };

  isSupported = false;
  isRRSupported = false;

  // Need to specify log folder here?
  CHECK_NVAPI(NVSDK_NGX_D3D12_Init(discoveryInfo.Identifier.v.ApplicationId, L".", d3d_device, &commonInfo), return onFailure());
  ngxInitialized = true;

  CHECK_NVAPI(NVSDK_NGX_D3D12_GetCapabilityParameters(&ngxParams), return onFailure());
  CHECK_NVAPI(NVSDK_NGX_D3D12_GetCapabilityParameters(&ngxParamsForSettings), return onFailure());

  {
    int needsUpdatedDriver = 0;
    uint32_t minDriverVersionMajor = 0;
    uint32_t minDriverVersionMinor = 0;
    auto ResultUpdatedDriver = ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);
    auto ResultMinDriverVersionMajor = ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minDriverVersionMajor);
    auto ResultMinDriverVersionMinor = ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &minDriverVersionMinor);
    if (ResultUpdatedDriver == NVSDK_NGX_Result_Success && ResultMinDriverVersionMajor == NVSDK_NGX_Result_Success &&
        ResultMinDriverVersionMinor == NVSDK_NGX_Result_Success)
    {
      if (needsUpdatedDriver)
      {
        D3D_ERROR("NVIDIA DLSS cannot be loaded due to outdated driver. Minimum Driver Version required : %u.%u",
          minDriverVersionMajor, minDriverVersionMinor);
        return onFailure();
      }
      else
        logdbg("NVIDIA DLSS Minimum driver version was reported as : %u.%u", minDriverVersionMajor, minDriverVersionMinor);
    }
    else
      logdbg("NVIDIA DLSS Minimum driver version was not reported.");
  }

  int dlssAvailable = 0;
  auto resultDLSS = ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssAvailable);
  if (resultDLSS != NVSDK_NGX_Result_Success || !dlssAvailable)
  {
    auto featureInitResult = NVSDK_NGX_Result_Fail;
    NVSDK_NGX_Parameter_GetI(ngxParams, NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult, (int *)&featureInitResult);
    logdbg("NVIDIA DLSS not available on this hardward/platform., FeatureInitResult = 0x%08x, info: %ls", featureInitResult,
      GetNGXResultAsString(featureInitResult));
    return onFailure();
  }

  NVSDK_NGX_FeatureRequirement req = {};
  NVSDK_NGX_FeatureRequirement reqRR = {};
  CHECK_NVAPI(NVSDK_NGX_D3D12_GetFeatureRequirements(dxgi_adapter, &discoveryInfo, &req), return onFailure());
  CHECK_NVAPI(NVSDK_NGX_D3D12_GetFeatureRequirements(dxgi_adapter, &discoveryInfoRR, &reqRR), return onFailure());

  if (req.FeatureSupported != NVSDK_NGX_FeatureSupportResult_Supported)
  {
    logdbg("DLSS is not supported on the current system.");
    return onFailure();
  }

  logdbg("DLSS is supported on the current system.");

  if (reqRR.FeatureSupported != NVSDK_NGX_FeatureSupportResult_Supported)
  {
    logdbg("DLSS-RR is not supported on the current system.");
    isRRSupported = false;
  }
  else
  {
    logdbg("DLSS-RR is supported on the current system.");
    isRRSupported = true;
  }

  /* This code can be used to search for updated DLSS presets.
    CHECK_NVAPI(NVSDK_NGX_UpdateFeature(appId, NVSDK_NGX_Feature_SuperSampling), );
  */

  isSupported = true;
  return true;
}

void DLSSSuperResolutionDirect::Teardown(ID3D12Device *d3d_device)
{
  if (dlssFeature)
  {
    NVSDK_NGX_D3D12_ReleaseFeature(dlssFeature);
    dlssFeature = nullptr;
  }
  if (ngxParams)
  {
    NVSDK_NGX_D3D12_DestroyParameters(ngxParams);
    ngxParams = nullptr;
  }
  if (ngxParamsForSettings)
  {
    NVSDK_NGX_D3D12_DestroyParameters(ngxParamsForSettings);
    ngxParamsForSettings = nullptr;
  }
  if (ngxInitialized)
  {
    NVSDK_NGX_D3D12_Shutdown1(d3d_device);
    ngxInitialized = false;
  }
  isSupported = false;
}

static ID3D12Resource *texture_to_resource(void *texture) { return (ID3D12Resource *)texture; }

bool DLSSSuperResolutionDirect::evaluate(const nv::DlssParams<void> &params, void *command_buffer)
{
  G_ASSERT_RETURN(dlssFeature, false);

  ID3D12GraphicsCommandList *d3dCommandList = (ID3D12GraphicsCommandList *)command_buffer;

  if (!params.inColor)
    return false;

  auto inColorDesc = texture_to_resource(params.inColor)->GetDesc();

  if (useRR)
  {
    NVSDK_NGX_D3D12_DLSSD_Eval_Params evalParams = {
      .pInDiffuseAlbedo = texture_to_resource(params.inAlbedo),
      .pInSpecularAlbedo = texture_to_resource(params.inSpecularAlbedo),
      .pInNormals = texture_to_resource(params.inNormalRoughness),
      .pInColor = texture_to_resource(params.inColor),
      .pInOutput = texture_to_resource(params.outColor),
      .pInDepth = texture_to_resource(params.inDepth),
      .pInMotionVectors = texture_to_resource(params.inMotionVectors),
      .InJitterOffsetX = params.inJitterOffsetX,
      .InJitterOffsetY = params.inJitterOffsetY,
      .InRenderSubrectDimensions{
        .Width = (UINT)inColorDesc.Width,
        .Height = inColorDesc.Height,
      },
      .InReset = 0, // We need to support this eventually.
      .InMVScaleX = params.inMVScaleX * (UINT)inColorDesc.Width,
      .InMVScaleY = params.inMVScaleY * inColorDesc.Height,
      .pInExposureTexture = texture_to_resource(params.inExposure),
      .pInScreenSpaceSubsurfaceScatteringGuide = texture_to_resource(params.inSsssGuide),
      .pInSpecularHitDistance = texture_to_resource(params.inHitDist),
    };

    CHECK_NVAPI(NGX_D3D12_EVALUATE_DLSSD_EXT(d3dCommandList, dlssFeature, ngxParams, &evalParams), return false);
  }
  else
  {
    NVSDK_NGX_D3D12_DLSS_Eval_Params evalParams = {
      .Feature{
        .pInColor = texture_to_resource(params.inColor),
        .pInOutput = texture_to_resource(params.outColor),
      },
      .pInDepth = texture_to_resource(params.inDepth),
      .pInMotionVectors = texture_to_resource(params.inMotionVectors),
      .InJitterOffsetX = params.inJitterOffsetX,
      .InJitterOffsetY = params.inJitterOffsetY,
      .InRenderSubrectDimensions{
        .Width = (UINT)inColorDesc.Width,
        .Height = inColorDesc.Height,
      },
      .InReset = 0, // We need to support this eventually.
      .InMVScaleX = params.inMVScaleX * (UINT)inColorDesc.Width,
      .InMVScaleY = params.inMVScaleY * inColorDesc.Height,
      .pInExposureTexture = texture_to_resource(params.inExposure),
    };

    CHECK_NVAPI(NGX_D3D12_EVALUATE_DLSS_EXT(d3dCommandList, dlssFeature, ngxParams, &evalParams), return false);
  }

  return true;
}

eastl::optional<nv::DLSS::OptimalSettings> DLSSSuperResolutionDirect::getOptimalSettings(Mode mode, IPoint2 output_resolution) const
{
  return getOptimalSettings(mode, output_resolution, ngxParamsForSettings);
}

eastl::optional<nv::DLSS::OptimalSettings> DLSSSuperResolutionDirect::getOptimalSettings(Mode mode, IPoint2 output_resolution,
  NVSDK_NGX_Parameter *params) const
{
  uint32_t renderOptimalWidth = 0;
  uint32_t renderOptimalHeight = 0;
  uint32_t renderMaxWidth = 0;
  uint32_t renderMaxHeight = 0;
  uint32_t renderMinWidth = 0;
  uint32_t renderMinHeight = 0;
  float sharpness = 0;

  CHECK_NVAPI(
    NGX_DLSS_GET_OPTIMAL_SETTINGS(params, output_resolution.x, output_resolution.y, convert_to_dlss(mode), &renderOptimalWidth,
      &renderOptimalHeight, &renderMaxWidth, &renderMaxHeight, &renderMinWidth, &renderMinHeight, &sharpness),
    return eastl::nullopt);

  return OptimalSettings{
    .renderWidth = renderOptimalWidth,
    .renderHeight = renderOptimalHeight,
    .renderMinWidth = renderMinWidth,
    .renderMinHeight = renderMinHeight,
    .renderMaxWidth = renderMaxWidth,
    .renderMaxHeight = renderMaxHeight,
    .rayReconstruction = false,
  };
}

bool DLSSSuperResolutionDirect::setOptions(Mode mode, IPoint2 output_resolution, bool use_rr, bool use_legacy_model)
{
  if (!isSupported)
    return false;

  initialize_dlss_on_backend(int(mode), output_resolution.x, output_resolution.y, use_rr, use_legacy_model);

  return true;
}

void DLSSSuperResolutionDirect::DeleteFeature()
{
  if (dlssFeature)
  {
    NVSDK_NGX_D3D12_ReleaseFeature(dlssFeature);
    dlssFeature = nullptr;
  }
}

static void set_preset(NVSDK_NGX_Parameter *params, uint32_t preset)
{
  NVSDK_NGX_Parameter_SetUI(params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, preset);
  NVSDK_NGX_Parameter_SetUI(params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, preset);
  NVSDK_NGX_Parameter_SetUI(params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, preset);
  NVSDK_NGX_Parameter_SetUI(params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, preset);
  NVSDK_NGX_Parameter_SetUI(params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, preset);
  NVSDK_NGX_Parameter_SetUI(params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality, preset);
}

bool DLSSSuperResolutionDirect::setOptionsBackend(ID3D12GraphicsCommandList *d3d_command_list, Mode mode, IPoint2 output_resolution,
  bool use_rr, bool use_legacy_model)
{
  DeleteFeature();

  auto optimalSettings = getOptimalSettings(mode, output_resolution, ngxParams);
  G_ASSERT_RETURN(optimalSettings, false);

  if (use_rr)
  {
    NVSDK_NGX_DLSSD_Create_Params dlssCreateParams = {
      .InDenoiseMode = NVSDK_NGX_DLSS_Denoise_Mode_DLUnified,
      .InRoughnessMode = NVSDK_NGX_DLSS_Roughness_Mode_Packed,
      .InUseHWDepth = NVSDK_NGX_DLSS_Depth_Type_HW,
      .InWidth = optimalSettings->renderWidth,
      .InHeight = optimalSettings->renderHeight,
      .InTargetWidth = (uint32_t)output_resolution.x,
      .InTargetHeight = (uint32_t)output_resolution.y,
      .InPerfQualityValue = convert_to_dlss(mode),
      .InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_AutoExposure | NVSDK_NGX_DLSS_Feature_Flags_DepthInverted |
                              NVSDK_NGX_DLSS_Feature_Flags_MVLowRes | NVSDK_NGX_DLSS_Feature_Flags_IsHDR,
    };

    CHECK_NVAPI(NGX_D3D12_CREATE_DLSSD_EXT(d3d_command_list, 1, 1, &dlssFeature, ngxParams, &dlssCreateParams), return false);
  }
  else
  {
    NVSDK_NGX_DLSS_Create_Params dlssCreateParams = {
      .Feature{
        .InWidth = optimalSettings->renderWidth,
        .InHeight = optimalSettings->renderHeight,
        .InTargetWidth = (uint32_t)output_resolution.x,
        .InTargetHeight = (uint32_t)output_resolution.y,
        .InPerfQualityValue = convert_to_dlss(mode),
      },
      .InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_AutoExposure | NVSDK_NGX_DLSS_Feature_Flags_DepthInverted | NVSDK_NGX_DLSS_Feature_Flags_MVLowRes,
    };

    set_preset(ngxParams, use_legacy_model ? NVSDK_NGX_DLSS_Hint_Render_Preset_K : NVSDK_NGX_DLSS_Hint_Render_Preset_Default);

    CHECK_NVAPI(NGX_D3D12_CREATE_DLSS_EXT(d3d_command_list, 1, 1, &dlssFeature, ngxParams, &dlssCreateParams), return false);
  }

  useRR = use_rr;

  return true;
}

nv::DLSS::State DLSSSuperResolutionDirect::getState()
{
  return isSupported ? nv::DLSS::State::SUPPORTED : nv::DLSS::State::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE;
}

bool DLSSSuperResolutionDirect::supportRayReconstruction() { return isRRSupported; }

uint64_t DLSSSuperResolutionDirect::getMemoryUsage()
{
  unsigned long long sizeInBytes = 0;
  if (ngxInitialized && ngxParamsForSettings)
  {
    CHECK_NVAPI(NGX_DLSS_GET_STATS(ngxParamsForSettings, &sizeInBytes), return 0);
  }
  return sizeInBytes;
}

dag::Expected<eastl::string, nv::SupportState> DLSSSuperResolutionDirect::getVersion() const { return "310.3.0"; }

} // namespace drv3d_dx12
