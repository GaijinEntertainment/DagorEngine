// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "waterFlowmapObstacles.h"
#include "waterFlowmapObstaclesEvent.h"

#include <daECS/core/updateStage.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <render/renderer.h>
#include <render/renderEvent.h>
#include <render/rendererFeatures.h>
#include <main/water.h>
#include <main/level.h>

ECS_DECLARE_BOXED_TYPE(WaterFlowmapObstacles);
ECS_REGISTER_BOXED_TYPE(WaterFlowmapObstacles, nullptr);
ECS_AUTO_REGISTER_COMPONENT(WaterFlowmapObstacles, "water_flowmap_obstacles", nullptr, 0);

template <typename Callable>
inline bool get_water_flowmap_obstacles_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline bool get_water_ecs_query(ecs::EntityId eid, Callable c);

static void destroy_water_flowmap_obstacles_entity()
{
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("water_flowmap_obstacles")));
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, EventLevelLoaded, ChangeRenderFeatures)
static void attempt_to_enable_water_flowmap_obstacles_es(const ecs::Event &, FFTWater &water)
{
  fft_water::WaterFlowmap *waterFlowmap = fft_water::get_flowmap(&water);
  if (!waterFlowmap || !fft_water::is_flowmap_active(&water))
  {
    destroy_water_flowmap_obstacles_entity();
    return;
  }
  if (g_entity_mgr->getSingletonEntity(ECS_HASH("water_flowmap_obstacles")))
    return;
  ecs::ComponentsInitializer init;
  g_entity_mgr->createEntityAsync("water_flowmap_obstacles");
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const FFTWater &water)
static void disable_water_flowmap_obstacles_es(const ecs::Event &) { destroy_water_flowmap_obstacles_entity(); }

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // required to increase parallelity (start `animchar_before_render_es` as early as possible)
static void update_water_flowmap_obstacles_es(const UpdateStageInfoBeforeRender &evt, FFTWater &water)
{
  if (evt.dt <= 0.0)
    return;

  TIME_D3D_PROFILE(water_flowmap_obstacles);
  fft_water::WaterFlowmap *waterFlowmap = fft_water::get_flowmap(&water);
  if (waterFlowmap)
  {
    waterFlowmap->circularObstacles.clear();
    waterFlowmap->rectangularObstacles.clear();
    WaterFlowmapObstacles::GatherObstacleEventCtx evtCtx{water, *waterFlowmap};
    g_entity_mgr->broadcastEventImmediate(GatherObstaclesForWaterFlowmap(&evtCtx));
  }
}
