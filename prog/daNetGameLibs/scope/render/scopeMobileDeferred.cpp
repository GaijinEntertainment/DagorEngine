// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scopeAimRender.h"
#include "scopeMobileNodes.h"

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_interface_table.h>
#include <render/deferredRenderer.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/gbufferConsts.h>

dabfg::NodeHandle mk_scope_begin_rp_mobile_node(d3d::RenderPass *opaqueWithScopeRenderPass)
{
  return dabfg::register_node("scope_begin_rp_mobile", DABFG_PP_NODE_SRC, [opaqueWithScopeRenderPass](dabfg::Registry registry) {
    dag::Vector<dabfg::VirtualResourceHandle<BaseTexture, true, false>> renderPassHndls;

    const bool simplified = renderer_has_feature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS);
    const size_t gbufCount = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_COUNT : MOBILE_GBUFFER_RT_COUNT;
    const auto *gbufNames = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES.data() : MOBILE_GBUFFER_RT_NAMES.data();

    auto appendRT = [&renderPassHndls, &registry](const char *name) {
      renderPassHndls.push_back(
        registry.modify(name).texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle());
    };

    for (size_t i = 0; i < gbufCount; ++i)
      appendRT(gbufNames[i]);

    appendRT("gbuf_depth_for_opaque");
    appendRT("target_for_resolve");

    const auto mainViewResolution = registry.getResolution<2>("main_view");
    renderPassHndls.push_back(registry.create("scope_lens_mask", dabfg::History::No)
                                .texture({(uint32_t)get_scope_mask_format() | TEXCF_RTARGET, mainViewResolution})
                                .atStage(dabfg::Stage::PS)
                                .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                .handle());
    registry.create("scope_lens_sampler", dabfg::History::No).blob(d3d::request_sampler({}));

    registry.requestState().setFrameBlock("global_frame");

    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    return [renderPassHndls, aimDataHndl, mainViewResolution, opaqueWithScopeRenderPass] {
      if (!aimDataHndl.ref().lensRenderEnabled)
        return;

      eastl::fixed_vector<RenderPassTarget, 6> targets;
      for (size_t i = 0; i < renderPassHndls.size(); ++i)
        targets.push_back({{renderPassHndls[i].get(), 0, 0}, make_clear_value(0, 0, 0, 0)});

      const IPoint2 res = mainViewResolution.get();
      d3d::begin_render_pass(opaqueWithScopeRenderPass, {0, 0, (uint32_t)res.x, (uint32_t)res.y, 0.0f, 1.0f}, targets.data());
    };
  });
}

dabfg::NodeHandle mk_scope_prepass_mobile_node()
{
  return dabfg::register_node("scope_prepass_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.read("gbuf_depth_for_opaque").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT);

    registry.requestState().setFrameBlock("global_frame");

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

dabfg::NodeHandle mk_scope_mobile_node()
{
  return dabfg::register_node("scope_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("scope_prepass_mobile");

    const bool simplified = renderer_has_feature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS);
    const size_t gbufCount = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_COUNT : MOBILE_GBUFFER_RT_COUNT;
    const auto *gbufNames = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES.data() : MOBILE_GBUFFER_RT_NAMES.data();

    for (size_t i = 0; i < gbufCount; ++i)
      registry.modify(gbufNames[i]).texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT);

    registry.read("gbuf_depth_for_opaque").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    return [aimDataHndl, scopeAimDataHndl, strmCtxHndl] {
      if (!aimDataHndl.ref().lensRenderEnabled)
        return;

      d3d::next_subpass();

      render_scope(scopeAimDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle mk_scope_lens_mask_mobile_node()
{
  return dabfg::register_node("scope_lens_mask_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("scope_mobile");
    registry.read("gbuf_depth_for_opaque").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT);
    registry.modify("scope_lens_mask").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT);

    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_WRITE_DISABLE);
    registry.requestState().enableOverride(overrideState).setFrameBlock("global_frame");

    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    return [aimDataHndl, scopeAimDataHndl, strmCtxHndl] {
      if (!aimDataHndl.ref().lensRenderEnabled)
        return;

      d3d::next_subpass();

      render_scope_lens_mask(scopeAimDataHndl.ref(), strmCtxHndl.ref());
    };
  });
}

dabfg::NodeHandle mk_scope_depth_cut_mobile_node()
{
  return dabfg::register_node("scope_depth_cut_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("scope_lens_mask_mobile");
    registry.orderMeBefore("opaque_mobile");
    registry.modify("scope_lens_mask").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT);
    registry.read("gbuf_depth_for_opaque").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT);

    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_FUNC);
    overrideState.zFunc = CMPF_ALWAYS;

    registry.requestState().enableOverride(overrideState).setFrameBlock("global_frame");

    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    const auto mainViewResolution = registry.getResolution<2>("main_view");
    return [aimDataHndl, scopeAimDataHndl, cameraHndl, strmCtxHndl, mainViewResolution] {
      if (!aimDataHndl.ref().lensRenderEnabled)
        return;

      ScopeRenderTarget scopeRt;

      d3d::next_subpass();
      const IPoint2 res = mainViewResolution.get();
      d3d::setview(0, 0, res.x, res.y, 0.0, 0.0f);

      const bool byMask = true;
      render_scope_lens_hole(scopeAimDataHndl.ref(), byMask, strmCtxHndl.ref());

      const auto &camera = cameraHndl.ref();
      d3d::settm(TM_VIEW, camera.viewTm);
    };
  });
}
