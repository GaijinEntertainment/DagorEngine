// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fidelityFXSuperResolution.h>

#include <3d/dag_textureIDHolder.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <render/resourceSlot/registerAccess.h>
#include <3d/dag_amdFsr.h>
#include <ioSys/dag_dataBlock.h>
#include <render/world/cameraParams.h>

using namespace amd;

bool FidelityFXSuperResolution::is_enabled()
{
  return FSR::isSupported() && FSR::getUpscalingMode(*::dgs_get_settings()->getBlockByNameEx("video")) != FSR::UpscalingMode::Off;
}

void FidelityFXSuperResolution::render(Texture *in_color, Texture *out_color, const AntiAliasing::OptionalInputParams &params)
{
  TIME_D3D_PROFILE(fsr);

  FSR::UpscalingArgs fsrParams{};
  fsrParams.colorTexture = in_color;
  fsrParams.depthTexture = params.depth;
  fsrParams.motionVectors = params.motion;
  fsrParams.outputTexture = out_color;
  fsrParams.reactiveTexture = params.reactive;
  fsrParams.jitter = jitterOffset;
  fsrParams.inputResolution = inputResolution;
  fsrParams.outputResolution = outputResolution;
  fsrParams.motionVectorScale = inputResolution;
  fsrParams.timeElapsed = deltaTimeMs;
  fsrParams.nearPlane = params.perspective.zn;
  fsrParams.farPlane = params.perspective.zf;
  fsrParams.fovY = atan2(1, params.perspective.hk) * 2;
  fsrParams.sharpness = -1;
  fsrParams.reset = frameCounter == 0;
  fsr->applyUpscaling(fsrParams);

  ShaderGlobal::set_color4(dlss_jitter_offsetVarId, jitterOffset);
}

FidelityFXSuperResolution::FidelityFXSuperResolution(const IPoint2 &outputResolution, bool enableHdr) :
  AntiAliasing(outputResolution, outputResolution), fsr(createFSR()), deltaTimeMs(0)
{
  if (!fsr->isLoaded())
    logerr("Failed to load amd_fidelityfx_*.dll");

  amd::FSR::ContextArgs args;
  args.enableHdr = enableHdr;
  args.outputWidth = outputResolution.x;
  args.outputHeight = outputResolution.y;
  args.mode = FSR::getUpscalingMode(*::dgs_get_settings()->getBlockByNameEx("video"));
  G_VERIFY(fsr->initUpscaling(args));

  inputResolution = fsr->getRenderingResolution(fsr->getUpscalingMode(), outputResolution);

  applierNode = resource_slot::register_access("fsr", DABFG_PP_NODE_SRC,
    {resource_slot::Create{"postfx_input_slot", "frame_for_postfx"}},
    [this](resource_slot::State slotsState, dabfg::Registry registry) {
      auto opaqueFinalTargetHndl =
        registry.readTexture("final_target_with_motion_blur").atStage(dabfg::Stage::CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

      auto depthHndl =
        registry.readTexture("depth_after_transparency").atStage(dabfg::Stage::CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
      auto motionVecsHndl =
        registry.readTexture("motion_vecs_after_transparency").atStage(dabfg::Stage::CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
      auto antialiasedHndl = registry
                               .createTexture2d(slotsState.resourceToCreateFor("postfx_input_slot"), dabfg::History::No,
                                 {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_RTARGET, registry.getResolution<2>("display")})
                               .atStage(dabfg::Stage::PS_OR_CS)
                               .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                               .handle();
      auto exposureNormFactorHndl = registry.readTexture("exposure_normalization_factor")
                                      .atStage(dabfg::Stage::PS_OR_CS)
                                      .useAs(dabfg::Usage::SHADER_RESOURCE)
                                      .handle();
      auto camera = registry.readBlob<CameraParams>("current_camera").handle();

      auto reactiveHandle =
        registry.readTexture("reactive_mask").atStage(dabfg::Stage::CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

      return
        [this, depthHndl, motionVecsHndl, opaqueFinalTargetHndl, antialiasedHndl, exposureNormFactorHndl, camera, reactiveHandle] {
          OptionalInputParams params;
          params.depth = depthHndl.view().getTex2D();
          params.motion = motionVecsHndl.view().getTex2D();
          params.exposure = exposureNormFactorHndl.view().getTex2D();
          params.reactive = reactiveHandle.view().getTex2D();
          params.perspective = camera.ref().noJitterPersp;
          render(opaqueFinalTargetHndl.view().getTex2D(), antialiasedHndl.view().getTex2D(), params);
        };
    });
}

FidelityFXSuperResolution::~FidelityFXSuperResolution() { fsr->teardownUpscaling(); }
