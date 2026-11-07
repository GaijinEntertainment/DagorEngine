// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"

#include <render/world/frameGraphHelpers.h>
#include <render/daFrameGraph/daFG.h>
#include <shaders/dag_postFxRenderer.h>

dafg::NodeHandle create_dynamic_mirror_envi_node()
{
  return get_dynamic_mirrors_namespace().registerNode("render_envi", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestRenderPass().depthReadTestOnly("mirror_gbuf_depth").color({"mirror_texture"});
    registry.createBlob<OrderingToken>("mirror_envi_rendered_token");
    registry.requestState().setFrameBlock("global_frame");

    auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();
    auto shaderVarsHndl = registry.readBlob<DynamicMirrorShaderVars>("mirror_vars").handle();

    return [mirrorActiveHndl, shaderVarsHndl, enviRenderer = PostFxRenderer("dynamic_mirror_envi")]() {
      if (!mirrorActiveHndl.ref())
        return;
      auto scopedVars = shaderVarsHndl.ref().getScopedVars();
      enviRenderer.render();
    };
  });
}
