// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <ecs/render/renderPasses.h>
#include <render/world/cameraParams.h>
#include <render/daFrameGraph/daFG.h>
#include <3d/dag_texStreamingContext.h>

dafg::NodeHandle create_dynamic_mirror_resolve_node(DynamicMirrorRenderer &mirror_renderer)
{
  return get_dynamic_mirrors_namespace().registerNode("resolve", DAFG_PP_NODE_SRC, [&mirror_renderer](dafg::Registry registry) {
    registry.readTexture("mirror_texture").atStage(dafg::Stage::PS).bindToShaderVar("dynamic_mirror_tex");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();

    shaders::OverrideState overrideState = {};
    overrideState.set(shaders::OverrideState::Z_BIAS);
    overrideState.zBias = 0.01;
    overrideState.set(shaders::OverrideState::Z_WRITE_DISABLE);

    registry.requestRenderPass().depthRw("gbuf_depth_after_resolve").color({"opaque_resolved"});

    registry.requestState().setFrameBlock("global_frame").enableOverride(overrideState);

    return [&mirror_renderer, cameraHndl, mirrorActiveHndl]() {
      if (!mirrorActiveHndl.ref())
        return;
      auto animcharMirrorData = mirror_renderer.getAnimcharMirrorData();
      if (animcharMirrorData == nullptr)
        return;

      ShaderGlobal::set_int(dynamic_texture_hdr_passVarId, 1);

      const auto &camera = cameraHndl.ref();
      render_dynamic_mirrors(cameraHndl.ref().cameraWorldPos, *animcharMirrorData, RENDER_MAIN, false, camera.viewItm,
        TexStreamingContext(0));
      ShaderGlobal::set_int(dynamic_texture_hdr_passVarId, 0);
    };
  });
}
