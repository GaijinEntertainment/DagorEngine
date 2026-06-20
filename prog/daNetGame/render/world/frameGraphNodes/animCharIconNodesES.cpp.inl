// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>

#include <render/world/frameGraphHelpers.h>
#include <render/renderEvent.h>
#include <render/renderAnimCharIcon.h>


ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_animchar_icon_nodes_es(const OnCameraNodeConstruction &evt)
{
  auto n = dafg::register_node("animchar_icon_render_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.modifyBlob<OrderingToken>("before_world_render_setup_token");

    return []() { animchar_icon::process_pending_picture(); };
  });

  evt.nodes->push_back(eastl::move(n));
}
