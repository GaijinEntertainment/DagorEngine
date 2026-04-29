// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "deepLearningSuperSampling.h"
#include "render/antialiasing.h"

#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <render/daFrameGraph/daFG.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_computeShaders.h>
#include <render/world/gbufferConsts.h>

bool DeepLearningSuperSampling::isFrameGenerationEnabled() const { return frameGenerationNode.valid(); }

DeepLearningSuperSampling::DeepLearningSuperSampling(const IPoint2 &outputResolution) :
  AntiAliasing(outputResolution, outputResolution), available(false)
{
  if (!render::antialiasing::try_init_dlss(outputResolution, inputResolution))
    return;

  if (render::antialiasing::is_ray_reconstruction_enabled())
  {
    rayReconstructionPrepareNode =
      dafg::register_node("dlss_ray_reconstruction_prepare_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
        read_gbuffer(registry, dafg::Stage::PS_OR_CS, readgbuffer::NORMAL | readgbuffer::MATERIAL);
        read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);

        registry
          .createTexture2d("packed_normal_roughness", {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("main_view")})
          .atStage(dafg::Stage::COMPUTE)
          .bindToShaderVar("dlss_normal_roughness")
          .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

        registry.createTexture2d("specular_albedo", {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("main_view")})
          .atStage(dafg::Stage::COMPUTE)
          .bindToShaderVar("dlss_specular_albedo")
          .clear(make_clear_value(0.5f, 0.5f, 0.5f, 0.f));

        return [this, renderer = ComputeShader("prepare_ray_reconstruction")] {
          renderer.dispatchThreads(inputResolution.x, inputResolution.y, 1);
        };
      });

    rayReconstructionWaterRenameNode =
      dafg::register_node("dlss_ray_reconstruction_renamer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        registry.renameTexture("packed_normal_roughness", "packed_normal_roughness_with_water");
        registry.renameTexture("specular_albedo", "specular_albedo_with_water");
        auto gbufAlbedoHndl = registry.readTexture("gbuf_0").atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::BLIT).handle();
        auto dllsAlbedoHndl =
          registry.create("dlss_albedo_with_water")
            .texture({FULL_GBUFFER_FORMATS[0] | TEXCF_UNORDERED | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
            .atStage(dafg::Stage::TRANSFER)
            .useAs(dafg::Usage::BLIT)
            .handle();
        return [gbufAlbedoHndl, dllsAlbedoHndl]() { dllsAlbedoHndl.get()->update(gbufAlbedoHndl.get()); };
      });

    colorBeforeTransparencyNode =
      dafg::register_node("dlss_ray_reconstruction_blitter", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        auto opaqueHndl = registry.readTexture("opaque_with_envi").atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::BLIT).handle();
        auto colorBeforeTransparencyHndl = registry.create("dlss_color_before_transparency")
                                             .texture({get_frame_render_target_format() | TEXCF_UPDATE_DESTINATION | TEXCF_RTARGET,
                                               registry.getResolution<2>("main_view")})
                                             .atStage(dafg::Stage::TRANSFER)
                                             .useAs(dafg::Usage::BLIT)
                                             .handle();
        return [opaqueHndl, colorBeforeTransparencyHndl]() { colorBeforeTransparencyHndl.get()->update(opaqueHndl.get()); };
      });
  }

  applierNode = dafg::register_node("dlss", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    auto opaqueFinalTargetHndl =
      registry.readTexture("target_for_transparency").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto depthHndl =
      registry.readTexture("depth_after_transparency").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto motionVectorsHndl = registry.readTexture("motion_vecs_after_transparency")
                               .atStage(dafg::Stage::PS_OR_CS)
                               .useAs(dafg::Usage::SHADER_RESOURCE)
                               .handle();
    auto exposureNormFactorHndl = !render::antialiasing::is_ray_reconstruction_enabled()
                                    ? eastl::make_optional(registry.readTexture("exposure_normalization_factor")
                                                             .atStage(dafg::Stage::PS_OR_CS)
                                                             .useAs(dafg::Usage::SHADER_RESOURCE)
                                                             .handle())
                                    : eastl::nullopt;
    auto antialiasedHndl =
      registry.createTexture2d("frame_after_aa", {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("display")})
        .atStage(dafg::Stage::PS_OR_CS)
        .useAs(dafg::Usage::SHADER_RESOURCE)
        .handle();
    auto camera = registry.readBlob<CameraParams>("current_camera").handle();
    auto cameraHistory = registry.readBlobHistory<CameraParams>("current_camera").handle();

    auto albedoHndl = render::antialiasing::is_ray_reconstruction_enabled()
                        ? eastl::make_optional(registry.read("dlss_albedo_with_water")
                                                 .texture()
                                                 .atStage(dafg::Stage::PS_OR_CS)
                                                 .useAs(dafg::Usage::SHADER_RESOURCE)
                                                 .handle())
                        : eastl::nullopt;
    auto specularAlbedoHndl = render::antialiasing::is_ray_reconstruction_enabled()
                                ? eastl::make_optional(registry.read("specular_albedo_with_water")
                                                         .texture()
                                                         .atStage(dafg::Stage::PS_OR_CS)
                                                         .useAs(dafg::Usage::SHADER_RESOURCE)
                                                         .handle())
                                : eastl::nullopt;
    auto normalRoughnessHndl = render::antialiasing::is_ray_reconstruction_enabled()
                                 ? eastl::make_optional(registry.read("packed_normal_roughness_with_water")
                                                          .texture()
                                                          .atStage(dafg::Stage::PS_OR_CS)
                                                          .useAs(dafg::Usage::SHADER_RESOURCE)
                                                          .handle())
                                 : eastl::nullopt;

    auto hitDistHndl =
      render::antialiasing::is_ray_reconstruction_enabled()
        ? eastl::make_optional(
            registry.read("rtr_tex_unfiltered").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle())
        : eastl::nullopt;

    auto ssssGuideHndl = render::antialiasing::is_ray_reconstruction_enabled()
                           ? eastl::make_optional(registry.readTexture("ssss_rr_guide")
                                                    .atStage(dafg::Stage::PS_OR_CS)
                                                    .useAs(dafg::Usage::SHADER_RESOURCE)
                                                    .optional()
                                                    .handle())
                           : eastl::nullopt;
    auto colorBeforeTransparencyHndl = render::antialiasing::is_ray_reconstruction_enabled()
                                         ? eastl::make_optional(registry.readTexture("dlss_color_before_transparency")
                                                                  .atStage(dafg::Stage::PS_OR_CS)
                                                                  .useAs(dafg::Usage::SHADER_RESOURCE)
                                                                  .handle())
                                         : eastl::nullopt;

    auto frameCounterHndl = registry.readBlob<uint32_t>("monotonic_frame_counter").handle();

    auto resources = eastl::make_tuple(depthHndl, motionVectorsHndl, exposureNormFactorHndl, opaqueFinalTargetHndl, antialiasedHndl,
      camera, cameraHistory, albedoHndl, normalRoughnessHndl, specularAlbedoHndl, hitDistHndl, frameCounterHndl, ssssGuideHndl,
      colorBeforeTransparencyHndl);

    return [this, resources = eastl::make_unique<decltype(resources)>(resources)] {
      auto [depthHndl, motionVectorsHndl, exposureNormFactorHndl, opaqueFinalTargetHndl, antialiasedHndl, camera, cameraHistory,
        albedoHndl, normalRoughnessHndl, specularAlbedoHndl, hitDistHndl, frameCounterHndl, ssssGuideHndl,
        colorBeforeTransparencyHndl] = *resources;

      render::antialiasing::ApplyContext applyContext{.projection = camera.ref().noJitterProjTm,
        .projectionInverse = inverse44(camera.ref().noJitterProjTm),
        .reprojection = inverse44(cameraHistory.ref().noJitterGlobtm) * camera.ref().noJitterGlobtm,
        .reprojectionInverse = inverse44(camera.ref().noJitterGlobtm) * cameraHistory.ref().noJitterGlobtm,
        .worldToView = camera.ref().viewTm,
        .viewToWorld = camera.ref().viewItm,
        .position = camera.ref().viewItm.getcol(3),
        .up = camera.ref().viewItm.getcol(1),
        .right = camera.ref().viewItm.getcol(0),
        .forward = camera.ref().viewItm.getcol(2),
        .nearZ = camera.ref().noJitterPersp.zn,
        .farZ = camera.ref().noJitterPersp.zf,
        .fov = 2 * atan(1.f / camera.ref().noJitterPersp.wk),
        .aspect = camera.ref().noJitterPersp.hk / camera.ref().noJitterPersp.wk,
        .depthTexture = depthHndl.get(),
        .motionTexture = motionVectorsHndl.get(),
        .albedoTexture = albedoHndl ? albedoHndl->get() : nullptr,
        .hitDistTexture = hitDistHndl ? hitDistHndl->get() : nullptr,
        .normalRoughnessTexture = normalRoughnessHndl ? normalRoughnessHndl->get() : nullptr,
        .specularAlbedoTexture = specularAlbedoHndl ? specularAlbedoHndl->get() : nullptr,
        .exposureTexture = exposureNormFactorHndl ? exposureNormFactorHndl->get() : nullptr,
        .ssssGuideTexture = ssssGuideHndl ? ssssGuideHndl->get() : nullptr,
        .colorBeforeTransparencyTexture = colorBeforeTransparencyHndl ? colorBeforeTransparencyHndl->get() : nullptr,
        .jitterPixelOffset = jitterOffset,
        .inputOffset = IPoint2::ZERO,
        .viewIndex = 0,
        .persp = camera.ref().noJitterPersp,
        .timeElapsed = 0.0f,
        .depthTexTransform = Point4::ZERO,
        .vrVrsMask = nullptr};

      render::antialiasing::apply_dlss(opaqueFinalTargetHndl.get(), applyContext, antialiasedHndl.get());
    };
  });

  if (render::antialiasing::is_frame_generation_enabled())
  {
    frameGenerationNode = dafg::register_node("dlss_g", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
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

    // the sole purpose of this node is to reference history of the resources used by dlss_g node in order to make these resources
    // "survive" until present()
    lifetimeExtenderNode = dafg::register_node("dlss_g_lifetime_extender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
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

float DeepLearningSuperSampling::getLodBias() const { return render::antialiasing::get_mip_bias(); }
