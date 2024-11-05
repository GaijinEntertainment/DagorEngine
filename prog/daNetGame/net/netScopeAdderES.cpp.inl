// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netScope.h"
#include <ecs/core/entityManager.h>
#include "game/player.h"
#include <daECS/net/connection.h>
#include <util/dag_console.h>
#include "net/net.h"

static inline void do_add_entity_in_net_scope(net::IConnection &conn, net::Object &no, bool *errlogged = nullptr)
{
  if (DAGOR_LIKELY(static_cast<net::Connection &>(conn).setObjectInScopeAlways(no)))
    return;
  if (!errlogged || !*errlogged)
  {
    ecs::EntityId plrEid = game::find_player_eid_for_connection(&conn);
    logerr("Failed to add entity %d<%s> into net scope for conn=#%d,user{name,id}=<%s>/%llu. MaxReplicas limit reached?\n"
           "Entity histogram dumped in log.",
      (ecs::entity_id_t)no.getEid(), g_entity_mgr->getEntityTemplateName(no.getEid()), (int)conn.getId(),
      g_entity_mgr->getOr(plrEid, ECS_HASH("name"), ecs::nullstr),
      g_entity_mgr->getOr(plrEid, ECS_HASH("userid"), matching::INVALID_USER_ID));
    console::command("ecs.entity_histogram 0"); // 0 means whole histogram
  }
  if (errlogged)
    *errlogged = true;
}

// TODO: This is a copy of add_entity_in_net_scope function from net_scope_adder_common.das
//       Remove this copy later, when it will cease to be used in the cpp
void add_entity_in_net_scope(ecs::EntityId eid, int connid)
{
  if (auto conn = static_cast<net::Connection *>(get_client_connection(connid)))
  {
    net::Object *netObj = ECS_GET_NULLABLE_RW(net::Object, eid, replication);
    if (netObj)
      do_add_entity_in_net_scope(*conn, *netObj);
  }
}
