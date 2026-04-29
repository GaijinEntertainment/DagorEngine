// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>

#include <render/renderEvent.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_monotonic_frame_access_node_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(dafg::register_node("monotonic_frame_counter_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto frameCounterHndl = registry.createBlob<uint32_t>("monotonic_frame_counter").handle();

    return [frameCounterHndl]() { frameCounterHndl.ref() = dagor_get_global_frame_id(); };
  }));
}
