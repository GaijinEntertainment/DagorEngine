//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <rendInst/riexHandle.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/entityId.h>
#include <daECS/core/event.h>

struct RiExtraComponent
{
  rendinst::riex_handle_t handle = rendinst::RIEX_HANDLE_NULL;
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb);
  RiExtraComponent(const ecs::EntityManager &mgr, ecs::EntityId eid);
  bool onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid);
};

ECS_DECLARE_RELOCATABLE_TYPE(RiExtraComponent);
ECS_BROADCAST_EVENT_TYPE(EventRendinstsLoaded);
ECS_BROADCAST_EVENT_TYPE(EventRendinstInitForLevel, rendinst::riex_handle_t /*riex_handle*/);
ECS_UNICAST_EVENT_TYPE(CmdDestroyRendinst, int32_t, bool); /* user_data, destroys_entity */

ecs::EntityId find_ri_extra_eid(rendinst::riex_handle_t handle);
void request_ri_resources(const ecs::resource_request_cb_t &res_cb, const char *riResName);
bool replace_ri_extra_res(ecs::EntityId eid, const char *res_name, bool destroy = false, bool add_restorable = false,
  bool create_destr = true);
