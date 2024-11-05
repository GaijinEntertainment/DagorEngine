// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netPhys.h"
#include "physUtils.h"
#include "physEvents.h"
#include <daECS/net/msgDecl.h>
#include <daECS/net/msgSink.h>
#include <EASTL/fixed_string.h>
#include <perfMon/dag_statDrv.h>
#include "net/net.h"
#include <ecs/core/entityManager.h>
#include <daECS/net/object.h>
#include <util/dag_convar.h>
#include "game/dasEvents.h"
#include "game/gameEvents.h"
#include "game/player.h"
#include <stdlib.h>
#include <startup/dag_globalSettings.h> // dgs_get_argv
#include <ioSys/dag_dataBlock.h>
#include <daECS/net/netbase.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <gamePhys/phys/queries.h>

#include "net/time.h"
#include "net/netStat.h"
#include "net/channel.h"
#include "net/recipientFilters.h"
#include "net/plosscalc.h"

#define PHYS_ECS_EVENT ECS_REGISTER_EVENT
DANETGAME_PHYS_ECS_EVENTS
#undef PHYS_ECS_EVENT
ECS_REGISTER_EVENT(CmdUpdateRemoteShadow)
ECS_REGISTER_EVENT(CmdPostPhysUpdate)
ECS_REGISTER_EVENT(CmdPostPhysUpdateRemoteShadow)
ECS_REGISTER_EVENT(QueryPhysActorsNotCollidable);
ECS_REGISTER_EVENT(QueryPhysActorsOneSideCollidable);
ECS_DEF_PULL_VAR(netPhysCode);


// this const is max lag of physics from current time because of absent controls
#define PHYS_MAX_LAGGED_PHYS_TIME 0.35f // TODO: extract this const from 'update_phys_for_multiplayer"

ConVarB cl_debug_extrapolation("net.cl_debug_extrapolation", false, "Show local skeletons of remote clients in extrapolation");
ConVarF cl_exp_aprch_k("net.cl_exp_aprch_k",
  0.05f,
  0.f,
  1.f,
  "Approach filter coefficient for smoothing remote shadows extrapolation. 0 - disable smoothing");
static ConVarF cl_max_extrapolation_time("net.cl_max_extrapolation_time",
  PHYS_MAX_LAGGED_PHYS_TIME + 0.25f,
  0.f,
  30.f,
  "Max time in seconds entity is allowed to be extrapolated into the unknown");
ConVarB sv_debug_phys_desyncs("phys.sv_debug_phys_desyncs", false, "Enable log dumps of physics desync errors");

float BasePhysActor::minTimeStep = 1.f / PHYS_DEFAULT_TICKRATE;
float BasePhysActor::maxTimeStep = 1.f / PHYS_DEFAULT_BOT_TICKRATE;
float DummyCustomPhys::aircraftCollisionImpulseSpeedMax = 0;

// on RELIABLE_UNORDERED reliability channel doesn't matter
ECS_NET_IMPL_MSG(AAStateMsg, net::ROUTING_SERVER_TO_CLIENT, &net::direct_connection_rcptf, RELIABLE_UNORDERED, NC_CONTROLS);
ECS_NET_IMPL_MSG(AAPartialStateMsg, net::ROUTING_SERVER_TO_CLIENT, &net::direct_connection_rcptf, UNRELIABLE, NC_CONTROLS);
ECS_NET_IMPL_MSG(ControlsMsg, // Dup this message for time redundancy (effectively this makes controls 2x of phys tickrate)
  net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER,
  ECS_NET_NO_RCPTF,
  UNRELIABLE_SEQUENCED,
  NC_CONTROLS,
  net::MF_DEFAULT_FLAGS,
  1000 / PHYS_DEFAULT_TICKRATE / 3);
// ControlsSeq is sent imm. after ControlsMsg; it's urgent to wake up the net thread and ensure controls and seq coupled in 1 packet
ECS_NET_IMPL_MSG(
  ControlsSeq, net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER, ECS_NET_NO_RCPTF, UNRELIABLE, NC_CONTROLS, net::MF_URGENT);
ECS_NET_IMPL_MSG(TeleportMsg, net::ROUTING_SERVER_TO_CLIENT, (&rcptf::entity_ctrl_conn<TeleportMsg, rcptf::TARGET_ENTITY>));
ECS_NET_IMPL_MSG(PhysReset, net::ROUTING_SERVER_TO_CLIENT, (&rcptf::entity_ctrl_conn<PhysReset, rcptf::TARGET_ENTITY>));
ECS_NET_IMPL_MSG(DesyncData, net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER, ECS_NET_NO_RCPTF, UNRELIABLE, NC_CONTROLS);

ECS_DECLARE_GET_FAST(TMatrix, transform);
ECS_DECLARE_GET_FAST_BASE(bool, phys__lowFreqTickrate, "phys__lowFreqTickrate");
ECS_DECLARE_GET_FAST(ecs::EidList, collidable_ignore_list);
ECS_DECLARE_GET_FAST(bool, havePairCollision);
ECS_DECLARE_GET_FAST(ecs::Tag, airplane);
ECS_DECLARE_GET_FAST(bool, isExploded);
ECS_DECLARE_GET_FAST(bool, isAlive);
ECS_DECLARE_GET_FAST(BasePhysActorPtr, base_net_phys_ptr);

void BasePhysActor::updateTransform(const TMatrix &in_tm) { ECS_SET(getEid(), transform, in_tm); }

static int cachedTickrate = -1, cachedBotTickrate = -1;
bool phys_set_tickrate(int tickrate, int bot_tickrate)
{
  const int previousTickrate = cachedTickrate;
  bool updated = false;
  if (cachedTickrate != ((tickrate < 0) ? PHYS_DEFAULT_TICKRATE : tickrate))
  {
    cachedTickrate = (tickrate < 0) ? PHYS_DEFAULT_TICKRATE : clamp(tickrate, PHYS_MIN_TICKRATE, PHYS_MAX_TICKRATE);
    updated = true;
  }
  if (cachedBotTickrate != ((bot_tickrate < 0) ? PHYS_DEFAULT_BOT_TICKRATE : bot_tickrate))
  {
    cachedBotTickrate = (bot_tickrate < 0) ? PHYS_DEFAULT_BOT_TICKRATE : clamp(bot_tickrate, PHYS_MIN_TICKRATE, PHYS_MAX_TICKRATE);
    updated = true;
  }
  if (updated)
  {
    BasePhysActor::minTimeStep = 1.f / cachedTickrate;
    BasePhysActor::maxTimeStep = 1.f / cachedBotTickrate;
    if (BasePhysActor::minTimeStep > BasePhysActor::maxTimeStep)
      eastl::swap(BasePhysActor::minTimeStep, BasePhysActor::maxTimeStep);

    PhysActorIterator pit;
    while (BasePhysActor *actor = pit.next())
      // Phys actors calculate timestep, taking current phys_get_tickrate() into account
      actor->setRoleAndTickrateType(actor->getRole(), actor->tickrateType);

    // The event is immediate, so the code, that relies on current tick calculations
    // can be adjusted because of tickrate change immediately, before it runs.
    g_entity_mgr->broadcastEventImmediate(EventTickrateChanged{previousTickrate, cachedTickrate});
  }

  return updated;
}

int phys_get_tickrate()
{
  if (DAGOR_UNLIKELY(cachedTickrate < 0))
    phys_set_tickrate(::dgs_get_settings()->getInt("tickrate", -1), ::dgs_get_settings()->getInt("bottickrate", -1));
  return cachedTickrate;
}

int phys_get_bot_tickrate()
{
  if (DAGOR_UNLIKELY(cachedBotTickrate < 0))
    phys_set_tickrate(::dgs_get_settings()->getInt("tickrate", -1), ::dgs_get_settings()->getInt("bottickrate", -1));
  return cachedBotTickrate;
}

float phys_get_timestep() { return BasePhysActor::minTimeStep; }

void cleanup_phys() { clear_all_phys_actors_lists(); }

/*static*/
StaticTab<phys_eids_list_t *, (int)PhysType::NUM> BasePhysActor::allPhysActorsLists;
/*static*/
phys_sync_states_packed_t *BasePhysActor::allSyncStates[(int)PhysType::NUM] = {nullptr};
namespace physreg
{
static StaticTab<free_actors_list_indexes_t *, (int)PhysType::NUM> freeActorIndexesLists;
static StaticTab<phys_id_gen_list_t *, (int)PhysType::NUM> physIdsGenerationsList;
static StaticTab<void (*)(), (int)PhysType::NUM> snapshotCacheResetCb;
StaticTab<const net::MessageClass *, (int)PhysType::NUM> snapshotMsgCls; //< parallel to snapshotMsgCb
StaticTab<void (*)(const net::IMessage *msg), (int)PhysType::NUM> snapshotMsgCb;
size_t snapshotSizeMax = 0;
const char *typeStrs[(int)PhysType::NUM] = {nullptr};
} // namespace physreg

void clear_all_phys_actors_lists()
{
  using namespace physreg;
  for (auto list_ptr : BasePhysActor::allPhysActorsLists)
    if (list_ptr)
      phys_eids_list_t().swap(*list_ptr);
  BasePhysActor::allPhysActorsLists.clear();
  for (auto falist_ptr : freeActorIndexesLists)
    for (auto &sublist : *falist_ptr)
      eastl::decay_t<decltype(sublist)>{}.swap(sublist);
  for (auto piglist_ptr : physIdsGenerationsList)
    phys_id_gen_list_t().swap(*piglist_ptr);
  BasePhysActor::locallyContolledClientEntities.clear();
  for (auto cb : snapshotCacheResetCb)
    cb();
}

void BasePhysActor::resizeSyncStates(int i)
{
  for (auto *syncstates : allSyncStates) // for each physics type
  {
    if (!syncstates)
      continue;
    if (i >= syncstates->size())
      syncstates->resize(i + 1);
    else
      (*syncstates)[i].clear();
  }
}

BasePhysActor::BasePhysActor(ecs::EntityId eid_, PhysType phys_type_) :
  eid(eid_),
  physType(phys_type_),
  tickrateType(ECS_GET_OR(eid, phys__lowFreqTickrate, false) ? PhysTickRateType::LowFreq : PhysTickRateType::Normal)
{
  ECS_SET(eid, base_net_phys_ptr, this);
}

const char *BasePhysActor::getPhysTypeStr() const { return physreg::typeStrs[(int)physType]; }

ECS_REGISTER_TYPE(BasePhysActorPtr, nullptr);
ECS_AUTO_REGISTER_COMPONENT(BasePhysActorPtr, "base_net_phys_ptr", nullptr, ecs::DataComponent::DONT_REPLICATE);
ECS_AUTO_REGISTER_COMPONENT(int, "collision_physMatId", nullptr, ecs::DataComponent::DONT_REPLICATE);

ECS_AUTO_REGISTER_COMPONENT(int, "net__physId", nullptr, 0);
void BasePhysActor::initNetPhysId(phys_eids_list_t &eidsList,
  phys_id_gen_list_t &physIdGenerations,
  free_actors_list_indexes_t &freeIndexes,
  phys_sync_states_packed_t &playersSyncStates,
  int *net__physId)
{
  G_ASSERTF_RETURN(net__physId, , "\"net.physId\" component is missing for entity %d of template <%s>, entity won't be synced",
    (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid));
  G_ASSERT(physIdGenerations.size() == eidsList.size());
  DAECS_EXT_ASSERT(eastl::find(eidsList.begin(), eidsList.end(), eid) == eidsList.end());

  static_assert(sizeof(NetPhysId) == sizeof(*net__physId));
  NetPhysId &netPhysIdRef = reinterpret_cast<NetPhysId &>(*net__physId); // netPhysIdRef.index == -1 if net__physId == -1

  if (is_server()) // server assigns ids
  {
    netPhysId = -1;

    if (eidsList.size() >= 128) // if would not fit in 7 bit (for effective BitStream::writeCompressed() implementation)
    {
      for (auto &freeIndexQueue : freeIndexes) // freeIndexes[0] are < 127 and are used first, freeIndexes[1] is everything else
      {
        if (freeIndexQueue.empty() || get_sync_time() < freeIndexQueue.front().availableAtTime)
          continue;

        netPhysIdRef.index = netPhysId = freeIndexQueue.front().freeIndex;
        netPhysIdRef.generation = ++physIdGenerations[netPhysId];

        G_ASSERTF(physIdGenerations[netPhysId] != 0,
          "phys id generation overflow is not supported" /*with phys id reuse timeout=1 this can happen at least after 18 hours*/);
        G_ASSERT(!eidsList[netPhysId]);
        eidsList[netPhysId] = eid;
        freeIndexQueue.pop_front();
        // Reset player sync states to default (PhysActorSyncState::Normal=0)
        unsigned stByteIdx = unsigned(netPhysId) / 4;
        unsigned stByteClearMask = ~(0b11 << (unsigned(netPhysId) % 4 * 2));
        for (auto &syncStates : playersSyncStates)
          if (uint8_t stbyte = (stByteIdx >= syncStates.size()) ? 0 : syncStates[stByteIdx])
            syncStates[stByteIdx] &= stByteClearMask;

        break;
      }
    }

    if (netPhysId < 0)
    {
      netPhysIdRef.index = netPhysId = (int)eidsList.size();
      netPhysIdRef.generation = 1;
      eidsList.push_back(eid);
      // server starts with generation 1, because 0 is used as default/invalid generation on the client
      physIdGenerations.push_back(netPhysIdRef.generation);
    }
  }
  else if (netPhysIdRef.index >= 0) // client resizes eidsList according to server assigned id
  {
    netPhysId = netPhysIdRef.index;
    if (netPhysId >= eidsList.size())
    {
      eidsList.resize(netPhysId + 1);
      physIdGenerations.resize(netPhysId + 1, 0); // generation 0 doesn't correspond to valid server-side generation
    }
    else if (eidsList[netPhysId])
      logwarn("Re-assign netPhysId attempt %d gen %u -> %u of %d<%s> -> %d<%s>", netPhysId, physIdGenerations[netPhysId],
        netPhysIdRef.generation, (ecs::entity_id_t)eidsList[netPhysId], g_entity_mgr->getEntityTemplateName(eidsList[netPhysId]),
        (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid));
    // Re-assignment in eidsList is possible and expected, but in rare situations it can be dangerous.
    // Because the creation of an entity can be postponed (due to resources load), relative order of
    // entities creation is not client-server consistent. Rarely an older entity can be created on the
    // client after a newer one. To mitigate this case, we have netPhysId generations checked here.
    if (netPhysIdRef.generation > physIdGenerations[netPhysId])
    {
      eidsList[netPhysId] = eid;
      physIdGenerations[netPhysId] = netPhysIdRef.generation;
    }
    else
      logwarn("Re-assign ignored for eid %d<%s>, received generation %u is <= of local %u", (ecs::entity_id_t)eid,
        g_entity_mgr->getEntityTemplateName(eid), netPhysIdRef.generation, physIdGenerations[netPhysId]);
  }
  else if (!clientSideOnly)
    G_ASSERTF(netPhysIdRef.index >= 0, "net.physId is not assigned for entity %d of template <%s>", (ecs::entity_id_t)eid,
      g_entity_mgr->getEntityTemplateName(eid)); // it's server duty to assign it
}

void BasePhysActor::postPhysUpdate(int32_t tick, float dt, bool is_for_real)
{
  g_entity_mgr->sendEventImmediate(eid, CmdPostPhysUpdate(tick, dt, is_for_real));
}

eastl::pair<float, float> phys_get_xpolation_times(int prev_tick, int last_tick, float timestep, float at_time)
{
  G_ASSERTF(last_tick > prev_tick, "%d <= %d", last_tick, prev_tick);
  float interpTime = (last_tick - prev_tick) * timestep;
  float cappedTime = min(at_time, last_tick * timestep + cl_max_extrapolation_time.get());
  float elapsedTime = cappedTime - prev_tick * timestep;
  return eastl::make_pair(interpTime, elapsedTime);
}

void teleport_phys_actor(ecs::EntityId eid, const TMatrix &tm)
{
  BasePhysActor *physObj = get_phys_actor(eid);
  if (!physObj)
    return;
  if (is_server())
    // Teleportation of a moving actor will most certanly result in noticeable desyncs due to time it takes TeleportMsg to arrive
    send_net_msg(eid, TeleportMsg(tm, true));
  physObj->teleportTo(tm, true);
}

void reset_phys_actor(ecs::EntityId eid)
{
  BasePhysActor *physObj = get_phys_actor(eid);
  if (is_server())
    send_net_msg(eid, PhysReset());
  physObj->reset();
}

using StashedMsgsVec = eastl::fixed_vector<eastl::tuple<ecs::EntityId, ControlsMsg>, 2, true, framemem_allocator>;
static StashedMsgsVec *stashedMessagesPtr = nullptr;
void stash_controls_msg(ecs::EntityId to_eid, ControlsMsg &&controlsMsg)
{
  stashedMessagesPtr->emplace_back(to_eid, eastl::move(controlsMsg));
}

void phys_enqueue_controls(float at_time)
{
  TIME_PROFILE(phys_enqueue_controls);

  StashedMsgsVec stashedMessages;
  stashedMessagesPtr = &stashedMessages;

  if (!BasePhysActor::locallyContolledClientEntities.empty())
  {
    for (ecs::EntityId eid : BasePhysActor::locallyContolledClientEntities)
      if (BasePhysActor *actor = get_phys_actor(eid))
        actor->enqueueCT(at_time);
      else
        G_ASSERTF(0, "Failed to get phys actor by eid %d<%s>", (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid));
  }
  else
  {
    PhysActorIterator pit;
    while (BasePhysActor *actor = pit.next())
      actor->enqueueCT(at_time);
  }

  if (!stashedMessages.empty()) // Flush
  {
    for (auto &it : stashedMessages)
      send_net_msg(eastl::get<ecs::EntityId>(it), eastl::get<ControlsMsg>(eastl::move(it)));
    if (game::Player *player = game::get_local_player())
      send_net_msg(player->getEid(), ControlsSeq{player->nextControlsSeq()}); // This message is urgent to wake up the net thread
  }
  stashedMessagesPtr = nullptr;
}

void BasePhysActor::sendDesyncData(const danet::BitStream &sync_cur_data) { send_net_msg(eid, DesyncData(sync_cur_data)); }

ecs::EntityId find_closest_net_phys_actor(const Point3 &pos)
{
  float minDist = FLT_MAX;
  ecs::EntityId closestEntity = ecs::INVALID_ENTITY_ID;
  PhysActorIterator pit;
  while (const BasePhysActor *actor = pit.next())
  {
    ecs::EntityId eid = actor->eid;
    if (!ECS_GET_OR(eid, isAlive, false))
      continue;
    float distSq = lengthSq(ECS_GET_OR(eid, transform, TMatrix::IDENT).getcol(3) - pos);
    if (distSq < minDist)
    {
      minDist = distSq;
      closestEntity = eid;
    }
  }
  return closestEntity;
}

int get_interp_delay_ticks(PhysTickRateType tr_type)
{
  game::Player *lplr = game::get_local_player();
  return eastl::clamp(lplr ? lplr->calcControlsTickDelta(tr_type, /*interp*/ true) : PHYS_MIN_INTERP_DELAY_TICKS,
    PHYS_MIN_INTERP_DELAY_TICKS, PHYS_MAX_INTERP_DELAY_TICKS);
}

IPhysBase *gamephys::phys_by_id(int id)
{
  BasePhysActor *actor = get_phys_actor(ecs::EntityId(static_cast<ecs::entity_id_t>(id)));
  return actor ? &actor->getPhys() : nullptr;
}

int calc_phys_update_to_tick(float at_time, float dt, int ctrl_tick, int ct_produced_at_tick, int last_aas_tick)
{
  const int maxTicksDelta = (int)ceilf(PHYS_MAX_CONTROLS_TICKS_DELTA_SEC / dt);
  const int curCeilTick = gamephys::ceilPhysicsTickNumber(at_time, dt);

  const int upToCtrlTick = min(ct_produced_at_tick >= 0 ? ct_produced_at_tick + 1 : curCeilTick, ctrl_tick + maxTicksDelta);

  // Do not update client physics if AAS is lagged > PHYS_MAX_AAS_DELAY_K of it's send period - this fix unfair advantage of lagged
  // clients
  if (last_aas_tick > 0)
  {
    const int maxAuthorityDelayTicks = (PHYS_SEND_AUTH_STATE_PERIOD_SEC * PHYS_MAX_AAS_DELAY_K) / dt;
    return min(upToCtrlTick, last_aas_tick + maxAuthorityDelayTicks);
  }

  return upToCtrlTick;
}

/*static*/
void BasePhysActor::registerPhys(PhysType s,
  const char *pname,
  const char *cname_str,
  ecs::component_t cname,
  ecs::component_type_t ctype,
  size_t type_sz,
  size_t snapshot_size,
  phys_id_gen_list_t *id_gen,
  free_actors_list_indexes_t *free_aidx,
  phys_sync_states_packed_t *ss_p,
  void (*snapshot_cache_reset_cb)(),
  const net::MessageClass *msg_cls,
  void (*msg_cb)(const net::IMessage *msg))
{
  using namespace physreg;
  G_ASSERTF((int)s >= 0 && (int)s < (int)PhysType::NUM, "bad slot %d for %s", (int)s, pname);
  G_ASSERTF(BasePhysActor::allSyncStates[(int)s] != ss_p && find_value_idx(freeActorIndexesLists, free_aidx) == -1 &&
              freeActorIndexesLists.size() < freeActorIndexesLists.capacity() &&
              find_value_idx(physIdsGenerationsList, id_gen) == -1 &&
              physIdsGenerationsList.size() < physIdsGenerationsList.capacity(),
    "dup init %d for %s", (int)s, pname);

  BasePhysActor::allSyncStates[(int)s] = ss_p;
  typeStrs[(int)s] = pname;
  if (free_aidx)
    freeActorIndexesLists.push_back(free_aidx);
  if (id_gen)
    physIdsGenerationsList.push_back(id_gen);
  if (snapshot_cache_reset_cb)
    snapshotCacheResetCb.push_back(snapshot_cache_reset_cb);
  if (msg_cls && msg_cb)
  {
    snapshotMsgCls.push_back(msg_cls);
    snapshotMsgCb.push_back(msg_cb);
  }

  if (snapshotSizeMax < snapshot_size)
    snapshotSizeMax = snapshot_size;
  debug("registerPhys[%d]: %-16s type=0x%x(sz=%5d), comp=0x%x:%-32s snapshotSize=%3d (max=%d)", (int)s, pname, ctype, type_sz, cname,
    cname_str, snapshot_size, snapshotSizeMax);
  G_UNUSED(ctype);
  G_UNUSED(cname);
  G_UNUSED(type_sz);
}
