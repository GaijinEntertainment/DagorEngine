// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "physKind.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/net/netbase.h>
#include <daECS/net/connection.h>
#include <gamePhys/collision/collisionLib.h>
#include <phys/dag_physics.h>
#include "net/dedicated.h"
#include "net/time.h"
#include "net/channel.h"
#include "phys/physUtils.h"
#include "phys/netPhys.cpp.inl"
#include "phys/netPhysSnapshots.h"

ECS_REGISTER_EVENT(CmdPhysKindStep)
ECS_REGISTER_EVENT(CmdPhysKindApplyState)

ECS_NET_DECL_MSG(PhysKindSnapshotsMsg, danet::BitStream);
ECS_NET_IMPL_MSG(PhysKindSnapshotsMsg,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::direct_connection_rcptf,
  UNRELIABLE,
  NC_PHYS_SNAPSHOTS,
  net::MF_DONT_COMPRESS);
IMPLEMENT_PHYS_ACTOR_NET_RCV(PhysKindSnapshotsMsg, get_phys_actor);

// The owner applies its controls as the server reads them
template <>
bool PhysKindActor::beforeEnqueueCT(double /*at_time*/)
{
  phys.producedCT.snapToWire();
  return true;
}

ECS_REGISTER_PHYS_ACTOR_TYPE(PhysKindActor);
ECS_AUTO_REGISTER_COMPONENT_DEPS(
  PhysKindActor, "phys_kind_net_phys", nullptr, 0, "?collres", "?phys_vars", "net__physId", "base_net_phys_ptr");

template class PhysActor<PhysKind, PhysKindCustomPhys, PhysType::CUSTOM8>;

static constexpr float PHYS_KIND_MAX_SHADOW_SIM_SEC = 1.f;

struct DngPhysKindHost final : public IPhysKindHost
{
  void step(PhysKind &phys, int32_t tick, float dt, bool is_for_real) override
  {
    const BasePhysActor &actor = *static_cast<BasePhysActor *>(phys.getActor());
    const bool useControls = actor.getRole() != IPhysActor::ROLE_REMOTELY_CONTROLLED_SHADOW;
    g_entity_mgr->sendEventImmediate(actor.getEid(), CmdPhysKindStep(tick, dt, is_for_real, useControls));
  }
  void applyState(PhysKind &phys, bool keep_body_motion) override
  {
    g_entity_mgr->sendEventImmediate(static_cast<BasePhysActor *>(phys.getActor())->getEid(), CmdPhysKindApplyState(keep_body_motion));
  }
};
static DngPhysKindHost dng_phys_kind_host;

// A replaced actor's controls carry another tag, below the sign bit
static uint32_t controls_seq_tag(int net_phys_id) { return uint32_t(net_phys_id) % 127 + 1; }

void PhysKindCustomPhys::onPreInit(ecs::EntityId eid, BasePhysActor &actor)
{
  PhysKindActor &kindActor = static_cast<PhysKindActor &>(actor);
  PhysKind &phys = kindActor.phys;
  // The first AAS must not beat a client's recreate
  phys.physicsTimeToSendState = phys.physicsTimeToSendPartialState = 1.f;
  phys.host = &dng_phys_kind_host;
  phys.external = g_entity_mgr->getOr(eid, ECS_HASH("phys_kind__external"), false);
  phys.shadowSim = g_entity_mgr->getOr(eid, ECS_HASH("phys_kind__shadowSim"), false);
  kindActor.customPhys.createdAt = get_sync_time_d();
  phys.lastCTSequenceNumber = int((controls_seq_tag(g_entity_mgr->getOr(eid, ECS_HASH("net__physId"), -1)) << 24) - 1);
}

// HIDDEN goes on the wire as MIN with no pose
static void write_snapshot_body(danet::BitStream &bs, const PhysKind &phys, PhysKindLod lod, bool hidden)
{
  bs.Write(hidden);
  if (!hidden)
    write_phys_kind_snap_pose(bs, lod, phys.currentState.location, phys.currentState.velocity);
  write_phys_kind_snap_ext(bs, phys.stateWire, phys.currentState.ext, phys.snapExtSize[(int)lod], lod);
}

void PhysKindCachedQuantizedInfo::update(const PhysKind &phys, ecs::EntityId)
{
  for (int lod = 0; lod < (int)PhysKindLod::Num; ++lod)
  {
    danet::BitStream bs(body[lod], BODY_MAX_BYTES, /*copy*/ false);
    bs.ResetWritePointer();
    write_snapshot_body(bs, phys, PhysKindLod(lod), /*hidden*/ false);
    bodyBits[lod] = uint16_t(bs.GetNumberOfBitsUsed());
    // A stream keeps a partial last byte high, WriteBits takes it low
    if (const unsigned rest = bodyBits[lod] & 7u)
      body[lod][bodyBits[lod] / 8] >>= 8 - rest;
  }
  atTick = phys.currentState.atTick;
}

template <>
void PhysKindActor::serializePhysSnapshotImpl(danet::BitStream &bs, PhysSnapSerializeType pst) const
{
  G_ASSERTF(!cachedQuantInfo.isInvalid(), "Attempt to serialize not updated phys!");
  // An asleep body keeps its cache, not its tick
  bs.Write(uint8_t(phys.currentState.atTick));
  if (pst == PhysSnapSerializeType::HIDDEN)
    write_snapshot_body(bs, phys, PhysKindLod::Min, /*hidden*/ true);
  else
    bs.WriteBits(cachedQuantInfo.body[(int)pst], cachedQuantInfo.bodyBits[(int)pst]);
  bs.AlignWriteToByteBoundary();
}

template <>
bool PhysKindActor::deserializePhysSnapshot(
  const danet::BitStream &bs, PhysSnapSerializeType pst, BasePhysSnapshot &basesnap, net::IConnection &)
{
  PhysKindSnapshot &snap = *new (&basesnap, _NEW_INPLACE) PhysKindSnapshot;
  snap.lod = uint8_t(pst);
  bool hidden = false;
  if (!bs.Read(snap.stateTickLow) || !bs.Read(hidden))
    return false;
  if (hidden)
  {
    snap.pos = Point3(HIDDEN_QWPOS_XYZ);
    snap.quat = Quat(0.f, 0.f, 0.f, 1.f);
    snap.vel.zero();
  }
  else
  {
    DPoint3 pos;
    if (!read_phys_kind_snap_pose(bs, PhysKindLod(pst), pos, snap.quat, snap.vel))
      return false;
    snap.pos = Point3::xyz(pos);
  }
  if (!read_phys_kind_snap_ext(bs, snap.ext, snap.extBits))
    return false;
  bs.AlignReadToByteBoundary();
  return true;
}

// Float wire time can miss a tick
static int snapshot_state_tick(const PhysKindSnapshot &snap)
{
  return snap.atTick + int8_t(uint8_t(snap.stateTickLow - uint8_t(snap.atTick)));
}

// A tickrate change leaves the low bytes in old ticks
static void rebase_snapshot_ticks(PhysKindActor &actor)
{
  const float was = eastl::exchange(actor.customPhys.snapTimeStep, actor.phys.timeStep);
  if (was == 0.f || was == actor.phys.timeStep)
    return;
  for (PhysKindSnapshot &snap : actor.snapshotsHistory)
    snap.stateTickLow = uint8_t(snap.atTick);
  actor.customPhys.lastCorrectedOwnerTick = actor.customPhys.lastShadowSnapTick = -1;
}

// The first receipt adds an extrapolated twin, HIDDEN can land before it
static PhysKindSnapshot &newest_received_snapshot(PhysKindActor &actor)
{
  auto &history = actor.snapshotsHistory;
  const bool backIsTwin = history.size() > 1 && history.back().atTick - history.front().atTick == 2 &&
                          history.front().stateTickLow == history.back().stateTickLow;
  return history[history.size() - 1 - backIsTwin];
}

static void merge_snapshot_ext(PhysKind &phys, const PhysKindSnapshot &snap)
{
  if (decode_phys_kind_snap_ext(phys.stateWire, snap.ext, snap.extBits, PhysKindLod(snap.lod), phys.currentState.ext,
        phys.snapExtSize[snap.lod]))
    return;
  const ecs::EntityId eid = static_cast<BasePhysActor *>(phys.getActor())->getEid();
  LOGERR_ONCE("phys_kind: %d bits of snapshot ext do not fit the state wire of %d<%s>", snap.extBits, ecs::entity_id_t(eid),
    g_entity_mgr->getEntityTemplateName(eid));
}

// A teleport or HIDDEN pair is no rotation
static Point3 snapshot_pair_omega(const PhysKindSnapshot &prev, const PhysKindSnapshot &last, float time_step)
{
  Quat delta = inverse(prev.quat) * last.quat;
  if (delta.w < 0.f)
    delta = Quat(-delta.x, -delta.y, -delta.z, -delta.w);
  const Point3 axisSin(delta.x, delta.y, delta.z);
  const float sinHalf = length(axisSin);
  const float dt = (snapshot_state_tick(last) - snapshot_state_tick(prev)) * time_step;
  if (sinHalf < 1e-6f || dt <= 0.f || lengthSq(last.pos - prev.pos) > PHYS_SNAP_TELEPORT_THRESHOLD_SQ)
    return Point3(0.f, 0.f, 0.f);
  return axisSin * (2.f * atan2f(sinHalf, delta.w) / (sinHalf * dt));
}

static constexpr float PHYS_KIND_SHADOW_ERROR_DECAY_SEC = 0.1f;

static void simulate_shadow_to(PhysKind &phys, int snap_tick, double present_time)
{
  const int lastSimTick = snap_tick + int(PHYS_KIND_MAX_SHADOW_SIM_SEC / phys.timeStep);
  const int toTick = eastl::min(gamephys::ceilPhysicsTickNumber(present_time, phys.timeStep), lastSimTick);
  for (int tick = phys.currentState.atTick; tick < toTick; ++tick)
  {
    phys.saveCurrentToPreviousState(tick);
    phys.updatePhys(double(tick) * phys.timeStep, phys.timeStep, false);
  }
  phys.interpolateVisualPosition(present_time);
}

// A correction per snapshot needs a short decay
static void decay_visual_error(PhysKind &phys, double now)
{
  const float keep = expf(-eastl::max(float(now - phys.visualLocationErrorProductionTime), 0.f) / PHYS_KIND_SHADOW_ERROR_DECAY_SEC);
  gamephys::Loc zero, error = phys.visualLocationError;
  zero.resetLoc();
  phys.visualLocationError.interpolate(error, zero, 1.f - keep);
  phys.visualLocationErrorProductionTime = now;
}

static void extrapolate_state(PhysKindState &state, float dt)
{
  state.location.P += dpoint3(state.velocity * dt);
  const Point3 turn = state.omega * dt;
  const float angle = length(turn);
  if (angle > 1e-6f)
    state.location.O.setQuat(normalize(state.location.O.getQuat() * Quat(turn / angle, angle)));
}

static void load_newest_snapshot(PhysKindActor &actor, int at_tick)
{
  PhysKind &phys = actor.phys;
  const auto &history = actor.snapshotsHistory;
  const PhysKindSnapshot &last = newest_received_snapshot(actor);
  const int lastTick = snapshot_state_tick(last);
  PhysKindState &state = phys.currentState;
  state.atTick = at_tick;
  state.location.P = last.pos;
  state.location.O.setQuat(last.quat);
  state.velocity = last.vel;
  if (history.size() > 1)
    state.omega = snapshot_pair_omega(history[history.size() - 2], last, phys.timeStep);
  merge_snapshot_ext(phys, last);
  state.flags &= ~PhysKindState::FLAG_SLEEP; // a shadowSim copy that slept steps again from a new snapshot
  extrapolate_state(state,
    eastl::clamp(float((at_tick - lastTick) * phys.timeStep), -PHYS_KIND_MAX_SHADOW_SIM_SEC, PHYS_KIND_MAX_SHADOW_SIM_SEC));
  // A snapshot can be newer than present_time
  phys.saveCurrentToPreviousState(at_tick - 1);
  extrapolate_state(phys.previousState, -phys.timeStep);
}

static void update_shadow_sim(PhysKindActor &actor, double present_time)
{
  PhysKind &phys = actor.phys;
  auto &history = actor.snapshotsHistory;
  if (history.empty())
    return;
  PhysKindSnapshot &last = newest_received_snapshot(actor);
  const int lastTick = snapshot_state_tick(last);
  int &simSnapTick = actor.customPhys.lastShadowSnapTick;
  // HIDDEN can replace a snapshot in place
  const bool isNewSnapshot = !eastl::exchange(last.shadowLoaded, true) || simSnapTick < 0;
  const bool hadState = simSnapTick >= 0;
  decay_visual_error(phys, present_time);
  gamephys::Loc visualBefore;
  if (isNewSnapshot && hadState)
  {
    // Compare both simulations at the same time
    simulate_shadow_to(phys, simSnapTick, present_time);
    visualBefore.add(phys.visualLocation, phys.visualLocationError);
  }
  if (isNewSnapshot)
  {
    simSnapTick = lastTick;
    // An engine body cannot replay, so it starts at the present
    load_newest_snapshot(actor, phys.external ? gamephys::ceilPhysicsTickNumber(present_time, phys.timeStep) : lastTick);
    if (phys.external)
      phys.applyState();
  }
  simulate_shadow_to(phys, lastTick, present_time);
  if (isNewSnapshot && hadState)
    phys.visualLocationError.substract(visualBefore, phys.visualLocation);
  // A HIDDEN or teleport jump snaps
  if (lengthSq(phys.visualLocationError.P) > PHYS_SNAP_TELEPORT_THRESHOLD_SQ)
    phys.visualLocationError.resetLoc();
  const gamephys::Loc visualSimulated = phys.visualLocation;
  phys.visualLocation.add(visualSimulated, phys.visualLocationError);
}

template <>
void PhysKindActor::updateRemoteShadow(double remote_time, float dt, const BasePhysActor::UpdateContext &uctx)
{
  rebase_snapshot_ticks(*this);
  if (phys.shadowSim)
    update_shadow_sim(*this, remote_time + PhysUpdateCtx::ctx.getInterpDelay(tickrateType, phys.timeStep));
  else if (const SnapshotPair *spair = updateRemoteShadowBase(remote_time, dt, uctx))
  {
    merge_snapshot_ext(phys, spair->lastSnap);
    phys.currentState.omega = snapshot_pair_omega(spair->prevSnap, spair->lastSnap, phys.timeStep);
  }
}

template <>
void PhysKindActor::teleportTo(const TMatrix &tm, bool)
{
  teleportToDefaultImpl(tm);
  phys.applyState();
}

static void shift_state(PhysKindState &state, const DPoint3 &pos_delta, const Quat &rot_delta, const Point3 &vel_delta)
{
  state.location.P += pos_delta;
  state.location.O.setQuat(normalize(rot_delta * state.location.O.getQuat()));
  state.velocity += vel_delta;
}

// Pose tick labels can be one off
static const PhysKindState *find_closest_history_state(const PhysKind &phys, int tick, const DPoint3 &pos)
{
  const PhysKindState *best = nullptr;
  const auto first = eastl::lower_bound(phys.historyStates.begin(), phys.historyStates.end(), tick - 1,
    [](const PhysKindState &state, int t) { return state.atTick < t; });
  // A body turning in place ties on position
  const auto key = [&](const PhysKindState &state) {
    return eastl::make_pair(lengthSq(state.location.P - pos), abs(state.atTick - tick));
  };
  for (auto it = first; it != phys.historyStates.end() && it->atTick <= tick + 1; ++it)
    if ((it == first || it[-1].atTick != it->atTick) && (!best || key(*it) < key(*best)))
      best = it;
  return best;
}

static constexpr float PHYS_KIND_FULL_CORRECTION_ERROR = 1.f;

// The owner runs a round trip ahead
static bool shift_owner_by_auth_error(PhysKind &phys,
  const gamephys::Loc &auth_loc,
  const Point3 &auth_vel,
  int auth_tick,
  PhysDesyncStats &stats,
  float min_error,
  float blend)
{
  stats.processed++;
  if (const PhysKindState *past = find_closest_history_state(phys, auth_tick, auth_loc.P))
  {
    DPoint3 posDelta = auth_loc.P - past->location.P;
    const double errorSq = lengthSq(posDelta);
    if (errorSq > sqr(0.1))
      stats.largeDifference++;
    else if (errorSq > sqr(0.01))
      stats.smallDifference++;
    if (errorSq < sqr(min_error))
      return false;
    if (errorSq > sqr(PHYS_KIND_FULL_CORRECTION_ERROR))
      blend = 1.f;
    posDelta *= blend;
    const Quat rotDelta = qinterp(Quat(0.f, 0.f, 0.f, 1.f), auth_loc.O.getQuat() * inverse(past->location.O.getQuat()), blend);
    const Point3 velDelta = (auth_vel - past->velocity) * blend;
    // The caller keeps the history from auth_tick
    for (PhysKindState &state : phys.historyStates)
      if (state.atTick >= auth_tick)
        shift_state(state, posDelta, rotDelta, velDelta);
    shift_state(phys.currentState, posDelta, rotDelta, velDelta);
    shift_state(phys.previousState, posDelta, rotDelta, velDelta);
    return true;
  }
  // A newer correction erased this tick
  if (!phys.historyStates.empty() && auth_tick < phys.historyStates.front().atTick)
    return false;
  stats.unmatched++;
  const float aheadSec = (phys.currentState.atTick - auth_tick) * phys.timeStep;
  for (PhysKindState *state : {&phys.currentState, &phys.previousState})
  {
    state->location = auth_loc;
    state->location.P += dpoint3(auth_vel * aheadSec);
    state->velocity = auth_vel;
  }
  return true;
}

// Below it the snapshot quantization noise
static constexpr float PHYS_KIND_OWNER_SNAPSHOT_MIN_ERROR = 0.02f;
// Partial steps average out sub-tick timing noise
static constexpr float PHYS_KIND_OWNER_SNAPSHOT_BLEND = 0.25f;

// The server applies the owner's inputs a few ticks late
static int owner_tick_for_controls(const PhysKind &phys, int controls_tick)
{
  return controls_tick + phys.currentState.atTick - phys.currentState.lastAppliedControlsForTick;
}

static void apply_external_authority_state(PhysKindActor &actor, double at_time, bool smooth)
{
  PhysKind &phys = actor.phys;
  Tab<gamephys::Loc> visibleBefore(framemem_ptr());
  phys.calcVisualLocFromLocation(at_time, phys.currentState.location, visibleBefore);
  rebase_snapshot_ticks(actor);
  // An older tick would match a newer corrected state
  int &correctedAtTick = actor.customPhys.lastCorrectedOwnerTick;
  const int correctedBefore = correctedAtTick;
  if (!phys.isAuthorityApprovedStateProcessed && phys.authorityApprovedState)
  {
    const PhysKindState &aas = *phys.authorityApprovedState;
    phys.isAuthorityApprovedStateProcessed = true;
    phys.lastProcessedAuthorityApprovedStateAtTick = aas.atTick;
    phys.applyUnapprovedCTAsAt(aas.lastAppliedControlsForTick + 1, true, 0, actor.getAuthorityUnitVersion());
    const int ownerTick = owner_tick_for_controls(phys, aas.lastAppliedControlsForTick);
    actor.customPhys.serverTickLag = aas.atTick - ownerTick;
    if (ownerTick > correctedAtTick &&
        shift_owner_by_auth_error(phys, aas.location, aas.velocity, ownerTick, phys.desyncStats, 0.f, 1.f))
      correctedAtTick = ownerTick;
  }
  CommonPhysPartialState &partial = phys.authorityApprovedPartialState;
  if (!partial.isProcessed && phys.lastProcessedAuthorityApprovedStateAtTick > 0 &&
      partial.atTick > phys.lastProcessedAuthorityApprovedStateAtTick)
  {
    partial.isProcessed = true;
    const int ownerTick = owner_tick_for_controls(phys, partial.lastAppliedControlsForTick);
    if (ownerTick > correctedAtTick &&
        shift_owner_by_auth_error(phys, partial.location, Point3::xyz(partial.velocity), ownerTick, phys.desyncStats, 0.f, 1.f))
      correctedAtTick = ownerTick;
  }
  if (!actor.snapshotsHistory.empty() && phys.lastProcessedAuthorityApprovedStateAtTick > 0)
  {
    const PhysKindSnapshot &snap = newest_received_snapshot(actor);
    const int snapTick = snapshot_state_tick(snap);
    // A HIDDEN snapshot carries no pose
    if (snapTick != actor.customPhys.lastOwnerSnapTick && lengthSq(snap.pos - Point3(HIDDEN_QWPOS_XYZ)) > HIDDEN_QWPOS_TOLERANCE_SQ)
    {
      actor.customPhys.lastOwnerSnapTick = snapTick;
      gamephys::Loc snapLoc;
      snapLoc.P = dpoint3(snap.pos);
      snapLoc.O.setQuat(snap.quat);
      const int ownerTick = snapTick - actor.customPhys.serverTickLag;
      if (ownerTick > correctedAtTick && shift_owner_by_auth_error(phys, snapLoc, snap.vel, ownerTick, phys.partialStateDesyncStats,
                                           PHYS_KIND_OWNER_SNAPSHOT_MIN_ERROR, PHYS_KIND_OWNER_SNAPSHOT_BLEND))
        correctedAtTick = ownerTick;
    }
  }
  if (correctedAtTick == correctedBefore)
    return;
  auto keepFrom = eastl::lower_bound(phys.historyStates.begin(), phys.historyStates.end(), correctedAtTick,
    [](const PhysKindState &state, int t) { return state.atTick < t; });
  phys.historyStates.erase(phys.historyStates.begin(), keepFrom);
  if (smooth)
    phys.updateVisualErrorForNewCorrection(at_time, visibleBefore, phys.currentState.location);
  else
    phys.visualLocationError.resetLoc();
  phys.applyState(/*keep_body_motion*/ true);
}

// A remote copy shows the past: the new owner starts at the present
static void seed_owner_from_snapshot(PhysKindActor &actor, double now)
{
  PhysKind &phys = actor.phys;
  if (actor.snapshotsHistory.empty())
    return;
  rebase_snapshot_ticks(actor);
  const gamephys::Loc visualBefore = phys.visualLocation;
  load_newest_snapshot(actor, gamephys::ceilPhysicsTickNumber(now, phys.timeStep));
  phys.currentState.lastAppliedControlsForTick = phys.currentState.atTick - 1;
  phys.historyStates.clear();
  phys.lastProcessedAuthorityApprovedStateAtTick = -1;
  actor.customPhys.lastCorrectedOwnerTick = -1;
  // AAS kept from an earlier ownership are stale
  phys.isAuthorityApprovedStateProcessed = phys.authorityApprovedPartialState.isProcessed = true;
  phys.applyState();
  phys.visualLocationError.substract(visualBefore, phys.currentState.location);
  phys.visualLocationErrorProductionTime = now;
}

ECS_BEFORE(after_net_phys_sync, phys_kind_phys_es)
ECS_AFTER(before_net_phys_sync)
ECS_REQUIRE_NOT(ecs::Tag disableUpdate)
static inline void phys_kind_apply_authority_state_es(const UpdatePhysEvent &info,
  PhysKindActor &phys_kind_net_phys,
  int net__physId,
  bool net_phys__disableAuthorityStateApplication = false,
  bool net_phys__smoothAuthorityApplication = true)
{
  PhysKind &phys = phys_kind_net_phys.phys;
  PhysKindCustomPhys &custom = phys_kind_net_phys.customPhys;
  const IPhysActor::NetRole role = phys_kind_net_phys.getRole();
  const int prevRole = eastl::exchange(custom.lastRole, int(role));
  // The real net__physId comes after onPreInit
  const uint8_t actorTag = uint8_t(controls_seq_tag(net__physId));
  phys.currentState.actorTag = actorTag;
  // A replaced actor's controls precede our first
  if (role == IPhysActor::ROLE_REMOTELY_CONTROLLED_AUTHORITY && !custom.ownControlsArrived)
  {
    Tab<PhysKindControls> &cts = phys.unapprovedCT;
    cts.erase(eastl::remove_if(cts.begin(), cts.end(),
                [actorTag](const PhysKindControls &ct) { return uint32_t(ct.sequenceNumber) >> 24 != actorTag; }),
      cts.end());
    custom.ownControlsArrived = !cts.empty();
  }
  if (role != IPhysActor::ROLE_LOCALLY_CONTROLLED_SHADOW)
    return;
  // A replaced actor's AAS is older, the next actor's can beat our recreate
  if (phys.authorityApprovedState &&
      (phys.authorityApprovedState->actorTag != actorTag ||
        phys.authorityApprovedState->atTick <= gamephys::floorPhysicsTickNumber(custom.createdAt, phys.timeStep)))
    phys.isAuthorityApprovedStateProcessed = true;
  if (prevRole == IPhysActor::ROLE_REMOTELY_CONTROLLED_SHADOW)
    seed_owner_from_snapshot(phys_kind_net_phys, info.curTimeD);
  if (net_phys__disableAuthorityStateApplication)
    return;
  if (phys.external)
    apply_external_authority_state(phys_kind_net_phys, info.curTimeD, net_phys__smoothAuthorityApplication);
  else
    net_phys_apply_authority_state_es(info, phys_kind_net_phys, net_phys__smoothAuthorityApplication);
}

ECS_BEFORE(after_net_phys_sync)
ECS_AFTER(before_net_phys_sync)
ECS_REQUIRE_NOT(ecs::Tag disableUpdate)
static inline void phys_kind_phys_es(const UpdatePhysEvent &info, PhysKindActor &phys_kind_net_phys, TMatrix &transform)
{
  const BasePhysActor::UpdateContext uctx{&transform, false, false};
  net_phys_update_es(info, phys_kind_net_phys, uctx);
  phys_kind_net_phys.phys.updatePhysInWorld(transform);
}

ECS_ON_EVENT(NET_PHYS_ES_EVENT_SET)
static inline void phys_kind_phys_events_es_event_handler(const ecs::Event &event, PhysKindActor &phys_kind_net_phys)
{
  auto evt = event.cast<ecs::EventNetMessage>();
  if (auto physSnapMsg = evt ? evt->get<0>()->cast<PhysKindSnapshotsMsg>() : nullptr)
  {
    G_FAST_ASSERT(physSnapMsg->connection);
    net_rcv_phys_snapshots(*physSnapMsg, /*to_msg_sink*/ false, physSnapMsg->get<0>(), *physSnapMsg->connection,
      PhysKindActor::physEidsList, &get_phys_actor, PhysKindActor::deserializePhysSnapshot, PhysKindActor::processSnapshotNoEntity);
  }
  else
    net_phys_event_handler(event, phys_kind_net_phys);
}

template <typename Callable>
static ecs::QueryCbResult phys_kind_external_ecs_query(Callable c);

// DNG steps this world on clients only
ECS_TAG(server)
ECS_AFTER(after_collision_sync)
static void phys_kind_server_world_step_es(const UpdatePhysEvent &info)
{
  if (!dedicated::is_dedicated())
    return;
  const bool haveExternalKinds = phys_kind_external_ecs_query([](const PhysKindActor &phys_kind_net_phys) {
    return phys_kind_net_phys.phys.external ? ecs::QueryCbResult::Stop : ecs::QueryCbResult::Continue;
  }) == ecs::QueryCbResult::Stop;
  if (PhysWorld *world = haveExternalKinds ? dacoll::get_phys_world() : nullptr)
  {
    // A ded frame can be 0.1 s long
    world->setMaxSubSteps(8);
    world->simulate(info.dt);
  }
}

PHYS_IMPLEMENT_ON_TICKRATE_CHANGED_ES(PhysKindActor, phys_kind_net_phys)
PHYS_IMPLEMENT_POST_UPDATE_ES(PhysKindActor, phys_kind_net_phys, after_net_phys_sync)

#include "phys/netPhysRegister.h"
REGISTER_PHYS_SLOT(PhysType::CUSTOM8, "phys_kind", PhysKindActor, phys_kind_net_phys, nullptr);

ECS_NO_ORDER
static inline void gather_phys_kind_net_phys_es_event_handler(const GatherNetPhysRecordsToSend &evt,
  ecs::EntityId eid,
  ECS_REQUIRE_NOT(ecs::Tag phys__disableSnapshots) PhysKindActor &phys_kind_net_phys,
  const net::Object &replication,
  const int *team,
  const Point4 *netLodZones,
  const Point4 *phys_kind__snapshotHz,
  ecs::Tag *physAlwaysSendSnapshot)
{
  PhysKind &phys = phys_kind_net_phys.phys;
  // An external owner corrects from its snapshots
  uint8_t connControlledBy{phys.external ? uint8_t(net::INVALID_CONNECTION_ID) : uint8_t(replication.getControlledBy())};
  decltype(PhysActorSendRecord::flags) flags{phys_kind_net_phys.isAsleep() ? PHYS_ASLEEP : NO_FLAGS};
  int physTick{phys.currentState.atTick};
  if (phys_kind_net_phys.tickrateType == PhysTickRateType::LowFreq)
  {
    flags |= LOW_FREQ_TICKRATE;
    if (physTick == phys_kind_net_phys.cachedQuantInfo.atTick)
      flags |= TICK_NOT_CHANGED;
  }
  if (!(flags & (TICK_NOT_CHANGED | PHYS_ASLEEP)) || phys_kind_net_phys.cachedQuantInfo.isInvalid())
    phys_kind_net_phys.cachedQuantInfo.update(phys, eid);
  if (physAlwaysSendSnapshot != nullptr)
    flags |= PHYS_ALWAYS_SEND_SHANPSHOT;
  const uint8_t lodRateShifts =
    phys_kind__snapshotHz ? PhysActorSendRecord::makeLodRateShifts(*phys_kind__snapshotHz, float(phys_get_tickrate())) : 0;
  evt.get<0>()->addRecToSend(
    {eid, flags, connControlledBy, (int8_t)(team ? *team : TEAM_UNASSIGNED), lodRateShifts, phys_kind_net_phys,
      Point3::xyz(phys.currentState.location.P), (uint16_t)phys_kind_net_phys.netPhysId, (uint16_t)physTick},
    PhysKindActor::staticPhysType, &send_snapshot_t<PhysKindSnapshotsMsg>,
    static_cast<serialize_phys_snap_t>(&PhysKindActor::serializePhysSnapshot), netLodZones);
}
