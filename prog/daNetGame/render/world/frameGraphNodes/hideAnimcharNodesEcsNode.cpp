// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <daECS/core/entityManager.h>

#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include "../hideNodesEvent.h"
#include "frameGraphNodes.h"


ECS_REGISTER_EVENT(HideNodesEvent, Point3 /*at*/)


dafg::NodeHandle makeHideAnimcharNodesEcsNode()
{
  return dafg::register_node("hide_animchar_nodes_ecs_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    const auto cameraParamsHndl = registry.readBlob<CameraParams>("current_camera").handle();

    registry.read("prepare_hero_cockpit_token").blob<OrderingToken>().optional();

    registry.create("hidden_animchar_nodes_token", dafg::History::No).blob<OrderingToken>();

    return [cameraParamsHndl]() {
      if (g_entity_mgr)
        g_entity_mgr->broadcastEventImmediate(HideNodesEvent(cameraParamsHndl.ref().viewItm.getcol(3)));
    };
  });
}
