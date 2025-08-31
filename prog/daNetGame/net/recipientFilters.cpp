// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "recipientFilters.h"
#include <daECS/net/msgSink.h>
#include <daECS/net/object.h>
#include "game/player.h"
#include "ecs/game/generic/team.h"
#include <memory/dag_framemem.h>
#include "net/net.h"

ECS_DECLARE_GET_FAST_BASE(int, team, "team");

namespace rcptf
{

dag::Span<net::IConnection *> pos_range_team_impl(const Point3 &pos,
  float range,
  int team,
  bool is_player_relative,
  bool filter_possessed,
  ecs::EntityId except_possessed,
  Tab<net::IConnection *> &out_conns)
{
  game::enum_connections_range_team(out_conns, pos, range, team, is_player_relative, filter_possessed, except_possessed);
  return make_span(out_conns);
}

dag::Span<net::IConnection *> entity_pos_range_team_impl(ecs::EntityId pos_eid,
  float range,
  ecs::EntityId team_eid,
  bool is_player_relative,
  bool filter_possessed,
  ecs::EntityId except_possessed,
  Tab<net::IConnection *> &out_conns)
{
  const Point3 pos = g_entity_mgr->getOr(pos_eid, ECS_HASH("transform"), TMatrix::IDENT).getcol(3);
  const int team = ECS_GET_OR(team_eid, team, (int)TEAM_UNASSIGNED);
  game::enum_connections_range_team(out_conns, pos, range, team, is_player_relative, filter_possessed, except_possessed);
  return make_span(out_conns);
}

} // namespace rcptf
