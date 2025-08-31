// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"
#include <render/daFrameGraph/daFG.h>
#include <shaders/dag_shAssert.h>


dafg::NodeHandle makeShaderAssertNode()
{
  shader_assert::init();
  return dafg::register_node("shader_assert_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("after_world_render_node");
    registry.executionHas(dafg::SideEffects::External);
    return shader_assert::readback;
  });
}
