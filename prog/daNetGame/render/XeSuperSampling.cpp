// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/XeSuperSampling.h>

#include <3d/dag_textureIDHolder.h>
#include <perfMon/dag_statDrv.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/antialiasing.h>
#include <render/world/cameraParams.h>

#include <EASTL/finally.h>

bool XeSuperSampling::isFrameGenerationEnabled() const { return frameGenerationNode.valid(); }

XeSuperSampling::XeSuperSampling(const IPoint2 &outputResolution) : AntiAliasing(outputResolution, outputResolution), available(false)
{
  if (!render::antialiasing::try_init_xess(outputResolution, inputResolution))
    return;

  applierNode = dafg::register_node("xess", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    auto opaqueFinalTargetHndl =
      registry.readTexture("target_for_transparency").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto depthHndl =
      registry.readTexture("depth_after_transparency").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto motionVecsHndl = registry.readTexture("motion_vecs_after_transparency")
                            .optional()
                            .atStage(dafg::Stage::CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();
    auto antialiasedHndl =
      registry.createTexture2d("frame_after_aa", {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("display")})
        .atStage(dafg::Stage::CS)
        .useAs(dafg::Usage::SHADER_RESOURCE)
        .handle();
    return [this, depthHndl, motionVecsHndl, opaqueFinalTargetHndl, antialiasedHndl] {
      Point2 mvScale = inputResolution;
      d3d::driver_command(Drv3dCommand::SET_XESS_VELOCITY_SCALE, &mvScale.x, &mvScale.y);
      render::antialiasing::ApplyContext ctx;
      ctx.depthTexture = depthHndl.get();
      ctx.motionTexture = motionVecsHndl.get();
      ctx.jitterPixelOffset = jitterOffset;
      ctx.resetHistory = frameCounter == 0;
      apply_xess(opaqueFinalTargetHndl.get(), ctx, antialiasedHndl.get());
    };
  });

  if (render::antialiasing::is_frame_generation_enabled())
  {
    frameGenerationNode = dafg::register_node("xess_fg", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto beforeUINs = registry.root() / "before_ui";
      auto finalFrameHandle =
        beforeUINs.readTexture("frame_done").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      auto uiHandle = registry.readTexture("ui_tex").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      auto depthHandle =
        registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      auto motionVectorsHandle = registry.readTexture("motion_vecs_after_transparency")
                                   .atStage(dafg::Stage::PS_OR_CS)
                                   .useAs(dafg::Usage::SHADER_RESOURCE)
                                   .handle();
      auto camera = registry.readBlob<CameraParams>("current_camera").handle();
      auto cameraHistory = registry.readBlobHistory<CameraParams>("current_camera").handle();

      return [this, finalFrameHandle, uiHandle, motionVectorsHandle, depthHandle, camera, cameraHistory] {
        render::antialiasing::FrameGenContext frameGenContext = {.viewItm = camera.ref().viewItm,
          .noJitterProjTm = camera.ref().noJitterProjTm,
          .noJitterGlobTm = camera.ref().noJitterGlobtm,
          .prevNoJitterGlobTm = cameraHistory.ref().noJitterGlobtm,
          .noJitterPersp = camera.ref().noJitterPersp,
          .finalImageTexture = finalFrameHandle.get(),
          .depthTexture = depthHandle.get(),
          .motionVectorTexture = motionVectorsHandle.get(),
          .uiTexture = uiHandle.get(),
          .timeElapsed = 0.0f,
          .jitterPixelOffset = jitterOffset,
          .resetHistory = frameCounter == 0};
        render::antialiasing::schedule_generated_frames(frameGenContext);
      };
    });

    // the sole purpose of this node is to reference history of the resources used by fsr_fg node in order to make these resources
    // "survive" until present()
    lifetimeExtenderNode = dafg::register_node("xess_fg_lifetime_extender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      // ordering before setup_world_rendering_node makes lifetimes as short as possible while making sure the resources are still
      // alive during present()
      registry.orderMeBefore("setup_world_rendering_node");
      registry.readTextureHistory("ui_tex").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE);
      registry.readTextureHistory("depth_for_postfx").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE);
      registry.readTextureHistory("motion_vecs_after_transparency").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE);
      return [] {};
    });
  }

  available = true;
}

float XeSuperSampling::getLodBias() const { return render::antialiasing::get_mip_bias(); }
