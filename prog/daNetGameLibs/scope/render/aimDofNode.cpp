// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/scopeAimRender/scopeAimRender.h>
#include "scopeFullDeferredNodes.h"

#include <render/daFrameGraph/daFG.h>
#include <render/deferredRenderer.h>
#include <render/rendererFeatures.h>
#include <render/world/postfxRender.h>
#include <render/world/antiAliasingMode.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>

// todo: fix dof render implementation dependencies on bloom
static bool should_prepare_aim_dof(const ScopeAimRenderingData &scopeAimData)
{
  return scopeAimData.isAiming && renderer_has_feature(FeatureRenderFlags::BLOOM);
}

dafg::NodeHandle makeAimDofPrepareNode()
{
  // TODO: This is a very ugly hack to fix dof being stuck when taking a screenshot
  static AimDofSettings firstIterationAimData;
  return dafg::register_node("aim_dof_prepare_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeBefore("prepare_post_fx_node");

    const bool hasStencilTest = renderer_has_feature(FeatureRenderFlags::CAMERA_IN_CAMERA);
    const uint32_t gbufDepthFormat = get_gbuffer_depth_format(hasStencilTest);
    auto lensDofDepthHndl = registry
                              .createTexture2d("lens_dof_blend_depth_tex", dafg::History::No,
                                {gbufDepthFormat | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
                              .atStage(dafg::Stage::PS)
                              .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                              .handle();

    auto depthHndl =
      registry.readTexture("depth_for_transparency").atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();

    auto depthWithTransparencyHndl = registry.readTexture("depth_with_transparency")
                                       .atStage(dafg::Stage::PS)
                                       .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                                       .optional()
                                       .handle();

    registry.requestState().setFrameBlock("global_frame");
    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();

    auto aimRenderingDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto scopeAimRenderingDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto aimDofHndl = registry.createBlob<AimDofSettings>("aimDof", dafg::History::No).handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    auto antiAliasingModeHndl = registry.readBlob<AntiAliasingMode>("anti_aliasing_mode").handle();

    return [lensDofDepthHndl, aimRenderingDataHndl, scopeAimRenderingDataHndl, depthHndl, depthWithTransparencyHndl, aimDofHndl,
             strmCtxHndl, antiAliasingModeHndl](dafg::multiplexing::Index multiplexing_index) {
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

      bool isFirstIteration = multiplexing_index == dafg::multiplexing::Index{};
      if (isFirstIteration)
        firstIterationAimData = aimDof;
      aimDofHndl.ref() = firstIterationAimData;
    };
  });
}

dafg::NodeHandle makeAimDofRestoreNode()
{
  return dafg::register_node("aim_dof_restore_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("post_fx_node");

    auto aimDofSettingsHndl = registry.readBlob<AimDofSettings>("aimDof").handle();
    registry.executionHas(dafg::SideEffects::External);

    return [aimDofSettingsHndl]() { restore_scope_aim_dof(aimDofSettingsHndl.ref()); };
  });
}