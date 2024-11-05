// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "player.h"
#include <daECS/core/utility/nullableComponent.h>
#include <memory/dag_framemem.h>
#include <EASTL/numeric.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <daECS/net/object.h>
#include <daECS/net/netbase.h>
#include <daECS/net/connection.h>
#include <startup/dag_globalSettings.h>
#include "net/net.h"
#include "net/dedicated.h"
#include "net/netScope.h"
#include <daECS/net/msgDecl.h>
#include "phys/netPhys.h"
#include "phys/physEvents.h"
#include "gameEvents.h"
#include <daECS/net/netEvents.h>
#include "main/ecsUtils.h"
#include <gamePhys/collision/collisionLib.h>

#include <game/dasEvents.h>
#include "net/time.h"
#include "net/recipientFilters.h"
#include "net/channel.h"
#include <daECS/core/baseIo.h>

template <typename Callable>
inline void players_ecs_query(Callable c);

template <typename Callable>
inline void players_connection_ecs_query(Callable c);

template <typename Callable>
inline void players_eid_connection_ecs_query(Callable c);

template <typename Callable>
inline void players_search_ecs_query(Callable c);

template <typename Callable>
inline void players_search_by_platfrom_ecs_query(Callable c);

namespace game
{
static ecs::EntityId localPlayerEid;

Player::Player(const ecs::EntityManager &, ecs::EntityId eid) : eid{eid} { resetSyncData(); }

int Player::calcControlsTickDelta(PhysTickRateType tr_type, bool interp)
{
  // basically get_server_conn() implies client code path
  net::IConnection *conn = (get_local_player_eid() == getEid()) ? get_server_conn() : getConn();
  danet::PeerQoSStat qst = conn ? conn->getPeerQoSStat() : danet::PeerQoSStat{};
  if (!qst.averagePing) // could be if packet throttle epoch is not passed yet (highly unlikely)
    return 0;
  RTTRec plrRTT{uint16_t(qst.lowestPing), uint16_t(qst.highestPingVariance)};
  // enet recalculates RTT each packetThrottleEpoch seconds (5 by default)
  if (rttSamples[(numRTTSamples - 1u) % rttSamples.static_size] == plrRTT)
    return controlsTickDelta[(int)tr_type];
  rttSamples[numRTTSamples++ % rttSamples.static_size] = plrRTT;
  int maxPlrRTT = 0, nSamples = eastl::min(numRTTSamples, rttSamples.static_size);
  for (int i = 0; i < nSamples; ++i)
    maxPlrRTT = eastl::max(maxPlrRTT, rttSamples[i].value());
  if (!interp) // For calculation of controls delta (i.e not interpolation) we use latency times (i.e. half of RTT)
    maxPlrRTT = (maxPlrRTT + 1) / 2;
  bool changed = false;
  for (int i = (interp ? 0 : (int)tr_type), lasti = (interp ? int(PhysTickRateType::Last) : (int)tr_type); i <= lasti; ++i)
  {
    float timeStep = BasePhysActor::getDefTimeStepByTickrateType((PhysTickRateType)i);
    int newControlsTickDelta = (int)ceilf(maxPlrRTT / 1000.f / timeStep); // round up
    int diff = newControlsTickDelta - controlsTickDelta[i];
    if (diff > 0 || diff <= -2) // allow tickDelta go up more easily then go down (this reduces oscillation)
    {
      changed = true;
      debug("Set player %d conn #%d for tickrate %d controlsTickDelta[%d]=%d -> %d", (ecs::entity_id_t)getEid(), (int)conn->getId(),
        int(1.f / timeStep + 0.5f), i, controlsTickDelta[i], newControlsTickDelta);
      controlsTickDelta[i] = newControlsTickDelta;
    }
  }
  if (changed)
  {
    char tmp[32];
    debug("Player %d conn #%d {min,avg}RTT=%d/%d {avg,max}RTTVar=%d/%d rttHist[%d]={%s}", (ecs::entity_id_t)getEid(),
      (int)conn->getId(), qst.lowestPing, qst.averagePing, qst.averagePingVariance, qst.highestPingVariance, nSamples,
      eastl::accumulate(rttSamples.begin(), rttSamples.begin() + eastl::min(numRTTSamples, rttSamples.static_size), eastl::string{},
        [&tmp](eastl::string &a, const RTTRec &rec) {
          snprintf(tmp, sizeof(tmp), "%d+-%d,", rec.rtt, rec.rttVar);
          a += tmp;
          return a;
        })
        .c_str());
  }
  return controlsTickDelta[(int)tr_type];
}

net::IConnection *Player::getConn() const
{
  int connid = ECS_GET(int, getEid(), connid);
  return (connid != net::INVALID_CONNECTION_ID) ? get_client_connection(connid) : nullptr;
}

void Player::resetSyncData()
{
  memset(physSnapshotsSeq, 0, sizeof(physSnapshotsSeq));
  numRTTSamples = 0;
  mem_set_0(rttSamples);
  mem_set_ff(controlsTickDelta); // set to -1
  controlsPlossCalc.resetSeq();
}

Player *find_player_for_connection(const net::IConnection *conn)
{
  if (!conn)
  {
    if (!dedicated::is_dedicated() && is_server()) // single player code path where we don't have connections but have local player
      return get_local_player();
    return nullptr;
  }
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());
  return ECS_GET_NULLABLE_RW(game::Player, plrEid, player);
}

ecs::EntityId find_player_eid_for_connection(const net::IConnection *conn)
{
  if (!conn)
  {
    if (!dedicated::is_dedicated() && is_server()) // single player code path where we don't have connections but have local player
      return get_local_player_eid();
    return ecs::INVALID_ENTITY_ID;
  }
  return ecs::EntityId((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());
}

ecs::EntityId find_player_eid_that_possess(ecs::EntityId eid) { return ECS_GET_OR(eid, possessedByPlr, ecs::INVALID_ENTITY_ID); }

Player *find_player_that_possess(ecs::EntityId eid)
{
  ecs::EntityId plrEid = find_player_eid_that_possess(eid);
  return ECS_GET_NULLABLE_RW(game::Player, plrEid, player);
}


Player *find_player_by_userid(matching::UserId uid)
{
  Player *result = nullptr;
  players_search_ecs_query([&](Player &player, uint64_t userid) {
    if (userid == uid)
    {
      result = &player;
      return ecs::QueryCbResult::Stop;
    }
    return ecs::QueryCbResult::Continue;
  });
  return result;
}

Player *find_player_by_platform_uid(const eastl::string &platform_uid)
{
  Player *result = nullptr;
  players_search_by_platfrom_ecs_query([&](Player &player, const eastl::string &platformUid) {
    if (!result && platformUid == platform_uid)
    {
      result = &player;
      return ecs::QueryCbResult::Stop;
    }
    return ecs::QueryCbResult::Continue;
  });
  return result;
}

int get_num_connected_players()
{
  int num = 0;
  players_connection_ecs_query(
    [&](bool disconnected ECS_REQUIRE_NOT(ecs::Tag playerIsBot) ECS_REQUIRE(const game::Player &player)) { num += !disconnected; });
  return num;
}

void enum_connections_range_team(Tab<net::IConnection *> &connections,
  const Point3 &pos,
  float range,
  int team_id,
  bool is_player_relative,
  bool filter_possessed,
  ecs::EntityId except_possessed)
{
  players_eid_connection_ecs_query(
    [&](ecs::EntityId eid, ecs::EntityId possessed, int connid, bool disconnected ECS_REQUIRE(const game::Player &player)) {
      if ((filter_possessed && possessed == except_possessed) || disconnected)
        return ecs::QueryCbResult::Continue;
      if (range > 0.f)
      {
        if (is_player_relative)
        {
          const Point3 *lat = player_get_looking_at(eid);
          if (!lat || lengthSq(pos - *lat) > sqr(range))
            return ecs::QueryCbResult::Continue;
        }
        else if (!g_entity_mgr->doesEntityExist(possessed) ||
                 lengthSq(pos - g_entity_mgr->getOr(possessed, ECS_HASH("transform"), TMatrix::IDENT).getcol(3)) > sqr(range))
          return ecs::QueryCbResult::Continue;
      }
      if (team_id != TEAM_UNASSIGNED && ECS_GET_OR(possessed, team, (int)TEAM_UNASSIGNED) != team_id)
        return ecs::QueryCbResult::Continue;
      connections.push_back(get_client_connection(connid));
      return ecs::QueryCbResult::Continue;
    });
}

void gather_players_poi(Tab<Point3> &points)
{
  players_ecs_query([&](ecs::EntityId possessed, bool disconnected ECS_REQUIRE(const game::Player &player)) {
    if (!disconnected && g_entity_mgr->doesEntityExist(possessed))
      points.push_back(g_entity_mgr->getOr(possessed, ECS_HASH("transform"), TMatrix::IDENT).getcol(3));
    return ecs::QueryCbResult::Continue;
  });
}

Player *get_local_player() { return ECS_GET_NULLABLE_RW(game::Player, localPlayerEid, player); }

ecs::EntityId get_local_player_eid() { return localPlayerEid; }

void set_local_player_eid(ecs::EntityId new_local_player)
{
  if (new_local_player == localPlayerEid)
    return;

  if (localPlayerEid)
    g_entity_mgr->set(localPlayerEid, ECS_HASH("is_local"), false);

  debug("%s %d -> %d", __FUNCTION__, (ecs::entity_id_t)localPlayerEid, (ecs::entity_id_t)new_local_player);

  localPlayerEid = new_local_player;

  if (new_local_player)
    g_entity_mgr->set(new_local_player, ECS_HASH("is_local"), true);
}

ecs::EntityId get_controlled_hero() { return ECS_GET_OR(localPlayerEid, possessed, ecs::INVALID_ENTITY_ID); }

ecs::EntityId get_watched_hero()
{
  ecs::EntityId targetEid = ECS_GET_OR(localPlayerEid, specTarget, ecs::INVALID_ENTITY_ID);
  if (targetEid)
    return targetEid;
  return get_controlled_hero();
}

void init_players() { localPlayerEid.reset(); }

ecs::EntityId player_get_looking_at_entity(ecs::EntityId player_eid, bool &spectating)
{
  ecs::EntityId lookingEid = ECS_GET_OR(player_eid, specTarget, ecs::INVALID_ENTITY_ID);
  spectating = (bool)lookingEid;
  if (!spectating)
    lookingEid = ECS_GET_OR(player_eid, possessed, ecs::INVALID_ENTITY_ID);
  return lookingEid;
}

const Point3 *player_get_looking_at(ecs::EntityId look_target, bool spectating, const Point3 **look_dir)
{
  auto tmPtr = ECS_GET_NULLABLE(TMatrix, look_target, transform);
  if (DAGOR_UNLIKELY(!tmPtr))
    return nullptr;
  if (look_dir && !spectating)
    g_entity_mgr->sendEventImmediate(look_target, CmdGetActorLookDir(look_dir));
  return &tmPtr->getcol(3);
}

const Point3 *player_get_looking_at(ecs::EntityId player_eid, const Point3 **look_dir)
{
  // To consider: return bitmask and store value in passed lookAt/lookDir refs
  bool spectating = false;
  ecs::EntityId lt = player_get_looking_at_entity(player_eid, spectating);
  return player_get_looking_at(lt, spectating, look_dir);
}

} // namespace game

ECS_REGISTER_RELOCATABLE_TYPE(game::Player, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(game::Player, "player", nullptr, 0, "?replication");
