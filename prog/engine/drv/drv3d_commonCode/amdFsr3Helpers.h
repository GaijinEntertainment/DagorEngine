// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "amdFsr.h"

#include <osApiWrappers/dag_unicode.h>
#include <ffx_api/ffx_upscale.hpp>

namespace amd
{

inline const char *get_error_string(ffxReturnCode_t retCode)
{
  switch (retCode)
  {
    case FFX_API_RETURN_OK: return "FFX_API_RETURN_OK - The oparation was successful.";
    case FFX_API_RETURN_ERROR: return "FFX_API_RETURN_ERROR - An error occurred that is not further specified.";
    case FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE:
      return "FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE - The structure type given was not recognized for the function or context with "
             "which it was used. This is likely a programming error.";
    case FFX_API_RETURN_ERROR_RUNTIME_ERROR:
      return "FFX_API_RETURN_ERROR_RUNTIME_ERROR - The underlying runtime (e.g. D3D12, Vulkan) or effect returned an error code.";
    case FFX_API_RETURN_NO_PROVIDER:
      return "FFX_API_RETURN_NO_PROVIDER - No provider was found for the given structure type. This is likely a programming error.";
    case FFX_API_RETURN_ERROR_MEMORY: return "FFX_API_RETURN_ERROR_MEMORY - A memory allocation failed.";
    case FFX_API_RETURN_ERROR_PARAMETER:
      return "FFX_API_RETURN_ERROR_PARAMETER - A parameter was invalid, e.g. a null pointer, empty resource or out-of-bounds enum "
             "value.";
  }

  return "";
}

inline uint32_t convert(FSR::UpscalingMode mode)
{
  switch (mode)
  {
    case FSR::UpscalingMode::NativeAA: return FFX_UPSCALE_QUALITY_MODE_NATIVEAA;
    case FSR::UpscalingMode::Quality: return FFX_UPSCALE_QUALITY_MODE_QUALITY;
    case FSR::UpscalingMode::Balanced: return FFX_UPSCALE_QUALITY_MODE_BALANCED;
    case FSR::UpscalingMode::Performance: return FFX_UPSCALE_QUALITY_MODE_PERFORMANCE;
    case FSR::UpscalingMode::UltraPerformance: return FFX_UPSCALE_QUALITY_MODE_ULTRA_PERFORMANCE;
    default: G_ASSERT(false); return -1;
  }
}

inline ffx::CreateContextDescUpscale convert(const FSR::ContextArgs &args, ffxApiHeader &next)
{
  ffx::CreateContextDescUpscale desc{};
  desc.header.pNext = &next;
  desc.maxUpscaleSize = {args.outputWidth, args.outputHeight};
  desc.maxRenderSize = {args.outputWidth, args.outputHeight};
  desc.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;
  if (args.invertedDepth)
    desc.flags |= FFX_UPSCALE_ENABLE_DEPTH_INVERTED;
  if (args.enableHdr)
    desc.flags |= FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
  if (args.enableDebug)
  {
    desc.flags |= FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
    desc.fpMessage = [](uint32_t type, const wchar_t *wideMessage) {
      char narrowMessage[1024];
      wcs_to_utf8(wideMessage, narrowMessage, sizeof(narrowMessage));
      if (type == FFX_API_MESSAGE_TYPE_ERROR)
        logerr("[AMDFSR][ERROR]: %s", narrowMessage);
      else
        logwarn("[AMDFSR][WARNING]: %s", narrowMessage);
    };
  }

  return desc;
}

inline Point2 get_next_jitter(uint32_t render_width, uint32_t output_width, uint32_t &jitter_index, ffxContext context,
  PfnFfxQuery query)
{
  ++jitter_index;

  Point2 jitter;

  ffx::QueryDescUpscaleGetJitterOffset jitterDesc{};
  jitterDesc.index = jitter_index;
  jitterDesc.pOutX = &jitter.x;
  jitterDesc.pOutY = &jitter.y;

  ffx::QueryDescUpscaleGetJitterPhaseCount phaseCountDesc{};
  phaseCountDesc.renderWidth = render_width;
  phaseCountDesc.displayWidth = output_width;
  phaseCountDesc.pOutPhaseCount = &jitterDesc.phaseCount;
  query(&context, &phaseCountDesc.header);

  query(&context, &jitterDesc.header);

  return jitter;
}

template <typename TFN>
inline bool apply_upscaling(const FSR::UpscalingPlatformArgs &args, void *command_list, const ffxContext context,
  const PfnFfxDispatch dispatch, TFN tfn)
{
  G_ASSERT(context);

  ffx::DispatchDescUpscale desc{};

  desc.commandList = command_list;
  desc.color = tfn(args.colorTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.depth = tfn(args.depthTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.motionVectors = tfn(args.motionVectors, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.exposure = tfn(args.exposureTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.output = tfn(args.outputTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.reactive = tfn(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.transparencyAndComposition = tfn(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.jitterOffset.x = args.jitter.x;
  desc.jitterOffset.y = args.jitter.y;
  desc.motionVectorScale.x = args.motionVectorScale.x;
  desc.motionVectorScale.y = args.motionVectorScale.y;
  desc.reset = args.reset;
  desc.enableSharpening = args.sharpness > 0;
  desc.sharpness = args.sharpness;
  desc.frameTimeDelta = args.timeElapsed * 1000.0f; // FSR expects milliseconds
  desc.preExposure = args.preExposure;
  desc.renderSize.width = args.inputResolution.x;
  desc.renderSize.height = args.inputResolution.y;
  desc.upscaleSize.width = args.outputResolution.x;
  desc.upscaleSize.height = args.outputResolution.y;
  desc.cameraFovAngleVertical = args.fovY;
  desc.cameraFar = args.farPlane;
  desc.cameraNear = args.nearPlane;
  desc.flags = args.debugView ? FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW : 0;

  dispatch(const_cast<ffxContext *>(&context), &desc.header);

  return true;
}

inline IPoint2 get_rendering_resolution(FSR::UpscalingMode mode, const IPoint2 &target_resolution, ffxContext context,
  PfnFfxQuery query)
{
  IPoint2 result;

  ffx::QueryDescUpscaleGetRenderResolutionFromQualityMode desc{};
  desc.displayWidth = target_resolution.x;
  desc.displayHeight = target_resolution.y;
  desc.qualityMode = convert(mode);
  desc.pOutRenderWidth = (uint32_t *)&result.x;
  desc.pOutRenderHeight = (uint32_t *)&result.y;
  query(const_cast<ffxContext *>(&context), &desc.header);

  return result;
}

} // namespace amd
