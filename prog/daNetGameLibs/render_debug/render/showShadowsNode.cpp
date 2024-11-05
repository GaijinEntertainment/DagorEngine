// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/viewVecs.h>
#include <render/world/cameraParams.h>

// TODO: Rewrite this code and console command on das.
void set_up_show_shadows_entity(int show_shadows)
{
  static int show_shadowsVarId = get_shader_variable_id("show_shadows");
  ShaderGlobal::set_int(show_shadowsVarId, show_shadows);
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("show_shadows")));
  if (show_shadows == -1)
    return;
  ecs::ComponentsInitializer init;
  init[ECS_HASH("showShadowsNode")] = dabfg::register_node("show_shadows", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");
    registry.requestRenderPass().color({"frame_with_debug"});
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.readTexture("depth_for_postfx").atStage(dabfg::Stage::PS).bindToShaderVar("depth_gbuf");
    read_gbuffer(registry);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [debugShadows = PostFxRenderer("debug_shadows"), cameraHndl] {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      debugShadows.render();
    };
  });
  g_entity_mgr->createEntityAsync("show_shadows", eastl::move(init));
}
