// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "amdFsr.h"
#include "drv_log_defs.h"

#include <osApiWrappers/dag_unicode.h>
#if __has_include(<ffx_upscale.h>) // Vulkan is currently not supported in SDK 2.0
#include <ffx_upscale.h>           // AMD FidelityFX™ SDK 2.0.0
#else                              //
#include <ffx_api/ffx_upscale.hpp> // AMD FidelityFX™ SDK 1.1.4
#endif

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

inline ffxCreateContextDescUpscale convert(const FSR::ContextArgs &args, ffxApiHeader &next)
{
  ffxCreateContextDescUpscale desc{
    .header{
      .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE,
      .pNext = &next,
    },
    .flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE,
    .maxRenderSize{
      .width = args.outputWidth,
      .height = args.outputHeight,
    },
    .maxUpscaleSize{
      .width = args.outputWidth,
      .height = args.outputHeight,
    },
  };
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
        D3D_ERROR("[AMDFSR][ERROR]: %s", narrowMessage);
      else
        logwarn("[AMDFSR][WARNING]: %s", narrowMessage);
    };
  }

  return desc;
}

inline Point2 get_next_jitter(uint32_t render_width, uint32_t output_width, int32_t &jitter_index, ffxContext context,
  PfnFfxQuery query)
{
  ++jitter_index;

  Point2 jitter;

  ffxQueryDescUpscaleGetJitterOffset jitterDesc{
    .header{
      .type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTEROFFSET,
    },
    .index = jitter_index,
    .pOutX = &jitter.x,
    .pOutY = &jitter.y,
  };

  ffxQueryDescUpscaleGetJitterPhaseCount phaseCountDesc{
    .header{
      .type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT,
    },
    .renderWidth = render_width,
    .displayWidth = output_width,
    .pOutPhaseCount = &jitterDesc.phaseCount,
  };
  query(&context, &phaseCountDesc.header);

  query(&context, &jitterDesc.header);

  return jitter;
}

template <typename TFN>
bool apply_upscaling(const FSR::UpscalingPlatformArgs &args, void *command_list, const ffxContext context,
  const PfnFfxDispatch dispatch, TFN tfn)
{
  G_ASSERT(context);

  ffxDispatchDescUpscale desc{
    .header{
      .type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE,
    },
    .commandList = command_list,
    .color = tfn(args.colorTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
    .depth = tfn(args.depthTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
    .motionVectors = tfn(args.motionVectors, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
    .exposure = tfn(args.exposureTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
    .reactive = tfn(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
    .transparencyAndComposition = tfn(nullptr, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
    .output = tfn(args.outputTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
    .jitterOffset{
      .x = args.jitter.x,
      .y = args.jitter.y,
    },
    .motionVectorScale{
      .x = args.motionVectorScale.x,
      .y = args.motionVectorScale.y,
    },
    .renderSize{
      .width = uint32_t(args.inputResolution.x),
      .height = uint32_t(args.inputResolution.y),
    },
    .upscaleSize{
      .width = uint32_t(args.outputResolution.x),
      .height = uint32_t(args.outputResolution.y),
    },
    .enableSharpening = args.sharpness > 0,
    .sharpness = args.sharpness,
    .frameTimeDelta = args.timeElapsed * 1000.0f, // FSR expects milliseconds
    .preExposure = args.preExposure,
    .reset = args.reset,
    .cameraNear = args.nearPlane,
    .cameraFar = args.farPlane,
    .cameraFovAngleVertical = args.fovY,
    .flags = args.debugView ? FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW : 0u,
  };

  dispatch(const_cast<ffxContext *>(&context), &desc.header);

  return true;
}

inline IPoint2 get_rendering_resolution(FSR::UpscalingMode mode, const IPoint2 &target_resolution, ffxContext context,
  PfnFfxQuery query)
{
  uint32_t outRenderWidth, outRenderHeight;

  ffxQueryDescUpscaleGetRenderResolutionFromQualityMode desc{
    .header{
      .type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE,
    },
    .displayWidth = uint32_t(target_resolution.x),
    .displayHeight = uint32_t(target_resolution.y),
    .qualityMode = convert(mode),
    .pOutRenderWidth = &outRenderWidth,
    .pOutRenderHeight = &outRenderHeight,
  };
  query(const_cast<ffxContext *>(&context), &desc.header);

  return {int(outRenderWidth), int(outRenderHeight)};
}

} // namespace amd
