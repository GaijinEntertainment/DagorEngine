// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/FSR2.h>

#include <3d/dag_textureIDHolder.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <render/daFrameGraph/daFG.h>

bool FSR2::is_enabled()
{
  return Fsr2State(d3d::driver_command(Drv3dCommand::GET_FSR2_STATE, nullptr, nullptr, nullptr)) == Fsr2State::READY;
}

static IPoint2 query_input_resolution()
{
  IPoint2 inputResolution;
  d3d::driver_command(Drv3dCommand::GET_FSR2_RESOLUTION, &inputResolution.x, &inputResolution.y, nullptr);
  return inputResolution;
}

void FSR2::render(Texture *in_color, Texture *out_color, const AntiAliasing::OptionalInputParams &params)
{
  TIME_D3D_PROFILE(fsr2);

  Fsr2Params fsr2Params{};
  fsr2Params.color = in_color;
  fsr2Params.depth = params.depth;
  fsr2Params.motionVectors = params.motion;
  fsr2Params.output = out_color;
  fsr2Params.jitterOffsetX = jitterOffset.x;
  fsr2Params.jitterOffsetY = jitterOffset.y;
  fsr2Params.renderSizeX = inputResolution.x;
  fsr2Params.renderSizeY = inputResolution.y;
  fsr2Params.motionVectorScaleX = inputResolution.x;
  fsr2Params.motionVectorScaleY = inputResolution.y;
  fsr2Params.frameTimeDelta = deltaTimeMs;
  fsr2Params.cameraNear = cameraNear;
  fsr2Params.cameraFar = cameraFar;
  fsr2Params.cameraFovAngleVertical = fov;
  fsr2Params.sharpness = -1;
  d3d::driver_command(Drv3dCommand::EXECUTE_FSR2, (void *)&fsr2Params, nullptr, nullptr);

  ShaderGlobal::set_color4(dlss_jitter_offsetVarId, jitterOffset);
}

FSR2::FSR2(const IPoint2 &outputResolution) :
  AntiAliasing(query_input_resolution(), outputResolution), deltaTimeMs(0), cameraNear(0), cameraFar(0), fov(0)
{
  applierNode = dafg::register_node("fsr2", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    auto opaqueFinalTargetHndl =
      registry.readTexture("final_target_with_motion_blur").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    auto depthHndl =
      registry.readTexture("depth_after_transparency").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto motionVecsHndl =
      registry.readTexture("motion_vecs_after_transparency").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto antialiasedHndl = registry
                             .createTexture2d("frame_after_aa", dafg::History::No,
                               {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("display")})
                             .atStage(dafg::Stage::PS_OR_CS)
                             .useAs(dafg::Usage::COLOR_ATTACHMENT)
                             .handle();
    return [this, depthHndl, motionVecsHndl, opaqueFinalTargetHndl, antialiasedHndl] {
      OptionalInputParams params;
      params.depth = depthHndl.view().getTex2D();
      params.motion = motionVecsHndl.view().getTex2D();
      render(opaqueFinalTargetHndl.view().getTex2D(), antialiasedHndl.view().getTex2D(), params);
    };
  });
}

Point2 FSR2::update(Driver3dPerspective &perspective)
{
  cameraNear = perspective.zn;
  cameraFar = perspective.zf;
  fov = atan(1 / perspective.hk) * 2;
  return AntiAliasing::update(perspective);
}
