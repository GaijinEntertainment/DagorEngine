// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/entityComponent.h>
#include <util/dag_simpleString.h>
#include <generic/dag_carray.h>
#include <matching/types.h>
#include <daGame/timers.h>
#include <generic/dag_tab.h>
#include <daECS/net/connid.h>
#include <daECS/core/event.h>
#include <math/dag_TMatrix.h> // for event
#include "net/plosscalc.h"
#include "game/team.h"
#include "phys/physConsts.h"

namespace net
{
class IConnection;
}
namespace ecs
{
struct Event;
class EventComponentChanged;
} // namespace ecs

enum ClientNetFlags : uint16_t
{
  CNF_REPLAY_RECORDING = 1,
  CNF_REPLICATE_PHYS_ACTORS = 2,
  CNF_CONN_UNRESPONSIVE = 4,
  CNF_DEVELOPER = 8, // Note: all users that connected to server using dev executable will have this flag
  CNF_SPAWNED_AT_LEAST_ONCE = 16,
  CNF_RECONNECT_FORBIDDEN = 32,
  // resv                   = 64,
  // resv                   = 128,
};

namespace game
{
class Player
{
public:
  Player(const ecs::EntityManager &mgr, ecs::EntityId eid);

  ecs::EntityId getEid() const { return eid; }
  net::IConnection *getConn() const;

  // return 0 if unknown (local, disconnected, etc...)
  int calcControlsTickDelta(PhysTickRateType tr_type = PhysTickRateType::Normal, bool interp = false);

  uint16_t *getPhysSnapshotsSeqRW() { return physSnapshotsSeq; }
  uint16_t nextControlsSeq() { return controlsPlossCalc.nextSeq(); }
  ControlsPlossCalc &getControlsPlossCalc() { return controlsPlossCalc; }
  uint16_t &getLastReportedCtrlSeq() { return lastReportedCtrlSeq; }

  void resetSyncData();

private:
  ecs::EntityId eid;
  uint32_t numRTTSamples = 0;
  struct RTTRec
  {
    union
    {
      struct
      {
        uint16_t rtt, rttVar;
      };
      int raw;
    };
    bool operator==(const RTTRec &rhs) const { return raw == rhs.raw; }
    int value() const { return int(rtt) + int(rttVar) * 2; }
  };
  carray<RTTRec, 12> rttSamples; // 8 x ENET_PEER_PACKET_THROTTLE_INTERVAL(5sec) = 1 min
  carray<int, (int)PhysTickRateType::Last + 1> controlsTickDelta;
  uint16_t physSnapshotsSeq[2] = {};
  ControlsPlossCalc controlsPlossCalc;
  uint16_t lastReportedCtrlSeq = 0;
};

void init_players(); // Note: there is no shutdown, it's destroyed by ecs EM
Player *get_local_player();
ecs::EntityId get_local_player_eid();
void set_local_player_eid(ecs::EntityId new_local_player);
Player *find_player_for_connection(const net::IConnection *conn);
ecs::EntityId find_player_eid_for_connection(const net::IConnection *conn);
Player *find_player_that_possess(ecs::EntityId eid);
ecs::EntityId find_player_eid_that_possess(ecs::EntityId eid);
Player *find_player_by_userid(matching::UserId uid);
Player *find_player_by_platform_uid(const eastl::string &platform_uid);
ecs::EntityId get_controlled_hero(); // shortcut for get_local_player()->getPossessedEntity()
ecs::EntityId get_watched_hero();
// Note: if 'conn' is NULL then local player will be created
int get_num_connected_players(); // Note: bots are not counted

void enum_connections_range_team(Tab<net::IConnection *> &connections,
  const Point3 &pos,
  float range,
  int team_id,
  bool is_player_relative,
  bool filter_possessed,
  ecs::EntityId except_possessed);

void gather_players_poi(Tab<Point3> &points);

ecs::EntityId player_get_looking_at_entity(ecs::EntityId player_eid, bool &spectating);
const Point3 *player_get_looking_at(ecs::EntityId look_target, bool spectating, const Point3 **look_dir = nullptr);
// To consider: return bitmask and store value in passed lookAt/lookDir refs
const Point3 *player_get_looking_at(ecs::EntityId player_eid, const Point3 **look_dir = nullptr);
}; // namespace game

ECS_DECLARE_RELOCATABLE_TYPE(game::Player);
