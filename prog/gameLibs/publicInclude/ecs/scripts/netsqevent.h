//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>
#include <daECS/core/entityId.h>

namespace net
{
class IConnection;
}
namespace ecs
{
struct SchemelessEvent;
}

class SqModules;
void register_net_sqevent(SqModules *module_mgr);

// Description:
//   Send/broadcast SQEvent locally and from server to clients.
//   No-op on client. Without network same as sendEvent/broadcastEvent of EntityManager
// Arguments:
//   'to_eid' - which entity send event to (same as in EntityManager::sendEvent)
//   'evt' - sq event to send
//   'to_whom' - pointer to array of connections to send this event to. If nullptr - broadcast (send to everyone), if empty - send to
//   no-one.
// Requirements:
//   'data' of SQEvent shall be SQTable or null
//   EntityId handles should be named '<something>eid' in order to be correctly resolved on remote system.
void server_send_net_sqevent(ecs::EntityId to_eid, ecs::SchemelessEvent &&evt, dag::Span<net::IConnection *> *to_whom = nullptr);
void server_broadcast_net_sqevent(ecs::SchemelessEvent &&evt, dag::Span<net::IConnection *> *to_whom = nullptr);
void client_send_net_sqevent(ecs::EntityId to_eid, const ecs::SchemelessEvent &evt, bool bcastevt = false);
