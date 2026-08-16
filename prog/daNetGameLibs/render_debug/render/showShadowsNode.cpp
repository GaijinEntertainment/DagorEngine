// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <render/viewportTiles.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/viewVecs.h>
#include <render/world/cameraParams.h>
#include <debug/dag_textMarks.h>
#include <EASTL/array.h>
#include <EASTL/string_view.h>

bool should_hide_debug();

static const eastl::array<eastl::string_view, 9> show_shadows_grid_modes = {
  "static", "csm", "csm_cascades", "contact", "combine", "clouds", "static_cascades", "ssss", "vsm"};

// TODO: Rewrite this code and console command on das.
void set_up_show_shadows_entity(int show_shadows)
{
  static int show_shadowsVarId = get_shader_variable_id("show_shadows");

  const bool grid = show_shadows == (int)show_shadows_grid_modes.size();
  ShaderGlobal::set_int(show_shadowsVarId, grid ? 0 : show_shadows);
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("show_shadows")));
  if (show_shadows == -1)
    return;
  ecs::ComponentsInitializer init;
  init[ECS_HASH("showShadowsNode")] = dafg::register_node("show_shadows", DAFG_PP_NODE_SRC, [grid](dafg::Registry registry) {
    auto debugNs = registry.root() / "debug";
    auto colorTarget = debugNs.modifyTexture("target_for_debug");
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");
    registry.requestRenderPass().color({colorTarget});
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");
    registry.readTexture("csm_texture").atStage(dafg::Stage::POST_RASTER).optional();
    read_gbuffer(registry);

    auto camera = registry.readBlob<CameraParams>("current_camera");
    CameraViewShvars{camera}.bindViewVecs();

    return [debugShadows = PostFxRenderer("debug_shadows"), grid] {
      if (!grid)
      {
        debugShadows.render();
        return;
      }

      const bool withLabels = !should_hide_debug();

      for_each_viewport_tile(3, 3, false, [&](int tile_x, int tile_y, int tile_w, int tile_h, int index) {
        G_UNUSED(tile_h);
        ShaderGlobal::set_int(show_shadowsVarId, index);
        debugShadows.render();

        if (!withLabels)
          return;
        const eastl::string_view name = show_shadows_grid_modes[index];
        add_debug_text_mark(tile_x + tile_w * 0.5f, tile_y + 12, name.data(), (int)name.size(), 0.8f);
      });
    };
  });
  g_entity_mgr->createEntityAsync("show_shadows", eastl::move(init));
}
