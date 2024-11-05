// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <render/rendererFeatures.h>

#include <render/cables.h>

#define TRANSPARENCY_NODE_PRIORITY_CABLES 4
ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void cabels_node_created_es_event_handler(const ecs::Event &, dabfg::NodeHandle &cable_trans_framegraph_node)
{
  auto nodeNs = dabfg::root() / "transparent" / "close";
  cable_trans_framegraph_node = nodeNs.registerNode("cables_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    render_transparency(registry);
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_CABLES);

    use_volfog(registry, dabfg::Stage::VS);

    registry.requestState().setFrameBlock("global_frame");
    return [] {
      auto *mgr = ::get_cables_mgr();
      // This should never happen
      // (unless someone considerably reworks cables mgr lifetime)
      G_ASSERT_RETURN(mgr != nullptr, );
      mgr->render(Cables::RENDER_PASS_TRANS);
    };
  });
}

// TODO: opaque cable node, etc
