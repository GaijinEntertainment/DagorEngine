// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <main/level.h>
#include <streaming/dag_streamingBase.h>

#include <render/daFrameGraph/daFG.h>
#include <render/renderEvent.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/wrDispatcher.h>

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_bin_scene_nodes_es(const OnCameraNodeConstruction &evt)
{
  BaseStreamingSceneHolder *binScene = WRDispatcher::getBinScene();
  if (!binScene)
    return;

  auto nodeNs = dafg::root() / "transparent" / "close";
  evt.nodes->push_back(nodeNs.registerNode("bin_scene_node", DAFG_PP_NODE_SRC, [binScene](dafg::Registry registry) {
    request_common_transparent_state(registry);
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_BIN_SCENE);

    registry.requestState().setFrameBlock("global_frame");
    return [binScene](const dafg::multiplexing::Index &multiplexing_index) {
      if (!binScene || is_level_unloading() || !is_level_loaded())
        return;

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
      binScene->renderTrans();
    };
  }));
}