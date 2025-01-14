// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_convar.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/deferredRenderer.h>
#include <render/daBfg/bfg.h>
#include <render/renderEvent.h>
#include <ecs/render/updateStageRender.h>

#include <daECS/core/entityManager.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <render/world/global_vars.h>
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/postfxRender.h>

CONSOLE_INT_VAL("render", srgb_anisotropy, 0, 0, 4);

dabfg::NodeHandle makeSSAANode()
{
  return dabfg::register_node("ssaa_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    // Repurposing existing shader
    registry.orderMeBefore("after_post_fx_ecs_event_node");
    registry.readTexture("postfxed_frame").atStage(dabfg::Stage::PS).bindToShaderVar("super_screenshot_tex");
    registry.requestRenderPass().color({"frame_after_postfx"});

    return [downscaleSuperresScreenshot = PostFxRenderer("downscale_superres_screenshot")] {
      ShaderGlobal::set_int(super_pixelsVarId, 2);
      downscaleSuperresScreenshot.render();
    };
  });
}

static auto registerFXAANode(dabfg::Registry registry, const char *target_name, bool external_target)
{
  registry.readTexture("postfxed_frame").atStage(dabfg::Stage::PS).bindToShaderVar("final_target_frame");

  (external_target ? registry.modifyTexture(target_name)
                   : registry.createTexture2d(target_name, dabfg::History::No,
                       {TEXCF_RTARGET | get_frame_render_target_format(), registry.getResolution<2>("main_view")}))
    .atStage(dabfg::Stage::PS)
    .useAs(dabfg::Usage::COLOR_ATTACHMENT);

  registry.requestRenderPass().color({target_name});

  auto fxaaResolutionVarId = get_shader_variable_id("fxaa_resolution");
  auto fxaaTcScaleOffsetVarId = get_shader_variable_id("fxaa_tc_scale_offset");
  const auto postFxResolution = registry.getResolution<2>("post_fx");
  return [fxaa = PostFxRenderer("fxaa"), frameSamplerVarId = get_shader_variable_id("final_target_frame_samplerstate"),
           fxaaResolutionVarId, fxaaTcScaleOffsetVarId, postFxResolution]() {
    const auto resolution = postFxResolution.get();
    ShaderGlobal::set_color4(fxaaResolutionVarId, resolution.x, resolution.y, 0, 0);
    ShaderGlobal::set_color4(fxaaTcScaleOffsetVarId, 1.0f, 1.0f, 0.0f, 0.0f);

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.anisotropic_max = srgb_anisotropy.get();
      ShaderGlobal::set_sampler(frameSamplerVarId, d3d::request_sampler(smpInfo));
    }
    fxaa.render();
  };
}

dabfg::NodeHandle makeFXAANode(const char *target_name, bool external_target)
{
  return dabfg::register_node("fxaa_node", DABFG_PP_NODE_SRC,
    [target_name, external_target](dabfg::Registry registry) { return registerFXAANode(registry, target_name, external_target); });
}

dabfg::NodeHandle makeStaticUpsampleNode(const char *source_name)
{
  return dabfg::register_node("static_upsample_node", DABFG_PP_NODE_SRC, [source_name](dabfg::Registry registry) {
    registry.orderMeAfter("post_fx_node");
    registry.orderMeBefore("after_post_fx_ecs_event_node");
    registry.readTexture(source_name).atStage(dabfg::Stage::PS).bindToShaderVar("upsampling_source_tex");
    registry.requestRenderPass().color({"frame_after_postfx"});

    return [staticUpsample = PostFxRenderer("static_upsample")] { staticUpsample.render(); };
  });
}

dabfg::NodeHandle makeAfterPostFxEcsEventNode()
{
  return dabfg::register_node("after_post_fx_ecs_event_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.multiplex(dabfg::multiplexing::Mode::None);
    registry.orderMeAfter("post_fx_node");
    registry.executionHas(dabfg::SideEffects::External);
    return []() {
      TIME_D3D_PROFILE(postfx_after_render);
      g_entity_mgr->broadcastEventImmediate(AfterRenderPostFx());
    };
  });
}

dabfg::NodeHandle makeDepthWithTransparencyNode()
{
  return dabfg::register_node("depth_with_transparency_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto depthHndl =
      registry.readTexture("depth_for_transparency").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto transparentDepthHndl = registry
                                  .createTexture2d("depth_with_transparency", dabfg::History::No,
                                    {TEXFMT_DEPTH32 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
                                  .atStage(dabfg::Stage::PS)
                                  .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                                  .handle();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto occlusionHndl = registry.readBlob<Occlusion *>("current_occlusion").handle();
    auto texCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.requestState().setFrameBlock("global_frame");

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();

    return [depthHndl, transparentDepthHndl, cameraHndl, occlusionHndl, texCtxHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      if (!g_entity_mgr)
        return;

      {
        SCOPE_RENDER_TARGET;
        d3d::settex(15, depthHndl.view().getTex2D());
        d3d::set_render_target(nullptr, 0);
        d3d::set_depth(transparentDepthHndl.view().getTex2D(), DepthAccess::RW);
        wr.copyDepth.render();
      }
      {
        d3d::set_render_target(nullptr, 0);
        d3d::set_depth(transparentDepthHndl.view().getTex2D(), DepthAccess::RW);

        CameraParams camera = cameraHndl.ref();

        ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));

        g_entity_mgr->broadcastEventImmediate(
          UpdateStageInfoRenderTrans(camera.viewTm, camera.jitterProjTm, camera.viewItm, occlusionHndl.ref(), texCtxHndl.ref()));
      }
    };
  });
}

dabfg::NodeHandle makeDownsampleDepthWithTransparencyNode()
{
  return dabfg::register_node("downsample_depth_with_transparency_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeBefore("prepare_post_fx_node");

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

    const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);

    auto dofFarDepthHndl = registry
                             .createTexture2d("far_depth_with_transparency", dabfg::History::No,
                               {TEXFMT_R32F | TEXCF_RTARGET, halfMainViewRes, wr.downsampledTexturesMipCount})
                             .atStage(dabfg::Stage::PS)
                             .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                             .handle();

    auto dofCloseDepthHndl = registry
                               .createTexture2d("near_depth_with_transparency", dabfg::History::No,
                                 {TEXFMT_R32F | TEXCF_RTARGET, halfMainViewRes, wr.downsampledTexturesMipCount})
                               .atStage(dabfg::Stage::PS)
                               .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                               .handle();

    registry.readTexture("depth_with_transparency")
      .atStage(dabfg::Stage::POST_RASTER)
      .bindToShaderVar("downsampled_depth_with_transparency");

    registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");

    registry.readTexture("close_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_close_depth_tex");

    return [dofFarDepthHndl, dofCloseDepthHndl, postfx = PostFxRenderer("downsample_depth_for_far_dof")]() {
      d3d::set_render_target({nullptr, 0}, DepthAccess::SampledRO,
        {{dofFarDepthHndl.view().getTex2D(), 0}, {dofCloseDepthHndl.view().getTex2D(), 0}});
      postfx.render();
    };
  });
}

resource_slot::NodeHandleWithSlotsAccess makePostFxNode()
{
  return resource_slot::register_access("post_fx_node", DABFG_PP_NODE_SRC, {resource_slot::Read{"postfx_input_slot"}},
    [](resource_slot::State slotsState, dabfg::Registry registry) {
      registry.orderMeAfter("prepare_post_fx_node");

      postfx_read_additional_textures_from_registry(registry);

      registry.readTexture(slotsState.resourceToReadFrom("postfx_input_slot"))
        .atStage(dabfg::Stage::POST_RASTER)
        .bindToShaderVar("frame_tex");

      d3d::SamplerInfo postfxInputSmpInfo;
      postfxInputSmpInfo.address_mode_u = postfxInputSmpInfo.address_mode_v = postfxInputSmpInfo.address_mode_w =
        d3d::AddressMode::Mirror;
      registry.create("postfx_input_mirror_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(postfxInputSmpInfo))
        .bindToShaderVar("frame_tex_samplerstate");

      // Binding of the dof_depth_blend shader var is done with setBlendDepthTex,
      // "depth_with_transparency" or "lens_dof_blend_depth_tex" or "depth_for_postfx"
      // binded to dof_depth_blend depending on context.

      registry.readTexture("depth_for_postfx").atStage(dabfg::Stage::PS).optional();

      registry.readTexture("depth_with_transparency").atStage(dabfg::Stage::PS).optional();

      registry.readTexture("dof_blend").atStage(dabfg::Stage::PS).bindToShaderVar("dof_blend_tex").optional();

      (registry.root() / "bloom")
        .readTexture("upsampled_bloom_mip_0")
        .atStage(dabfg::Stage::PS)
        .bindToShaderVar("bloom_tex")
        .optional();
      registry.read("bloom_clamp_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("bloom_tex_samplerstate").optional();

      registry.readTexture("screen_bleed_tex").atStage(dabfg::Stage::PS).bindToShaderVar("screen_bleed_tex").optional();

      auto targetHndl =
        registry.modifyTexture("postfxed_frame").atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle();

      registry.readTexture("blood_texture").atStage(dabfg::Stage::PS).bindToShaderVar("blood_texture").optional();
      // in shader we use this sampler for two textures, so bind it without optional logic
      registry.create("postfx_common_wrap_linear_sampler", dabfg::History::No)
        .blob(d3d::request_sampler({}))
        .bindToShaderVar("blood_texture_samplerstate");

      auto closeupsNs = registry.root() / "opaque" / "closeups";
      postfx_bind_additional_textures_from_namespace(closeupsNs);

      registry.readTexture("flash_blind_tex").atStage(dabfg::Stage::PS).bindToShaderVar("flash_blind_screen_tex").optional();

      // TODO: we can pass a per-project callbacks which requests such textures for postfx.
      (registry.root() / "sprite_glare")
        .read("result")
        .texture()
        .atStage(dabfg::Stage::PS)
        .bindToShaderVar("sprite_glare_output_tex")
        .optional();

      g_entity_mgr->broadcastEventImmediate(RegisterPostfxResources(registry));

      return [targetHndl, postfx = PostFxRenderer("postfx")] {
        d3d::set_render_target(targetHndl.view().getBaseTex(), 0);
        if (Texture *secondaryPostFxTarget = d3d::get_secondary_backbuffer_tex())
          d3d::set_render_target(1, secondaryPostFxTarget, 0);
        postfx.render();
      };
    });
}

resource_slot::NodeHandleWithSlotsAccess makePostFxInputSlotProviderNode()
{
  return resource_slot::register_access("postfx_input_slot_provider", DABFG_PP_NODE_SRC,
    {resource_slot::Create{"postfx_input_slot", "frame_for_postfx"}}, [](resource_slot::State slotsState, dabfg::Registry registry) {
      registry.renameTexture("frame_after_distortion", slotsState.resourceToCreateFor("postfx_input_slot"), dabfg::History::No);
      return []() {};
    });
}

extern ConVarB adaptationIlluminance;

resource_slot::NodeHandleWithSlotsAccess makePreparePostFxNode()
{
  return resource_slot::register_access("prepare_post_fx_node", DABFG_PP_NODE_SRC, {resource_slot::Read{"postfx_input_slot"}},
    [](resource_slot::State slotsState, dabfg::Registry registry) {
      // need for "dof_downsampled"
      auto closeDepthHndl = registry.readTexture("near_depth_with_transparency")
                              .atStage(dabfg::Stage::POST_RASTER)
                              .optional()
                              .useAs(dabfg::Usage::SHADER_RESOURCE)
                              .handle();
      registry.readTexture("far_depth_with_transparency")
        .atStage(dabfg::Stage::POST_RASTER)
        .optional()
        .bindToShaderVar("downsampled_far_depth_tex");

      auto finalFrameHndl = registry.readTexture(slotsState.resourceToReadFrom("postfx_input_slot"))
                              .atStage(dabfg::Stage::POST_RASTER)
                              .useAs(dabfg::Usage::SHADER_RESOURCE)
                              .handle();

      auto downsampledFrameHndl = registry.readTexture("downsampled_color")
                                    .atStage(dabfg::Stage::POST_RASTER)
                                    .useAs(dabfg::Usage::SHADER_RESOURCE)
                                    .optional()
                                    .handle();

      auto depthForPostFxHndl =
        registry.modifyTexture("depth_for_postfx").atStage(dabfg::Stage::ALL_GRAPHICS).useAs(dabfg::Usage::DEPTH_ATTACHMENT).handle();
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");

      auto cameraHndl = registry.read("current_camera").blob<CameraParams>().handle();

      return [depthForPostFxHndl, closeDepthHndl, finalFrameHndl, downsampledFrameHndl, cameraHndl,
               depth_gbufVarId = get_shader_variable_id("depth_gbuf")]() {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
        ShaderGlobal::set_texture(depth_gbufVarId, depthForPostFxHndl.d3dResId());

        TextureIDPair postfxInput = {finalFrameHndl.view().getTex2D(), finalFrameHndl.view().getTexId()};

        wr.postfx.prepare(postfxInput, downsampledFrameHndl.view(), closeDepthHndl.view(), depthForPostFxHndl.view(),
          cameraHndl.ref().jitterPersp.zn, cameraHndl.ref().jitterPersp.zf, cameraHndl.ref().jitterPersp.hk);

        ShaderGlobal::set_texture(depth_gbufVarId, BAD_TEXTUREID);
      };
    });
}

dabfg::NodeHandle makeFrameBeforeDistortionProducerNode()
{
  return dabfg::register_node("frame_before_distortion_producer", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto srcFrameHndl = registry.readTexture("frame_after_aa").atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::BLIT).handle();

    auto dstFrameHndl = registry
                          .createTexture2d("frame_before_distortion", dabfg::History::No,
                            {TEXFMT_A16B16G16R16F | TEXCF_RTARGET, registry.getResolution<2>("post_fx")})
                          .atStage(dabfg::Stage::TRANSFER)
                          .useAs(dabfg::Usage::BLIT)
                          .handle();

    auto hasThermalRender = registry.readBlob<OrderingToken>("thermal_spectre_rendered").optional().handle();

    auto distortionPostfxRequired = registry.createBlob<bool>("distortion_postfx_required", dabfg::History::No).handle();

    return [srcFrameHndl, dstFrameHndl, hasThermalRender, distortionPostfxRequired]() {
      distortionPostfxRequired.ref() = !hasThermalRender.get();

      if (!distortionPostfxRequired.ref())
        d3d::stretch_rect(srcFrameHndl.view().getTex2D(), dstFrameHndl.get());
    };
  });
}

dabfg::NodeHandle makeDistortionFxNode()
{
  return dabfg::register_node("distortion_postfx_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto frame = registry.renameTexture("frame_before_distortion", "frame_after_distortion", dabfg::History::No);

    registry.requestRenderPass().color({frame});

    registry.readTexture("frame_after_aa").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("frame_tex");

    auto distortionPostfxRequired = registry.readBlob<bool>("distortion_postfx_required").handle();

    registry.readTexture("depth_after_transparency").atStage(dabfg::Stage::PS).bindToShaderVar("haze_scene_depth_tex");

    registry.readTexture("haze_offset").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("haze_offset_tex").optional();
    registry.create("haze_default_sampler", dabfg::History::No)
      .blob(d3d::request_sampler({}))
      .bindToShaderVar("haze_offset_tex_samplerstate")
      .bindToShaderVar("haze_depth_tex_samplerstate")
      .optional();
    registry.readTexture("haze_color").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("haze_color_tex").optional();
    registry.readTexture("haze_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("haze_depth_tex").optional();

    return [distortionPostfxRequired, distortionPostfx = PostFxRenderer("distortion_postfx")]() {
      if (distortionPostfxRequired.ref())
        distortionPostfx.render();
    };
  });
}