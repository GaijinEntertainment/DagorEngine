// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <daECS/core/entityManager.h>

#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include "../hideNodesEvent.h"
#include "frameGraphNodes.h"


ECS_REGISTER_EVENT(HideNodesEvent, Point3 /*at*/)


dabfg::NodeHandle makeHideAnimcharNodesEcsNode()
{
  return dabfg::register_node("hide_animchar_nodes_ecs_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    const auto cameraParamsHndl = registry.readBlob<CameraParams>("current_camera").handle();

    registry.create("hidden_animchar_nodes_token", dabfg::History::No).blob<OrderingToken>();

    return [cameraParamsHndl]() {
      if (g_entity_mgr)
        g_entity_mgr->broadcastEventImmediate(HideNodesEvent(cameraParamsHndl.ref().viewItm.getcol(3)));
    };
  });
}
