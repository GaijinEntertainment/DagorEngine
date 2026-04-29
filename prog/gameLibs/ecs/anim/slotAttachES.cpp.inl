// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/slotAttach.h>
#include <ecs/anim/anim.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/utility/ecsRecreate.h>

#include <animChar/dag_animCharacter2.h>
#include <vecmath/dag_vecMath.h>

#include <ioSys/dag_dataBlock.h>
#include <daECS/delayedAct/actInThread.h>

ECS_REGISTER_EVENT_NS(anim, EventItemAttached);
ECS_REGISTER_EVENT_NS(anim, EventItemDetached);
ECS_REGISTER_EVENT_NS(anim, CmdInitSkeletonAttach);
ECS_REGISTER_EVENT_NS(anim, CmdInitSlotAttach);

void anim::attach(int &slot_attach__slotId, ecs::EntityId eid, int slot_id, ecs::EntityId attach_eid)
{
  auto &mgr = *g_entity_mgr; // fixme: should be passed as argument
  mgr.set(attach_eid, ECS_HASH("animchar_attach__attachedTo"), eid);
  slot_attach__slotId = slot_id;
}

void anim::detach(int &slot_attach__slotId, ecs::EntityId attach_eid)
{
  auto &mgr = *g_entity_mgr; // fixme: should be passed as argument
  ecs::EntityId &attachedTo = mgr.getRW<ecs::EntityId>(attach_eid, ECS_HASH("animchar_attach__attachedTo"));
  slot_attach__slotId = -1;
  attachedTo = ecs::INVALID_ENTITY_ID;
}

void anim::init_attachements(const DataBlock &blk)
{
  const DataBlock *slotsBlk = blk.getBlockByNameEx("attachmentSlots");
  for (int slotId = 0; slotId < slotsBlk->paramCount(); ++slotId)
  {
    const char *slotName = slotsBlk->getStr(slotId);
    const int curSlotId = AnimCharV20::addSlotId(slotName);
    if (slotId != curSlotId)
      logerr("Prefetch slot ids mismatch: %s - %d != %d", slotName, curSlotId, slotId);
  }
}
