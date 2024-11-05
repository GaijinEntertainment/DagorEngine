// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/world/cameraParams.h>
#include <render/lightProbeSpecularCubesContainer.h>
#include <util/dag_console.h>
#include <render/world/wrDispatcher.h>
#include "scene/dag_tiledScene.h"
#include <shaders/dag_DynamicShaderHelper.h>
#include <drv/3d/dag_draw.h>
#include <render/indoorProbeManager.h>
#include <render/indoorProbeScenes.h>

enum class DebugIndoorProbeBoxesMode
{
  OFF,
  ACTIVE,
  ALL
};

enum class DebugIndoorProbeBoxesDepth
{
  GBUFFER,
  BOXES
};

void set_up_debug_indoor_probe_boxes_entity(DebugIndoorProbeBoxesMode mode, DebugIndoorProbeBoxesDepth depth, float scale)
{
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("debug_indoor_probe_boxes")));
  if (mode == DebugIndoorProbeBoxesMode::OFF)
    return;
  if (mode == DebugIndoorProbeBoxesMode::ALL)
    WRDispatcher::ensureIndoorProbeDebugBuffersExist();
  ShaderGlobal::set_real(get_shader_variable_id("debug_indoor_boxes_size"), scale);
  ecs::ComponentsInitializer init;
  init[ECS_HASH("debugIndoorProbeBoxesNode")] =
    dabfg::register_node("debug_indoor_probe_boxes", DABFG_PP_NODE_SRC, [mode, depth](dabfg::Registry registry) {
      auto pass = registry.requestRenderPass()
                    .color({"target_for_transparency"})
                    .depthRw(depth == DebugIndoorProbeBoxesDepth::GBUFFER
                               ? registry.modifyTexture("depth_for_transparency")
                               : registry.createTexture2d("debug_indoor_probe_boxes_depth", dabfg::History::No,
                                   {TEXFMT_DEPTH32 | TEXCF_RTARGET, registry.getResolution<2>("main_view")}));

      if (depth == DebugIndoorProbeBoxesDepth::BOXES)
        pass = eastl::move(pass).clear("debug_indoor_probe_boxes_depth", make_clear_value(0.f, 0));
      registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
      return
        [mode, debugIndoorProbeBoxes = DynamicShaderHelper(
                 mode == DebugIndoorProbeBoxesMode::ACTIVE ? "debug_indoor_probe_active_boxes" : "debug_indoor_probe_all_boxes")] {
          debugIndoorProbeBoxes.shader->setStates();

          uint32_t boxCount = mode == DebugIndoorProbeBoxesMode::ACTIVE
                                ? LightProbeSpecularCubesContainer::INDOOR_PROBES
                                : WRDispatcher::getIndoorProbeManager()->getIndoorProbeScenes()->getAllNodesCount();
          d3d::draw(PRIM_TRILIST, 0, 12 * boxCount);
        };
    });
  g_entity_mgr->createEntityAsync("debug_indoor_probe_boxes", eastl::move(init));
}

static bool debug_indoor_probe_boxes_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("render", "debug_indoor_probe_boxes", 1, 4)
  {
    auto mode = DebugIndoorProbeBoxesMode::OFF;
    auto depth = DebugIndoorProbeBoxesDepth::GBUFFER;
    auto scale = 1.0f;
    if (argc == 1)
      console::print("render.debug_indoor_probe_boxes mode=(off|active|all) depth=(gbuffer|boxes) scale=(float)");
    if (argc >= 2)
      mode = strcmp(argv[1], "active") == 0
               ? DebugIndoorProbeBoxesMode::ACTIVE
               : (strcmp(argv[1], "all") == 0 ? DebugIndoorProbeBoxesMode::ALL : DebugIndoorProbeBoxesMode::OFF);
    if (argc >= 3)
      depth = strcmp(argv[2], "boxes") == 0 ? DebugIndoorProbeBoxesDepth::BOXES : DebugIndoorProbeBoxesDepth::GBUFFER;
    if (argc == 4)
      scale = console::to_real(argv[3]);
    set_up_debug_indoor_probe_boxes_entity(mode, depth, scale);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(debug_indoor_probe_boxes_console_handler);
size_t debug_indoor_probe_boxes_console_handler_pull = 0;
