// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>

namespace var
{
static ShaderVariableInfo dao_show_normal("dao_show_normal", true);
}

void set_up_show_depth_above_entity(bool render, bool show_normal)
{
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("debug_show_depth_above")));
  ecs::ComponentsInitializer init;
  init[ECS_HASH("debugShowDepthAboveNode")] =
    dafg::register_node("debug_show_depth_above", DAFG_PP_NODE_SRC, [render, show_normal](dafg::Registry registry) {
      auto debugNs = registry.root() / "debug";
      auto colorTarget = debugNs.modifyTexture("target_for_debug");
      registry.orderMeAfter("post_fx_node");
      read_gbuffer(registry);
      registry.readTexture("depth_for_postfx").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("depth_gbuf").optional();
      registry.requestRenderPass().color({colorTarget});
      return [render, show_normal, debugRenderer = PostFxRenderer("debug_depth_above")]() {
        if (!render)
          return;
        ShaderGlobal::set_int(var::dao_show_normal, show_normal ? 1 : 0);
        debugRenderer.render();
      };
    });
  g_entity_mgr->createEntityAsync("debug_show_depth_above", eastl::move(init));
}
