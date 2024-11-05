// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <blood_puddles/public/render/bloodPuddles.h>

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <render/daBfg/bfg.h>
#include <render/daBfg/ecs/frameGraphNode.h>

#include <shaders/dag_postFxRenderer.h>
#include <util/dag_console.h>

#include <render/world/cameraParams.h>

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
void init_blood_puddles_node_es(const ecs::Event &, dabfg::NodeHandle &blood_puddle_dbg__render_node)
{
  auto ns = dabfg::root() / "opaque" / "statics";

  blood_puddle_dbg__render_node = ns.registerNode("blood_puddles_debug", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestRenderPass()
      .depthRoAndBindToShaderVars("depth_for_postfx", {"depth_gbuf"})
      .color({"frame_with_debug"})
      .clear("frame_with_debug", make_clear_value(0, 0, 0, 0));

    DynamicShaderHelper shHolder;
    shHolder.init("blood_puddles_debug", nullptr, 0, "blood_puddles_debug", false);

    PostFxRenderer shResolveHolder("blood_puddles_resolve_debug");

    auto accBloodBufHndl =
      registry.readTexture("blood_acc_buf0").atStage(dabfg::Stage::PS).bindToShaderVar("blood_acc_buf0").optional().handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    registry.requestState().setFrameBlock("global_frame");

    return [accBloodBufHndl, cameraHndl, shHolder = std::move(shHolder), shResolveHolder = std::move(shResolveHolder)]() {
      BloodPuddles *mgr = get_blood_puddles_mgr();
      if (mgr && is_blood_enabled())
      {
        if (accBloodBufHndl.get())
          shResolveHolder.render();
        else
          mgr->renderShElem(cameraHndl.get()->viewTm, cameraHndl.get()->jitterProjTm, shHolder.shader);
      }
    };
  });
}

static bool blood_puddle_dbg_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("blood_puddles", "show_edge", 1, 1)
  {
    ecs::EntityId dbgEntity = g_entity_mgr->getSingletonEntity(ECS_HASH("blood_puddles__debugger"));
    if (dbgEntity)
    {
      g_entity_mgr->destroyEntity(dbgEntity);
      return found;
    }

    g_entity_mgr->createEntityAsync("blood_puddles__debugger");
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(blood_puddle_dbg_console_handler);