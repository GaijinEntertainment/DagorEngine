// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "physObj.h"
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/phys/turretControl.h>
#include <daECS/net/netbase.h>
#include <daECS/net/connection.h>
#include "net/time.h"
#include "net/channel.h"

#include "phys/physUtils.h"
#include "phys/netPhys.cpp.inl"

ECS_NET_DECL_MSG(PhysObjSnapshotsMsg, danet::BitStream);
ECS_NET_IMPL_MSG(PhysObjSnapshotsMsg,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::direct_connection_rcptf,
  UNRELIABLE,
  NC_PHYS_SNAPSHOTS,
  net::MF_DONT_COMPRESS);
IMPLEMENT_PHYS_ACTOR_NET_RCV(PhysObjSnapshotsMsg, get_phys_actor);

ECS_REGISTER_PHYS_ACTOR_TYPE(PhysObjActor);
ECS_AUTO_REGISTER_COMPONENT_DEPS(PhysObjActor,
  "phys_obj_net_phys",
  nullptr,
  0,
  "collres",
  "?phys_vars",
  "?animchar_render",
  "?animchar_node_wtm",
  "net__physId",
  "base_net_phys_ptr",
  "collision_physMatId");

/* static */ PhysObjCustomPhys::SnapshotCacheType PhysObjCustomPhys::snapshotCache;

struct PhysObjActorUpdateContext : public BasePhysActor::UpdateContext
{
  int *prevRShadowTick = nullptr;
};

template class PhysActor<PhysObj, PhysObjCustomPhys, PhysType::PHYSOBJ>;

void PhysObjCachedQuantizedInfo::update(const PhysObj &phys, ecs::EntityId eid, const ecs::EidList *turret_control__gunEids)
{
  BasePhysObjCachedQuantizedInfo::update(phys, eid);

  if (turret_control__gunEids)
  {
    turrets.resize(turret_control__gunEids->size());
    int i = 0;
    for (ecs::EntityId turretEid : *turret_control__gunEids)
    {
      auto turretState = ECS_GET_NULLABLE(TurretState, turretEid, turret_state_remote);
      turrets[i++] = eastl::make_pair(turretEid, turretState ? turretState->remote.getPackedState() : 0u);
    }
  }
}

ECS_TAG(netClient)
ECS_ON_EVENT(on_appear)
static void phys_obj_apply_snapshot_cache_on_creation_es_event_handler(
  const ecs::Event &, PhysObjActor &phys_obj_net_phys, int net__physId)
{
  auto it = PhysObjCustomPhys::snapshotCache.find(reinterpret_cast<const NetPhysId &>(net__physId).index);
  if (it != eastl::cend(PhysObjCustomPhys::snapshotCache))
  {
    phys_obj_net_phys.savePhysSnapshot(eastl::move(it->second));
    PhysObjCustomPhys::snapshotCache.erase(it);
  }
}

ECS_BEFORE(after_net_phys_sync, human_phys_es)
ECS_AFTER(before_net_phys_sync)
ECS_REQUIRE_NOT(ecs::Tag disableUpdate)
static inline void phys_obj_phys_es(const UpdatePhysEvent &info,
  PhysObjActor &phys_obj_net_phys,
  TMatrix &transform,
  int *net_phys__prevTick,
  const ecs::Tag *phys__broadcastAAS)
{
  PhysObjActorUpdateContext uctx;
  uctx.transform = &transform;
  uctx.physObjBroadcastAAS = phys__broadcastAAS != nullptr;
  if (net_phys__prevTick && phys_obj_net_phys.getRole() == IPhysActor::ROLE_REMOTELY_CONTROLLED_SHADOW)
    uctx.prevRShadowTick = net_phys__prevTick;

  net_phys_update_es(info, phys_obj_net_phys, uctx);
  phys_obj_net_phys.phys.updatePhysInWorld(transform);

  if (uctx.prevRShadowTick)
    *uctx.prevRShadowTick = gamephys::nearestPhysicsTickNumber(get_sync_time(), phys_obj_net_phys.phys.timeStep);
}

ECS_ON_EVENT(NET_PHYS_ES_EVENT_SET)
static inline void phys_obj_phys_events_es_event_handler(const ecs::Event &event, PhysObjActor &phys_obj_net_phys)
{
  auto evt = event.cast<ecs::EventNetMessage>();
  if (auto physSnapMsg = evt ? evt->get<0>()->cast<PhysObjSnapshotsMsg>() : nullptr)
  {
    G_FAST_ASSERT(physSnapMsg->connection);
    net_rcv_phys_snapshots(*physSnapMsg, /*to_msg_sink*/ false, physSnapMsg->get<0>(), *physSnapMsg->connection,
      PhysObjActor::physEidsList, &get_phys_actor, PhysObjActor::deserializePhysSnapshot, PhysObjActor::processSnapshotNoEntity);
  }
  else
    net_phys_event_handler(event, phys_obj_net_phys);
}

template <>
void PhysObjActor::serializePhysSnapshotImpl(danet::BitStream &bs, PhysSnapSerializeType pst) const
{
  uint8_t turretsCount = (pst <= PhysSnapSerializeType::MIN) ? cachedQuantInfo.turrets.size() : 0;
  if (turretsCount)
    bs.WriteBits(&turretsCount, CHAR_BIT - PST_BITS);

  serializePhysSnapshotBase(bs, pst);

  if (pst > PhysSnapSerializeType::MIN)
    return;

  if (turretsCount)
    for (auto &turrInfo : cachedQuantInfo.turrets)
    {
      net::write_server_eid((ecs::entity_id_t)turrInfo.first, bs);
      bs.WriteBits((const uint8_t *)&turrInfo.second, TurretWishDirPacked::TotalBits);
    }

  bs.AlignWriteToByteBoundary();
}

template <>
bool PhysObjActor::deserializePhysSnapshot(
  const danet::BitStream &bs, PhysSnapSerializeType pst, BasePhysSnapshot &basesnap, net::IConnection &conn)
{
  PhysObjSnapshot &snap = *new (&basesnap, _NEW_INPLACE) PhysObjActor::SnapshotType;

  uint8_t turretsCount = 0;
  bool isOk = bs.ReadBits(&turretsCount, CHAR_BIT - PST_BITS);

  isOk &= deserializeBasePhysSnapshot(bs, pst, snap, conn);

  if (pst > PhysSnapSerializeType::MIN)
    return isOk;

  if (turretsCount)
  {
    snap.turrets.resize(turretsCount);
    for (int i = 0; i < turretsCount; ++i)
    {
      isOk &= net::read_eid(bs, snap.turrets[i].first);
      uint32_t packedTurrDir = 0;
      isOk &= bs.ReadBits((uint8_t *)&packedTurrDir, TurretWishDirPacked::TotalBits);
      snap.turrets[i].second = TurretWishDirPacked(packedTurrDir).unpack();
    }
  }

  G_ASSERT((bs.GetReadOffset() % CHAR_BIT) == 0);

  return isOk;
}

template <>
void PhysObjActor::processSnapshotNoEntity(uint16_t netPhysId, BasePhysSnapshot &&basesnap)
{
  SnapshotType &snap = static_cast<SnapshotType &>(basesnap);
  auto ins = PhysObjCustomPhys::snapshotCache.emplace(netPhysId, eastl::move(snap));
  if (!ins.second)
    ins.first->second = eastl::move(snap);
  eastl::destroy_at(&snap);
}

template <>
void PhysObjActor::updateRemoteShadow(float remote_time, float dt, const BasePhysActor::UpdateContext &uctx_)
{
  if (ECS_GET_NULLABLE(ecs::Tag, getEid(), phys__disableSnapshots) == nullptr)
  {
    const SnapshotPair *spair = updateRemoteShadowBase(remote_time, dt, uctx_);
    if (!spair)
      return;

    for (auto &turrInfo : spair->lastSnap.turrets)
      if (auto turretState = ECS_GET_NULLABLE_RW(TurretState, turrInfo.first, turret_state_remote))
        turretState->remote.setWishDirectionRaw(turrInfo.second);
  }
  g_entity_mgr->sendEventImmediate(getEid(), CmdUpdateRemoteShadow(phys.currentState.atTick, phys.timeStep));
}

PhysObjActor &PhysObjCustomPhys::getPhysObj()
{
  return *reinterpret_cast<PhysObjActor *>((char *)this - offsetof(PhysObjActor, customPhys));
}

/* static */
void PhysObjCustomPhys::onPostInit(ecs::EntityId eid, BasePhysActor &act)
{
  auto &phys = static_cast<PhysObjActor &>(act).phys;
  float curTimeDelayed = max(get_sync_time() - phys_get_present_time_delay_sec(act.tickrateType, phys.timeStep), 0.f);
  phys.currentState.atTick = gamephys::nearestPhysicsTickNumber(curTimeDelayed, phys.timeStep);
  phys.currentState.velocity = g_entity_mgr->getOr(eid, ECS_HASH("start_vel"), phys.currentState.velocity);
  phys.previousState.velocity = phys.currentState.velocity;
  phys.currentState.omega = g_entity_mgr->getOr(eid, ECS_HASH("start_omega"), phys.currentState.omega);
  if (const ecs::EidList *ignoreObjsEids = ECS_GET_NULLABLE(ecs::EidList, eid, ignoreObjs__eids))
    for (int i = 0; i < ignoreObjsEids->size(); ++i)
      phys.ignoreGameObjs.push_back(static_cast<ecs::entity_id_t>((*ignoreObjsEids)[i]));
  phys.ignoreGameObjsTime = g_entity_mgr->getOr(eid, ECS_HASH("ignoreObjs__time"), 0.0f);
  ECS_SET(eid, collision_physMatId, phys.physMatId);
}

void PhysObjCustomPhys::onPostUpdate(float, const BasePhysActor::UpdateContext &uctx_)
{
  auto &physobj = getPhysObj();
  auto &uctx = static_cast<const PhysObjActorUpdateContext &>(uctx_);
  if (uctx.prevRShadowTick)
    if (gamephys::ceilPhysicsTickNumber(get_sync_time(), physobj.phys.timeStep) > *uctx.prevRShadowTick)
      g_entity_mgr->sendEventImmediate(physobj.getEid(),
        CmdPostPhysUpdateRemoteShadow(physobj.phys.currentState.atTick, physobj.phys.timeStep));
}

PHYS_IMPLEMENT_ON_TICKRATE_CHANGED_ES(PhysObjActor, phys_obj_net_phys)
PHYS_IMPLEMENT_COMMON_ESES(PhysObjActor, phys_obj_net_phys, phys_obj_phys_es, "collidableToPhysObj")

#include "phys/netPhysRegister.h"
REGISTER_PHYS_SLOT(PhysType::PHYSOBJ, "phys_obj", PhysObjActor, phys_obj_net_phys, &PhysObjCustomPhys::onSnapshotCacheReset);
MAKE_GATHER_SEND_RECORDS_HANDLER(PhysObjActor, phys_obj_net_phys, PhysObjSnapshotsMsg);

#include "phys/undergroundTeleporter.h"
ECS_TAG(server)
ECS_NO_ORDER
static void underground_physobj_teleporter_es(const ecs::UpdateStageInfoAct &info,
  ecs::EntityId eid,
  const PhysObjActor &phys_obj_net_phys,
  float &underground_teleporter__timeToCheck,
  float underground_teleporter__timeBetweenChecks = 5.f,
  float underground_teleporter__heightOffset = 0.7f)
{
  underground_teleport(info.dt, eid, phys_obj_net_phys.phys, underground_teleporter__timeToCheck,
    underground_teleporter__timeBetweenChecks, underground_teleporter__heightOffset);
}
