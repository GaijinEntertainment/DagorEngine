// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"

dabfg::NodeHandle makeDelayedRenderCubeNode()
{
  return dabfg::register_node("delayed_render_cube_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.setPriority(dabfg::PRIO_AS_LATE_AS_POSSIBLE);
    // priority modifies node order inside render pass, but as node is not tied to any rendering passes
    // we must use proper ordering constraints, right now via orderMe* as internals of node are not properly FG-like handled
    registry.orderMeAfter("after_world_render_node");
    registry.multiplex(dabfg::multiplexing::Mode::None);
    registry.executionHas(dabfg::SideEffects::External);
    return []() { static_cast<WorldRenderer *>(get_world_renderer())->renderDelayedCube(); };
  });
}

dabfg::NodeHandle makeDelayedRenderDepthAboveNode()
{
  return dabfg::register_node("delayed_render_depth_above", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.setPriority(dabfg::PRIO_AS_LATE_AS_POSSIBLE);
    // priority modifies node order inside render pass, but as node is not tied to any rendering passes
    // we must use proper ordering constraints, right now via orderMe* as internals of node are not properly FG-like handled
    registry.orderMeAfter("after_world_render_node");
    registry.requestState().setFrameBlock("global_frame");
    registry.multiplex(dabfg::multiplexing::Mode::None);
    registry.executionHas(dabfg::SideEffects::External);
    // This node provides 2 resources that are read during the frame.
    // At the edges of the toroidal bbox the contents will be wrong for a frame.
    // It should not be visible, and this node is better rendered late to reduce waiting on the culling job.
    return []() { static_cast<WorldRenderer *>(get_world_renderer())->renderDelayedDepthAbove(); };
  });
}