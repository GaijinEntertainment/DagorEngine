// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dlss.h"

#include "wrapped_command_buffer.h"
#include "globals.h"
#include "backend.h"
#include "device_context.h"

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_vk.h>
#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_helpers_vk.h>

/* On Linux, need to link against
 * libsdk_nvngx.a
 * libstdc++.so
 * libdl.so
 */

namespace drv3d_vulkan
{

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

DLSSSuperResolutionDirect::DLSSSuperResolutionDirect() {}

DLSSSuperResolutionDirect::~DLSSSuperResolutionDirect() {}

bool DLSSSuperResolutionDirect::Initialize(VkInstance vk_instance, VkPhysicalDevice vk_phys, VkDevice vk_device)
{
  G_ASSERT(!ngxInitialized);

  discoveryInfo.Identifier.v.ApplicationId = ::dgs_get_settings()->getInt("nvidia_app_id", 0);

  auto onFailure = [&]() {
    Teardown(vk_device);
    return false;
  };

  isSupported = false;

  // Need to specify log folder here?
  CHECK_NVAPI(NVSDK_NGX_VULKAN_Init(discoveryInfo.Identifier.v.ApplicationId, L".", vk_instance, vk_phys, vk_device, nullptr, nullptr,
                &commonInfo),
    return onFailure());
  ngxInitialized = true;

  CHECK_NVAPI(NVSDK_NGX_VULKAN_GetCapabilityParameters(&ngxParams), return onFailure());
  CHECK_NVAPI(NVSDK_NGX_VULKAN_GetCapabilityParameters(&ngxParamsForSettings), return onFailure());

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
        logerr("NVIDIA DLSS cannot be loaded due to outdated driver. Minimum Driver Version required : %u.%u", minDriverVersionMajor,
          minDriverVersionMinor);
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
  CHECK_NVAPI(NVSDK_NGX_VULKAN_GetFeatureRequirements(vk_instance, vk_phys, &discoveryInfo, &req), return onFailure());

  if (req.FeatureSupported != NVSDK_NGX_FeatureSupportResult_Supported)
  {
    logdbg("DLSS is not supported on the current system.");
    return onFailure();
  }

  /* This code can be used to search for updated DLSS presets.
    CHECK_NVAPI(NVSDK_NGX_UpdateFeature(appId, NVSDK_NGX_Feature_SuperSampling), );
  */

  logdbg("DLSS is supported on the current system.");

  isSupported = true;
  return true;
}

void DLSSSuperResolutionDirect::Teardown(VkDevice vk_device)
{
  if (dlssFeature)
  {
    NVSDK_NGX_VULKAN_ReleaseFeature(dlssFeature);
    dlssFeature = nullptr;
  }
  if (ngxParams)
  {
    NVSDK_NGX_VULKAN_DestroyParameters(ngxParams);
    ngxParams = nullptr;
  }
  if (ngxParamsForSettings)
  {
    NVSDK_NGX_VULKAN_DestroyParameters(ngxParamsForSettings);
    ngxParamsForSettings = nullptr;
  }
  if (ngxInitialized)
  {
    NVSDK_NGX_VULKAN_Shutdown1(vk_device);
    ngxInitialized = false;
  }
  isSupported = false;
}

static eastl::optional<NVSDK_NGX_Resource_VK> texture_to_resource(void *texture, bool out = false)
{
  if (!texture)
    return eastl::nullopt;

  auto &[image, info, view] = *(eastl::tuple<VkImage, VkImageCreateInfo, VkImageView> *)(texture);
  if (!image)
    return eastl::nullopt;

  VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  return NVSDK_NGX_Create_ImageView_Resource_VK(view, image, subresourceRange, info.format, info.extent.width, info.extent.height,
    out);
}

bool DLSSSuperResolutionDirect::evaluate(const nv::DlssParams<void> &params, void *command_buffer)
{
  G_ASSERT_RETURN(dlssFeature, false);

  VkCommandBuffer vk_cmd = (VkCommandBuffer)command_buffer;

  auto &[image, info, view] = *(eastl::tuple<VkImage, VkImageCreateInfo, VkImageView> *)(params.inColor);
  if (!image)
    return false;

  auto unresolvedColorResource = texture_to_resource(params.inColor);
  auto resolvedColorResource = texture_to_resource(params.outColor, true);
  auto motionVectorsResource = texture_to_resource(params.inMotionVectors);
  auto depthResource = texture_to_resource(params.inDepth);
  auto exposureResource = texture_to_resource(params.inExposure);

  NVSDK_NGX_VK_DLSS_Eval_Params evalParams = {
    .Feature{
      .pInColor = unresolvedColorResource ? &unresolvedColorResource.value() : nullptr,
      .pInOutput = resolvedColorResource ? &resolvedColorResource.value() : nullptr,
    },
    .pInDepth = depthResource ? &depthResource.value() : nullptr,
    .pInMotionVectors = motionVectorsResource ? &motionVectorsResource.value() : nullptr,
    .InJitterOffsetX = params.inJitterOffsetX,
    .InJitterOffsetY = params.inJitterOffsetY,
    .InRenderSubrectDimensions{
      .Width = info.extent.width,
      .Height = info.extent.height,
    },
    .InReset = 0, // We need to support this eventually.
    .InMVScaleX = params.inMVScaleX * info.extent.width,
    .InMVScaleY = params.inMVScaleY * info.extent.height,
    .pInExposureTexture = exposureResource ? &exposureResource.value() : nullptr,
  };

  CHECK_NVAPI(NGX_VULKAN_EVALUATE_DLSS_EXT(vk_cmd, dlssFeature, ngxParams, &evalParams), return false);

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
    .rayReconstruction = false,
  };
}

bool DLSSSuperResolutionDirect::setOptions(Mode mode, IPoint2 output_resolution)
{
  if (!isSupported)
    return false;

  Globals::ctx.initializeDLSS(int(mode), output_resolution.x, output_resolution.y);
  return true;
}

void DLSSSuperResolutionDirect::DeleteFeature()
{
  if (dlssFeature)
  {
    NVSDK_NGX_VULKAN_ReleaseFeature(dlssFeature);
    dlssFeature = nullptr;
  }
}

bool DLSSSuperResolutionDirect::setOptionsBackend(VkCommandBuffer vk_cmd, Mode mode, IPoint2 output_resolution)
{
  DeleteFeature();

  auto optimalSettings = getOptimalSettings(mode, output_resolution, ngxParams);
  G_ASSERT_RETURN(optimalSettings, false);

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

  CHECK_NVAPI(NGX_VULKAN_CREATE_DLSS_EXT(vk_cmd, 1, 1, &dlssFeature, ngxParams, &dlssCreateParams), return false);

  return true;
}

nv::DLSS::State DLSSSuperResolutionDirect::getState()
{
  return isSupported ? nv::DLSS::State::SUPPORTED : nv::DLSS::State::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE;
}

dag::Expected<eastl::string, nv::SupportState> DLSSSuperResolutionDirect::getVersion() const { return "310.3.0"; }

const eastl::vector<eastl::string> &DLSSSuperResolutionDirect::getRequiredDeviceExtensions(VkInstance vk_instance,
  VkPhysicalDevice vk_phys) const
{
  requiredDeviceExtensions.clear();

  uint32_t reqDeviceExtensionCount = 0;
  VkExtensionProperties *deviceExtensions = nullptr;
  NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(vk_instance, vk_phys, &discoveryInfo, &reqDeviceExtensionCount,
    &deviceExtensions);

  for (int i = 0; i < reqDeviceExtensionCount; ++i)
    requiredDeviceExtensions.emplace_back(deviceExtensions[i].extensionName);

  return requiredDeviceExtensions;
}

const eastl::vector<eastl::string> &DLSSSuperResolutionDirect::getRequiredInstanceExtensions() const
{
  requiredInstanceExtensions.clear();

  uint32_t reqInstanceExtensionCount = 0;
  VkExtensionProperties *instanceExtensions = nullptr;
  NVSDK_NGX_VULKAN_GetFeatureInstanceExtensionRequirements(&discoveryInfo, &reqInstanceExtensionCount, &instanceExtensions);

  for (int i = 0; i < reqInstanceExtensionCount; ++i)
    requiredInstanceExtensions.emplace_back(instanceExtensions[i].extensionName);

  return requiredInstanceExtensions;
}

} // namespace drv3d_vulkan
