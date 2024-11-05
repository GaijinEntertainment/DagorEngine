//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <generic/dag_tab.h>
#include <daECS/core/entityId.h>
#include <daECS/net/message.h>

namespace rcptf
{
net::IConnection *get_entity_ctrl_conn(ecs::EntityId eid);

static constexpr int NO_ENTITY = -1;
static constexpr int TARGET_ENTITY = -2;
template <int I>
struct GetEntityImpl
{
  template <typename T>
  static ecs::EntityId get(ecs::EntityId, const T &tup)
  {
    return tup.template get<I>();
  }
};
template <>
struct GetEntityImpl<NO_ENTITY>
{
  template <typename T>
  static ecs::EntityId get(ecs::EntityId, const T &)
  {
    return ecs::INVALID_ENTITY_ID;
  }
};
template <>
struct GetEntityImpl<TARGET_ENTITY>
{
  template <typename T>
  static ecs::EntityId get(ecs::EntityId to_eid, const T &)
  {
    return to_eid;
  }
};

template <typename M, int EntityI>
inline dag::Span<net::IConnection *> entity_ctrl_conn(Tab<net::IConnection *> &out_conns, ecs::EntityId to_eid,
  const net::IMessage &msg_)
{
  auto msg = msg_.cast<M>();
  G_ASSERT(msg);
  ecs::EntityId eid = GetEntityImpl<EntityI>::get(to_eid, *msg);
  if (net::IConnection *conn = get_entity_ctrl_conn(eid))
    out_conns.push_back(conn);
  return make_span(out_conns);
}

}; // namespace rcptf
