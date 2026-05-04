// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "burntGroundEvents.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <gamePhys/collision/collisionLib.h>
#include <EASTL/array.h>
#include <landMesh/biomeQuery.h>
#include <ecs/render/updateStageRender.h>
#include <rendInst/rendInstExtraAccess.h>
#include <util/dag_console.h>
#include <camera/sceneCam.h>
#include <daECS/net/time.h>

ECS_REGISTER_EVENT(FireOnGroundEvent);

static void burn_ground(const Point3 &pos, float radius, bool animated)
{
  if (radius <= 0)
    return;

  float terrainDistance = radius * 3;
  rendinst::RendInstDesc riDesc;
  // Ray starts from top of sphere
  if (!dacoll::traceray_normalized(pos + Point3(0, radius, 0), Point3(0, -1, 0), terrainDistance, nullptr, nullptr,
        dacoll::ETF_HEIGHTMAP | dacoll::ETF_LMESH | dacoll::ETF_FRT | dacoll::ETF_RI, &riDesc))
    return;
  // Distance from center of sphere
  terrainDistance -= radius;
  float terrainHeight = pos.y - terrainDistance;
  terrainDistance = abs(terrainDistance);
  if (terrainDistance >= radius)
    return;

  // Skip rendinsts other than rendinst_clipmap
  if (riDesc.isValid() && !rendinst::is_riextra_rendinst_clipmap(riDesc.getRiExtraHandle()))
    return;

  const float adjustedRadius = sqrt(radius * radius - terrainDistance * terrainDistance);

  ecs::ComponentsInitializer attrs;
  TMatrix tm;
  tm.identity();
  tm.setcol(3, pos);
  ECS_INIT(attrs, transform, tm);
  ECS_INIT(attrs, burnt_ground_biome_query__biome_query_id, -1);
  ECS_INIT(attrs, burnt_ground_biome_query__attempts_remaining, 16); // max attempts to create biome queries
  ECS_INIT(attrs, burnt_ground_biome_query__radius, radius);
  ECS_INIT(attrs, burnt_ground_biome_query__adjusted_radius, adjustedRadius);
  ECS_INIT(attrs, burnt_ground_biome_query__ground_height, terrainHeight);
  ECS_INIT(attrs, burnt_ground_biome_query__animated, animated);
  g_entity_mgr->createEntityAsync("burnt_ground_biome_query", eastl::move(attrs));
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void burnt_ground__on_appear_event_es(
  const ecs::Event &, const TMatrix &transform, float burnt_ground__radius, float burnt_ground__placement_time)
{
  const float maxDelayForAnimation = 10; // seconds
  const bool isAnimated = get_sync_time() - burnt_ground__placement_time < maxDelayForAnimation;
  burn_ground(transform.getcol(3), burnt_ground__radius, isAnimated);
}

ECS_TAG(render)
static void burnt_ground_biome_query_update_es(const UpdateStageInfoBeforeRender &,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  const TMatrix &transform,
  int &burnt_ground_biome_query__biome_query_id,
  float burnt_ground_biome_query__radius,
  float burnt_ground_biome_query__adjusted_radius,
  float burnt_ground_biome_query__ground_height,
  int &burnt_ground_biome_query__attempts_remaining,
  bool burnt_ground_biome_query__animated)
{
  if (burnt_ground_biome_query__biome_query_id == -1)
  {
    if (burnt_ground_biome_query__attempts_remaining == 0)
    {
      manager.destroyEntity(eid);
      return;
    }
    --burnt_ground_biome_query__attempts_remaining;
    burnt_ground_biome_query__biome_query_id = biome_query::query(transform.getcol(3), burnt_ground_biome_query__adjusted_radius);
    if (burnt_ground_biome_query__biome_query_id == -1)
      return;
  }

  BiomeQueryResult qResult;
  GpuReadbackResultState state = biome_query::get_query_result(burnt_ground_biome_query__biome_query_id, qResult);
  if (state == GpuReadbackResultState::IN_PROGRESS)
    return;
  if (state == GpuReadbackResultState::SUCCEEDED)
  {
    manager.broadcastEvent(FireOnGroundEvent(transform.getcol(3), burnt_ground_biome_query__radius,
      burnt_ground_biome_query__adjusted_radius, burnt_ground_biome_query__ground_height, burnt_ground_biome_query__animated,
      {qResult.mostFrequentBiomeGroupIndex, qResult.secondMostFrequentBiomeGroupIndex},
      {qResult.mostFrequentBiomeGroupWeight, qResult.secondMostFrequentBiomeGroupWeight}));
  }
  manager.destroyEntity(eid);
}

static bool burnt_ground_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  // Places several decals on the ground along camera view dir
  CONSOLE_CHECK_NAME("burnt_ground", "place_decals", 1, 4)
  {
    float radius = 1;
    int count = 1;
    bool animated = true;
    if (argc > 1)
      radius = console::to_real(argv[1]);
    if (argc > 2)
      count = console::to_int(argv[2]);
    if (argc > 3)
      animated = console::to_bool(argv[3]);
    const TMatrix &itm = get_cam_itm();
    const auto pos = itm.getcol(3);
    const auto dir = normalize(itm.getcol(2));
    float distance = 2000;
    for (int i = 0; i < count; ++i)
    {
      const auto rayStartPos = pos + dir * (radius * 2.f) * (i + 1.f) + Point3(0, 1000, 0);
      const auto rayDir = Point3(0, -1, 0);
      if (!dacoll::traceray_normalized(rayStartPos, rayDir, distance, nullptr, nullptr,
            dacoll::ETF_HEIGHTMAP | dacoll::ETF_LMESH | dacoll::ETF_FRT, nullptr))
        continue;
      const auto decalPos = rayStartPos + rayDir * distance;
      burn_ground(decalPos, radius, animated);
    }
  }
  // Places a single decal where the camera view dir intersects the ground
  CONSOLE_CHECK_NAME("burnt_ground", "place_decal", 1, 3)
  {
    float radius = 1;
    bool animated = true;
    if (argc > 1)
      radius = console::to_real(argv[1]);
    if (argc > 2)
      animated = console::to_bool(argv[2]);
    const TMatrix &itm = get_cam_itm();
    const auto pos = itm.getcol(3);
    const auto dir = normalize(itm.getcol(2));
    float distance = 2000;
    if (dacoll::traceray_normalized(pos, dir, distance, nullptr, nullptr, dacoll::ETF_HEIGHTMAP | dacoll::ETF_LMESH | dacoll::ETF_FRT,
          nullptr))
    {
      const auto decalPos = pos + dir * distance;
      burn_ground(decalPos, radius, animated);
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(burnt_ground_console_handler);