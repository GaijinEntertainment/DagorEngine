//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/entityComponent.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_smallTab.h>
#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>

class DataBlock;


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
