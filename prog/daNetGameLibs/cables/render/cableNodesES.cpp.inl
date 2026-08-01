// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <render/rendererFeatures.h>
#include <render/renderEvent.h>

#include <render/cables.h>

#define TRANSPARENCY_NODE_PRIORITY_CABLES 4
static dafg::NodeHandle make_cables_node(const char *view_ns, bool is_main_view)
{
  auto nodeNs = dafg::root() / "transparent" / "close" / view_ns;
  return nodeNs.registerNode("cables_node", DAFG_PP_NODE_SRC, [view_ns, is_main_view](dafg::Registry registry) {
    request_common_transparent_state_per_view(registry, view_ns);
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_CABLES);

    use_volfog(registry, dafg::Stage::VS);

    registry.requestState().setFrameBlock("global_frame");
    return [is_main_view]() {
      const camera_in_camera::ApplyMasterState camcam{is_main_view};

      auto *mgr = ::get_cables_mgr();
      // This should never happen
      // (unless someone considerably reworks cables mgr lifetime)
      G_ASSERT_RETURN(mgr != nullptr, );
      mgr->render(Cables::RENDER_PASS_TRANS);
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraPerViewNodeConstruction)
ECS_REQUIRE(ecs::Tag cables_nodes_registrator)
static void cables_view_nodes_es(const OnCameraPerViewNodeConstruction &evt)
{
  evt.nodes->push_back(make_cables_node(evt.viewNsName, evt.isMainView));
}

// TODO: opaque cable node, etc
