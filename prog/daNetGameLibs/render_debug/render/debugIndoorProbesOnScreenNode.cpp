// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/viewVecs.h>
#include <render/world/cameraParams.h>

// TODO: Rewrite this code and console command on das.
void set_up_debug_indoor_probes_on_screen_entity(bool render)
{
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("debug_indoor_probes_on_screen")));
  if (!render)
    return;
  ecs::ComponentsInitializer init;
  init[ECS_HASH("debugIndoorProbesOnScreenNode")] =
    dafg::register_node("debug_indoor_probes_on_screen", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.requestRenderPass().color({"frame_with_debug"});
      registry.requestState().setFrameBlock("global_frame");
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
      registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");
      read_gbuffer(registry, dafg::Stage::PS, readgbuffer::NORMAL);
      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      return [debugIndoorProbesOnScreen = PostFxRenderer("debug_indoor_probes_on_screen"), cameraHndl] {
        set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
        debugIndoorProbesOnScreen.render();
      };
    });
  g_entity_mgr->createEntityAsync("debug_indoor_probes_on_screen", eastl::move(init));
}
