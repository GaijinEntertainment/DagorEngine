// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/scopeAimRender/scopeAimRender.h>
#include "scopeMobileNodes.h"

#include <3d/dag_render.h>
#include <render/daFrameGraph/daFG.h>
#include <render/world/global_vars.h>

dafg::NodeHandle mk_scope_lens_mobile_node()
{
  return dafg::register_node("scope_lens_mobile", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // it depends on depth_for_transparent_effects only
    // and might break RP during trans rendering, so we need a direct order here
    registry.orderMeBefore("occlusion_preparing_mobile");

    registry.requestRenderPass().color({registry.modifyTexture("target_after_darg_in_world_panels_trans")});

    registry.requestState().setFrameBlock("global_frame");

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();

    const auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    const auto scopeAimDataHndl = registry.readBlob<ScopeAimRenderingData>("scope_aim_render_data").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.readTexture("scope_lens_mask").atStage(dafg::Stage::PS).bindToShaderVar("scope_lens_mask");
    registry.read("scope_lens_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("scope_lens_mask_samplerstate");

    return [aimDataHndl, scopeAimDataHndl, strmCtxHndl]() {
      if (aimDataHndl.ref().lensRenderEnabled)
        render_scope_trans_forward(scopeAimDataHndl.ref(), strmCtxHndl.ref());
      ShaderGlobal::set_int(lens_night_vision_onVarId,
        aimDataHndl.ref().lensRenderEnabled && scopeAimDataHndl.ref().nightVision ? 1 : 0);
    };
  });
}