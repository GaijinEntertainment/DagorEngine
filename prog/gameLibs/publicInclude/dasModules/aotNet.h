//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/event.h>
#include <daECS/core/entityId.h>
#include <daECS/net/connection.h>
#include <daECS/net/netbase.h>
#include <daECS/net/replay.h>
#include <daECS/net/object.h>
#include <daECS/net/schemelessEventSerialize.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/component_replication_filter.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <ecs/scripts/netsqevent.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasDataBlock.h>
#include <daNet/disconnectionCause.h>
#include <daNet/bitStream.h>
#include <matching/types.h>

#include <daECS/net/netbase.h>
#include <daECS/net/recipientFilters.h>

DAS_BIND_ENUM_CAST(net::CompReplicationFilter);

MAKE_TYPE_FACTORY(NetObject, net::Object);
MAKE_TYPE_FACTORY(IConnection, net::IConnection);
MAKE_TYPE_FACTORY(ConnectionId, net::ConnectionId)

namespace das
{
template <>
struct cast<net::ConnectionId>
{
  static net::ConnectionId to(vec4f x) { return {das::cast<int>::to(x)}; }
  static vec4f from(net::ConnectionId x) { return das::cast<int>::from(x.value); }
};

template <>
struct WrapType<net::ConnectionId>
{
  enum
  {
    value = true
  };
  typedef int type;
  typedef int rettype;
};
} // namespace das

namespace circuit
{
const DataBlock *get_conf();
}

namespace net
{
matching::SessionId get_session_id();
}

namespace bind_dascript
{
vec4f _builtin_send_net_blobevent(das::Context &, das::SimNode_CallBase *call, vec4f *args);
vec4f _builtin_broadcast_net_blobevent(das::Context &, das::SimNode_CallBase *call, vec4f *args);
vec4f _builtin_send_net_blobevent_with_recipients(das::Context &, das::SimNode_CallBase *call, vec4f *args);
vec4f _builtin_broadcast_net_blobevent_with_recipients(das::Context &, das::SimNode_CallBase *call, vec4f *args);

inline bool get_client_connections(const das::TBlock<void, das::TTemporary<das::TArray<net::IConnection *>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  dag::Span<net::IConnection *> connections = ::get_client_connections();
  if (connections.empty())
    return false;

  das::Array arr;
  arr.data = (char *)connections.data();
  arr.size = connections.size();
  arr.capacity = connections.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return true;
}

inline void server_send_schemeless_event(ecs::EntityId eid, const char *evt_name, const das::TBlock<void, ecs::Object> &block,
  das::Context *context, das::LineInfoArg *at)
{
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  if (!is_server())
  {
    context->stackWalk(at, false, false);
    logerr("server_send_schemeless_event can be used only on server");
  }
#endif
  ecs::Object data;
  vec4f arg = das::cast<char *>::from((char *)&data);
  context->invoke(block, &arg, nullptr, at);
  const char *delimiter = strstr(evt_name, "::");
  const char *realName = delimiter ? delimiter + 2 : evt_name;
  ecs::SchemelessEvent evt(ECS_HASH_SLOW(realName).hash, eastl::move(data));
  server_send_net_sqevent(eid, eastl::move(evt));
}

inline void server_send_schemeless_event_to(ecs::EntityId eid, const char *evt_name, ecs::EntityId to_whom,
  const das::TBlock<void, ecs::Object> &block, das::Context *context, das::LineInfoArg *at)
{
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  if (!is_server())
  {
    context->stackWalk(at, false, false);
    logerr("server_send_schemeless_event can be used only on server");
  }
#endif
  ecs::Object data;
  vec4f arg = das::cast<char *>::from((char *)&data);
  context->invoke(block, &arg, nullptr, at);
  const char *delimiter = strstr(evt_name, "::");
  const char *realName = delimiter ? delimiter + 2 : evt_name;
  ecs::SchemelessEvent evt(ECS_HASH_SLOW(realName).hash, eastl::move(data));
  net::IConnection *conn = rcptf::get_entity_ctrl_conn(to_whom);
  auto toWhom = make_span(&conn, 1);
  server_send_net_sqevent(eid, eastl::move(evt), &toWhom);
}

inline void server_broadcast_schemeless_event(const char *evt_name, const das::TBlock<void, ecs::Object> &block, das::Context *context,
  das::LineInfoArg *at)
{
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  if (!is_server())
  {
    context->stackWalk(at, false, false);
    logerr("server_broadcast_schemeless_event can be used only on server");
  }
#endif
  ecs::Object data;
  vec4f arg = das::cast<char *>::from((char *)&data);
  context->invoke(block, &arg, nullptr, at);
  const char *delimiter = strstr(evt_name, "::");
  const char *realName = delimiter ? delimiter + 2 : evt_name;
  ecs::SchemelessEvent evt(ECS_HASH_SLOW(realName).hash, eastl::move(data));
  server_broadcast_net_sqevent(eastl::move(evt));
}

inline void server_broadcast_schemeless_event_to(const char *evt_name, ecs::EntityId to_whom,
  const das::TBlock<void, ecs::Object> &block, das::Context *context, das::LineInfoArg *at)
{
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  if (!is_server())
  {
    context->stackWalk(at, false, false);
    logerr("server_broadcast_schemeless_event can be used only on server");
  }
#endif
  ecs::Object data;
  vec4f arg = das::cast<char *>::from((char *)&data);
  context->invoke(block, &arg, nullptr, at);
  const char *delimiter = strstr(evt_name, "::");
  const char *realName = delimiter ? delimiter + 2 : evt_name;
  ecs::SchemelessEvent evt(ECS_HASH_SLOW(realName).hash, eastl::move(data));
  net::IConnection *conn = rcptf::get_entity_ctrl_conn(to_whom);
  auto toWhom = make_span(&conn, 1);
  server_broadcast_net_sqevent(eastl::move(evt), &toWhom);
}

inline void client_send_schemeless_event(ecs::EntityId eid, const char *evt_name, const das::TBlock<void, ecs::Object> &block,
  das::Context *context, das::LineInfoArg *at)
{
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  if (is_true_net_server())
  {
    context->stackWalk(at, false, false);
    logerr("client_send_schemeless_event can be used only on clients");
  }
#endif
  ecs::Object data;
  vec4f arg = das::cast<char *>::from((char *)&data);
  context->invoke(block, &arg, nullptr, at);
  const char *delimiter = strstr(evt_name, "::");
  const char *realName = delimiter ? delimiter + 2 : evt_name;
  ecs::SchemelessEvent evt(ECS_HASH_SLOW(realName).hash, eastl::move(data));
  if (has_network())
    client_send_net_sqevent(eid, eastl::move(evt));
  else
    g_entity_mgr->sendEvent(eid, eastl::move(evt));
}

inline void client_broadcast_schemeless_event(const char *evt_name, const das::TBlock<void, ecs::Object> &block, das::Context *context,
  das::LineInfoArg *at)
{
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  if (is_true_net_server())
  {
    context->stackWalk(at, false, false);
    logerr("client_broadcast_schemeless_event can be used only on clients");
  }
#endif
  ecs::Object data;
  vec4f arg = das::cast<char *>::from((char *)&data);
  context->invoke(block, &arg, nullptr, at);
  const char *delimiter = strstr(evt_name, "::");
  const char *realName = delimiter ? delimiter + 2 : evt_name;
  ecs::SchemelessEvent evt(ECS_HASH_SLOW(realName).hash, eastl::move(data));
  if (has_network())
    client_send_net_sqevent(ecs::INVALID_ENTITY_ID, eastl::move(evt), /*bcastevt*/ true);
  else
    g_entity_mgr->broadcastEvent(eastl::move(evt));
}

inline int net_object_getControlledBy(const net::Object &netObject) { return netObject.getControlledBy(); }
inline int connection_getId(const net::IConnection &conn) { return conn.getId(); }
inline void net_object_setControlledBy(net::Object &netObject, int connid) { netObject.setControlledBy(connid); }
inline void connection_setUserEid(net::IConnection &conn, ecs::EntityId eid)
{
  conn.setUserPtr((void *)(uintptr_t)(ecs::entity_id_t)eid);
}

inline bool setObjectInScopeAlways(net::IConnection &conn, net::Object &no)
{
  return static_cast<net::Connection &>(conn).setObjectInScopeAlways(no);
}

inline bool setEntityInScopeAlways(net::IConnection &conn, ecs::EntityId eid)
{
  return static_cast<net::Connection &>(conn).setEntityInScopeAlways(eid);
}

inline bool addObjectInScope(net::IConnection &conn, net::Object &no)
{
  return static_cast<net::Connection &>(conn).addObjectInScope(no);
}

inline void clearObjectInScopeAlways(net::IConnection &conn, net::Object &no)
{
  return static_cast<net::Connection &>(conn).clearObjectInScopeAlways(no);
}

inline void clearObjectInScope(net::IConnection &conn, net::Object &no)
{
  return static_cast<net::Connection &>(conn).clearObjectInScope(no);
}

inline const DataBlock &get_circuit_conf()
{
  const DataBlock *res = circuit::get_conf();
  return res ? *res : DataBlock::emptyBlock;
}
} // namespace bind_dascript
