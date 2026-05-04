// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/ecsQuery.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include "render/renderEvent.h"
#include <main/level.h>
#include <main/main.h>
#include "tiledMapContext.h"
#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <daGame/timers.h>
#include <math/dag_mathUtils.h>

namespace WorldRenderSatellite
{
extern ConVarT<int, true> size;
extern ConVarT<bool, false> scan;
// start position offset
extern ConVarT<float, false> scan_x0;
extern ConVarT<float, false> scan_y0;
extern ConVarT<float, false> scan_x1;
extern ConVarT<float, false> scan_y1;
extern ConVarT<int, true> zlevels;
extern ConVarT<bool, false> use_land_based_height_offset;
extern ConVarT<float, false> height_fixed;
extern ConVarT<float, false> height_ofs;
// before saving result, wait for some frames for streaming to finish its nasty job
extern ConVarT<int, false> scan_settle_frames;
} // namespace WorldRenderSatellite


template <typename Callable>
void tiled_map_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
void set_active_camera_down_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
void set_adaptation_override_ecs_query(ecs::EntityManager &manager, Callable);

ECS_TAG(render, gameClient)
ECS_REQUIRE(bool level__loaded)
static inline void gen_tiled_map_es(const EventLevelLoaded &, ecs::EntityManager &manager)
{
  bool enable = dgs_get_settings()->getBool("generate_tiled_map", false);
  if (!enable)
    return;

  bool hasTiledMap = false;
  float exposure = 1.0f;
  tiled_map_ecs_query(manager,
    [&](Point2 tiled_map__leftTop, Point2 tiled_map__rightBottom, int tiled_map__tileWidth, int tiled_map__zlevels,
      float tiled_map__genExposure = 1.0f, float tiled_map__genHeight = -1.0f, float tiled_map__genHeightOffset = -1.0f) {
      hasTiledMap = true;
      BBox2 bbox;
      bbox += tiled_map__leftTop;
      bbox += tiled_map__rightBottom;
      WorldRenderSatellite::scan_x0.set(tiled_map__leftTop.x);
      WorldRenderSatellite::scan_y0.set(tiled_map__leftTop.y);
      WorldRenderSatellite::scan_x1.set(tiled_map__rightBottom.x);
      WorldRenderSatellite::scan_y1.set(tiled_map__rightBottom.y);
      WorldRenderSatellite::zlevels.set(tiled_map__zlevels);
      WorldRenderSatellite::size.set(tiled_map__tileWidth);
      if (tiled_map__genHeight > 0.0)
      {
        WorldRenderSatellite::use_land_based_height_offset = false;
        WorldRenderSatellite::height_fixed = tiled_map__genHeight;
      }
      if (tiled_map__genHeightOffset > 0.0)
      {
        WorldRenderSatellite::height_ofs = tiled_map__genHeightOffset;
      }
      exposure = tiled_map__genExposure;
    });

  if (!hasTiledMap)
    DAG_FATAL("Game launched with `generate_tiled_map` flag, but no `tiled_map` entity found");

  set_adaptation_override_ecs_query(manager, [exposure](ecs::Object &adaptation_override_settings) {
    adaptation_override_settings.addMember("adaptation__on", false);
    adaptation_override_settings.addMember("adaptation__fixedExposure", exposure);
  });

  console::command("screencap.uncompressed_screenshots true"); // png for better quality

  game::g_timers_mgr.demandInit();
  game::Timer timer;
  game::g_timers_mgr->setTimer(
    timer,
    []() {
      console::command("app.timespeed 0");
      set_active_camera_down_ecs_query(*g_entity_mgr, [](TMatrix &transform, bool camera__active, float &fov) {
        if (camera__active)
        {
          fov = 80.0f;
          Point3 eye = transform.getcol(3) + Point3(0, 100, 0);
          Point3 at = eye + Point3(0, -1, 0);
          lookAt(eye, at, Point3(0, 1, 0), transform);
        }
      });
      WorldRenderSatellite::scan.set(true);
    },
    20.0f);

  timer.release(); // fire & forget
}