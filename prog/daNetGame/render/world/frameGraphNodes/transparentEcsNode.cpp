// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <ecs/render/updateStageRender.h>
#include <render/rendererFeatures.h>
#include <render/world/cameraParams.h>
#include <daECS/core/entityManager.h>
#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>
#include <render/world/frameGraphNodes/prevFrameTexRequests.h>


dabfg::NodeHandle makeTransparentEcsNode()
{
  auto nodeNs = dabfg::root() / "transparent" / "close";
  return nodeNs.registerNode("ecs_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    render_transparency(registry);
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_ECS);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    read_prev_frame_tex(registry);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    auto occlusionHndl = registry.readBlob<Occlusion *>("current_occlusion").handle();
    return [cameraHndl, strmCtxHndl, occlusionHndl] {
      if (!g_entity_mgr)
        return;

      auto currentOcclusion = occlusionHndl.ref();
      const auto &camera = cameraHndl.ref();
      const auto &texCtx = strmCtxHndl.ref();

      g_entity_mgr->broadcastEventImmediate(
        UpdateStageInfoRenderTrans(camera.viewTm, camera.jitterProjTm, camera.viewItm, currentOcclusion, texCtx));
    };
  });
}
