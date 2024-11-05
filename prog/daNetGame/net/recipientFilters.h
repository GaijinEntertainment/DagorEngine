// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <generic/dag_tab.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/net/message.h>
#include <daECS/net/recipientFilters.h>

namespace rcptf
{

dag::Span<net::IConnection *> pos_range_team_impl(const Point3 &pos,
  float range,
  int team,
  bool is_player_relative,
  bool filter_possessed,
  ecs::EntityId except_possessed,
  Tab<net::IConnection *> &out_conns);
dag::Span<net::IConnection *> entity_pos_range_team_impl(ecs::EntityId pos_eid,
  float range,
  ecs::EntityId team_eid,
  bool is_player_relative,
  bool filter_possessed,
  ecs::EntityId except_possessed,
  Tab<net::IConnection *> &out_conns);

template <typename M, int PosEntityI, int RangeEntityI, int DefaultBcastRange = 25, bool IsPlayerRelative = false>
inline dag::Span<net::IConnection *> entity_pos_range(
  Tab<net::IConnection *> &out_conns, ecs::EntityId to_eid, const net::IMessage &msg_)
{
  auto msg = msg_.cast<M>();
  G_ASSERT(msg);
  ecs::EntityId rangeEntity = GetEntityImpl<RangeEntityI>::get(to_eid, *msg);
  ecs::EntityId posEntity = GetEntityImpl<PosEntityI>::get(to_eid, *msg);
  float range = g_entity_mgr->getOr(rangeEntity, ECS_HASH("bcastRange"), float(DefaultBcastRange));
  return entity_pos_range_team_impl(posEntity, range, ecs::INVALID_ENTITY_ID, IsPlayerRelative,
    /*filter_possessed*/ false, ecs::INVALID_ENTITY_ID, out_conns);
}

template <typename M, int PosEntityI, int RangeEntityI, int DefaultBcastRange = 25, bool IsPlayerRelative = false>
inline dag::Span<net::IConnection *> entity_pos_range_except_hero(
  Tab<net::IConnection *> &out_conns, ecs::EntityId to_eid, const net::IMessage &msg_)
{
  auto msg = msg_.cast<M>();
  G_ASSERT(msg);
  ecs::EntityId rangeEntity = GetEntityImpl<RangeEntityI>::get(to_eid, *msg);
  ecs::EntityId posEntity = GetEntityImpl<PosEntityI>::get(to_eid, *msg);
  float range = g_entity_mgr->getOr(rangeEntity, ECS_HASH("bcastRange"), float(DefaultBcastRange));
  return entity_pos_range_team_impl(posEntity, range, ecs::INVALID_ENTITY_ID, IsPlayerRelative,
    /*filter_possessed*/ true, to_eid, out_conns);
}

}; // namespace rcptf
