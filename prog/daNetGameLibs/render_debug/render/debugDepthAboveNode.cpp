// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>

namespace var
{
static ShaderVariableInfo dao_show_mode("dao_show_mode", true);
}

// show_mode: 0 = plain, 1 = normals, 2 = texel-size chessboard (see dao_show_mode in debugDepthAbove.dshl)
void set_up_show_depth_above_entity(bool render, int show_mode)
{
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("debug_show_depth_above")));
  ecs::ComponentsInitializer init;
  init[ECS_HASH("debugShowDepthAboveNode")] =
    dafg::register_node("debug_show_depth_above", DAFG_PP_NODE_SRC, [render, show_mode](dafg::Registry registry) {
      auto debugNs = registry.root() / "debug";
      auto colorTarget = debugNs.modifyTexture("target_for_debug");
      registry.orderMeAfter("post_fx_node");
      read_gbuffer(registry);
      registry.readTexture("depth_for_postfx").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("depth_gbuf").optional();
      registry.requestRenderPass().color({colorTarget});
      return [render, show_mode, debugRenderer = PostFxRenderer("debug_depth_above")]() {
        if (!render)
          return;
        ShaderGlobal::set_int(var::dao_show_mode, show_mode);
        debugRenderer.render();
      };
    });
  g_entity_mgr->createEntityAsync("debug_show_depth_above", eastl::move(init));
}
