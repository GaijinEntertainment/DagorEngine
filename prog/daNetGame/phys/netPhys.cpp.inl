// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "phys/netPhys.h"
#include "game/gameEvents.h"
#include "physEvents.h"
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include <daECS/net/object.h>
#include <daECS/net/netbase.h>
#include <ecs/phys/physVars.h>
#include <ecs/phys/collRes.h>
#include <ecs/phys/physEvents.h>
#include <ecs/game/generic/grid.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/bitset.h>
#include "net/net.h"
#include "net/netStat.h"
#include "net/time.h"
#include <gameMath/quantization.h>
#include <daECS/net/msgDecl.h>
#include "net/dedicated.h"
#include <crypto/base64.h>
#include <daECS/net/msgSink.h>
#include "game/player.h"
#include "game/team.h"
#include "main/app.h"
#include "main/level.h"
#include "phys/physUtils.h"
#include "camera/sceneCam.h"
#include <ioSys/dag_dataBlock.h>
#if !DISABLE_SYNC_DEBUG
#include <syncDebug/syncDebug.h>
#endif
#include <gamePhys/phys/utils.h>
#include <util/dag_string.h>
#include <util/dag_convar.h>
#include <util/dag_delayedAction.h>
#include "net/plosscalc.h"
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/collisionResponse.h>
#include <gamePhys/collision/contactData.h>
#include <gamePhys/phys/physToSolverBody.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <scene/dag_physMat.h>
#include "phys/gridCollision.h"
#include <ecs/anim/anim.h>
#include <math/dag_geomTree.h>
#include "navMeshPhysProxy.h"

#include <gamePhys/phys/physResync.inc.cpp>

extern ConVarF cl_exp_aprch_k;
extern ConVarB sv_debug_phys_desyncs;

#define DESTROY_ENTITY_FLOOR_Y          -(QuantizedWorldPosYScale() - 100.f)
#define PHYS_SNAP_TELEPORT_THRESHOLD_SQ (400.f)

// World box includes non-quantized physics (i.e. airplanes)
static const BBox3 PHYS_ENTITY_WORLD_BBOX(Point3(-1e5f, DESTROY_ENTITY_FLOOR_Y, -1e5f), Point3(1e5f, 3e4f, 1e5f));
static const QuantizedWorldLocSmall HIDDEN_QWLOC(Point3(HIDDEN_QWPOS_XYZ), Quat{0, 0, 0, 1}, /*in_motion*/ false);

class CollisionResource;

namespace physreg
{
extern const char *typeStrs[(int)PhysType::NUM];
}

#define ECS_GETC(T, v, eid, n) T *v = g_entity_mgr->getNullableRW<T>(eid, ECS_HASH(#n))
#define TEMPLATE_PHYS_ACTOR    template <typename PhysImpl, typename CustomPhys, PhysType phys_type>
#define PHYS_ACTOR             PhysActor<PhysImpl, CustomPhys, phys_type>
#define SAFE_TIME_TO_REUSE_PHYS_ACTOR_INDEX \
  1.f // Assume that this much sec is enough to replicate death of entity to all remote systems
#if !DISABLE_SYNC_DEBUG && DAGOR_DBGLEVEL > 0
#define PHYS_DEBUG_DESYNCS_BITSTREAM &fm_sync_dump_output_stream
#else
#define PHYS_DEBUG_DESYNCS_BITSTREAM nullptr
#endif

template <typename PhysActor, typename PhysImpl, typename... Args>
static void apply_authority_state_and_log_desync(PhysActor *unit, PhysImpl &phys, Args &&...args)
{
  uint32_t prevProcessedAAS{phys.desyncStats.processed};
  uint32_t prevDesyncsAAS{phys.desyncStats.desyncs()};
  uint32_t prevProcessedPAAS{phys.partialStateDesyncStats.processed};
  uint32_t prevDesyncsPAAS{phys.partialStateDesyncStats.desyncs()};

  apply_authority_state(unit, phys, eastl::forward<Args>(args)...);

  netstat::inc(netstat::CT_AAS_PROCESSED, phys.desyncStats.processed - prevProcessedAAS);
  netstat::inc(netstat::CT_AAS_DESYNCS, phys.desyncStats.desyncs() - prevDesyncsAAS);
  netstat::inc(netstat::CT_PAAS_PROCESSED, phys.partialStateDesyncStats.processed - prevProcessedPAAS);
  netstat::inc(netstat::CT_PAAS_DESYNCS, phys.partialStateDesyncStats.desyncs() - prevDesyncsPAAS);

  {
    const gamephys::Loc &posDiff = phys.desyncStats.lastPosDifference;
    double diffLength = posDiff.P.length();
    netstat::max(netstat::CT_AAS_DESYNC_POS_DIFF, diffLength * 100); // * 100 to get 2 decimal places
  }
  {
    const gamephys::Loc &posDiff = phys.partialStateDesyncStats.lastPosDifference;
    double diffLength = posDiff.P.length();
    netstat::max(netstat::CT_PAAS_DESYNC_POS_DIFF, diffLength * 100); // * 100 to get 2 decimal places
  }
}

template <typename PhysSnapshot>
static double position_diff_wrt_velocity_sq(const PhysSnapshot &prev, const PhysSnapshot &last, float interp_time)
{
  auto expectedPos = prev.pos + prev.vel * interp_time;
  return lengthSq(expectedPos - last.pos);
}

// To consider: rework this to phys actor trait
template <typename T>
static bool is_snapshot_quantized(const T *)
{
  return QUANTIZE_PHYS_SNAPSHOTS;
}

TEMPLATE_PHYS_ACTOR
/*static*/ phys_eids_list_t PHYS_ACTOR::physEidsList;
TEMPLATE_PHYS_ACTOR
/*static*/ phys_id_gen_list_t PHYS_ACTOR::physIdGenerations;
TEMPLATE_PHYS_ACTOR
/*static*/ free_actors_list_indexes_t PHYS_ACTOR::freeActorIndexes;
TEMPLATE_PHYS_ACTOR
/*static*/ phys_sync_states_packed_t PHYS_ACTOR::syncStatesPacked;

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::initRole() const // FIXME: why const?
{
  G_ASSERTF(!(is_server() && clientSideOnly), "Client-side only entity created on server");
  auto role = (is_server() || clientSideOnly) ? ROLE_LOCALLY_CONTROLLED_AUTHORITY : ROLE_REMOTELY_CONTROLLED_SHADOW;
  const_cast<PHYS_ACTOR *>(this)->setRoleAndTickrateType(role, tickrateType);
}

ECS_DECLARE_GET_FAST_BASE(ecs::Tag, phys__broadcastAAS, "phys__broadcastAAS");
extern void stash_controls_msg(ecs::EntityId to_eid, ControlsMsg &&controlsMsg);

// FIXME: move this to base phys code
template <typename Phys>
class ControlsSerializer
{
public:
  static void beforeEnqueueAsUnapproved(Phys &) {}
  static void serialize(danet::BitStream &bs, const Phys &phys)
  {
    phys.unapprovedCT.back().serialize(bs);
    phys.appendControlsHistory(bs, PHYS_CONTROLS_NET_REDUNDANCY - 1);
  }
  static bool deserialize(const danet::BitStream &bs, Phys &phys)
  {
    int32_t anomalyFlags = 0;
    constexpr int maxQueueSize = PHYS_MAX_CONTROLS_TICKS_DELTA_SEC * PHYS_MAX_TICKRATE * 2;
    return phys.receiveControlsPacket(bs, maxQueueSize, anomalyFlags);
  }
};
TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::doEnqueueCT(float at_time)
{
  if (!isLocalControl() || phys.isAsleep())
    return;

  int ctTick = gamephys::floorPhysicsTickNumber(at_time, phys.timeStep);

  ControlsSerializer<PhysImpl>::beforeEnqueueAsUnapproved(phys); // client must update data for unapproved CT
  if (!phys.isNeedToSaveControlsAt(ctTick) || !beforeEnqueueCT(at_time))
    return;

  phys.saveProducedCTAsUnapproved(ctTick);
  phys.producedCT.stepHistory();

  if (getRole() == IPhysActor::ROLE_LOCALLY_CONTROLLED_SHADOW)
  {
    // In case of phys__broadcastAAS ASS will be sent to all of the clients
    // Such entity hasn't any controls.
    // So, don't even try to send ControlsMsg
    if (PHYS_ACTOR::staticPhysType != PhysType::PHYSOBJ || // Note: assume that only physObj might have broadcastAAS
        DAGOR_LIKELY(!ECS_GET_NULLABLE(ecs::Tag, getEid(), phys__broadcastAAS)))
    {
      ControlsMsg msg(danet::BitStream(128, framemem_ptr()));
      ControlsSerializer<PhysImpl>::serialize(msg.get<0>(), phys);
      stash_controls_msg(eid, eastl::move(msg));
    }
  }

  afterEnqueueCT();
}

template <typename T>
static void destroy_fallthrough_phys_actor(const T &actor, const TMatrix &tm)
{
  const Point3 &pos = tm.getcol(3);
  if (DAGOR_UNLIKELY(!(PHYS_ENTITY_WORLD_BBOX & pos)))
  {
    int logLev = (pos.y < DESTROY_ENTITY_FLOOR_Y) ? LOGLEVEL_WARN : LOGLEVEL_ERR;
    logmessage(logLev, "Destroying entity %d<%s> because it's pos %@ exceeds 'world bbox' (%@, %@)", ecs::entity_id_t(actor.getEid()),
      g_entity_mgr->getEntityTemplateName(actor.getEid()), pos, PHYS_ENTITY_WORLD_BBOX.lim[0], PHYS_ENTITY_WORLD_BBOX.lim[1]);
    // Force remove it from coll grid (for the duration of this frame) since objects in NaNs or "in space" might cause UB or asserts in
    // code that isn't expecting it
    if (auto grid_obj = g_entity_mgr->getNullableRW<GridObjComponent>(actor.getEid(), ECS_HASH("grid_obj")))
      grid_obj->removeFromGrid();
    g_entity_mgr->setOptional(actor.getEid(), ECS_HASH("beh_tree__enabled"), false); // ditto
    g_entity_mgr->destroyEntity(actor.getEid());
  }
}

ECS_DECLARE_GET_FAST_BASE(int, net__upToCtrlTick, "net__upToCtrlTick");

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::updateNetPhys(IPhysActor::NetRole my_role, float at_time)
{
  TIME_PROFILE(PhysActor__updateNetPhys);
  if (!(my_role & (IPhysActor::URF_AUTHORITY | IPhysActor::URF_LOCAL_CONTROL)))
    return;
  if (my_role == ROLE_LOCALLY_CONTROLLED_AUTHORITY)
  {
    update_phys_for_multiplayer(this, phys, at_time, true, get_timespeed(), NULL, false, false, PHYS_DEBUG_DESYNCS_BITSTREAM);
    return;
  }
  const int maxTicksDelta = (int)ceilf(PHYS_MAX_CONTROLS_TICKS_DELTA_SEC / phys.timeStep);
  int curCeilTick = gamephys::ceilPhysicsTickNumber(at_time, phys.timeStep);
  int minTick = eastl::min(phys.currentState.lastAppliedControlsForTick, phys.currentState.atTick);
  if ((curCeilTick - minTick) > maxTicksDelta * 2) // initial update (or too lagged physics somehow)
  {
    phys.currentState.atTick = curCeilTick;
    phys.previousState.atTick = curCeilTick - 1;
    phys.currentState.lastAppliedControlsForTick = phys.previousState.atTick - calcControlsTickDelta();
    return;
  }
  G_ASSERT(phys.currentState.lastAppliedControlsForTick > 0);
  if (my_role & IPhysActor::URF_AUTHORITY) // server
  {
    G_ASSERT(!(my_role & IPhysActor::URF_LOCAL_CONTROL));
    int upToCtrlTick = curCeilTick - calcControlsTickDelta();
    int ctrlTick = min(phys.currentState.lastAppliedControlsForTick + 1, upToCtrlTick - 1);
    int physTick = curCeilTick - (upToCtrlTick - ctrlTick);
    for (int ctick = ctrlTick; ctick < upToCtrlTick; ++ctick) // forge missing controls
    {
      auto it = eastl::lower_bound(phys.unapprovedCT.begin(), phys.unapprovedCT.end(), ctick,
        [](const auto &ctrl, int tick) { return ctrl.producedAtTick < tick; }); // <= ctick
      if (it != phys.unapprovedCT.end() && it->producedAtTick == ctick)
        continue;
      const decltype(phys.appliedCT) *forgedCtrlBase = nullptr;
      int at = (it != phys.unapprovedCT.end()) ? eastl::distance(phys.unapprovedCT.begin(), it) : phys.unapprovedCT.size();
      for (int i = at - 1, j = 0; i >= 0 && j < 3; --i, ++j)
        if (!phys.unapprovedCT[i].isControlsForged())
        {
          forgedCtrlBase = &phys.unapprovedCT[i];
          break;
        }
      auto ctrl = forgedCtrlBase ? *forgedCtrlBase : phys.appliedCT;
      ctrl.producedAtTick = ctick;
      ctrl.setControlsForged(true);
      insert_item_at(phys.unapprovedCT, at, ctrl);
      if (forgedCtrlBase) // otherwise, we may forge controls only because there were no controls received from this actor YET
        ++phys.numForgedCT;
    }
    // limit how much ticks per frame we allow to update
    upToCtrlTick = min(upToCtrlTick, ctrlTick + (maxTicksDelta / 2) + (maxTicksDelta & 1));
    ResyncState resyncState = start_update_phys_for_multiplayer(this, phys, at_time, true, get_timespeed(), NULL);
    for (const int wasPhysTick = phys.currentState.atTick; ctrlTick < upToCtrlTick; ++physTick, ++ctrlTick)
      if (physTick >= wasPhysTick)
        tick_phys_for_multiplayer(this, phys, physTick, ctrlTick, resyncState, true, false, false, PHYS_DEBUG_DESYNCS_BITSTREAM);
      else // This happens when controlsTickDelta reduces from bigger values towards smaller ones
        phys.currentState.lastAppliedControlsForTick = ctrlTick;
    finish_update_phys_for_multiplayer(this, phys, resyncState, at_time);
  }
  else // client
  {
    G_ASSERT(my_role & IPhysActor::URF_LOCAL_CONTROL);

    int ctrlTick = phys.currentState.lastAppliedControlsForTick + 1;

    const int producedAtTick = phys.unapprovedCT.empty() ? -1 : phys.unapprovedCT.back().producedAtTick;
    const int lastAASTick = getLastAASTick();

    int upToCtrlTick = calc_phys_update_to_tick(at_time, phys.timeStep, ctrlTick, producedAtTick, lastAASTick);

    if (PHYS_ACTOR::staticPhysType == PhysType::PHYSOBJ)
    {
      // This is a trick on the client in case we have the train
      // Train must be updated according to the player on its client
      // Otherwise, we will get a desync: the train will be several ticks
      // in the past relative to the player.
      // Very noticeable when the player is close to the train edges (in a collision).
      // The server will solve the collision and the player at the correct position.
      // But the client will still get the collision over and over again.
      // The reason: the train is shifted in time relative to the player (shifted backward).
      // So, this is the reason why we need external control over the tick.
      const int *net__upToCtrlTick = ECS_GET_NULLABLE(int, getEid(), net__upToCtrlTick);
      if (DAGOR_UNLIKELY(net__upToCtrlTick))
        if (*net__upToCtrlTick > 0)
          upToCtrlTick = *net__upToCtrlTick;
    }

    ResyncState resyncState = start_update_phys_for_multiplayer(this, phys, at_time, true, get_timespeed(), NULL);
    for (int physTick = phys.currentState.atTick; ctrlTick < upToCtrlTick; ++physTick, ++ctrlTick)
    {
      tick_phys_for_multiplayer(this, phys, physTick, ctrlTick, resyncState, true, false, false, PHYS_DEBUG_DESYNCS_BITSTREAM);
      // preserve initially set rttTickDelta shift (in case of missing controls)
      phys.currentState.lastAppliedControlsForTick = ctrlTick;
    }
    // We intentionally don't use finish_update_phys_for_multiplayer() here since we using two different times here
    // (actorTime & at_time). Also we don't need most of it on client anyway
    gamephys::Loc visErr;
    phys.calculateCurrentVisualLocationError(at_time, visErr); // Note: same timeline that passed in apply_authority_state()
    int tdelta = phys.currentState.atTick - phys.currentState.lastAppliedControlsForTick - 1;
    float physActorTime = at_time + tdelta * phys.timeStep;
    const gamephys::Loc &visPosInterpolated =
      (phys.currentState.atTick > phys.previousState.atTick)
        ? phys.lerpVisualLoc(phys.previousState, phys.currentState, phys.timeStep, physActorTime)
        : phys.currentState.location; // local controls wasn't generated by some reason (e.g. dead)?
    phys.visualLocation.add(visPosInterpolated, visErr);
  }
}

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::update(float at_time, float remote_time, float dt, const BasePhysActor::UpdateContext &uctx)
{
  TIME_PROFILE(PhysActor__update);
  IPhysActor::NetRole thisRole = getRole();
  customPhys.onPreUpdate(dt, uctx);
  updateNetPhys(thisRole, at_time);
  if (thisRole == IPhysActor::ROLE_REMOTELY_CONTROLLED_SHADOW)
  {
    TIME_PROFILE(PhysActor__updateRemoteShadow);
    if (PHYS_ACTOR::staticPhysType == PhysType::HUMAN && uctx.humanAnimCharVisible)
      // manually update trace cache as no-one is going to otherwise (for legsIK, visual entity offset, etc...)
      phys.validateTraceCache();
    updateRemoteShadow(remote_time, dt, uctx);
  }
  customPhys.onPostUpdate(at_time, uctx);
  if (thisRole == IPhysActor::ROLE_REMOTELY_CONTROLLED_AUTHORITY)
  {
    // Note: assume that only physObj might have broadcastAAS
    if (PHYS_ACTOR::staticPhysType == PhysType::PHYSOBJ && DAGOR_UNLIKELY(uctx.physObjBroadcastAAS))
      send_authority_state(this, phys, PHYS_SEND_AUTH_STATE_PERIOD_SEC, &phys_send_broadcast_auth_state,
        &phys_send_broadcast_part_auth_state);
    else
      send_authority_state(this, phys, PHYS_SEND_AUTH_STATE_PERIOD_SEC, &phys_send_auth_state, &phys_send_part_auth_state);
  }
  *uctx.transform = phys.visualLocation.makeTM();
  if (thisRole & IPhysActor::URF_AUTHORITY)
    destroy_fallthrough_phys_actor(*this, *uctx.transform);
}

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::calcPosVelQuatAtTime(double at_time, DPoint3 &out_pos, Point3 &out_vel, Quat &out_quat) const
{
  DPoint3 veld;
  phys.calcPosVelAtTime(at_time, out_pos, veld);
  out_vel.set_xyz(veld);
  DPoint3 omega;
  phys.calcQuatOmegaAtTime(at_time, out_quat, omega);
}

TEMPLATE_PHYS_ACTOR
DPoint3 PHYS_ACTOR::calcPosAtTime(double at_time) const
{
  DPoint3 pos, vel;
  phys.calcPosVelAtTime(at_time, pos, vel);
  return pos;
}

TEMPLATE_PHYS_ACTOR
Quat PHYS_ACTOR::calcQuatAtTime(double at_time) const
{
  Quat quat;
  DPoint3 omega;
  phys.calcQuatOmegaAtTime(at_time, quat, omega);
  return quat;
}


TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::teleportToDefaultImpl(const TMatrix &in_tm)
{
  TMatrix prevTm = phys.visualLocation.makeTM();
  phys.setTmRough(in_tm);
  updateTransform(phys.visualLocation.makeTM());
  if (auto ac = g_entity_mgr->getNullableRW<AnimV20::AnimcharBaseComponent>(getEid(), ECS_HASH("animchar")))
    ac->resetFastPhysSystem();
  if (getRole() & IPhysActor::URF_AUTHORITY)
    cachedQuantInfo.invalidate();
  g_entity_mgr->sendEventImmediate(eid, EventOnEntityTeleported(in_tm, prevTm)); // immediate to update grid collision right away
}

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::reset()
{
  if (!(getRole() & IPhysActor::URF_AUTHORITY))
    snapshotsHistory.clear();
  phys.reset();
  phys.resetProducedCt();
  // reset possible control shift as entity might change it's role from remote to local control
  phys.currentState.lastAppliedControlsForTick = phys.currentState.atTick - 1;
  g_entity_mgr->sendEvent(eid, EventOnEntityReset());
}

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::onCollisionDamage(dag::Span<gamephys::SeqImpulseInfo> collisions,
  dag::Span<gamephys::CollisionContactData> contacts,
  int offender_id,
  double collision_impulse_damage_multiplier,
  float at_time)
{
  customPhys.onCollisionDamage(eid, *this, collisions, contacts, offender_id, collision_impulse_damage_multiplier, at_time);
}

TEMPLATE_PHYS_ACTOR
dag::ConstSpan<NetWeapon *> PHYS_ACTOR::getAllWeapons() const { return customPhys.getAllWeapons(); }

TEMPLATE_PHYS_ACTOR
NetAutopilot *PHYS_ACTOR::getAutopilot() const { return customPhys.getAutopilot(); }

TEMPLATE_PHYS_ACTOR
bool PHYS_ACTOR::serializePhysSnapshotBase(danet::BitStream &bs, PhysSnapSerializeType pst) const
{
  G_ASSERTF(phys.currentState.atTick >= 0, "Attempt to serialize not updated phys!");
  bs.AlignWriteToByteBoundary();

  if (!is_snapshot_quantized(this))
  {
    const gamephys::Loc &loc = phys.currentState.location;
    if (pst != PhysSnapSerializeType::HIDDEN)
      bs.Write(Point3(loc.P));
    else
      bs.Write(Point3(HIDDEN_QWPOS_XYZ));
    bs.Write(normalize(loc.O.getQuat()));
    bs.Write(Point3(phys.currentState.velocity));
    return /*inMotion*/ true;
  }

  bool inMotion;
  if (pst == PhysSnapSerializeType::CLOSEST)
    inMotion = cachedQuantInfo.serializeQLoc(bs, cachedQuantInfo.quantLoc, phys.currentState.location.P);
  else
  {
    const QuantizedWorldLocSmall &qloc = (pst == PhysSnapSerializeType::HIDDEN) ? HIDDEN_QWLOC : cachedQuantInfo.quantLocSmall;
    inMotion = cachedQuantInfo.serializeQLoc(bs, qloc, phys.currentState.location.P);
  }

  if (inMotion)
    cachedQuantInfo.serializeVelocity(bs);

  return inMotion;
}

TEMPLATE_PHYS_ACTOR
/*static*/ bool PHYS_ACTOR::deserializeBasePhysSnapshot(const danet::BitStream &bs,
  PhysSnapSerializeType pst,
  BasePhysSnapshot &snap,
  net::IConnection &,
  bool *out_in_motion,
  bool *out_quat_ext2_bit)
{
  bool isOk = true;
  bs.AlignReadToByteBoundary();

  bool inMotion = true;
  if (!is_snapshot_quantized((PHYS_ACTOR *)NULL))
  {
    isOk &= bs.Read(snap.pos);
    isOk &= bs.Read(snap.quat);
  }
  else if (pst == PhysSnapSerializeType::CLOSEST)
    isOk &= CachedQuantizedInfo::template deserializeQLoc<>(bs, snap.pos, snap.quat, inMotion, out_quat_ext2_bit);
  else
    isOk &= CachedQuantizedInfo::template deserializeQLoc<QuantizedWorldLocSmall>(bs, snap.pos, snap.quat, inMotion);

  if (!isOk)
    return false;

  if (is_snapshot_quantized((PHYS_ACTOR *)NULL))
    isOk &= CachedQuantizedInfo::deserializeVelocity(bs, inMotion, snap.vel);
  else
    isOk &= bs.Read(snap.vel);

  if (out_in_motion)
    *out_in_motion = inMotion;

  return true;
}

TEMPLATE_PHYS_ACTOR
/* static */ void PHYS_ACTOR::processSnapshotNoEntity(uint16_t /* netPhysId */, BasePhysSnapshot &&basesnap)
{
  eastl::destroy_at(&static_cast<SnapshotType &>(basesnap));
}

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::savePhysSnapshot(BasePhysSnapshot &&basesnap, float additional_delay)
{
  SnapshotType &snap = static_cast<SnapshotType &>(basesnap);
  snap.atTick = gamephys::ceilPhysicsTickNumber(snap.atTime, phys.timeStep);
  if (snapshotsHistory.empty() || snap.atTime >= snapshotsHistory.front().atTime)
  {
    int maxDynCapacity = (additional_delay < 1e-6f)
                           ? 0
                           : (eastl::min(gamephys::floorPhysicsTickNumber(additional_delay, phys.timeStep),
                                gamephys::floorPhysicsTickNumber(PHYS_MAX_INTERP_FROM_TIMESPEED_DELAY_SEC, phys.timeStep)) +
                               2);
    if (snapshotsHistory.size() >= eastl::max(PHYS_REMOTE_SNAPSHOTS_HISTORY_CAPACITY, maxDynCapacity)) // full?
    {
      if (!maxDynCapacity && snapshotsHistory.capacity() > PHYS_REMOTE_SNAPSHOTS_HISTORY_CAPACITY) // free extra heap memory
      {
        snapshotsHistory.erase(snapshotsHistory.begin(),
          snapshotsHistory.begin() + (snapshotsHistory.size() - PHYS_REMOTE_SNAPSHOTS_HISTORY_CAPACITY));
        snapshotsHistory.set_capacity(snapshotsHistory.size());
      }
      snapshotsHistory.erase(snapshotsHistory.begin()); // erase oldest to free place for new snapshot
    }
    if (!snapshotsHistory.empty())
    {
      auto ins = snapshotsHistory.insert(eastl::move(snap));
      if (!ins.second) // do accept dups since it's might happen due to reliable state change (vis<->invis)
      {
        *ins.first = eastl::move(snap);
        eastl::destroy_at(&snap); // destroy it manually since calling code isn't calling snapshot destructors
      }
    }
    else // first received -> create fake pair (to be able use it for interpolation)
    {
      snapshotsHistory.push_back_unsorted(typename PHYS_ACTOR::SnapshotType(snap)); // explicit copy
      typename PHYS_ACTOR::SnapshotType &last = snapshotsHistory.back();
      last.atTick--; // as if it was previous
      last.atTime -= phys.timeStep;

      snap.pos += snap.vel * phys.timeStep; // trying to predict the next snapshot using simple extrapolation
      snap.atTick++;                        // it will be overriden by the actual snapshot, if it arrives
      snap.atTime += phys.timeStep;

      snapshotsHistory.emplace_back_unsorted(eastl::move(snap));
    }
  }
  else
  {
    debug("discard phys snapshot for old tick %d, last tick is %d", snap.atTick, snapshotsHistory.rbegin()->atTick);
    eastl::destroy_at(&snap); // destroy it manually since calling code isn't calling snapshot destructors
  }
}

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::resetAAS()
{
  phys.physicsTimeToSendState = 0.f;
  phys.authorityApprovedPartialState.isProcessed = phys.isAuthorityApprovedStateProcessed = true;
  phys.lastProcessedAuthorityApprovedStateAtTick = -1;
  phys.authorityApprovedState.reset();
  clear_and_shrink(phys.historyStates);
  phys.forEachCustomStateSyncer([](auto ss) { return ss->clear(); });
  phys.authorityApprovedPartialState.atTick = -1;
  phys.authorityApprovedPartialState.lastAppliedControlsForTick = -1;
}

struct NoMinPhysTickrateValue
{
  static constexpr int MinPhysTickrateValue = -1;
};
template <typename T>
class MinPhysTickrateDetector
{
  template <typename U>
  static U test(decltype(&U::MinPhysTickrateValue));
  template <typename U>
  static NoMinPhysTickrateValue test(...);

public:
  typedef decltype(test<T>(0)) type;
};
template <typename T>
static inline float phys_calc_timestep(ecs::EntityId eid, PhysTickRateType trtype, const T &)
{
  int tickrate = g_entity_mgr->getOr(eid, ECS_HASH("phys__tickRate"), -1); // templates override has priority over default one
  if (tickrate < 0)
    tickrate = (trtype == PhysTickRateType::LowFreq) ? phys_get_bot_tickrate() : phys_get_tickrate();
  int minTr = eastl::max(PHYS_MIN_TICKRATE, MinPhysTickrateDetector<T>::type::MinPhysTickrateValue);
  return 1.f / eastl::clamp(tickrate, minTr, PHYS_MAX_TICKRATE);
}

TEMPLATE_PHYS_ACTOR
void PHYS_ACTOR::setRoleAndTickrateType(IPhysActor::NetRole new_role, PhysTickRateType trtype)
{
  G_FAST_ASSERT(new_role & URF_INITIALIZED);

  auto prevNetRole = this->netRole;
  setRole(new_role);
  if (!(new_role & URF_AUTHORITY) && (prevNetRole & URF_LOCAL_CONTROL) != (new_role & URF_LOCAL_CONTROL))
  {
    if (new_role & URF_LOCAL_CONTROL)
    {
      G_ASSERTF(find_value_idx(locallyContolledClientEntities, eid) < 0, "%d", (ecs::entity_id_t)eid);
      locallyContolledClientEntities.push_back(eid);
    }
    else
      G_VERIFYF(erase_item_by_value(locallyContolledClientEntities, eid), "%d", (ecs::entity_id_t)eid);
  }

  if (!g_entity_mgr->has(eid, ECS_HASH("phys__lowFreqTickrate")))
    trtype = PhysTickRateType::Normal;
  float ts = phys_calc_timestep(eid, trtype, customPhys);
  if (fabsf(ts - phys.timeStep) < 1e-6f)
    return;
  tickrateType = trtype;
  phys.setTimeStep(ts);
  // Recalc ticks to new timeStep
  float curTimeDelta = (new_role & IPhysActor::URF_LOCAL_CONTROL) ? 0.f : phys_get_present_time_delay_sec(trtype, ts);
  phys.currentState.atTick = gamephys::floorPhysicsTickNumber(get_sync_time() - curTimeDelta, ts);
  // On server we assume that this method is called before phys update and hence use previuos (floor) tick instead of current (ceil)
  // in order to able update it further in phys update of this frame
  if (!(new_role & IPhysActor::URF_AUTHORITY))
    phys.currentState.atTick++;
  phys.previousState.atTick = phys.currentState.atTick - 1;
  phys.currentState.lastAppliedControlsForTick = phys.previousState.atTick - calcControlsTickDelta();
  clear_and_shrink(phys.unapprovedCT);

  resetAAS(); // When tickrate changes, next  AAS may have tick smaller, than what we have processed, so we forget AASes.

  if (new_role & IPhysActor::URF_AUTHORITY) // server assigns tickrate/timeStep
  {
    if (bool *lowFreqTickrate = g_entity_mgr->getNullableRW<bool>(eid, ECS_HASH("phys__lowFreqTickrate")))
      *lowFreqTickrate = trtype == PhysTickRateType::LowFreq;
  }
  else
  {
    for (auto &snap : snapshotsHistory)
      snap.atTick = gamephys::ceilPhysicsTickNumber(snap.atTime, ts);
    G_ASSERT(eastl::is_sorted(snapshotsHistory.begin(), snapshotsHistory.end()));
    auto snapEq = [](const BasePhysSnapshot &a, const BasePhysSnapshot &b) { return a.atTick == b.atTick; };
    // remove dups
    snapshotsHistory.erase(eastl::unique(snapshotsHistory.begin(), snapshotsHistory.end(), snapEq), snapshotsHistory.end());
  }
}

TEMPLATE_PHYS_ACTOR
const typename PHYS_ACTOR::SnapshotPair *PHYS_ACTOR::findPhysSnapshotPair(float at_time) const
{
  if (snapshotsHistory.size() <= 1)
    return nullptr;
  // Search by tick (instead of time) since we using ticks for interpolation's alpha calculation
  int tick = gamephys::floorPhysicsTickNumber(at_time, phys.timeStep);
  for (auto rit = snapshotsHistory.rbegin(), rend = snapshotsHistory.rend(); rit != rend; ++rit) // search for lower bound
    if (rit->atTick <= tick)
    {
      const SnapshotType *itlb = &*rit;
      return reinterpret_cast<const SnapshotPair *>((itlb == &snapshotsHistory.back()) ? (itlb - 1) : itlb);
    }
  return reinterpret_cast<const SnapshotPair *>(&snapshotsHistory.front());
}

TEMPLATE_PHYS_ACTOR
const typename PHYS_ACTOR::SnapshotPair *PHYS_ACTOR::updateRemoteShadowBase(
  float remote_time, float dt, const BasePhysActor::UpdateContext &)
{
  const SnapshotPair *spair = findPhysSnapshotPair(remote_time);
  if (!spair)
    return nullptr;

  if (spair->lastSnap.atTick != phys.currentState.atTick)
    phys.previousState = phys.currentState;

  float interpTime = 1.0f, elapsedTime = 0.0f;
  eastl::tie(interpTime, elapsedTime) =
    phys_get_xpolation_times(spair->prevSnap.atTick, spair->lastSnap.atTick, phys.timeStep, remote_time);
  float invIterpTime = safeinv(interpTime);
  // do quick check before heavy velocity check
  if (DAGOR_UNLIKELY(lengthSq(spair->lastSnap.pos - spair->prevSnap.pos) > PHYS_SNAP_TELEPORT_THRESHOLD_SQ) &&
      position_diff_wrt_velocity_sq(spair->prevSnap, spair->lastSnap, interpTime) > PHYS_SNAP_TELEPORT_THRESHOLD_SQ) // -V1051
  {
    // too far for smooth *polation, snap in place
    phys.visualLocation.P = spair->lastSnap.pos;
    phys.visualLocation.O.setQuat(spair->lastSnap.quat);
    phys.currentState.velocity = spair->lastSnap.vel;
    // Don't try to animate non-phys movement because of huge instant velocities (esp important enter/exit to hidden location)
    if (auto ac = g_entity_mgr->getNullableRW<AnimV20::AnimcharBaseComponent>(getEid(), ECS_HASH("animchar")))
      if (lengthSq(spair->lastSnap.pos - ac->getNodeTree().getRootWposScalar()) > PHYS_SNAP_TELEPORT_THRESHOLD_SQ)
        ac->resetFastPhysSystem();
    invIterpTime = 0.f; // no accel
  }
  else if (elapsedTime > interpTime) // extrapolation
  {
    // Projective Velocity Blending / Believable Dead Reckoning for Networked Games ( https://sci-hub.tw/10.1201/b11333-21 )
    float idt = elapsedTime - interpTime;
    float alpha1 = eastl::min(idt / phys.timeStep, 1.f); // assume that one tick (i.e. timeStep) is enough to converge
    Point3 velb = lerp(Point3::xyz(phys.currentState.velocity), spair->lastSnap.vel, alpha1); // velocity blending
    Point3 pos1 = Point3::xyz(phys.visualLocation.P) + velb * dt;                             // projecting from where we were
    Point3 pos2 = spair->lastSnap.pos + spair->lastSnap.vel * idt;                            // projecting from last known
    phys.visualLocation.P = lerp(pos1, pos2, alpha1);
    phys.currentState.velocity = velb;

    float turnPart = approach(0.f, 1.f, dt, cl_exp_aprch_k.get());
    Quat wishQuat = normalize(qinterp(spair->prevSnap.quat, spair->lastSnap.quat, elapsedTime / interpTime));
    Quat interpQuat = normalize(qinterp(phys.visualLocation.O.getQuat(), wishQuat, turnPart));
    phys.visualLocation.O.setQuat(interpQuat);
  }
  else // interpolation
  {
    float alpha = elapsedTime / interpTime;
    phys.visualLocation.P = lerp(spair->prevSnap.pos, spair->lastSnap.pos, alpha);
    phys.visualLocation.O.setQuat(normalize(qinterp(spair->prevSnap.quat, spair->lastSnap.quat, alpha)));
    phys.currentState.velocity = spair->lastSnap.vel;
  }

  phys.currentState.atTick = spair->lastSnap.atTick;
  phys.currentState.lastAppliedControlsForTick = spair->lastSnap.atTick - 1;
  phys.currentState.location.P = spair->lastSnap.pos;
  phys.currentState.location.O.setQuat(spair->lastSnap.quat);
  phys.currentState.acceleration = (spair->lastSnap.vel - spair->prevSnap.vel) * invIterpTime;

  return spair;
}

EA_DISABLE_VC_WARNING(4582 4583) // constructor/destructor is not implicitly called

TEMPLATE_PHYS_ACTOR
PHYS_ACTOR::PhysActor(const ecs::EntityManager & /*mgr*/, ecs::EntityId in_eid) :
  BasePhysActor(in_eid, staticPhysType),
  phys(offsetof(PhysActor, phys),
    g_entity_mgr->getNullableRW<PhysVars>(in_eid, ECS_HASH("phys_vars")),
    phys_calc_timestep(in_eid, BasePhysActor::tickrateType, customPhys))
{
  // Do not delay physics updates in interpolation mode. This is also important for control-less physics like phys objs.
  if (PHYS_ENABLE_INTERPOLATION)
    phys.setMaxTimeDeferredControls(0.f);

  static bool physTypeInited = false;
  if (!physTypeInited)
  {
    CustomPhysType::initOnce();
    physTypeInited = true;
  }

  if ((int)physType >= allPhysActorsLists.size() || !allPhysActorsLists[(int)physType])
  {
    allPhysActorsLists.resize(eastl::max(allPhysActorsLists.size(), (unsigned)physType + 1));
    allPhysActorsLists[(int)physType] = &physEidsList;
  }

  clientSideOnly = g_entity_mgr->has(in_eid, ECS_HASH("clientSide"));

  initRole();

  if (!clientSideOnly)
  {
    if (getRole() & IPhysActor::URF_AUTHORITY)
      new (&cachedQuantInfo, _NEW_INPLACE) CachedQuantizedInfo();
    else
      new (&snapshotsHistory, _NEW_INPLACE) SnapshotsHistoryType();
  }
}

TEMPLATE_PHYS_ACTOR
PHYS_ACTOR::~PhysActor()
{
  if (netPhysId >= 0)
  {
    if (is_server())
    {
      float availableAt = has_network() ? (get_sync_time() + SAFE_TIME_TO_REUSE_PHYS_ACTOR_INDEX) : 0.f;
      freeActorIndexes[int(netPhysId >= 128)].push_back(FreePhysActorIndex{availableAt, (unsigned)netPhysId});
    }
    if (physEidsList[netPhysId] == eid)
      physEidsList[netPhysId] = ecs::INVALID_ENTITY_ID;
    else
      ; // slot was reused for other entity
  }

  if (!clientSideOnly)
  {
    if (getRole() & IPhysActor::URF_AUTHORITY)
      eastl::destroy_at(&cachedQuantInfo);
    else
      decltype(snapshotsHistory)().swap(snapshotsHistory);
  }

  if (getRole() == ROLE_LOCALLY_CONTROLLED_SHADOW)
    erase_item_by_value(locallyContolledClientEntities, eid); // Might fail if role was already reset
}

EA_RESTORE_VC_WARNING()

#define IMPLEMENT_PHYS_ACTOR_NET_RCV(MSGCLS, EID2ACTOR)                                                               \
  TEMPLATE_PHYS_ACTOR const net::MessageClass *PHYS_ACTOR::getMsgCls()                                                \
  {                                                                                                                   \
    return &MSGCLS::messageClass;                                                                                     \
  }                                                                                                                   \
  TEMPLATE_PHYS_ACTOR void PHYS_ACTOR::net_rcv_snapshots(const net::IMessage *msg)                                    \
  {                                                                                                                   \
    net_rcv_phys_snapshots(*msg, /*to_msg_sink*/ true, msg->cast<MSGCLS>()->get<0>(), *msg->connection, physEidsList, \
      (phys_actor_resolve_by_eid_t)&EID2ACTOR, deserializePhysSnapshot, processSnapshotNoEntity);                     \
  }

static inline TMatrix &validate_phys_actor_tm(TMatrix &tm, ecs::EntityId eid)
{
  if (float det = tm.det(); DAGOR_UNLIKELY(det < 1e-12f))
  {
    logerr("Creating phys actor %d<%s> with non-orthonormalized (det=%f) matrix: %@", (ecs::entity_id_t)eid,
      g_entity_mgr->getEntityTemplateName(eid), det, tm);
    tm.orthonormalize();
  }
  if (vec3f vpos = v_ldu_p3(&tm.getcol(3).x);
      DAGOR_UNLIKELY(v_extract_y(vpos) < DESTROY_ENTITY_FLOOR_Y || v_extract_x(v_length3_sq_x(vpos)) >= 9e8f))
    if (!(PHYS_ENTITY_WORLD_BBOX & tm.getcol(3)))
      logerr("Attempt to create phys actor %d<%s> with pos %@ out of `world bbox`: (%@, %@)", (ecs::entity_id_t)eid,
        g_entity_mgr->getEntityTemplateName(eid), tm.getcol(3), PHYS_ENTITY_WORLD_BBOX.lim[0], PHYS_ENTITY_WORLD_BBOX.lim[1]);
  return tm;
}

TEMPLATE_PHYS_ACTOR
bool PHYS_ACTOR::onLoaded(ecs::EntityManager &mgr, ecs::EntityId)
{
  const CollisionResource *collres = mgr.getNullable<CollisionResource>(eid, ECS_HASH("collres"));

  char attrName[64];
  SNPRINTF(attrName, sizeof(attrName), "%s_net_phys__blk", physreg::typeStrs[(int)physType]);

  const char *blkNameAttr = mgr.getOr(eid, ECS_HASH_SLOW(attrName), ecs::nullstr);
  G_ASSERT_RETURN(blkNameAttr, false);

  char blkName[DAGOR_MAX_PATH] = {0};
  strncpy(blkName, blkNameAttr, sizeof(blkName) - 1);
  const char *blockName = NULL;
  char *pColon = strchr(blkName, ':');
  if (pColon)
  {
    *pColon = '\0'; // cut block name off
    blockName = pColon + 1;
  }

  const TMatrix &tm = validate_phys_actor_tm(mgr.getRW<TMatrix>(eid, ECS_HASH("transform")), eid);
  // Before phys load (as it might depend on correct world location being set, e.g. for adding phys bodies to phys world)
  phys.currentState.location.fromTM(tm);

  customPhys.onPreInit(eid, *this);

  {
    DataBlock blk;
    {
      TIME_PROFILE(phys_blk_load);
      blk.load(blkName);
    }
    loadPhysFromBlk(blockName ? blk.getBlockByName(blockName) : &blk, collres, blkName);
    phys.addCollisionToWorld();
  }

  // setTmRough() implicitly depends on loadPhysFromBlk b/c of validateTraceCache() impl (hence it should be called after it)
  phys.setTmRough(tm);

  customPhys.onPostInit(eid, *this);

  // Level is expected to be loaded at this point. Make sure that level entity is placed before any phys actors in scene.
  // On client order of creation is same as on server
  G_ASSERT(is_level_loaded());
  float curTimeDelayed = get_sync_time() - phys_get_present_time_delay_sec(tickrateType, phys.timeStep, clientSideOnly);
  phys.previousState.atTick = gamephys::floorPhysicsTickNumber(curTimeDelayed, phys.timeStep);
  phys.currentState.atTick = phys.previousState.atTick + 1;
  phys.currentState.lastAppliedControlsForTick = phys.previousState.atTick - calcControlsTickDelta();

  initNetPhysId(physEidsList, physIdGenerations, freeActorIndexes, syncStatesPacked,
    g_entity_mgr->getNullableRW<int>(eid, ECS_HASH("net__physId")));

  if (eid == game::get_controlled_hero())
  {
    debug("hero phys actor %d/%p created", (ecs::entity_id_t)eid, this);
    g_entity_mgr->broadcastEvent(EventHeroChanged(eid));
  }

  return true;
}

template <typename T>
static inline void net_phys_apply_authority_state_es(const UpdatePhysEvent &info, T &actor)
{
  if (actor.getRole() == IPhysActor::ROLE_LOCALLY_CONTROLLED_SHADOW)
    apply_authority_state_and_log_desync(&actor, actor.phys, info.curTime, false, true, false, PHYS_DEBUG_DESYNCS_BITSTREAM,
      /*update_by_controls*/ true);
}

template <typename T>
static inline void net_phys_update_es(const UpdatePhysEvent &info, T &actor, const BasePhysActor::UpdateContext &uctx)
{
  float remoteTime = (actor.getRole() == IPhysActor::ROLE_REMOTELY_CONTROLLED_SHADOW)
                       ? eastl::max(info.curTime - PhysUpdateCtx::ctx.getInterpDelay(actor.tickrateType, actor.phys.timeStep), 0.f)
                       : info.curTime;
  actor.update(info.curTime, remoteTime, info.dt, uctx);
}


static inline daphys::SolverBodyInfo phys_to_solver_body(const IPhysBase *phys, ecs::EntityId eid)
{
  daphys::SolverBodyInfo info = gamephys::phys_to_solver_body(phys);
  info.tm = g_entity_mgr->getOr(eid, ECS_HASH("transform"), TMatrix::IDENT);
  info.itm = inverse(info.tm);
  return info;
}

// TODO: redo the omega in aircraft phys, so it'll not be needed.
static inline void patch_solver_body_for_phys(ecs::EntityId eid, daphys::SolverBodyInfo &body)
{
  if (g_entity_mgr->has(eid, ECS_HASH("phys__inverseOmega")))
    body.omega = -body.omega;
}

static inline void patch_solver_body_res(ecs::EntityId eid, daphys::SolverBodyInfo &body)
{
  if (g_entity_mgr->has(eid, ECS_HASH("phys__inverseOmega")))
    body.addOmega = -body.addOmega;
}

static inline void move_collision_objects(IPhysBase &phys, const TMatrix &tm)
{
  for (const auto &co : phys.getCollisionObjects())
    if (dacoll::is_obj_active(co))
      dacoll::set_collision_object_tm(co, tm);
}

template <typename T>
static inline void net_phys_collision_es_impl(const UpdatePhysEvent &info,
  T &actor,
  float bounding_rad,
  const GridHolders &grids_to_search,
  const ecs::HashedConstString &tag_to_check)
{
  if (!(actor.getRole() & (IPhysActor::URF_AUTHORITY | IPhysActor::URF_LOCAL_CONTROL)))
    return;

  dag::ConstSpan<CollisionObject> curColl = actor.getPhys().getCollisionObjects();
  uint64_t curCollActive = actor.getPhys().getActiveCollisionObjectsBitMask();
  const TMatrix &curPhysTm = actor.getPhys().getCollisionObjectsMatrix();
  const TraceMeshFaces *curTraceCache = actor.getPhys().getTraceHandle();
  bool worldContactsProcessed = false;
  PairCollisionData curBody;
  query_pair_collision_data(actor.eid, curBody);
  const bool curIsAsleep = actor.isAsleep() && !curBody.hasCustomMoveLogic;
  mat44f vCurPhysTm;
  v_mat44_make_from_43cu_unsafe(vCurPhysTm, curPhysTm.array);
  BSphere3 bounding;
  const float boundingExt = 1.2f; // add some padding
  v_stu(&bounding.c.x,
    v_perm_xyzd(vCurPhysTm.col3, v_splat_x(v_mul_x(v_set_x(bounding_rad * boundingExt), v_mat44_max_scale43_x(vCurPhysTm)))));
  bounding.r2 = sqr(bounding.r);
  alignas(NavMeshPhysProxy) char nmeshProxyStorage[sizeof(NavMeshPhysProxy)];

  for_each_entity_in_grids(grids_to_search, bounding, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec4f wbsph) {
    if (eid == actor.eid)
      return;
    if (!g_entity_mgr->has(eid, tag_to_check)) // they don't have pair collision
      return;

    PairCollisionData testBody;
    query_pair_collision_data(eid, testBody);
    BasePhysActor *testActor = get_phys_actor(eid);

    // don't check same collision pair twice
    if (ecs::entity_id_t(actor.eid) > ecs::entity_id_t(eid) && testBody.havePairCollision && testActor &&
        testActor->getRole() & (IPhysActor::URF_AUTHORITY | IPhysActor::URF_LOCAL_CONTROL))
    {
      if (testBody.pairCollisionTag && g_entity_mgr->has(actor.eid, ECS_HASH_SLOW(testBody.pairCollisionTag->c_str())))
        return;
    }

    QueryPhysActorsNotCollidable qcoll(actor.eid);
    g_entity_mgr->sendEventImmediate(eid, qcoll);
    if (!qcoll.shouldCollide)
      return;
    qcoll.otherEid = eid;
    g_entity_mgr->sendEventImmediate(actor.eid, qcoll);
    if (!qcoll.shouldCollide)
      return;

    IPhysBase *testPhys = nullptr;
    if (testActor)
    {
      if (curIsAsleep && testActor->isAsleep() && !testBody.hasCustomMoveLogic)
        return;
      testPhys = &testActor->getPhys();
    }
    else if (testBody.nphysPairCollision)
      testPhys = new (&nmeshProxyStorage, _NEW_INPLACE) NavMeshPhysProxy(eid, info.dt);

    if (testPhys == nullptr)
      return;

    dag::ConstSpan<CollisionObject> testColl = testPhys->getCollisionObjects();
    uint64_t testCollActive = testPhys->getActiveCollisionObjectsBitMask();
    const TMatrix &testTm = testPhys->getCollisionObjectsMatrix();
    Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
    if (!dacoll::test_pair_collision(curColl, curCollActive, testColl, testCollActive, contacts,
          dacoll::TestPairFlags::Default & ~dacoll::TestPairFlags::CheckInWorld))
      return;

    int contactsNum = contacts.size();

    if (!worldContactsProcessed)
    {
      dacoll::test_collision_world(curColl, bounding.r, contacts, curTraceCache);
      worldContactsProcessed = true;
    }
    const int contactsNum1 = contacts.size() - contactsNum;

    Point3_vec4 testPos;
    v_st(&testPos.x, wbsph);
    dacoll::test_collision_world(testColl, length(testPos - testTm.getcol(3)) + v_extract_w(wbsph), contacts,
      testPhys->getTraceHandle());
    const int contactsNum2 = contacts.size() - contactsNum1 - contactsNum;

    daphys::SolverBodyInfo curBodyInfo = phys_to_solver_body(&actor.getPhys(), actor.eid);
    daphys::SolverBodyInfo testBodyInfo = phys_to_solver_body(testPhys, eid);
    patch_solver_body_for_phys(actor.eid, curBodyInfo);
    patch_solver_body_for_phys(eid, testBodyInfo);

    if (!curBody.isKinematicBody && !testBody.isKinematicBody)
    {
      const float maxMassRatio = max(curBody.maxMassRatioForPushOnCollision, testBody.maxMassRatioForPushOnCollision);
      if (maxMassRatio > 0.f)
      {
        // If one of the bodies is way heavier than the other don't push it
        // The code fixes cases like a human pushes a tank
        const float massRatio = safediv(actor.getPhys().getMass(), testPhys->getMass());
        if (massRatio >= maxMassRatio)
        {
          curBodyInfo.invMass = 0.f;
          curBodyInfo.invMoi.zero();
        }
        if (safeinv(massRatio) >= maxMassRatio)
        {
          testBodyInfo.invMass = 0.f;
          testBodyInfo.invMoi.zero();
        }
      }
    }

    if (curBody.isKinematicBody)
    {
      curBodyInfo.invMass = 0.f;
      curBodyInfo.invMoi.zero();
    }

    if (testBody.isKinematicBody)
    {
      testBodyInfo.invMass = 0.f;
      testBodyInfo.invMoi.zero();
    }

    // Onesided collision ignoring:
    // Bot cannot affect players but playres can affect bots
    // So, this code should skip contacts with bots when it's called for player
    // but solve contacts when it's called for bot
    QueryPhysActorsOneSideCollidable curBodyOneSide(eid);
    QueryPhysActorsOneSideCollidable testBodyOneSide(actor.eid);
    g_entity_mgr->sendEventImmediate(eid, testBodyOneSide);
    g_entity_mgr->sendEventImmediate(actor.eid, curBodyOneSide);

    if (testBodyOneSide.oneSideCollidable)
    {
      testBodyInfo.invMass = 0.f;
      testBodyInfo.invMoi.zero();
    }
    if (curBodyOneSide.oneSideCollidable)
    {
      curBodyInfo.invMass = 0.f;
      curBodyInfo.invMoi.zero();
    }

    if (curBody.isAirplane)
    {
      curBodyInfo.commonImpulseLimit = DummyCustomPhys::aircraftCollisionImpulseSpeedMax * actor.getPhys().getMass();
      move_collision_objects(actor.getPhys(), curPhysTm);
    }
    if (testBody.isAirplane)
    {
      testBodyInfo.commonImpulseLimit = DummyCustomPhys::aircraftCollisionImpulseSpeedMax * testPhys->getMass();
      move_collision_objects(*testPhys, testTm);
    }

    carray<double, BasePhysActor::MAX_COLLISION_SUBOBJECTS> impulseLimits1;
    eastl::fill(impulseLimits1.data(), impulseLimits1.data() + impulseLimits1.size(), VERY_BIG_NUMBER);
    curBodyInfo.impulseLimits.set(make_span(impulseLimits1));
    carray<double, BasePhysActor::MAX_COLLISION_SUBOBJECTS> impulseLimits2;
    eastl::fill(impulseLimits2.data(), impulseLimits2.data() + impulseLimits2.size(), VERY_BIG_NUMBER);
    testBodyInfo.impulseLimits.set(make_span(impulseLimits2));

    Tab<gamephys::SeqImpulseInfo> collisions(framemem_ptr());
    collisions.reserve(contacts.size() * 3 + contactsNum1 + contactsNum2);
    contacts_to_solver_data(&curBodyInfo, &testBodyInfo, make_span(contacts).first(contactsNum), collisions);
    const int collisionsNum = collisions.size();

    contacts_to_solver_data(&curBodyInfo, nullptr, make_span(contacts).subspan(contactsNum, contactsNum1), collisions);
    for (int i = collisionsNum; i < collisions.size(); ++i)
      collisions[i].contactIndex += contactsNum;
    const int collisionsNum1 = collisions.size() - collisionsNum;

    contacts_to_solver_data(nullptr, &testBodyInfo, make_span(contacts).subspan(contactsNum + contactsNum1, contactsNum2), collisions);
    for (int i = collisionsNum + collisionsNum1; i < collisions.size(); ++i)
      collisions[i].contactIndex += contactsNum + contactsNum1;

    actor.getPhys().prepareCollisions(curBodyInfo, testBodyInfo, true, 0.7f, make_span(contacts), make_span(collisions));
    testPhys->prepareCollisions(curBodyInfo, testBodyInfo, false, 0.7f, make_span(contacts), make_span(collisions));

    Point3 lastCollisionPos = curPhysTm.getcol(3);
    float maxImpulse = 0.f;
    if (daphys::resolve_pair_velocity(curBodyInfo, testBodyInfo, make_span(collisions)))
    {
      actor.getPhys().wakeUp();
      actor.getPhys().rescheduleAuthorityApprovedSend();
      testPhys->wakeUp();
      testPhys->rescheduleAuthorityApprovedSend();

      for (const gamephys::SeqImpulseInfo &collision : collisions)
      {
        if (collision.appliedImpulse <= maxImpulse)
          continue;
        maxImpulse = collision.appliedImpulse;
        lastCollisionPos = Point3::xyz(collision.pos);
      }

      Point3 bodyTestVel = Point3::xyz(testBodyInfo.vel);
      Point3 bodyCurVel = Point3::xyz(curBodyInfo.vel);
      g_entity_mgr->sendEvent(actor.eid,
        EventOnCollision(Point3::xyz(curBodyInfo.addVel), bodyCurVel, lastCollisionPos, eid, bodyTestVel, info.dt, -1.f));
      g_entity_mgr->sendEvent(eid,
        EventOnCollision(Point3::xyz(testBodyInfo.addVel), bodyTestVel, lastCollisionPos, actor.eid, bodyCurVel, info.dt, -1.f));
    }

    if (curBody.isHuman || testBody.isHuman)
    {
      // In case of human collision checks we must skip entities that have humanAdditionalCollisionChecks
      // Human already done all collision checks inside his physics
      if (curBody.humanAdditionalCollisionChecks || testBody.humanAdditionalCollisionChecks)
        return;
    }

    patch_solver_body_res(actor.eid, curBodyInfo);
    patch_solver_body_res(eid, testBodyInfo);
    actor.getPhys().applyVelOmegaDelta(curBodyInfo.addVel, curBodyInfo.addOmega);
    testPhys->applyVelOmegaDelta(testBodyInfo.addVel, testBodyInfo.addOmega);

    daphys::resolve_pair_penetration(curBodyInfo, testBodyInfo, collisions, 2.0, 5);
    actor.getPhys().applyPseudoVelOmegaDelta(curBodyInfo.pseudoVel * actor.getPhys().getTimeStep(),
      curBodyInfo.pseudoOmega * actor.getPhys().getTimeStep());
    testPhys->applyPseudoVelOmegaDelta(testBodyInfo.pseudoVel * testPhys->getTimeStep(),
      testBodyInfo.pseudoOmega * testPhys->getTimeStep());

    actor.getPhys().wakeUp();
    testPhys->wakeUp();
    if (NavMeshPhysProxy::vtblPtr && ((void **)testPhys)[0] == NavMeshPhysProxy::vtblPtr) // hacky RTTI check
      static_cast<const NavMeshPhysProxy *>(testPhys)->applyChangesTo(eid);

    if (is_server())
    {
      actor.onCollisionDamage(make_span(collisions), make_span(contacts), eid.index(), 1.0, info.curTime);
      if (testActor != nullptr)
        testActor->onCollisionDamage(make_span(collisions), make_span(contacts), actor.eid.index(), 1.0, info.curTime);
    }
  });

  if (!curIsAsleep)
    for (const CollisionObject &collObj : curColl)
      if (collObj.isValid())
        rendinstdestr::test_dynobj_to_ri_phys_collision(collObj, bounding.r);
}

template <typename T>
inline void net_phys_event_handler(const ecs::Event &event, T &actor)
{
  if (auto evt = event.cast<ecs::EventNetMessage>())
  {
    const net::IMessage *msg = evt->get<0>().get();
    if (auto aasMsg = msg->cast<AAStateMsg>()) // executed on client
    {
      if (!actor.phys.receiveAuthorityApprovedState(aasMsg->get<0>(), 0, aasMsg->get<1>()))
        logerr("Cannot read authority approved for entity %d", (ecs::entity_id_t)actor.eid);
    }
    else if (auto aasPMsg = msg->cast<AAPartialStateMsg>()) // executed on client
    {
      if (!actor.phys.receivePartialAuthorityApprovedState(aasPMsg->get<0>()))
        logerr("Cannot read authority approved partial for entity %d", (ecs::entity_id_t)actor.eid);
    }
    else if (auto ctlMsg = msg->cast<ControlsMsg>()) // executed on server
    {
      const danet::BitStream &cbs = ctlMsg->get<0>();
      if (DAGOR_UNLIKELY(!ControlsSerializer<decltype(actor.phys)>::deserialize(cbs, actor.phys)))
      {
        static eastl::bitset<NET_MAX_PLAYERS> ctrlLogBitmap;
        int connId = msg->connection->getId();
        if (!ctrlLogBitmap.test(connId))
        {
          ctrlLogBitmap.set(connId, true);
          logerr("Failed to read controls packet of %d bytes from conn #%d to entity %d<%s>", cbs.GetNumberOfBytesUsed(), connId,
            (ecs::entity_id_t)actor.getEid(), g_entity_mgr->getEntityTemplateName(actor.getEid()));
          using fmemstring = eastl::basic_string<char, framemem_allocator>;
          debug("%s", base64::encode<fmemstring>(cbs.GetData(), eastl::min(cbs.GetNumberOfBytesUsed(), 256u)).c_str());
        }
        return;
      }
    }
    else if (auto tpMsg = msg->cast<TeleportMsg>())
      actor.teleportTo(tpMsg->get<0>(), tpMsg->get<1>());
    else if (msg->cast<PhysReset>())
      actor.reset();
    else if (auto desyncMsg = msg->cast<DesyncData>())
    {
#if !DISABLE_SYNC_DEBUG && DAGOR_DBGLEVEL > 0
      const danet::BitStream *previousData = actor.phys.fmsyncPreviousData;
      if (!sv_debug_phys_desyncs.get() || !previousData || previousData->GetNumberOfBitsUsed() == 0)
        return;

      previousData->ResetReadPointer();

      const danet::BitStream &bs = desyncMsg->get<0>();
      if (acesfm::sync_dumps_equal(*previousData, bs))
        return;

      const char *entTempl = g_entity_mgr->getEntityTemplateName(actor.eid);
      String outString(tmpmem);
      outString.aprintf(16384, "==> Dumping desync data for entity <%d>(%s)\n\n==> Server data:\n", (ecs::entity_id_t)actor.eid,
        entTempl);
      previousData->ResetReadPointer();
      acesfm::sync_dump_debug(*previousData, outString);
      debug("%s", outString.str());

      outString.clear();

      outString.aprintf(16384, "\n==> Client data:\n");
      bs.ResetReadPointer();
      acesfm::sync_dump_debug(bs, outString);
      outString.aprintf(64, "\n==> End of desync data for entity <%d>(%s)\n", (ecs::entity_id_t)actor.eid, entTempl);
      debug("%s", outString.str());
#else
      G_UNUSED(desyncMsg);
#endif
    }
  }
  else if (auto evt = event.cast<CmdTeleportEntity>())
  {
    actor.teleportTo(evt->get<0>(), evt->get<1>());
    if (is_server())
      send_net_msg(actor.getEid(), TeleportMsg(evt->get<0>(), evt->get<1>()));
  }
  else
    G_ASSERT(0);
}

template <typename Phys>
static void apply_phys_modifications_impl(const ecs::EntityId eid, const ecs::string &phys_modifications_blk, Phys &phys)
{
  DataBlock blk;
  dblk::load_text(blk, make_span_const(phys_modifications_blk.c_str(), phys_modifications_blk.length()), dblk::ReadFlag::ROBUST);
  if (blk.isValid())
    phys.applyModifications(blk);
  else
    logerr("Invalid modificatins blk for %@ (%@):\n'%s'", (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid),
      phys_modifications_blk.c_str());
}

#define NET_PHYS_ES_EVENT_SET ecs::EventNetMessage, CmdTeleportEntity

/*
 * Warning: changes to these macroses DO NOT trigger ecs codegen!
 * You have to touch manually all modules that include this (netPhys.cpp.inl) file.
 */
#define PHYS_IMPLEMENT_COLL_ES(phys_type, phys_comp_name, coll_tag_str)                                                             \
  ECS_AFTER(after_net_phys_sync)                                                                                                    \
  ECS_BEFORE(after_collision_sync)                                                                                                  \
  static inline void phys_comp_name##_collision_es(const UpdatePhysEvent &info, phys_type &phys_comp_name,                          \
    ECS_REQUIRE(eastl::true_type havePairCollision = true) const GridObjComponent *grid_obj, const CollisionResource &collres,      \
    const GridHolders &pair_collision__gridHolders, const ecs::string *pair_collision__tag)                                         \
  {                                                                                                                                 \
    if (grid_obj && !grid_obj->inserted)                                                                                            \
      return;                                                                                                                       \
    ecs::HashedConstString tagToCheck = pair_collision__tag ? ECS_HASH_SLOW(pair_collision__tag->c_str()) : ECS_HASH(coll_tag_str); \
    net_phys_collision_es_impl(info, phys_comp_name, collres.getBoundingSphereRad(), pair_collision__gridHolders, tagToCheck);      \
  }

#define PHYS_IMPLEMENT_ON_TICKRATE_CHANGED_ES(phys_type, phys_comp_name)                                                  \
  ECS_TRACK(phys__lowFreqTickrate)                                                                                        \
  static inline void phys_comp_name##_on_tickrate_changed_es_event_handler(const ecs::Event &, phys_type &phys_comp_name, \
    bool phys__lowFreqTickrate)                                                                                           \
  {                                                                                                                       \
    auto trt = phys__lowFreqTickrate ? PhysTickRateType::LowFreq : PhysTickRateType::Normal;                              \
    phys_comp_name.setRoleAndTickrateType(phys_comp_name.getRole(), trt);                                                 \
  }

#define PHYS_IMPLEMENT_APPLY_AUTHORITY_STATE(phys_type, phys_comp_name, phys_update_es)                                \
  ECS_BEFORE(after_net_phys_sync, phys_update_es)                                                                      \
  ECS_AFTER(before_net_phys_sync)                                                                                      \
  ECS_REQUIRE_NOT(ecs::Tag disableUpdate)                                                                              \
  static inline void phys_comp_name##_apply_authority_state_es(const UpdatePhysEvent &info, phys_type &phys_comp_name) \
  {                                                                                                                    \
    net_phys_apply_authority_state_es(info, phys_comp_name);                                                           \
  }

#define PHYS_IMPLEMENT_COMMON_ESES(phys_type, phys_comp_name, phys_update_es, coll_tag_str)                               \
  PHYS_IMPLEMENT_COLL_ES(phys_type, phys_comp_name, coll_tag_str)                                                         \
  PHYS_IMPLEMENT_APPLY_AUTHORITY_STATE(phys_type, phys_comp_name, phys_update_es)                                         \
  /* This ES is needed for accessing phys state without depending on actual phys type*/                                   \
  ECS_AFTER(phys_comp_name##_collision_es)                                                                                \
  ECS_BEFORE(after_collision_sync)                                                                                        \
  static inline void phys_comp_name##_post_update_es(const UpdatePhysEvent &info, const phys_type &phys_comp_name,        \
    int &net_phys__atTick, int &net_phys__lastAppliedControlsForTick, float &net_phys__timeStep,                          \
    Point3 &net_phys__currentStateVelocity, Point3 &net_phys__currentStatePosition, Point4 &net_phys__currentStateOrient, \
    Point3 &net_phys__previousStateVelocity, int &net_phys__role, float *net_phys__interpK)                               \
  {                                                                                                                       \
    const auto &phys = phys_comp_name.phys;                                                                               \
    net_phys__atTick = phys.getCurrentTick();                                                                             \
    net_phys__lastAppliedControlsForTick = phys.getLastAppliedControlsForTick();                                          \
    net_phys__timeStep = phys.timeStep;                                                                                   \
    net_phys__currentStateVelocity = phys.getCurrentStateVelocity();                                                      \
    net_phys__currentStatePosition = phys.getCurrentStatePosition();                                                      \
    net_phys__previousStateVelocity = phys.getPreviousStateVelocity();                                                    \
    const Quat &q = phys.currentState.location.O.getQuat();                                                               \
    net_phys__currentStateOrient.set(q.x, q.y, q.z, q.w);                                                                 \
    net_phys__role = phys_comp_name.getRole();                                                                            \
    if (net_phys__interpK)                                                                                                \
      *net_phys__interpK = calc_interpk(phys_comp_name, info.curTime);                                                    \
  }
