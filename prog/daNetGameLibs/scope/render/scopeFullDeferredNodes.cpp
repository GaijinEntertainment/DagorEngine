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


#define SCOPE_VRS_MASK_VARS  \
  VAR(scope_vrs_mask_rate)   \
  VAR(scope_vrs_mask_tile_x) \
  VAR(scope_vrs_mask_tile_y)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
SCOPE_VRS_MASK_VARS
#undef VAR


dabfg::NodeHandle makeScopePrepassNode()
{
  auto ns = dabfg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_prepass_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
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
      {get_frame_render_target_format() | TEXCF_RTARGET, registry.getResolution<2>("main_view")})});

    registry.readTexture("depth_for_transparency").atStage(dabfg::Stage::PS).bindToShaderVar("depth_gbuf");
    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();

    auto frameHndl =
      registry.readTexture("target_for_transparency").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    auto lensSourceHndl =
      registry.readTexture("lens_source_target").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).optional().handle();

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [aimRenderDataHndl, scopeAimRenderDataHndl, frameHndl, lensSourceHndl, strmCtxHndl]() {
      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderDataHndl.ref();
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      if (scopeAimRenderData.nightVision)
        d3d::set_depth(nullptr, DepthAccess::RW);

      render_lens_frame(scopeAimRenderData, frameHndl.d3dResId(), lensSourceHndl.d3dResId(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle makeRenderLensOpticsNode()
{
  return dabfg::register_node("render_lens_optics_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass()
      .color({"target_before_motion_blur"})
      .depthRoAndBindToShaderVars("depth_for_transparency", {"depth_gbuf"});
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");

    registry.readTexture("lens_frame_target").atStage(dabfg::Stage::PS).bindToShaderVar("lens_frame_tex");
    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, fadingRenderer = PostFxRenderer("solid_color_shader")]() {
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderDataHndl.ref();
      if (scopeAimRenderData.nightVision)
        d3d::set_depth(nullptr, DepthAccess::RW);

      render_lens_optics(scopeAimRenderData, strmCtxHndl.ref(), fadingRenderer);
    };
  });
}

dabfg::NodeHandle makeRenderCrosshairNode()
{
  auto ns = dabfg::root() / "before_ui";
  return ns.registerNode("render_scope_crosshair_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().color({"frame"});

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    auto closeupsNs = registry.root() / "opaque" / "closeups";
    closeupsNs.readTexture("scope_lens_mask").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("scope_lens_mask").optional();

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
