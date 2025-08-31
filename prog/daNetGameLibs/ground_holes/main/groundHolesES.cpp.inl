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
#include <rendInst/rendInstGen.h>
#include <gui/dag_visualLog.h>
#include "main/level.h"
#include <groundHolesCore/helpers.h>

ECS_ON_EVENT(EventLevelLoaded)
static void ground_holes_on_level_loaded_es(const ecs::Event &, uint8_t &ground_holes_gen)
{
  if (hasGroundHoleManager())
    ground_holes_gen++;
  else
    logerr("ground_holes_updater: Ground hole manager wasn't found! Please enable it "
           "(level__useGroundHolesCollision:b=yes) and reload level");
}

template <typename Callable>
static void set_holes_changed_ecs_query(Callable c);

ECS_TRACK(transform, ground_hole_sphere_shape, ground_hole_shape_intersection)
ECS_REQUIRE(TMatrix transform, bool ground_hole_sphere_shape, bool ground_hole_shape_intersection)
ECS_ON_EVENT(on_appear, on_disappear)
static void ground_holes_changed_es(const ecs::Event &)
{
  bool hasGroundHolesMgr = hasGroundHoleManager();
  set_holes_changed_ecs_query([&](uint8_t &ground_holes_gen, bool *should_render_ground_holes) {
    if (hasGroundHolesMgr)
      ground_holes_gen++;
    if (should_render_ground_holes)
      *should_render_ground_holes = true;
  });
}

template <typename Callable>
static void gather_holes_ecs_query(Callable c);

ECS_TRACK(ground_holes_gen)
ECS_ON_EVENT(on_appear)
static void ground_holes_update_coll_es(const ecs::Event &, uint8_t &ground_holes_gen, bool *should_render_ground_holes)
{
  LandMeshManager *lmeshMgr = getLmeshMgr();
  if (!lmeshMgr)
    return;
  // To consider: may be wait for gameObjects's creation before starting to prepare rendInsts?
  if (DAGOR_UNLIKELY(!rendinst::isRIGenPrepareFinished()))
  {
    ground_holes_gen++; // RendInst gen in progress -> can't modify holes (yet) -> retry on next frame
    return;
  }
  dacoll::fetch_sim_res(true); // HeightField might call ground holes methods

  Tab<LandMeshHolesManager::HoleArgs> holes(framemem_ptr());
  // TODO: rework these bools to ecs::Tag
  gather_holes_ecs_query([&](const TMatrix &transform, bool ground_hole_sphere_shape, bool ground_hole_shape_intersection) {
    holes.emplace_back(transform, ground_hole_sphere_shape, ground_hole_shape_intersection);
  });

  lmeshMgr->clearAndAddHoles(holes);
  if (should_render_ground_holes)
    *should_render_ground_holes = true;
}
