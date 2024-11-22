// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/XeSuperSampling.h>

#include <3d/dag_textureIDHolder.h>
#include <perfMon/dag_statDrv.h>
#include <render/resourceSlot/registerAccess.h>

bool XeSuperSampling::is_enabled()
{
  bool ready = XessState(d3d::driver_command(Drv3dCommand::GET_XESS_STATE)) == XessState::READY;
  return ready;
}

static IPoint2 query_input_resolution()
{
  IPoint2 inputResolution;
  d3d::driver_command(Drv3dCommand::GET_XESS_RESOLUTION, &inputResolution.x, &inputResolution.y);

  debug("Rendering resolution applied by XeSS: %dx%d.", inputResolution.x, inputResolution.y);

  return inputResolution;
}

XeSuperSampling::XeSuperSampling(const IPoint2 &outputResolution) : AntiAliasing(query_input_resolution(), outputResolution)
{
  float x = inputResolution.x;
  float y = inputResolution.y;
  d3d::driver_command(Drv3dCommand::SET_XESS_VELOCITY_SCALE, &x, &y);

  applierNode = dabfg::register_node("xess", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    auto opaqueFinalTargetHndl =
      registry.readTexture("final_target_with_motion_blur").atStage(dabfg::Stage::CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    auto depthHndl =
      registry.readTexture("depth_after_transparency").atStage(dabfg::Stage::CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    auto motionVecsHndl = registry.readTexture("motion_vecs_after_transparency")
                            .optional()
                            .atStage(dabfg::Stage::CS)
                            .useAs(dabfg::Usage::SHADER_RESOURCE)
                            .handle();
    auto antialiasedHndl = registry
                             .createTexture2d("frame_after_aa", dabfg::History::No,
                               {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("display")})
                             .atStage(dabfg::Stage::CS)
                             .useAs(dabfg::Usage::SHADER_RESOURCE)
                             .handle();
    return [this, depthHndl, motionVecsHndl, opaqueFinalTargetHndl, antialiasedHndl] {
      OptionalInputParams params;
      params.depth = depthHndl.view().getTex2D();
      params.motion = motionVecsHndl.view().getTex2D();
      xess_render(opaqueFinalTargetHndl.view().getTex2D(), antialiasedHndl.view().getTex2D(), inputResolution, jitterOffset, params);
    };
  });
}

void xess_render(Texture *in_color,
  Texture *out_color,
  Point2 input_resolution,
  Point2 jitter_offset,
  const AntiAliasing::OptionalInputParams &params)
{
  XessParams xessParams = {};
  xessParams.inColor = in_color;
  xessParams.inDepth = params.depth;
  xessParams.inMotionVectors = params.motion;
  xessParams.inJitterOffsetX = jitter_offset.x;
  xessParams.inJitterOffsetY = jitter_offset.y;
  xessParams.outColor = out_color;
  xessParams.inInputWidth = input_resolution.x;
  xessParams.inInputHeight = input_resolution.y;

  ShaderGlobal::set_color4(get_shader_variable_id("dlss_jitter_offset", true), jitter_offset);

  d3d::driver_command(Drv3dCommand::EXECUTE_XESS, &xessParams);
}