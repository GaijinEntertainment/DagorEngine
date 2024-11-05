//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/riexHandle.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h>
#include <rendInst/constants.h>
#include <3d/dag_resPtr.h>

struct RiExtraComponent
{
  rendinst::riex_handle_t handle = rendinst::RIEX_HANDLE_NULL;
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb);
  RiExtraComponent(const ecs::EntityManager &mgr, ecs::EntityId eid);
};

ECS_DECLARE_RELOCATABLE_TYPE(RiExtraComponent);
ECS_BROADCAST_EVENT_TYPE(EventRendinstsLoaded);
ECS_BROADCAST_EVENT_TYPE(EventRendinstInitForLevel, rendinst::riex_handle_t /*riex_handle*/);
ECS_BROADCAST_EVENT_TYPE(EventOnRendinstDamage, rendinst::riex_handle_t, TMatrix, BBox3);
ECS_UNICAST_EVENT_TYPE(EventRendinstSpawned);
ECS_UNICAST_EVENT_TYPE(CmdDestroyRendinst, int32_t, bool);                            /* user_data, destroys_entity */
ECS_UNICAST_EVENT_TYPE(EventOnRendinstDestruction, int32_t);                          /* user_data */
ECS_UNICAST_EVENT_TYPE(EventRendinstImpulse, float, Point3, Point3, Point3, int32_t); /* impulse, impulse_dir, impulse_pos,
                                                                                         collision_normal, user_data */


ecs::EntityId find_ri_extra_eid(rendinst::riex_handle_t handle);
void request_ri_resources(const ecs::resource_request_cb_t &res_cb, const char *riResName);
bool replace_ri_extra_res(ecs::EntityId eid, const char *res_name, bool destroy = false, bool add_restorable = false,
  bool create_destr = true);
int get_or_add_ri_extra_res_id(const char *res_name, rendinst::AddRIFlags ri_flags);
void update_per_draw_gathered_data(uint32_t id);
const UniqueBuf &get_per_draw_gathered_data();

namespace rendinst
{
void init_phys();
}