// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "phys/dasPhysObj.h"
#include <ecs/core/entityManager.h>

#include "phys/physUtils.h"
#include "phys/netPhys.cpp.inl"
#include <phys/physEvents.h>

TEMPLATE_PHYS_ACTOR
const net::MessageClass *PHYS_ACTOR::getMsgCls() { return nullptr; }
TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::net_rcv_snapshots(const net::IMessage *) {}

ECS_REGISTER_PHYS_ACTOR_TYPE(DasPhysObjActor);
ECS_AUTO_REGISTER_COMPONENT_DEPS(DasPhysObjActor, "das_phys_obj_net_phys", nullptr, 0, "net__physId", "base_net_phys_ptr");

template class PhysActor<DasPhysObj, DummyCustomPhys, PhysType::DAS_PHYSOBJ>;

ECS_BEFORE(after_net_phys_sync)
ECS_AFTER(before_net_phys_sync)
ECS_REQUIRE(eastl::true_type updatable)
static inline void das_phys_obj_walker_phys_es(const UpdatePhysEvent &info, DasPhysObjActor &das_phys_obj_net_phys, TMatrix &transform)
{
  BasePhysActor::UpdateContext uctx;
  uctx.transform = &transform;

  net_phys_update_es(info, das_phys_obj_net_phys, uctx);
}

ECS_ON_EVENT(NET_PHYS_ES_EVENT_SET)
static inline void das_phys_obj_phys_events_es_event_handler(const ecs::Event &event, DasPhysObjActor &das_phys_obj_net_phys)
{
  net_phys_event_handler(event, das_phys_obj_net_phys);
}

PHYS_IMPLEMENT_APPLY_AUTHORITY_STATE(DasPhysObjActor, das_phys_obj_net_phys, das_phys_obj_walker_phys_es)

#include "netPhysRegister.h"
REGISTER_PHYS_SLOT(PhysType::DAS_PHYSOBJ, "das_phys_obj", DasPhysObjActor, das_phys_obj_net_phys, nullptr);
