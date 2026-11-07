// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/scopeAimRender/scopeAimRender.h>
#include "scopeFullDeferredNodes.h"
#include <render/compressedRtOverrideFlag.h>

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
#include <drv/3d/dag_matricesAndPerspective.h>
#include <shaders/dag_dynSceneRes.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_occlusionZBuffer.h>
#include <render/dynmodelRenderer.h>

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

class DynamicPrevViewProjScope
{
public:
  DynamicPrevViewProjScope(const TMatrix4_vec4 &prev_view_tm, const TMatrix4_vec4 &prev_proj_tm)
  {
    dynrend::get_prev_view_proj(prevViewTm, prevProjTm);
    dynrend::set_prev_view_proj(prev_view_tm, prev_proj_tm);
  }
  DynamicPrevViewProjScope(const CameraParams &camera, const CameraParams &prev_camera) :
    DynamicPrevViewProjScope(prev_camera.viewRotTm, get_prev_proj_tm_with_cur_jitter(prev_camera, camera))
  {}
  DynamicPrevViewProjScope(const DynamicPrevViewProjScope &) = delete;
  DynamicPrevViewProjScope &operator=(const DynamicPrevViewProjScope &) = delete;
  ~DynamicPrevViewProjScope() { dynrend::set_prev_view_proj(prevViewTm, prevProjTm); }

private:
  TMatrix4_vec4 prevViewTm;
  TMatrix4_vec4 prevProjTm;
};

template <bool use_jittered_proj = true>
static auto request_common_scope_state(dafg::Registry registry)
{
  static constexpr auto ProjBind = use_jittered_proj ? &CameraParams::jitterProjTm : &CameraParams::noJitterProjTm;

  auto camera =
    registry.readBlob<CameraParams>("current_cockpit_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<ProjBind>().handle();
  auto prevCamera = registry.readBlobHistory<CameraParams>("current_cockpit_camera").handle();

  auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
  auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
  auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

  auto tuple = eastl::make_tuple(aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera);
  return eastl::make_unique<decltype(tuple)>(tuple);
}


dafg::NodeHandle makeScopePrepassNode()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("scope_prepass_opaque_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("hidden_animchar_nodes_token");

    registry.requestState().setFrameBlock("global_frame");
    render_to_gbuffer_prepass(registry);

    auto scopeDataHndls = request_common_scope_state(registry);

    return [state = eastl::move(scopeDataHndls)]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

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

    return [state = eastl::move(scopeDataHndls)]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

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
    registry.requestRenderPass().color({"target_for_transparency"}).depthReadTestOnly("depth_for_transparency");

    auto scopeDataHndls = request_common_scope_state(registry);

    return [state = eastl::move(scopeDataHndls)]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

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

    registry.create("scope_lens_mask")
      .texture({TEXFMT_R8 | TEXCF_RTARGET | COMPRESSED_RT_OVERRIDE_FLAG, registry.getResolution<2>("main_view")})
      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_WRITE_DISABLE);
    registry.requestState().enableOverride(overrideState).setFrameBlock("global_frame");

    registry.requestRenderPass().color({"scope_lens_mask"}).depth("gbuf_depth");

    auto scopeDataHndls = request_common_scope_state(registry);

    return [state = eastl::move(scopeDataHndls)]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

      extern void scope_fire_debug_assert(const AimRenderingData &aimRenderData);
      scope_fire_debug_assert(aimRenderDataHndl.ref());

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
    registry.create("scope_lens_hzb_mask")
      .texture({TEXFMT_R32F | TEXCF_RTARGET, IPoint2{OcclusionZBuffer::WIDTH, OcclusionZBuffer::HEIGHT}})
      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

    registry.requestRenderPass().color({"scope_lens_hzb_mask"});
    registry.requestState().setFrameBlock("global_frame");

    auto scopeDataHndls = request_common_scope_state(registry);

    return [state = eastl::move(scopeDataHndls)]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

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

          prepare_scope_vrs_mask(scopeVRSMaskCS, vrsTextureHndl.get(), depthGbufHndl.get());
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
    // registry.requestRenderPass().depth(registry.renameTexture(depth_name, depth_rename_to));
    auto depthHndl = registry.renameTexture(depth_name, depth_rename_to)
                       .atStage(dafg::Stage::POST_RASTER)
                       .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                       .handle();

    registry.requestState().setFrameBlock("global_frame").enableOverride(stateOverride);

    auto scopeDataHndls = request_common_scope_state(registry);

    return [state = eastl::move(scopeDataHndls), depthHndl]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      d3d::set_render_target({depthHndl.view().getBaseTex(), 0, 0}, DepthAccess::RW, {});
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
    registry.create("dynmodel_global_state_access_optional_token").blob<OrderingToken>();

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
    registry.requestRenderPass().color({registry.modify("gbuf_2_done").texture().optional()}).depth("gbuf_depth_done");

    registry.readTexture("scope_lens_mask").atStage(dafg::Stage::PS).bindToShaderVar("scope_lens_mask");

    auto scopeDataHndls = request_common_scope_state(registry);

    const auto mainViewResolution = registry.getResolution<2>("main_view");

    return [state = eastl::move(scopeDataHndls), mainViewResolution, writeCamcamStencilMask]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

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
    registry.requestRenderPass().color(
      {registry
          .createTexture2d("lens_frame_target",
            {get_frame_render_target_format() | TEXCF_RTARGET | COMPRESSED_RT_OVERRIDE_FLAG, registry.getResolution<2>("post_fx")})
          .clear(make_clear_value(0.f, 0.f, 0.f, 0.f))});

    auto frameHndl =
      registry.readTexture("final_target_with_motion_blur").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto lensSourceHndl =
      registry.readTexture("lens_source_target").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();

    auto scopeDataHndls = request_common_scope_state(registry);

    auto postfxResolution = registry.getResolution<2>("post_fx");

    auto hasThermalRender = registry.readBlob<OrderingToken>("thermal_spectre_rendered").optional().handle();

    return [state = eastl::move(scopeDataHndls), postfxResolution, frameHndl, lensSourceHndl, hasThermalRender]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderDataHndl.ref();
      if (!aimRenderDataHndl.ref().lensRenderEnabled || hasThermalRender.get())
        return;

      const auto &res = postfxResolution.get();
      ShaderGlobal::set_float4(screen_pos_to_texcoordVarId, 1.0f / res.x, 1.0f / res.y, 0, 0);

      render_lens_frame(scopeAimRenderData, frameHndl.view().getBaseTex(), lensSourceHndl.view().getBaseTex(), strmCtxHndl.ref());
    };
  });
}

dafg::NodeHandle makeRenderOpticsPrepassNode()
{
  return dafg::register_node("scope_prepass_optics_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("hidden_animchar_nodes_token");
    registry.requestState().setFrameBlock("global_frame");

    dafg::Texture2dCreateInfo cInfo;
    cInfo.creationFlags = get_gbuffer_depth_format() | TEXCF_RTARGET;
    cInfo.resolution = registry.getResolution<2>("post_fx");
    auto depthForOpticsHandle = registry.createTexture2d("aim_scope_optics_depth", cInfo)
                                  .atStage(dafg::Stage::POST_RASTER)
                                  .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                                  .handle();

    const bool noJitter = false;
    auto scopeDataHndls = request_common_scope_state<noJitter>(registry);

    auto hasOpticsPrepass = registry.createBlob<bool>("has_aim_scope_optics_prepass").handle();
    auto mainViewRes = registry.getResolution<2>("main_view");
    auto postfxRes = registry.getResolution<2>("post_fx");

    return [state = eastl::move(scopeDataHndls), hasOpticsPrepass, mainViewRes, postfxRes, depthForOpticsHandle]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

      d3d::set_render_target({depthForOpticsHandle.view().getTex2D(), 0, 0}, DepthAccess::RW, {});
      d3d::clearview(CLEAR_ZBUFFER, E3DCOLOR(0, 0, 0), 0.f, 0u);

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

static TMatrix4 calc_lens_centered_view(
  const TMatrix4 &view_rot_tm, const TMatrix &lens_wtm, const float magnification, const float scope_length)
{
  // here we fake the gun inside the lens area
  // For this:
  // 1. Turn the gun about the lens center until its optical axis points at the eye.
  // 2. Sway puts the lens off the screen center, and the magnification pushes the magnified lens center
  //    further out, so turn the camera until it lands on the cockpit lens center.

  const Point3 lensPos = lens_wtm.getcol(3);
  const Point3 lensAxis = lens_wtm.getcol(1);
  const Point3 opticalAxis = lensAxis * lensPos > 0.f ? lensAxis : -lensAxis;
  TMatrix4 alignOpticalAxisToEyeTm = makeTM4(quat_rotation_arc(opticalAxis, lensPos));
  alignOpticalAxisToEyeTm.setrow(3, Point4::xyz1(lensPos - lensPos % alignOpticalAxisToEyeTm - normalize(lensPos) * scope_length));

  const Point3 lensVs = lensPos % view_rot_tm;
  const Point3 targetVs(lensVs.x / magnification, lensVs.y / magnification, lensVs.z);
  const TMatrix4 recenterMagnifiedLensCenterTm = makeTM4(quat_rotation_arc(lensVs, targetVs));

  return alignOpticalAxisToEyeTm * view_rot_tm * recenterMagnifiedLensCenterTm;
}

static TMatrix4 calc_lens_gun_proj(const TMatrix4 &lens_proj_tm, const CameraParams &cockpit_view, float magnification)
{
  TMatrix4 projTm = lens_proj_tm;
  projTm(0, 0) = cockpit_view.noJitterPersp.wk * magnification;
  projTm(1, 1) = cockpit_view.noJitterPersp.hk * magnification;
  return projTm;
}

dafg::NodeHandle makeScopeLensViewProjNode(const bool align_gun)
{
  return dafg::register_node("scope_lens_view_proj_node", DAFG_PP_NODE_SRC, [align_gun](dafg::Registry registry) {
    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto lensCamera = registry.readBlob<CameraParams>("lens_area_camera").optional().handle();
    auto cockpitCamera = registry.readBlob<CameraParams>("current_cockpit_camera").handle();
    auto mainCamera = registry.readBlob<CameraParams>("current_camera").handle();

    auto gunViewHndl = registry.createBlob<TMatrix4>("scope_gun_view").withHistory().handle();
    auto gunProjHndl = registry.createBlob<TMatrix4>("scope_gun_proj").handle();
    auto gunNoJitterProjHndl = registry.createBlob<TMatrix4>("scope_gun_no_jitter_proj").withHistory().handle();
    auto crosshairViewHndl = registry.createBlob<TMatrix4>("scope_crosshair_view").withHistory().handle();

    auto tuple = eastl::make_tuple(aimRenderDataHndl, lensCamera, cockpitCamera, mainCamera, gunViewHndl, gunProjHndl,
      gunNoJitterProjHndl, crosshairViewHndl);
    auto viewProjHndls = eastl::make_unique<decltype(tuple)>(tuple);

    return [align_gun, state = eastl::move(viewProjHndls)]() {
      const auto &[aimRenderDataHndl, lensCamera, cockpitCamera, mainCamera, gunViewHndl, gunProjHndl, gunNoJitterProjHndl,
        crosshairViewHndl] = *state;
      const AimRenderingData &aimRenderData = aimRenderDataHndl.ref();

      if (!aimRenderData.lensRenderEnabled)
        return;

      const DynamicRenderableSceneInstance *lensScene = get_scope_lens(aimRenderData);
      if (!lensScene)
        return;

      const CameraParams *lensView = lensCamera.get() ? lensCamera.get() : mainCamera.get();

      if (!align_gun)
      {
        gunViewHndl.ref() = lensView->viewRotTm;
        gunProjHndl.ref() = lensView->jitterProjTm;
        gunNoJitterProjHndl.ref() = lensView->noJitterProjTm;
        crosshairViewHndl.ref() = cockpitCamera.ref().viewRotTm;
        return;
      }

      const TMatrix &lensWtm = lensScene->getNodeWtm(aimRenderData.lensNodeId);
      const float magnification = lensView->noJitterPersp.wk / mainCamera.ref().noJitterPersp.wk;

      const float scopeFakeLengthMul = 0.85f;
      const float scopeLength = aimRenderData.scopeLength * scopeFakeLengthMul;

      gunViewHndl.ref() = calc_lens_centered_view(lensView->viewRotTm, lensWtm, magnification, scopeLength);
      gunProjHndl.ref() = calc_lens_gun_proj(lensView->jitterProjTm, cockpitCamera.ref(), magnification);
      gunNoJitterProjHndl.ref() = calc_lens_gun_proj(lensView->noJitterProjTm, cockpitCamera.ref(), magnification);
      crosshairViewHndl.ref() = calc_lens_centered_view(cockpitCamera.ref().viewRotTm, lensWtm, 1.f, 0.f);
    };
  });
}

static dafg::NodeHandle makeScopeGunOpaqueNode(const bool prepass)
{
  auto ns = dafg::root() / "opaque" / "decorations";
  const char *nodeName = prepass ? "scope_opaque_gun_prepass_node" : "scope_opaque_gun_node";
  return ns.registerNode(nodeName, DAFG_PP_NODE_SRC, [prepass](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    registry.read("shadow_visibility_token").blob<OrderingToken>();
    registry.read("hidden_animchar_nodes_token").blob<OrderingToken>().optional();

    if (prepass)
      render_to_gbuffer_prepass(registry);
    else
      render_to_gbuffer(registry);

    registry.readBlob<TMatrix4>("scope_gun_view").bindAsView();
    auto gunProjHndl = registry.readBlob<TMatrix4>("scope_gun_proj").bindAsProj().handle();
    auto gunNoJitterProjHndl = registry.readBlob<TMatrix4>("scope_gun_no_jitter_proj").handle();
    auto prevGunViewHndl = registry.readBlobHistory<TMatrix4>("scope_gun_view").handle();
    auto prevGunNoJitterProjHndl = registry.readBlobHistory<TMatrix4>("scope_gun_no_jitter_proj").handle();

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);

    return [prepass, aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, gunProjHndl, gunNoJitterProjHndl, prevGunViewHndl,
             prevGunNoJitterProjHndl](const dafg::multiplexing::Index &multiplexing_index) {
      if (camera_in_camera::is_main_view(multiplexing_index))
        return;

      camera_in_camera::ApplyMasterState camcam(multiplexing_index);

      const AimRenderingData &aimRenderData = aimRenderDataHndl.ref();

      if (!aimRenderData.lensRenderEnabled)
        return;

      const DynamicRenderableSceneInstance *lensScene = get_scope_lens(aimRenderData);
      if (!lensScene)
        return;

      DynamicPrevViewProjScope prevViewProjScope(prevGunViewHndl.ref(),
        get_prev_proj_tm_with_cur_jitter(prevGunNoJitterProjHndl.ref(), gunProjHndl.ref(), gunNoJitterProjHndl.ref()));

      const bool gunOnly = true;
      if (prepass)
        render_scope_prepass(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref(), gunOnly);
      else
        render_scope(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref(), gunOnly);
    };
  });
}

void makeScopeGunOpaqueNodes(dag::Vector<dafg::NodeHandle> &nodes)
{
  nodes.push_back(makeScopeGunOpaqueNode(true));
  nodes.push_back(makeScopeGunOpaqueNode(false));
}

template <typename T>
using BlobHandle = dafg::VirtualResourceHandle<T, false, false>;

template <typename T>
using OptionalBlobHandle = dafg::VirtualResourceHandle<T, false, true>;

dafg::NodeHandle makeRenderLensOpticsNode()
{
  // workaround for intel vk arc/xe issue - ignored memory barrier for RW DEPTH - RW DEPTH, force different layouts
  bool useROdepth = d3d::get_driver_desc().info.vendor == GpuVendor::INTEL;
  return dafg::register_node("render_lens_optics_node", DAFG_PP_NODE_SRC, [useROdepth](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto hasOpticsPrepass = registry.readBlob<bool>("has_aim_scope_optics_prepass").handle();
    // todo: disable blur instead when thermal is active
    auto hasThermalRender = registry.readBlob<OrderingToken>("thermal_spectre_rendered").optional().handle();

    auto opticsPrepassDepthHndl =
      registry.modifyTexture("aim_scope_optics_depth").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();
    auto depthAfterTransparentsHndl = registry.readTexture("depth_after_transparency")
                                        .atStage(dafg::Stage::POST_RASTER)
                                        .useAs(dafg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                        .handle();
    auto frameHndl =
      registry.modifyTexture("frame_for_postfx").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

    registry.readTexture("lens_crosshair_gui_target").atStage(dafg::Stage::PS).bindToShaderVar("lens_frame_tex");

    request_and_bind_scope_lens_reflections(registry);

    const bool noJitter = false;
    auto scopeDataHndls = request_common_scope_state<noJitter>(registry);

    struct BlobsToInit
    {
      BlobHandle<const bool> hasOpticsPrepass;
      OptionalBlobHandle<const OrderingToken> hasThermalRender;
      bool useROdepth;
    };
    auto extraBlobs = eastl::shared_ptr<BlobsToInit>(new BlobsToInit{hasOpticsPrepass, hasThermalRender, useROdepth});

    return [state = eastl::move(scopeDataHndls), bs = std::move(extraBlobs), opticsPrepassDepthHndl, depthAfterTransparentsHndl,
             frameHndl, fadingRenderer = PostFxRenderer("solid_color_shader")]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());

      if (!aimRenderDataHndl.ref().lensRenderEnabled)
        return;

      if (bs->hasThermalRender.get())
        d3d::clear_rt({frameHndl.get()}, make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));

      if (bs->hasOpticsPrepass.ref())
        d3d::set_render_target({opticsPrepassDepthHndl.view().getTex2D(), 0, 0},
          bs->useROdepth ? DepthAccess::SampledRO : DepthAccess::RW, {{frameHndl.get(), 0, 0}});
      else
        d3d::set_render_target({depthAfterTransparentsHndl.view().getTex2D(), 0, 0}, DepthAccess::SampledRO,
          {{frameHndl.get(), 0, 0}});

      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderDataHndl.ref();
      render_lens_optics(scopeAimRenderData, aimRenderDataHndl.ref().lensBoundingSphereRadius, strmCtxHndl.ref(), fadingRenderer);
    };
  });
}

dafg::NodeHandle makeRenderCrosshairNode()
{
  return dafg::register_node("render_scope_crosshair_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().color({registry.renameTexture("lens_frame_target", "lens_crosshair_target")});

    registry.readBlob<TMatrix4>("scope_crosshair_view").bindAsView();
    auto prevCrosshairViewHndl = registry.readBlobHistory<TMatrix4>("scope_crosshair_view").handle();
    auto camera = registry.readBlob<CameraParams>("current_cockpit_camera").bindAsProj<&CameraParams::noJitterProjTm>().handle();
    auto prevCamera = registry.readBlobHistory<CameraParams>("current_cockpit_camera").handle();

    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera, prevCrosshairViewHndl]() {
      DynamicPrevViewProjScope prevViewProjScope(prevCrosshairViewHndl.ref(),
        get_prev_proj_tm_with_cur_jitter(prevCamera.ref(), camera.ref()));

      const AimRenderingData &aimRenderData = aimRenderDataHndl.ref();
      if (!aimRenderData.lensRenderEnabled)
        return;

      const DynamicRenderableSceneInstance *lensScene = get_scope_lens(aimRenderData);
      if (!lensScene)
        return;

      render_scope_trans_forward(scopeAimRenderDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dafg::NodeHandle makeRenderCrosshairGUINode()
{
  return dafg::register_node("render_scope_crosshair_gui_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock(nullptr);
    registry.requestRenderPass().color({registry.renameTexture("lens_crosshair_target", "lens_crosshair_gui_target")});

    auto camera = registry.readBlob<CameraParams>("current_cockpit_camera").handle();
    auto crosshairViewHndl = registry.readBlob<TMatrix4>("scope_crosshair_view").handle();
    auto aimRenderDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto viewportResHndl = registry.getResolution<2>("main_view");

    return [aimRenderDataHndl, camera, crosshairViewHndl, viewportResHndl]() {
      const AimRenderingData &aimRenderData = aimRenderDataHndl.ref();
      if (!aimRenderData.lensRenderEnabled)
        return;

      const DynamicRenderableSceneInstance *lensScene = get_scope_lens(aimRenderData);
      if (!lensScene)
        return;

      CameraParams crosshairCamera = camera.ref();
      crosshairCamera.viewRotProjTm = crosshairViewHndl.ref() * camera.ref().noJitterProjTm;

      render_scope_crosshair_gui(aimRenderData, crosshairCamera, viewportResHndl.get());
    };
  });
}

constexpr int reflection_tex_size = 128;

dafg::NodeHandle makeRenderReflectionsNode()
{
  const bool supportsAlternateReflections =
    dgs_get_settings()->getBlockByNameEx("graphics")->getBool("scopeHasOutdoorReflections", false);

  if (!supportsAlternateReflections)
    return {};

  return dafg::register_node("render_scope_reflections_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    auto rt = registry
                .createTexture2d("scope_lens_reflections",
                  {TEXFMT_R11G11B10F | TEXCF_RTARGET, IPoint2(reflection_tex_size, reflection_tex_size)})
                .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

    registry.create("scope_lens_reflections_sampler").blob(d3d::request_sampler({}));

    registry.requestRenderPass().color({rt});

    auto scopeDataHndls = request_common_scope_state(registry);
    registry.readBlob<AimRenderingData>("aim_render_data")
      .bindToShaderVar<&AimRenderingData::lensBoundingSphereRadius>("scope_lens_bounding_sphere_radius")
      .bindToShaderVar<&AimRenderingData::aimingTime>("scope_reflection_head_visibility");

    Ptr<ShaderMaterial> mat{new_shader_material_by_name("scope_lens_reflections")};
    Ptr<ShaderElement> elem;
    if (mat)
      elem = mat->make_elem();
    else
      logerr("Render Issue: render_scope_reflections_node: scope_lens_reflections shader is not found");

    auto invTexelSizeHndl = registry.createBlob<float>("inv_scope_lens_reflection_texel_size").handle();

    return [_ = eastl::move(mat), elemStorage = eastl::move(elem), state = eastl::move(scopeDataHndls), invTexelSizeHndl]() {
      const auto &[aimRenderDataHndl, scopeAimRenderDataHndl, strmCtxHndl, camera, prevCamera] = *state;
      DynamicPrevViewProjScope prevViewProjScope(camera.ref(), prevCamera.ref());
      invTexelSizeHndl.ref() = 1.0 / static_cast<float>(reflection_tex_size);

      const AimRenderingData &aimRenderData = aimRenderDataHndl.ref();
      if (!aimRenderData.entityWithScopeLensEid || !elemStorage)
        return;

      render_scope(aimRenderData.entityWithScopeLensEid, aimRenderData.lensNodeId, strmCtxHndl.ref(), elemStorage.get());
    };
  });
}
