// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <render/daFrameGraph/daFG.h>

dafg::NodeHandle create_dynamic_mirror_end_prepass_node()
{
  return get_dynamic_mirrors_namespace().registerNode("end_prepass", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.rename("mirror_prepass_gbuf_depth", "mirror_gbuf_depth");
    registry.requestRenderPass()
      .depthRw("mirror_gbuf_depth")
      .color({"mirror_gbuf_0", "mirror_gbuf_1", registry.modify("mirror_gbuf_2").texture().optional(),
        registry.modify("mirror_gbuf_3").texture().optional()});
    return []() {};
  });
}