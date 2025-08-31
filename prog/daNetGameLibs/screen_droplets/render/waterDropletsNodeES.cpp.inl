// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderEvent.h>
#include <ecs/render/updateStageRender.h>
#include <render/screenDroplets.h>

#include "screenDroplets.h"

template <typename Callable>
static ecs::QueryCbResult find_water_droplets_needs_ecs_query(Callable);
static ShaderVariableInfo screen_droplets_renderedVarId;

ECS_TAG(render)
ECS_ON_EVENT(UpdateStageInfoBeforeRender)
inline void update_water_droplets_node_es(ecs::Event, dafg::NodeHandle &water_droplets_node, bool &screen_droplets__visible)
{
  bool found = find_water_droplets_needs_ecs_query([](ecs::Tag needsWaterDroplets) {
    G_UNUSED(needsWaterDroplets);
    return ecs::QueryCbResult::Stop;
  }) == ecs::QueryCbResult::Stop;
  if (!found)
  {
    if (water_droplets_node)
      water_droplets_node = {};
    return;
  }
  const bool wasVisible = screen_droplets__visible;
  screen_droplets__visible = get_screen_droplets_mgr() && get_screen_droplets_mgr()->isVisible();
  if (wasVisible != screen_droplets__visible)
  {
    if (screen_droplets__visible)
      water_droplets_node = makeWaterDropletsNode();
    else
      water_droplets_node = {};
  }
  ShaderGlobal::set_int(screen_droplets_renderedVarId, screen_droplets__visible);
}

ECS_TAG(render)
ECS_REQUIRE(bool screen_droplets__visible)
static void register_screen_droplets_for_postfx_es(const RegisterPostfxResources &evt)
{
  screen_droplets_renderedVarId = get_shader_variable_id("screen_droplets_rendered");
  evt.get<0>().readTexture("screen_droplets_tex").atStage(dafg::Stage::PS).bindToShaderVar("screen_droplets_tex").optional();
  evt.get<0>()
    .read("screen_droplets_sampler")
    .blob<d3d::SamplerHandle>()
    .bindToShaderVar("screen_droplets_tex_samplerstate")
    .optional();
}
