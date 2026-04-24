// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <render/daFrameGraph/daFG.h>
#include <render/world/global_vars.h>
#include <rendInst/rendInstGenRender.h>


dafg::NodeHandle create_dynamic_mirror_render_static_node(DynamicMirrorRenderer &mirror_renderer)
{
  return get_dynamic_mirrors_namespace().registerNode("render_static", DAFG_PP_NODE_SRC, [&mirror_renderer](dafg::Registry registry) {
    const auto mirrorResolution = get_mirror_resolution(registry);

    auto mirrorCameraHndl = use_mirror_gbuffer_pass(registry);

    registry.requestState().setFrameBlock("global_frame");

    auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();
    auto shaderVarsHndl = registry.readBlob<DynamicMirrorShaderVars>("mirror_vars").handle();

    return [&mirror_renderer, mirrorResolution, mirrorActiveHndl, mirrorCameraHndl, shaderVarsHndl]() {
      if (!mirrorActiveHndl.ref())
        return;
      const auto cameraData = mirrorCameraHndl.get();
      const auto resolution = mirrorResolution.get();
      auto scopedVars = shaderVarsHndl.ref().getScopedVars();

      mirror_renderer.waitStaticsVisibility();

      Color4 originalWorldPos = ShaderGlobal::get_float4(world_view_posVarId);
      Color4 originalZnZfar = ShaderGlobal::get_float4(zn_zfarVarId);

      TexStreamingContext texStreamingCtx = TexStreamingContext(cameraData->persp, resolution.x);
      ShaderGlobal::set_float4(world_view_posVarId, cameraData->viewPos.x, cameraData->viewPos.y, cameraData->viewPos.z, 1);
      ShaderGlobal::set_float4(zn_zfarVarId, cameraData->persp.zn, cameraData->persp.zf, 0, 0);

      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, mirror_renderer.getVisibility(), cameraData->viewItm,
        rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra, rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All,
        nullptr, texStreamingCtx);

      ShaderGlobal::set_float4(world_view_posVarId, originalWorldPos);
      ShaderGlobal::set_float4(zn_zfarVarId, originalZnZfar);
    };
  });
}