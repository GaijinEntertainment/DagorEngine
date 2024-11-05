// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "netPhys.h"
#include "netPhysSnapshots.h"

#define REGISTER_PHYS_SLOT(SLOT, NAME, TYPE, COMP, SS_RESET_CB)                                                             \
  ECS_NO_ORDER                                                                                                              \
  static inline void register_##COMP##_phys_on_appinit_es_event_handler(const EventOnGameInit &)                            \
  {                                                                                                                         \
    G_STATIC_ASSERT(ecs::ComponentTypeInfo<TYPE>::is_boxed); /*== for base_net_phys_ptr */                                  \
    BasePhysActor::registerPhys(SLOT, NAME, #COMP, ECS_HASH(#COMP).hash, ecs::ComponentTypeInfo<TYPE>().type, sizeof(TYPE), \
      sizeof(TYPE::SnapshotType), &TYPE::physIdGenerations, &TYPE::freeActorIndexes, &TYPE::syncStatesPacked, SS_RESET_CB,  \
      TYPE::getMsgCls(), &TYPE::net_rcv_snapshots);                                                                         \
  }

#define MAKE_GATHER_SEND_RECORDS_HANDLER(PTYPE, PNAME, MSG)                                                                          \
  ECS_NO_ORDER                                                                                                                       \
  static inline void gather_##PNAME##_es_event_handler(const GatherNetPhysRecordsToSend &evt, ecs::EntityId eid,                     \
    ECS_REQUIRE_NOT(ecs::Tag phys__disableSnapshots) PTYPE &PNAME, const net::Object &replication, const int *team,                  \
    const Point4 *netLodZones, const ecs::EidList *turret_control__gunEids, ecs::Tag *physAlwaysSendSnapshot)                        \
  {                                                                                                                                  \
    /* Odd init via curly braces because '=' trips ecs codegen (due to default args impl) */                                         \
    uint8_t connControlledBy{(uint8_t)replication.getControlledBy()};                                                                \
    decltype(PhysActorSendRecord::flags) flags{PNAME.isAsleep() ? PHYS_ASLEEP : NO_FLAGS};                                           \
    int physTick{PNAME.phys.currentState.atTick};                                                                                    \
    if (PNAME.tickrateType == PhysTickRateType::LowFreq)                                                                             \
    {                                                                                                                                \
      flags |= LOW_FREQ_TICKRATE;                                                                                                    \
      if (physTick == PNAME.cachedQuantInfo.atTick)                                                                                  \
        flags |= TICK_NOT_CHANGED;                                                                                                   \
    }                                                                                                                                \
    if (!(flags & (TICK_NOT_CHANGED | PHYS_ASLEEP)) || PNAME.cachedQuantInfo.isInvalid())                                            \
      PNAME.cachedQuantInfo.update(PNAME.phys, eid, turret_control__gunEids);                                                        \
    if (physAlwaysSendSnapshot != nullptr)                                                                                           \
      flags |= PHYS_ALWAYS_SEND_SHANPSHOT;                                                                                           \
    evt.get<0>()->addRecToSend({eid, flags, connControlledBy, (int8_t)(team ? *team : TEAM_UNASSIGNED), PNAME,                       \
                                 Point3::xyz(PNAME.phys.currentState.location.P), (uint16_t)PNAME.netPhysId, (uint16_t)physTick},    \
      PTYPE::staticPhysType, &send_snapshot_t<MSG>, static_cast<serialize_phys_snap_t>(&PTYPE::serializePhysSnapshot), netLodZones); \
  }
