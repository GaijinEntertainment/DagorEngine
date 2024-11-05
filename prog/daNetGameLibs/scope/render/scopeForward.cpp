// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scopeAimRender.h"
#include "scopeMobileNodes.h"

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_info.h>

#include <render/world/defaultVrsSettings.h>
#include <render/world/frameGraphHelpers.h>

#include <shaders/dag_computeShaders.h>
#include <util/dag_convar.h>


extern ConVarT<bool, false> vrs_dof;

#define SCOPE_VRS_MASK_VARS  \
  VAR(scope_vrs_mask_rate)   \
  VAR(scope_vrs_mask_tile_x) \
  VAR(scope_vrs_mask_tile_y)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
SCOPE_VRS_MASK_VARS
#undef VAR

dabfg::NodeHandle mk_scope_prepass_forward_node()
{
  return dabfg::register_node("scope_prepass_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().depthRw("depth_for_opaque");

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [aimDataHndl, scopeAimDataHndl, strmCtxHndl] {
      if (!aimDataHndl.ref().lensRenderEnabled)
        return;

      render_scope_lens_prepass(scopeAimDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle mk_scope_forward_node()
{
  return dabfg::register_node("scope_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("scope_prepass_forward");
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().color({"target_for_opaque"}).depthRw("depth_for_opaque");

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [aimDataHndl, scopeAimDataHndl, strmCtxHndl] {
      if (!aimDataHndl.ref().lensRenderEnabled)
        return;

      render_scope(scopeAimDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle mk_scope_lens_mask_forward_node()
{
  return dabfg::register_node("scope_lens_mask_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_WRITE_DISABLE);

    registry.requestState().allowWireframe().setFrameBlock("global_frame").enableOverride(overrideState);
    registry.requestRenderPass()
      .color({registry.create("scope_lens_mask", dabfg::History::No)
                .texture({TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})})
      .depthRoAndBindToShaderVars("depth_for_opaque", {"depth_gbuf"});
    registry.create("scope_lens_sampler", dabfg::History::No).blob(d3d::request_sampler({}));

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [aimDataHndl, scopeAimDataHndl, strmCtxHndl] {
      if (!aimDataHndl.ref().lensRenderEnabled)
        return;

      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      render_scope_lens_mask(scopeAimDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle mk_scope_vrs_mask_forward_node()
{
  if (!d3d::get_driver_desc().caps.hasVariableRateShadingTexture)
    return dabfg::NodeHandle();

  return dabfg::register_node("scope_vrs_mask_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("scope_lens_mask_forward");
    registry.orderMeBefore("scope_lens_hole_forward_node");

    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.readTexture("scope_lens_mask").atStage(dabfg::Stage::COMPUTE).bindToShaderVar("scope_lens_mask");
    registry.read("scope_lens_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("scope_lens_mask_samplerstate");

    auto scopeVrsHndl =
      registry.create("scope_vrs_mask_tex", dabfg::History::No)
        .texture({TEXFMT_R8UI | TEXCF_VARIABLE_RATE | TEXCF_UNORDERED, registry.getResolution<2>("texel_per_vrs_tile")})
        .atStage(dabfg::Stage::COMPUTE)
        .useAs(dabfg::Usage::SHADER_RESOURCE)
        .handle();

    auto depthHndl =
      registry.readTexture("depth_for_opaque").atStage(dabfg::Stage::COMPUTE).useAs(dabfg::Usage::DEPTH_ATTACHMENT).handle();

    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();

    ShaderGlobal::set_int(scope_vrs_mask_rateVarId, eastl::to_underlying(VRSRATE::RATE_2x2));
    ShaderGlobal::set_int(scope_vrs_mask_tile_xVarId, d3d::get_driver_desc().variableRateTextureTileSizeX);
    ShaderGlobal::set_int(scope_vrs_mask_tile_yVarId, d3d::get_driver_desc().variableRateTextureTileSizeY);

    return [aimDataHndl, scopeVrsHndl, depthHndl, scopeVRSMaskCS = Ptr(new_compute_shader("scope_vrs_mask_cs", false))] {
      const auto &aimData = aimDataHndl.ref();
      if (!aimData.lensRenderEnabled || !vrs_dof)
        return;

      prepare_scope_vrs_mask(scopeVRSMaskCS, scopeVrsHndl.get(), depthHndl.view().getTexId());
    };
  });
}

dabfg::NodeHandle mk_scope_lens_hole_forward_node()
{
  return dabfg::register_node("scope_lens_hole_forward_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_FUNC);
    overrideState.zFunc = CMPF_ALWAYS;

    registry.requestState().allowWireframe().setFrameBlock("global_frame").enableOverride(overrideState);
    registry.requestRenderPass()
      .depthRw("depth_opaque_static")
      // Clear material to prevent rest of the scope and weapon geometry behind lens
      // be treated as dynamic objects (as logically we have sky behind the lens after this pass).
      .color({registry.modifyTexture("material_gbuf_tex").optional()});

    registry.readTexture("scope_lens_mask").atStage(dabfg::Stage::PS).bindToShaderVar("scope_lens_mask");
    registry.read("scope_lens_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("scope_lens_mask_samplerstate");

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    const auto renderingResolution = registry.getResolution<2>("main_view");

    return [aimDataHndl, scopeAimDataHndl, strmCtxHndl, renderingResolution] {
      if (!aimDataHndl.ref().lensRenderEnabled)
        return;

      const IPoint2 res = renderingResolution.get();
      d3d::setview(0, 0, res.x, res.y, 0, 0);
      const bool byMask = true;
      render_scope_lens_hole(scopeAimDataHndl.ref(), byMask, strmCtxHndl.ref());
      d3d::setview(0, 0, res.x, res.y, 0, 1);
    };
  });
}