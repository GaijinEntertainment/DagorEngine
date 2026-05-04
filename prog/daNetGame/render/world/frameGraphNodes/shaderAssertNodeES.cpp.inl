// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>

#include <shaders/dag_shAssert.h>

#include <render/renderEvent.h>


dafg::NodeHandle makeShaderAssertNode()
{
  shader_assert::init();
  return dafg::register_node("shader_assert_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("after_world_render_node");
    registry.executionHas(dafg::SideEffects::External);
    return shader_assert::readback;
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_shader_assert_node_es(const OnCameraNodeConstruction &evt)
{
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  bool shaderAssertsEnabled = graphicsBlk->getBool("enableShaderAsserts", false);

  if (shaderAssertsEnabled)
    evt.nodes->push_back(makeShaderAssertNode());
}
