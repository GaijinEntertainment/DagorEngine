//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/entityComponent.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_smallTab.h>
#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>

class DataBlock;

inline void remove_from_eid_list(ecs::EntityId attach_eid, ecs::EidList &list)
{
  auto it = eastl::find(list.begin(), list.end(), attach_eid);
  if (it != list.end())
    list.erase(it);
}

namespace anim
{
void init_attachements(const DataBlock &blk);

const mat44f *get_attach_slot_wtm(int slot_attach__slotId, ecs::EntityId attach_eid);
void attach(int &slot_attach_slotId, ecs::EntityId eid, int slot_id, ecs::EntityId attach_eid);
void detach(int &slot_attach_slotId, ecs::EntityId attach_eid);

ECS_UNICAST_EVENT_TYPE(EventItemAttached, ecs::EntityId /*to*/);
ECS_UNICAST_EVENT_TYPE(EventItemDetached, ecs::EntityId /*from*/);

ECS_UNICAST_EVENT_TYPE(CmdInitSkeletonAttach);
ECS_UNICAST_EVENT_TYPE(CmdInitSlotAttach);
} // namespace anim
