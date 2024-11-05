// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/net/network.h>
#include <daECS/net/netEvent.h>
#include <daECS/net/message.h>
#include <memory/dag_framemem.h>
#include <math/dag_Matrix3.h>
#include <math/random/dag_random.h>

#include <EASTL/vector.h>

#include "phys/quantization.h"
#include "net/net.h"
#include "net/channel.h"
#include "net/recipientFilters.h"

#include "net/locSnapshots.h"

#include "game/player.h"

ECS_REGISTER_RELOCATABLE_TYPE(LocSnapshotsList, nullptr);
ECS_AUTO_REGISTER_COMPONENT(LocSnapshotsList, "loc_snapshots__snapshotData", nullptr, 0);

ECS_BROADCAST_EVENT_TYPE(TransformSnapshots, danet::BitStream);
ECS_REGISTER_EVENT(TransformSnapshots);

ECS_REGISTER_NET_EVENT(
  TransformSnapshots, net::Er::Broadcast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf, UNRELIABLE, NC_PHYS_SNAPSHOTS);

ECS_BROADCAST_EVENT_TYPE(TransformSnapshotsReliable, danet::BitStream);
ECS_REGISTER_EVENT(TransformSnapshotsReliable);

ECS_REGISTER_NET_EVENT(TransformSnapshotsReliable,
  net::Er::Broadcast,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::broadcast_rcptf,
  RELIABLE_UNORDERED,
  NC_PHYS_SNAPSHOTS);


ECS_UNICAST_EVENT_TYPE(EventSnapshotBlink, TMatrix /*from*/, TMatrix /*to*/, float /*dt*/);
ECS_REGISTER_EVENT(EventSnapshotBlink);

// TODO: move net:: to gameLibs, otherwise this code is doomed to be DNG specific
extern net::CNetwork *get_net_internal();

struct SnapshotEntityData
{
  ecs::EntityId eid;
  Point3 pos;
  Quat orientation;
  bool blink;
};

// TODO: move to das
ECS_ON_EVENT(on_appear)
static void init_snapshot_send_period_es_event_handler(
  const ecs::Event &, float loc_snapshots__sendRate, float &loc_snapshots__sendPeriod)
{
  loc_snapshots__sendPeriod = safeinv(loc_snapshots__sendRate);
}

inline bool verify_snapshot_quat(const Quat &q)
{
  unsigned mi = 0;
  float mx = fabsf(q[0]);
  for (unsigned i = 1; i < 4; ++i)
  {
    float fqi = fabsf(q[i]);
    if (fqi > mx)
    {
      mx = fqi;
      mi = i;
    }
  }
  bool sign = q[mi] < 0.0f;
  for (unsigned i = 0; i < 4; ++i)
  {
    if (i != mi)
    {
      float val = (sign ? -q[i] : q[i]);
      float unitVal = (val < 0 ? -val : val) / M_SQRT1_2;
      if (unitVal > 1.0f)
        return false;
    }
  }
  return true;
}

template <typename Callable>
static void gather_transform_snapshot_entities_ecs_query(Callable c);

template <typename Callable>
static void apply_transform_snapshot_ecs_query(ecs::EntityId eid, Callable c);

static void gather_snapshot_data(float at_time, eastl::vector<SnapshotEntityData, framemem_allocator> &buffer)
{
  // simplest implementation, which does not know anything about network LODs and just sends quantized
  // snapshots with a fixed framerate. we don't anticipate to have more than 10 entities with these snapshots
  // right now, but it must change if we'll start to have more entities with snapshots of this type
  // TODO: implement support for network LODs and thus reduce traffic
  gather_transform_snapshot_entities_ecs_query(
    [&](ECS_REQUIRE_NOT(ecs::Tag deadEntity) ECS_REQUIRE_NOT(ecs::Tag loc_snaphots__disabled) ecs::EntityId eid,
      const TMatrix &transform, float &loc_snapshots__sendAtTime, float loc_snapshots__sendPeriod,
      const ecs::Tag *loc_snapshots__scaledTransform, bool *loc_snapshots__blink) {
      if (at_time < loc_snapshots__sendAtTime)
        return;
      const float timeOverflow = at_time - loc_snapshots__sendAtTime;
      if (timeOverflow < loc_snapshots__sendPeriod)
        loc_snapshots__sendAtTime += loc_snapshots__sendPeriod;
      else
        loc_snapshots__sendAtTime = at_time + loc_snapshots__sendPeriod * gfrnd();

      bool blink = loc_snapshots__blink ? *loc_snapshots__blink : false;
      if (loc_snapshots__blink)
        *loc_snapshots__blink = false;

      if (!loc_snapshots__scaledTransform)
      {
        Quat rotation(transform);
#if DAGOR_DBGLEVEL > 0 && DEBUG_QUANTIZATION_CLAMPING
        if (!verify_snapshot_quat(rotation))
        {
          logerr(
            "%@: <%@> snapshot rotation is invalid: rotation='%f,%f,%f,%f' transform='%@' (loc_snapshots__scaledTransform missed?)",
            eid, g_entity_mgr->getEntityTemplateName(eid), rotation.x, rotation.y, rotation.z, rotation.w, transform);
          rotation.identity();
        }
#endif
        buffer.push_back({eid, transform.getcol(3), rotation, blink});
      }
      else
      {
        TMatrix tm = transform;
        tm.orthonormalize();
        buffer.push_back({eid, transform.getcol(3), Quat(tm), blink});
      }
    });
}

static void send_snapshots(const eastl::vector<SnapshotEntityData, framemem_allocator> &snap_data, float at_time)
{
  danet::BitStream tmpBs(framemem_ptr());
  const size_t num = snap_data.size();
  constexpr size_t batchSize = 32;
  G_STATIC_ASSERT(batchSize < eastl::numeric_limits<uint8_t>::max());
  const size_t batches = (num + batchSize - 1) / batchSize;
  for (size_t i = 0; i < batches; ++i)
  {
    size_t j = i * batchSize;
    const size_t to = eastl::min<size_t>((i + 1) * batchSize, num);

    tmpBs.ResetWritePointer();
    tmpBs.Write(at_time);
    tmpBs.Write(uint8_t(to - j));
    for (; j < to; ++j)
    {
      const SnapshotEntityData &entityData = snap_data[j];
      tmpBs.Write(entityData.eid);
      // TODO: add in motion flag, right now we don't know if this entity is in motion or not
      QuantizedWorldLocDefault qloc(entityData.pos, entityData.orientation, true);
      CachedQuantizedInfoT<>::template serializeQLoc<>(tmpBs, qloc, DPoint3(entityData.pos));
      tmpBs.Write(entityData.blink);
      // Note: the interval is not written. Because this loc snapshot implementation does not use it.
    }
    g_entity_mgr->broadcastEvent(TransformSnapshots(tmpBs));
  }
}

ECS_NO_ORDER
ECS_TAG(server, net)
static void send_transform_snapshots_es(const ecs::UpdateStageInfoAct &info)
{
  // Gather data
  eastl::vector<SnapshotEntityData, framemem_allocator> snapEntityData;
  gather_snapshot_data(info.curTime, snapEntityData);
  // Send data
  if (snapEntityData.size() > 0)
    send_snapshots(snapEntityData, info.curTime);
}

ECS_TAG(gameClient)
static void rcv_loc_snapshots_es_event_handler(const TransformSnapshots &evt)
{
  // decode snapshot
  const danet::BitStream &bs = evt.get<0>();

  bool isOk = true;

  float atTime = 0.f;
  isOk &= bs.Read(atTime);
  uint8_t num = 0;
  isOk &= bs.Read(num);

  if (!isOk)
    return;

  for (int i = 0; i < int(num); ++i)
  {
    ecs::EntityId eid;
    isOk &= bs.Read(eid);

    LocSnapshot snap;
    snap.atTime = atTime;
    bool inMotion;
    isOk &= CachedQuantizedInfoT<>::template deserializeQLoc<>(bs, snap.pos, snap.quat, inMotion);
    isOk &= bs.Read(snap.blink);

    if (!isOk)
    {
#if DAGOR_DBGLEVEL > 0
      logerr("%@: <%@> snapshot stream reading completed with errors (%d/%d). "
             "snap.pos='%@' snap.quat='%f,%f,%f,%f' inMotion='%@' atTime='%f'",
        eid, g_entity_mgr->getEntityTemplateName(eid), i, int(num) - 1, snap.pos, snap.quat.x, snap.quat.y, snap.quat.z, snap.quat.w,
        inMotion, atTime);
#endif
      break;
    }

    apply_transform_snapshot_ecs_query(eid, [&](LocSnapshotsList &loc_snapshots__snapshotData) {
      if (!loc_snapshots__snapshotData.empty() && loc_snapshots__snapshotData.back().atTime > snap.atTime)
        return; // we have a newer snapshot already, this one arrived out-of order
      loc_snapshots__snapshotData.emplace_back(eastl::move(snap));
    });
  }
}


ECS_NO_ORDER
ECS_TAG(gameClient)
ECS_REQUIRE_NOT(ecs::Tag loc_snapshots__dynamicInterpTime)
static void cleanup_loc_snapshots_es(
  const ecs::UpdateStageInfoAct &info, LocSnapshotsList &loc_snapshots__snapshotData, float loc_snapshots__interpTimeOffset = 0.15f)
{
  const float interpTime = info.curTime - loc_snapshots__interpTimeOffset;
  for (int i = loc_snapshots__snapshotData.size() - 2; i >= 0; --i)
  {
    const LocSnapshot &nextSnap = loc_snapshots__snapshotData[i + 1];
    if (nextSnap.atTime >= interpTime)
      continue;
    loc_snapshots__snapshotData.erase(loc_snapshots__snapshotData.begin(), loc_snapshots__snapshotData.begin() + i);
    break;
  }
}

ECS_BEFORE(before_animchar_update_sync)
ECS_TAG(gameClient)
ECS_REQUIRE_NOT(ecs::Tag loc_snapshots__dontUpdate, ecs::Tag loc_snapshots__dynamicInterpTime)
static void interp_loc_snapshots_es(const ecs::UpdateStageInfoAct &info,
  TMatrix &transform,
  const LocSnapshotsList &loc_snapshots__snapshotData,
  ecs::EntityId eid,
  float loc_snapshots__interpTimeOffset = 0.15f)
{
  if (loc_snapshots__snapshotData.empty())
  {
    // we don't have any snapshots for this entity - hide it
    transform.setcol(3, Point3(HIDDEN_QWPOS_XYZ));
    return;
  }
  if (loc_snapshots__snapshotData.size() == 1)
  {
    // we have only one snapshot - just use it for position
    const LocSnapshot &snap = loc_snapshots__snapshotData[0];
    transform = makeTM(snap.quat);
    transform.setcol(3, snap.pos);
    return;
  }
  const float interpTime = info.curTime - loc_snapshots__interpTimeOffset;
  for (int i = loc_snapshots__snapshotData.size() - 2; i >= 0; --i)
  {
    const LocSnapshot &curSnap = loc_snapshots__snapshotData[i];
    const LocSnapshot &nextSnap = loc_snapshots__snapshotData[i + 1];
    if (curSnap.atTime > interpTime)
      continue;
    // curSnap time is _before_ our curtime, use this pair to interpolate
    float interpK = nextSnap.blink ? 1.0 : cvt(interpTime, curSnap.atTime, nextSnap.atTime, 0.f, 1.f);
    transform = makeTM(qinterp(curSnap.quat, nextSnap.quat, interpK));
    transform.setcol(3, lerp(curSnap.pos, nextSnap.pos, interpK));
    if (nextSnap.blink && interpTime - info.dt <= curSnap.atTime)
    {
      TMatrix prevTransform = makeTM(curSnap.quat);
      prevTransform.setcol(3, curSnap.pos);
      g_entity_mgr->sendEvent(eid, EventSnapshotBlink(prevTransform, transform, nextSnap.atTime - curSnap.atTime));
    }
    break;
  }
}

void send_transform_snapshots_event(danet::BitStream &bitstream)
{
  g_entity_mgr->broadcastEvent(TransformSnapshots(eastl::move(bitstream)));
}

void send_reliable_transform_snapshots_event(danet::BitStream &bitstream)
{
  g_entity_mgr->broadcastEvent(TransformSnapshotsReliable(eastl::move(bitstream)));
}