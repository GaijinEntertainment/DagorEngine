// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <ecs/render/updateStageRender.h>
#include <render/rendererFeatures.h>
#include <render/world/cameraParams.h>
#include <daECS/core/entityManager.h>
#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>
#include <render/world/frameGraphNodes/prevFrameTexRequests.h>
#include <render/world/cameraViewVisibilityManager.h>


dafg::NodeHandle makeTransparentEcsNode()
{
  auto nodeNs = dafg::root() / "transparent" / "close";
  return nodeNs.registerNode("ecs_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto cameraHndl = request_common_transparent_state(registry, nullptr, false);
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_ECS);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    read_prev_frame_tex(registry);
    registry.read("glass_depth_for_merge").texture().atStage(dafg::Stage::POST_RASTER).bindToShaderVar("glass_hole_mask").optional();
    registry.read("glass_target").texture().atStage(dafg::Stage::POST_RASTER).bindToShaderVar("glass_rt_reflection").optional();

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    return [cameraHndl, strmCtxHndl](const dafg::multiplexing::Index &multiplexing_index) {
      if (!g_entity_mgr)
        return;

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      const auto &camera = cameraHndl.ref();
      Occlusion *occlusion = camera.jobsMgr->getOcclusion();
      const auto &texCtx = strmCtxHndl.ref();

      g_entity_mgr->broadcastEventImmediate(
        UpdateStageInfoRenderTrans(camera.viewTm, camera.jitterProjTm, camera.viewItm, occlusion, texCtx));
    };
  });
}
