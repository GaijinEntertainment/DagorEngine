// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scopeAimRender.h"
#include "scopeFullDeferredNodes.h"

#include <render/daBfg/bfg.h>
#include <render/deferredRenderer.h>
#include <render/rendererFeatures.h>
#include <render/world/postfxRender.h>
#include <render/world/antiAliasingMode.h>
#include <render/world/global_vars.h>

// todo: fix dof render implementation dependencies on bloom
static bool should_prepare_aim_dof(const ScopeAimRenderingData &scopeAimData)
{
  return scopeAimData.isAiming && renderer_has_feature(FeatureRenderFlags::BLOOM);
}

dabfg::NodeHandle makeAimDofPrepareNode()
{
  // TODO: This is a very ugly hack to fix dof being stuck when taking a screenshot
  static AimDofSettings firstIterationAimData;
  return dabfg::register_node("aim_dof_prepare_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeBefore("prepare_post_fx_node");

    auto lensDofDepthHndl = registry
                              .createTexture2d("lens_dof_blend_depth_tex", dabfg::History::No,
                                {TEXFMT_DEPTH32 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
                              .atStage(dabfg::Stage::PS)
                              .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                              .handle();

    auto depthHndl =
      registry.readTexture("depth_for_transparency").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT).handle();

    auto depthWithTransparencyHndl = registry.readTexture("depth_with_transparency")
                                       .atStage(dabfg::Stage::PS)
                                       .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                                       .optional()
                                       .handle();

    registry.requestState().setFrameBlock("global_frame");
    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>();

    auto aimRenderingDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderingDataHndl =
      registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").bindAsProj<&ScopeAimRenderingData::jitterProjTm>().handle();
    auto aimDofHndl = registry.createBlob<AimDofSettings>("aimDof", dabfg::History::No).handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    auto antiAliasingModeHndl = registry.readBlob<AntiAliasingMode>("anti_aliasing_mode").handle();

    return [lensDofDepthHndl, aimRenderingDataHndl, scopeAimRenderingDataHndl, depthHndl, depthWithTransparencyHndl, aimDofHndl,
             strmCtxHndl, antiAliasingModeHndl](dabfg::multiplexing::Index multiplexing_index) {
      const AimRenderingData &aimRenderData = aimRenderingDataHndl.ref();
      const ScopeAimRenderingData &scopeAimRenderData = scopeAimRenderingDataHndl.ref();
      bool shouldRenderLensDofDepth = aimRenderData.lensRenderEnabled && should_prepare_aim_dof(scopeAimRenderData);
      auto depthView = depthWithTransparencyHndl.get() ? depthWithTransparencyHndl.view() : depthHndl.view();
      if (shouldRenderLensDofDepth)
      {
        lensDofDepthHndl.get()->update(depthView.getTex2D());
      }
      AimDofSettings aimDof;

      if (should_prepare_aim_dof(scopeAimRenderData))
        prepare_aim_dof(scopeAimRenderData, aimRenderData, aimDof, lensDofDepthHndl.get(), strmCtxHndl.ref());

      set_dof_blend_depth_tex(shouldRenderLensDofDepth ? lensDofDepthHndl.d3dResId() : depthView.getTexId());
      if (antiAliasingModeHndl.ref() == AntiAliasingMode::TSR)
        ShaderGlobal::set_int(antialiasing_typeVarId,
          shouldRenderLensDofDepth ? DofAntiAliasingType::AA_TYPE_DLSS : DofAntiAliasingType::AA_TYPE_TSR);

      bool isFirstIteration = multiplexing_index == dabfg::multiplexing::Index{};
      if (isFirstIteration)
        firstIterationAimData = aimDof;
      aimDofHndl.ref() = firstIterationAimData;
    };
  });
}

dabfg::NodeHandle makeAimDofRestoreNode()
{
  return dabfg::register_node("aim_dof_restore_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("after_post_fx_ecs_event_node");

    auto aimDofSettingsHndl = registry.readBlob<AimDofSettings>("aimDof").handle();
    registry.executionHas(dabfg::SideEffects::External);

    return [aimDofSettingsHndl]() { restore_scope_aim_dof(aimDofSettingsHndl.ref()); };
  });
}