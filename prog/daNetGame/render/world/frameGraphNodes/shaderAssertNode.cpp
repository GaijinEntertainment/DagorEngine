// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"
#include <render/daBfg/bfg.h>
#include <shaders/dag_shAssert.h>


dabfg::NodeHandle makeShaderAssertNode()
{
  shader_assert::init();
  return dabfg::register_node("shader_assert_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("after_world_render_node");
    registry.executionHas(dabfg::SideEffects::External);
    return shader_assert::readback;
  });
}
