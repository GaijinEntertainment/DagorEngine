// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/scopeAimRender/scopeAimRender.h>
#include "scopeFullDeferredNodes.h"

#include <render/daFrameGraph/daFG.h>
#include <render/deferredRenderer.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/defaultVrsSettings.h>
#include <shaders/dag_computeShaders.h>
#include <util/dag_convar.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <math/dag_occlusionTest.h>

#include <EASTL/shared_ptr.h>

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


static auto request_common_scope_state(dafg::Registry registry, const bool use_jittered_proj = true)
{
  auto camera = registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>();
  if (use_jittered_proj)
    eastl::move(camera).bindAsProj<&CameraParams::jitterProjTm>();
  else
    eastl::move(camera).bindAsProj<&CameraParams::noJitterProjTm>();

  auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
  auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
  auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

  return eastl::make_tuple(aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl);
}

dafg::NodeHandle makeScopePrepassNode()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_prepass_opaque_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");
    render_to_gbuffer_prepass(registry);

    auto scopeDataHndls = request_common_scope_state(registry);

    return [scopeDataHndls]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope_prepass(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dafg::NodeHandle makeScopeOpaqueNode()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_opaque_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    render_to_gbuffer(registry);

    auto scopeDataHndls = request_common_scope_state(registry);

    return [scopeDataHndls]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dafg::NodeHandle makeScopeTransNode()
{
  return dafg::register_node("render_scope_transparency_except_lens", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("transparent_scene_late_node");
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().color({"target_for_transparency"}).depthRo("depth_for_transparency");

    auto scopeDataHndls = request_common_scope_state(registry);

    return [scopeDataHndls]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope_trans_except_lens(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dafg::NodeHandle makeScopeLensMaskNode()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_lens_mask_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // We want all of scope to already be present in the gbuffer so as
    // to occlude the lens mask with it. Technically, this is more of a
    // read than a write, but decompressing the depth buffer is expensive.
    // TODO: consider making this a read somehow
    registry.orderMeAfter("scope_opaque_node");

    registry.create("scope_lens_mask", dafg::History::No)
      .texture({TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));
    registry.create("scope_lens_sampler", dafg::History::No).blob(d3d::request_sampler({}));

    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_WRITE_DISABLE);
    registry.requestState().enableOverride(overrideState).setFrameBlock("global_frame");

    registry.requestRenderPass().color({"scope_lens_mask"}).depthRw("gbuf_depth");

    auto scopeDataHndls = request_common_scope_state(registry);

    return [scopeDataHndls]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope_lens_mask(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dafg::NodeHandle makeScopeHZBMask()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_lens_hzb_mask_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("scope_lens_hzb_mask", dafg::History::No)
      .texture({TEXFMT_R32F | TEXCF_RTARGET, IPoint2{OCCLUSION_W, OCCLUSION_H}})
      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

    registry.requestRenderPass().color({"scope_lens_hzb_mask"});
    registry.requestState().setFrameBlock("global_frame");

    auto scopeDataHndls = request_common_scope_state(registry);

    return [scopeDataHndls]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      const bool useDepthValueAsMask = true;
      render_scope_lens_mask(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref(), useDepthValueAsMask);
    };
  });
}

dafg::NodeHandle makeScopeVrsMaskNode()
{
  const bool vrsSupported = d3d::get_driver_desc().caps.hasVariableRateShadingTexture;
  if (vrsSupported)
    return dafg::register_node("scope_vrs_mask_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      auto vrsTextureHndl =
        registry.modifyTexture(VRS_RATE_TEXTURE_NAME).atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();

      registry.requestState().setFrameBlock("global_frame");

      auto closeupsNs = registry.root() / "opaque" / "closeups";

      closeupsNs.readTexture("scope_lens_mask").atStage(dafg::Stage::COMPUTE).bindToShaderVar("scope_lens_mask");
      closeupsNs.read("scope_lens_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("scope_lens_mask_samplerstate");

      auto depthGbufHndl =
        closeupsNs.read("gbuf_depth").texture().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();

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
    return dafg::NodeHandle();
}

dafg::NodeHandle makeScopeDownsampleStencilNode(const char *node_name, const char *depth_name, const char *depth_rename_to)
{
  auto ns = dafg::root();
  return ns.registerNode(node_name, DAFG_PP_NODE_SRC, [depth_name, depth_rename_to](dafg::Registry registry) {
    shaders::OverrideState stateOverride;
    stateOverride.set(shaders::OverrideState::Z_WRITE_DISABLE);
    stateOverride.set(shaders::OverrideState::STENCIL);
    stateOverride.stencil.set(CMPF_ALWAYS, STNCLOP_KEEP, STNCLOP_KEEP, STNCLOP_REPLACE, 0xFF, 0xFF);

    // TODO: restore this when have requestRenderPass().clearStencil()
    // registry.requestRenderPass().depthRw(registry.renameTexture(depth_name, depth_rename_to, dafg::History::No));
    auto depthHndl = registry.renameTexture(depth_name, depth_rename_to, dafg::History::No)
                       .atStage(dafg::Stage::POST_RASTER)
                       .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                       .handle();

    registry.requestState().setFrameBlock("global_frame").enableOverride(stateOverride);

    auto scopeDataHndls = request_common_scope_state(registry);

    return [scopeDataHndls, depthHndl]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      d3d::set_render_target(nullptr, 0);
      d3d::set_depth(depthHndl.view().getBaseTex(), DepthAccess::RW);
      d3d::clearview(CLEAR_STENCIL, 0, 0, 0);

      const int lensAreaViewportIndex = 1;
      shaders::set_stencil_ref(lensAreaViewportIndex);

      const bool byMask = false;
      render_scope_lens_hole(scopeAimRenderDataHndl.ref(), byMask, strmCtxHndl.ref());
    };
  });
}

dafg::NodeHandle makeScopeCutDepthNode()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_cut_depth_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // Ordering token to avoid data races when running concurrently with "async_animchar_rendering_start_node"
    registry.create("dynmodel_global_state_access_optional_token", dafg::History::No).blob<OrderingToken>();

    shaders::OverrideState stateOverride;
    stateOverride.set(shaders::OverrideState::Z_FUNC);
    stateOverride.zFunc = CMPF_ALWAYS;

    const bool writeCamcamStencilMask = renderer_has_feature(FeatureRenderFlags::CAMERA_IN_CAMERA);
    if (writeCamcamStencilMask)
    {
      stateOverride.set(shaders::OverrideState::STENCIL);
      stateOverride.stencil.set(CMPF_ALWAYS, STNCLOP_KEEP, STNCLOP_KEEP, STNCLOP_REPLACE, 0xFF, 0xFF);
    }

    registry.requestState().setFrameBlock("global_frame").enableOverride(stateOverride);

    // We modify the "done" version of the gbuffer to ensure that
    // everything else has already been rendered
    registry.requestRenderPass().color({registry.modify("gbuf_2_done").texture().optional()}).depthRw("gbuf_depth_done");

    registry.readTexture("scope_lens_mask").atStage(dafg::Stage::PS).bindToShaderVar("scope_lens_mask");
    registry.read("scope_lens_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("scope_lens_mask_samplerstate");

    auto scopeDataHndls = request_common_scope_state(registry);

    const auto mainViewResolution = registry.getResolution<2>("main_view");

    return [scopeDataHndls, mainViewResolution, writeCamcamStencilMask]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      if (writeCamcamStencilMask)
      {
        const int lensAreaViewportIndex = 1;
        shaders::set_stencil_ref(lensAreaViewportIndex);
      }

      const IPoint2 res = mainViewResolution.get();
      d3d::setview(0, 0, res.x, res.y, 0.0, 0.0f);
      const bool byMask = true;
      render_scope_lens_hole(scopeAimRenderDataHndl.ref(), byMask, strmCtxHndl.ref());
      d3d::setview(0, 0, res.x, res.y, 0, 1);
    };
  });
}

dafg::NodeHandle makeRenderLensFrameNode()
{
  return dafg::register_node("render_lens_frame_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().color({registry.createTexture2d("lens_frame_target", dafg::History::No,
      {get_frame_render_target_format() | TEXCF_RTARGET, registry.getResolution<2>("post_fx")})});

    auto frameHndl =
      registry.readTexture("frame_after_distortion").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto lensSourceHndl =
      registry.readTexture("lens_source_target").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();

    auto scopeDataHndls = request_common_scope_state(registry);

    auto postfxResolution = registry.getResolution<2>("post_fx");

    return [scopeDataHndls, postfxResolution, frameHndl, lensSourceHndl]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderDataHndl.ref();
      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      const auto &res = postfxResolution.get();
      ShaderGlobal::set_color4(screen_pos_to_texcoordVarId, 1.0f / res.x, 1.0f / res.y, 0, 0);

      render_lens_frame(scopeAimRenderData, frameHndl.d3dResId(), lensSourceHndl.d3dResId(), strmCtxHndl.ref());
    };
  });
}

dafg::NodeHandle makeRenderOpticsPrepassNode()
{
  return dafg::register_node("scope_prepass_optics_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");

    dafg::Texture2dCreateInfo cInfo;
    cInfo.creationFlags = get_gbuffer_depth_format() | TEXCF_RTARGET;
    cInfo.resolution = registry.getResolution<2>("post_fx");
    auto depthForOptics =
      registry.createTexture2d("aim_scope_optics_depth", dafg::History::No, cInfo).clear(make_clear_value(0.0f, 0.0f));
    registry.requestRenderPass().depthRw(depthForOptics);

    const bool noJitter = false;
    auto scopeDataHndls = request_common_scope_state(registry, noJitter);

    auto hasOpticsPrepass = registry.createBlob<bool>("has_aim_scope_optics_prepass", dafg::History::No).handle();
    auto mainViewRes = registry.getResolution<2>("main_view");
    auto postfxRes = registry.getResolution<2>("post_fx");

    return [scopeDataHndls, hasOpticsPrepass, mainViewRes, postfxRes]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      if (mainViewRes.get() == postfxRes.get())
      {
        hasOpticsPrepass.ref() = false;
        return;
      }

      render_scope_prepass(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
      hasOpticsPrepass.ref() = true;
    };
  });
}

template <typename T>
using BlobHandle = dafg::VirtualResourceHandle<T, false, false>;

template <typename T>
using OptionalBlobHandle = dafg::VirtualResourceHandle<T, false, true>;

dafg::NodeHandle makeRenderLensOpticsNode()
{
  return dafg::register_node("render_lens_optics_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto hasOpticsPrepass = registry.readBlob<bool>("has_aim_scope_optics_prepass").handle();
    // todo: disable blur instead when thermal is active
    auto hasThermalRender = registry.readBlob<OrderingToken>("thermal_spectre_rendered").optional().handle();

    auto opticsPrepassDepthHndl =
      registry.modifyTexture("aim_scope_optics_depth").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();
    auto depthAfterTransparentsHndl =
      registry.readTexture("depth_after_transparency").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();
    auto frameHndl =
      registry.modifyTexture("frame_for_postfx").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

    registry.readTexture("lens_frame_target").atStage(dafg::Stage::PS).bindToShaderVar("lens_frame_tex");

    const bool noJitter = false;
    auto scopeDataHndls = request_common_scope_state(registry, noJitter);

    struct BlobsToInit
    {
      BlobHandle<const bool> hasOpticsPrepass;
      OptionalBlobHandle<const OrderingToken> hasThermalRender;
    };
    auto extraBlobs = eastl::shared_ptr<BlobsToInit>(new BlobsToInit{hasOpticsPrepass, hasThermalRender});

    return [scopeDataHndls, bs = std::move(extraBlobs), opticsPrepassDepthHndl, depthAfterTransparentsHndl, frameHndl,
             fadingRenderer = PostFxRenderer("solid_color_shader")]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      if (bs->hasThermalRender.get())
        d3d::clear_rt({frameHndl.get()}, make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));

      d3d::set_render_target(frameHndl.get(), 0);
      if (bs->hasOpticsPrepass.ref())
        d3d::set_depth(opticsPrepassDepthHndl.view().getTex2D(), DepthAccess::RW);
      else
        d3d::set_depth(depthAfterTransparentsHndl.view().getTex2D(), DepthAccess::SampledRO);

      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderDataHndl.ref();
      render_lens_optics(scopeAimRenderData, strmCtxHndl.ref(), fadingRenderer);
    };
  });
}

dafg::NodeHandle makeRenderCrosshairNode()
{
  return dafg::register_node("render_scope_crosshair_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().color({"lens_frame_target"});

    const bool noJitter = false;
    auto scopeDataHndls = request_common_scope_state(registry, noJitter);

    return [scopeDataHndls]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl] = scopeDataHndls;

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      render_scope_trans_forward(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}
