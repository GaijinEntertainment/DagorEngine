// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/coreEvents.h>
#include <3d/dag_resPtr.h>
#include <EASTL/vector.h>
#include <math/dag_Point4.h>
#include <gamePhys/collision/collisionLib.h>
#include <landMesh/lmeshManager.h>
#include <gui/dag_visualLog.h>
#include "main/level.h"
#include "../helpers.h"

ECS_TAG(dev)
ECS_ON_EVENT(EventLevelLoaded)
ECS_REQUIRE(Point4 ground_holes_scale_offset, int ground_holes_main_tex_size)
void ground_holes_physics_init_es_event_handler(const ecs::Event &)
{
  if (!hasGroundHoleManager())
    visuallog::logmsg("ground_holes_updater: Ground hole manager wasn't found! Please enable it "
                      "(level__useGroundHolesCollision:b=yes) and reload level");
}

template <typename Callable>
static void set_holes_changed_ecs_query(Callable c);

ECS_TRACK(transform, ground_hole_sphere_shape, ground_hole_shape_intersection)
ECS_REQUIRE(TMatrix transform, bool ground_hole_sphere_shape, bool ground_hole_shape_intersection)
ECS_ON_EVENT(on_appear, on_disappear)
void ground_holes_changed_es(const ecs::Event &)
{
  set_holes_changed_ecs_query([&](bool &should_update_ground_holes_coll, bool *should_render_ground_holes) {
    should_update_ground_holes_coll = true;
    if (should_render_ground_holes)
      *should_render_ground_holes = true;
  });
}

template <typename Callable>
static void gather_holes_ecs_query(Callable c);

ECS_NO_ORDER
void ground_holes_update_coll_es(const ecs::UpdateStageInfoAct &, bool &should_update_ground_holes_coll)
{
  LandMeshManager *lmeshMgr = getLmeshMgr();
  if (!should_update_ground_holes_coll && lmeshMgr)
    return;

  eastl::vector<LandMeshHolesManager::HoleArgs> holes;
  gather_holes_ecs_query([&](const TMatrix &transform, bool ground_hole_sphere_shape, bool ground_hole_shape_intersection) {
    holes.emplace_back(transform, ground_hole_sphere_shape, ground_hole_shape_intersection);
  });

  lmeshMgr->clearAndAddHoles(holes);

  should_update_ground_holes_coll = false;
}