// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scopeAimRender.h"
#include "scopeFullDeferredNodes.h"

#include <render/daBfg/bfg.h>
#include <render/deferredRenderer.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/defaultVrsSettings.h>
#include <shaders/dag_computeShaders.h>
#include <util/dag_convar.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>

extern ConVarT<bool, false> vrs_dof;

#define SCOPE_LENS_VARS VAR(screen_pos_to_texcoord)

#define SCOPE_VRS_MASK_VARS  \
  VAR(scope_vrs_mask_rate)   \
  VAR(scope_vrs_mask_tile_x) \
  VAR(scope_vrs_mask_tile_y)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
SCOPE_VRS_MASK_VARS
SCOPE_LENS_VARS
#undef VAR


dabfg::NodeHandle makeScopePrepassNode()
{
  auto ns = dabfg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_prepass_opaque_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");
    render_to_gbuffer_prepass(registry);

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.read("current_camera").blob<CameraParams>().bindAsProj<&CameraParams::jitterProjTm>();
    registry.read("viewtm_no_offset").blob<TMatrix>().bindAsView();

    return [aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl]() {
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope_lens_prepass(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle makeScopeOpaqueNode()
{
  auto ns = dabfg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_opaque_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    render_to_gbuffer(registry);

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.read("current_camera").blob<CameraParams>().bindAsProj<&CameraParams::jitterProjTm>();
    registry.read("viewtm_no_offset").blob<TMatrix>().bindAsView();

    return [aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl]() {
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle makeScopeLensMaskNode()
{
  auto ns = dabfg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_lens_mask_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    // We want all of scope to already be present in the gbuffer so as
    // to occlude the lens mask with it. Technically, this is more of a
    // read than a write, but decompressing the depth buffer is expensive.
    // TODO: consider making this a read somehow
    registry.orderMeAfter("scope_opaque_node");

    registry.create("scope_lens_mask", dabfg::History::No)
      .texture({TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")});
    registry.create("scope_lens_sampler", dabfg::History::No).blob(d3d::request_sampler({}));

    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_WRITE_DISABLE);
    registry.requestState().enableOverride(overrideState).setFrameBlock("global_frame");

    registry.requestRenderPass()
      .color({"scope_lens_mask"})
      .depthRw("gbuf_depth")
      .clear("scope_lens_mask", make_clear_value(0.f, 0.f, 0.f, 0.f));

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.read("current_camera").blob<CameraParams>().bindAsProj<&CameraParams::jitterProjTm>();
    registry.read("viewtm_no_offset").blob<TMatrix>().bindAsView();

    return [aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl]() {
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope_lens_mask(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle makeScopeVrsMaskNode()
{
  const bool vrsSupported = d3d::get_driver_desc().caps.hasVariableRateShadingTexture;
  if (vrsSupported)
    return dabfg::register_node("scope_vrs_mask_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      auto vrsTextureHndl =
        registry.modifyTexture(VRS_RATE_TEXTURE_NAME).atStage(dabfg::Stage::COMPUTE).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

      registry.requestState().setFrameBlock("global_frame");

      auto closeupsNs = registry.root() / "opaque" / "closeups";

      closeupsNs.readTexture("scope_lens_mask").atStage(dabfg::Stage::COMPUTE).bindToShaderVar("scope_lens_mask");
      closeupsNs.read("scope_lens_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("scope_lens_mask_samplerstate");

      auto depthGbufHndl =
        closeupsNs.read("gbuf_depth").texture().atStage(dabfg::Stage::COMPUTE).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

      auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();

      ShaderGlobal::set_int(scope_vrs_mask_rateVarId, eastl::to_underlying(VRSRATE::RATE_2x2));
      ShaderGlobal::set_int(scope_vrs_mask_tile_xVarId, d3d::get_driver_desc().variableRateTextureTileSizeX);
      ShaderGlobal::set_int(scope_vrs_mask_tile_yVarId, d3d::get_driver_desc().variableRateTextureTileSizeY);

      return
        [aimRenderDataHndl, vrsTextureHndl, depthGbufHndl, scopeVRSMaskCS = Ptr(new_compute_shader("scope_vrs_mask_cs", false))]() {
          const AimRenderingData &aimRenderData = aimRenderDataHndl.ref();

          if (!aimRenderData.lensRenderEnabled)
            return;

          prepare_scope_vrs_mask(scopeVRSMaskCS, vrsTextureHndl.get(), depthGbufHndl.view().getTexId());
        };
    });
  else
    return dabfg::NodeHandle();
}

dabfg::NodeHandle makeScopeCutDepthNode()
{
  auto ns = dabfg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_cut_depth_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    // Ordering token to avoid data races when running concurrently with "async_animchar_rendering_start_node"
    registry.create("dynmodel_global_state_access_optional_token", dabfg::History::No).blob<OrderingToken>();

    shaders::OverrideState zAlwaysStateOverride;
    zAlwaysStateOverride.set(shaders::OverrideState::Z_FUNC);
    zAlwaysStateOverride.zFunc = CMPF_ALWAYS;
    registry.requestState().setFrameBlock("global_frame").enableOverride(zAlwaysStateOverride);

    // We modify the "done" version of the gbuffer to ensure that
    // everything else has already been rendered
    registry.requestRenderPass().color({registry.modify("gbuf_2_done").texture().optional()}).depthRw("gbuf_depth_done");

    registry.readTexture("scope_lens_mask").atStage(dabfg::Stage::PS).bindToShaderVar("scope_lens_mask");
    registry.read("scope_lens_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("scope_lens_mask_samplerstate");
    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.read("current_camera").blob<CameraParams>().bindAsProj<&CameraParams::jitterProjTm>();
    registry.read("viewtm_no_offset").blob<TMatrix>().bindAsView();

    const auto mainViewResolution = registry.getResolution<2>("main_view");

    return [aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, mainViewResolution]() {
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      const IPoint2 res = mainViewResolution.get();
      d3d::setview(0, 0, res.x, res.y, 0.0, 0.0f);
      const bool byMask = true;
      render_scope_lens_hole(scopeAimRenderDataHndl.ref(), byMask, strmCtxHndl.ref());
      d3d::setview(0, 0, res.x, res.y, 0, 1);
    };
  });
}

dabfg::NodeHandle makeRenderLensFrameNode()
{
  return dabfg::register_node("render_lens_frame_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().color({registry.createTexture2d("lens_frame_target", dabfg::History::No,
      {get_frame_render_target_format() | TEXCF_RTARGET, registry.getResolution<2>("post_fx")})});

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();

    auto frameHndl =
      registry.readTexture("frame_after_distortion").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    auto lensSourceHndl =
      registry.readTexture("lens_source_target").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).optional().handle();

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    auto postfxResolution = registry.getResolution<2>("post_fx");

    return [postfxResolution, aimRenderDataHndl, scopeAimRenderDataHndl, frameHndl, lensSourceHndl, strmCtxHndl]() {
      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderDataHndl.ref();
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      const auto &res = postfxResolution.get();
      ShaderGlobal::set_color4(screen_pos_to_texcoordVarId, 1.0f / res.x, 1.0f / res.y, 0, 0);

      render_lens_frame(scopeAimRenderData, frameHndl.d3dResId(), lensSourceHndl.d3dResId(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle makeRenderOpticsPrepassNode()
{
  return dabfg::register_node("scope_prepass_optics_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");

    dabfg::Texture2dCreateInfo cInfo;
    cInfo.creationFlags = get_gbuffer_depth_format() | TEXCF_RTARGET;
    cInfo.resolution = registry.getResolution<2>("post_fx");
    auto depthForOptics = registry.createTexture2d("aim_scope_optics_depth", dabfg::History::No, cInfo);
    registry.requestRenderPass().depthRw(depthForOptics).clear(depthForOptics, make_clear_value(0.0f, 0.0f));

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.read("current_camera")
      .blob<CameraParams>()
      .bindAsProj<&CameraParams::jitterProjTm>()
      .bindAsView<&CameraParams::viewRotTm>();

    auto hasOpticsPrepass = registry.createBlob<bool>("has_aim_scope_optics_prepass", dabfg::History::No).handle();
    auto mainViewRes = registry.getResolution<2>("main_view");
    auto postfxRes = registry.getResolution<2>("post_fx");

    return [hasOpticsPrepass, mainViewRes, postfxRes, aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl]() {
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      if (mainViewRes.get() == postfxRes.get())
      {
        hasOpticsPrepass.ref() = false;
        return;
      }

      render_scope_lens_prepass(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
      hasOpticsPrepass.ref() = true;
    };
  });
}

dabfg::NodeHandle makeRenderLensOpticsNode()
{
  return dabfg::register_node("render_lens_optics_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto hasOpticsPrepass = registry.readBlob<bool>("has_aim_scope_optics_prepass").handle();
    auto opticsPrepassDepthHndl = registry.modifyTexture("aim_scope_optics_depth")
                                    .atStage(dabfg::Stage::PRE_RASTER)
                                    .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                                    .handle();
    auto depthAfterTransparentsHndl = registry.readTexture("depth_after_transparency")
                                        .atStage(dabfg::Stage::PRE_RASTER)
                                        .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                                        .handle();
    auto frameHndl =
      registry.modifyTexture("frame_for_postfx").atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle();

    registry.readTexture("lens_frame_target").atStage(dabfg::Stage::PS).bindToShaderVar("lens_frame_tex");
    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [hasOpticsPrepass, opticsPrepassDepthHndl, depthAfterTransparentsHndl, frameHndl, aimRenderDataHndl, scopeAimRenderDataHndl,
             strmCtxHndl, fadingRenderer = PostFxRenderer("solid_color_shader")]() {
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      d3d::set_render_target(frameHndl.get(), 0);
      if (hasOpticsPrepass.ref())
        d3d::set_depth(opticsPrepassDepthHndl.view().getTex2D(), DepthAccess::RW);
      else
        d3d::set_depth(depthAfterTransparentsHndl.view().getTex2D(), DepthAccess::SampledRO);

      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderDataHndl.ref();
      render_lens_optics(scopeAimRenderData, strmCtxHndl.ref(), fadingRenderer);
    };
  });
}

dabfg::NodeHandle makeRenderCrosshairNode()
{
  return dabfg::register_node("render_scope_crosshair_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().color({"lens_frame_target"});

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.readBlob<CameraParams>("current_camera")
      .bindAsView<&CameraParams::viewRotTm>()
      .bindAsProj<&CameraParams::noJitterProjTm>();

    return [scopeAimRenderDataHndl, aimRenderDataHndl, strmCtxHndl]() {
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope_trans_forward(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}
