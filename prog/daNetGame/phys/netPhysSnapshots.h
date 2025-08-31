// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "netPhys.h"
#include "physEvents.h"
#include "net/channel.h"
#include <generic/dag_smallTab.h>
#include <memory/dag_framemem.h>
#include <math/dag_Point4.h>

enum PhysActorStateFlags : uint8_t
{
  NO_FLAGS = 0,
  TICK_NOT_CHANGED = 1 << 0,
  IN_VEHICLE = 1 << 1,
  LOW_FREQ_TICKRATE = 1 << 2,
  PHYS_ASLEEP = 1 << 3,
  PHYS_ALWAYS_SEND_SHANPSHOT = 1 << 4
};

struct PhysActorSendRecord
{
  ecs::EntityId eid; //< Note: this can be omitted since we might accesss it via "actor.eid"
  eastl::underlying_type_t<PhysActorStateFlags> flags;
  uint8_t connControlledBy;
  int8_t team; //< Rarely used, could be removed if really need to
  // one byte padding
  const BasePhysActor &actor;
  //! Note: intentionally by value in order to avoid pointer chasing. Also manually aligned to be in offset 16 (on 64 bit platforms)
  Point3 pos;
  uint16_t netPhysId;
  uint16_t physTick;

  PhysTickRateType getTickrateType() const
  {
    return (flags & LOW_FREQ_TICKRATE) ? PhysTickRateType::LowFreq : PhysTickRateType::Normal;
  }
};

typedef void (BasePhysActor::*serialize_phys_snap_t)(danet::BitStream &, PhysSnapSerializeType) const;
typedef void (*send_msg_t)(ecs::EntityId eid, danet::BitStream &&bs, net::IConnection &conn, bool reliable);

struct SendRecordsTypeRange
{
  uint16_t size;
  PhysType physType;
  serialize_phys_snap_t serializePhysSnapshot;
  send_msg_t sendCb;
  vec4f netLodZones;
};

static constexpr const net::MessageNetDesc reliable_phys_snap_msg_net_desc = {
  net::ROUTING_SERVER_TO_CLIENT, RELIABLE_UNORDERED, NC_DEFAULT, net::MF_DONT_COMPRESS, /*dupDelay*/ 0, &net::direct_connection_rcptf};

template <typename M>
inline void send_snapshot_t(ecs::EntityId eid, danet::BitStream &&bs, net::IConnection &conn, bool reliable)
{
  M msg;
  msg.template get<0>().swap(bs);
  msg.connection = &conn;
  send_net_msg(eid, eastl::move(msg), reliable ? &reliable_phys_snap_msg_net_desc : nullptr);
  msg.template get<0>().swap(bs); //< swap it back to preserve allocated memory buffers (in case if it wasn't really moved)
}

struct GatherRecordsToSend
{
  //! CLOSEST, NORMAL, FARAWAY, MIN
  static constexpr const vec4f def_net_lod_zones_dist = DECL_VECFLOAT4(sqr(30.f), sqr(150.f), sqr(250.f), sqr(500.f));

  PhysType curPhysType = PhysType::NUM;
  SmallTab<PhysActorSendRecord, framemem_allocator> sendRecords;
  StaticTab<SendRecordsTypeRange, (int)PhysType::NUM> sendRecordsRanges;

  inline void addRecToSend(
    PhysActorSendRecord &&r, PhysType pt, send_msg_t send_cb, serialize_phys_snap_t serialize_cb, const Point4 *netLodZones)
  {
    sendRecords.push_back(eastl::move(r));
    if (!sendRecordsRanges.empty() && curPhysType == pt)
      sendRecordsRanges.back().size++;
    else
    {
      auto &curRange = sendRecordsRanges.push_back();
      curRange.size = 1;
      curRange.physType = curPhysType = pt;
      curRange.sendCb = send_cb;
      curRange.serializePhysSnapshot = serialize_cb;
      curRange.netLodZones = netLodZones ? v_ldu(&netLodZones->x) : def_net_lod_zones_dist;
    }
  }
};

ECS_BROADCAST_EVENT_TYPE(GatherNetPhysRecordsToSend, GatherRecordsToSend *)
