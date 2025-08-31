// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <blood_puddles/public/render/bloodPuddles.h>

#include <drv/3d/dag_renderStates.h>

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>

#include <shaders/dag_postFxRenderer.h>
#include <util/dag_console.h>

#include <render/world/cameraParams.h>

namespace
{
dafg::NodeHandle create_show_edge_node()
{
  auto ns = dafg::root() / "opaque" / "statics";

  return ns.registerNode("blood_puddles_debug", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestRenderPass().depthRoAndBindToShaderVars("depth_for_postfx", {"depth_gbuf"}).color({"frame_with_debug"});

    DynamicShaderHelper shHolder;
    shHolder.init("blood_puddles_debug", nullptr, 0, "blood_puddles_debug", false);

    PostFxRenderer shResolveHolder("blood_puddles_resolve_debug");

    auto accBloodBufHndl =
      registry.readTexture("blood_acc_buf0").atStage(dafg::Stage::PS).bindToShaderVar("blood_acc_buf0").optional().handle();
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

static dafg::NodeHandle create_show_bbox_node()
{
  auto ns = dafg::root() / "opaque" / "statics";

  return ns.registerNode("blood_puddles_bbox_debug", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestRenderPass().depthRoAndBindToShaderVars("depth_for_postfx", {"depth_gbuf"}).color({"frame_with_debug"});

    DynamicShaderHelper shHolder;
    shHolder.init("blood_puddles_bbox_debug", nullptr, 0, "blood_puddles_bbox_debug", false);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.requestState().setFrameBlock("global_frame");

    return [cameraHndl, shHolder = std::move(shHolder)]() {
      BloodPuddles *mgr = get_blood_puddles_mgr();
      if (mgr && is_blood_enabled())
      {
        d3d::setwire(true);
        mgr->renderShElem(cameraHndl.get()->viewTm, cameraHndl.get()->jitterProjTm, shHolder.shader);
        d3d::setwire(false);
      }
    };
  });
}

enum class DbgMode : int
{
  None = 0,
  ShowEdge = 1,
  ShowBbox = 2,
};

DbgMode name_to_dbg_mode(const int argc, const char *name)
{
  DbgMode mode = DbgMode::None;

  if (argc != 2)
    return mode;

  if (!strcmp(name, "edge"))
    mode = DbgMode::ShowEdge;
  else if (!strcmp(name, "bbox"))
    mode = DbgMode::ShowBbox;

  return mode;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
void init_blood_puddles_node_es(const ecs::Event &, dafg::NodeHandle &blood_puddle_dbg__render_node, const int blood_puddle_dbg__mode)
{
  const DbgMode mode = (DbgMode)blood_puddle_dbg__mode;
  blood_puddle_dbg__render_node = [mode]() {
    if (mode == DbgMode::ShowEdge)
      return create_show_edge_node();
    else if (mode == DbgMode::ShowBbox)
      return create_show_bbox_node();

    return dafg::NodeHandle{};
  }();
}

bool blood_puddle_dbg_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("blood_puddles", "dbg", 1, 2)
  {
    ecs::EntityId dbgEntity = g_entity_mgr->getSingletonEntity(ECS_HASH("blood_puddles__debugger"));
    const DbgMode mode = name_to_dbg_mode(argc, argv[1]);

    if (dbgEntity)
    {
      g_entity_mgr->destroyEntity(dbgEntity);
      if (mode == DbgMode::None && argc != 2)
        return found;
    }

    if (mode == DbgMode::None)
    {
      console::print("blood_puddles.dbg [mode], where mode is {edge, bbox}");
      return found;
    }

    ecs::ComponentsInitializer attrs;
    ECS_INIT(attrs, blood_puddle_dbg__mode, (int)mode);

    g_entity_mgr->createEntityAsync("blood_puddles__debugger", std::move(attrs));
  }
  return found;
}
} // namespace

REGISTER_CONSOLE_HANDLER(blood_puddle_dbg_console_handler);