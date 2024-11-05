// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/editMode.h"
#include "main/ecsUtils.h"

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <daECS/core/utility/nullableComponent.h>
#include <3d/dag_render.h>
#include "game/dasEvents.h"
#include "game/player.h"
#include "phys/netPhys.h"
#include "camera/sceneCam.h"


static bool enabled = false;

template <typename Callable>
static inline void make_animchar_not_updatable_ecs_query(Callable);

void editmode::toggle()
{
  enabled = !enabled;

  if (enabled)
  {
    make_animchar_not_updatable_ecs_query([](ecs::EntityId eid ECS_REQUIRE_NOT(bool isAlive, ecs::Tag disableUpdate) ECS_REQUIRE(
                                            ecs::auto_type animchar)) { add_sub_template_async(eid, "disable_update"); });
  }
}

void editmode::select_nearest_actor()
{
  if (!enabled)
    return;

  ecs::EntityId heroEid = game::get_controlled_hero();
  ecs::EntityId closestEntity = find_closest_net_phys_actor(::get_cam_itm().getcol(3));
  if (closestEntity == heroEid)
    return;

  add_sub_template_async(game::get_controlled_hero(), "disable_update");

  // g_entity_mgr->tick();//ensure recreation async applied
  g_entity_mgr->sendEvent(game::get_local_player_eid(), PossessTargetByPlayer{closestEntity});
}

void editmode::toggle_pause_current()
{
  if (!enabled)
    return;
  const ecs::EntityId hero = game::get_controlled_hero();
  if (g_entity_mgr->has(hero, ECS_HASH("disableUpdate")))
    remove_sub_template_async(hero, "disable_update");
  else
    add_sub_template_async(hero, "disable_update");
  // g_entity_mgr->tick();//ensure recreation async applied
}
