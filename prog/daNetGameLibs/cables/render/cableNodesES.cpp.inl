// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <render/rendererFeatures.h>
#include <render/renderEvent.h>

#include <render/cables.h>

#define TRANSPARENCY_NODE_PRIORITY_CABLES 4
ECS_TAG(render)
ECS_ON_EVENT(on_appear, ChangeRenderFeatures)
static void cabels_node_created_es_event_handler(const ecs::Event &evt, dafg::NodeHandle &cable_trans_framegraph_node)
{
  if (auto *e = evt.cast<ChangeRenderFeatures>())
    if (!e->isFeatureChanged(CAMERA_IN_CAMERA))
      return;

  auto nodeNs = dafg::root() / "transparent" / "close";
  cable_trans_framegraph_node = nodeNs.registerNode("cables_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    request_common_transparent_state(registry);
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_CABLES);

    use_volfog(registry, dafg::Stage::VS);

    registry.requestState().setFrameBlock("global_frame");
    return [](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      auto *mgr = ::get_cables_mgr();
      // This should never happen
      // (unless someone considerably reworks cables mgr lifetime)
      G_ASSERT_RETURN(mgr != nullptr, );
      mgr->render(Cables::RENDER_PASS_TRANS);
    };
  });
}

// TODO: opaque cable node, etc
