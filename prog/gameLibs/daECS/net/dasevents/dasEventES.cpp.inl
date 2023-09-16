#include <daECS/net/dasEvents.h>
#include <dasModules/aotEcsEvents.h>
#include <ecs/core/entityManager.h>

#include <daECS/net/component_replication_filter.h>
#include <daECS/net/object.h>
#include <daECS/net/netbase.h>
#include <daECS/net/msgDecl.h>
#include <daECS/net/replay.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/serialize.h>
#include <daECS/net/network.h>

#include <quirrel/sqModules/sqModules.h>

#include <daECS/net/netEvents.h>

static constexpr int DAS_NET_EVENTS_DEF_NET_CHANNEL = 0; // Note: this might be unresolved link time dependency if apps really need to
                                                         // configure that
static constexpr int DAS_NET_EVENTS_MAX_PLAYERS = 128;

ECS_NET_DECL_MSG(DasEventMsg, danet::BitStream /*payload*/);
ECS_NET_DECL_MSG(ClientDasEventMsg, danet::BitStream /*payload*/);
ECS_NET_DECL_MSG(ClientControlledEntityDasEventMsg, danet::BitStream /*payload*/);

ECS_NET_DECL_MSG(DasEventHandshakeMsg, /*proto_version*/ uint32_t);

namespace ecs
{
extern const int MAX_STRING_LENGTH;
}

namespace net
{
#if DAGOR_DBGLEVEL > 0
#define DAS_EVENT_STAT_ENABLED 1
struct DasEventStat
{
  ecs::event_type_t type;
  size_t bytes;
  uint32_t packets, maxBytes, minBytes;
  bool operator<(const DasEventStat &rhs) const { return type < rhs.type; }
};

using DasEventStats = eastl::vector_set<DasEventStat>;
static DasEventStats txDasEventsStats;
static DasEventStats rxDasEventsStats;

void trace_das_event(DasEventStats &dasEventStats, ecs::event_type_t event_type, uint32_t bytes, int packages = 1)
{
  if (packages <= 0)
    return; // corner case, empty connections list and no packages for example
  auto item = dasEventStats.find(DasEventStat{event_type});
  if (item != dasEventStats.end())
  {
    item->bytes += bytes * packages;
    item->packets += packages;
    item->maxBytes = eastl::max(item->maxBytes, bytes);
    item->minBytes = eastl::min(item->minBytes, bytes);
  }
  else
  {
    dasEventStats.emplace(DasEventStat{event_type, bytes, 1, bytes, bytes});
  }
}

void dump_das_event_stat(DasEventStats &dasEventStats, const char *prefix)
{
  if (dasEventStats.empty())
    return;
  size_t maxNameLen = 0;
  size_t totalBytes = 0;
  size_t totalPackets = 0;
  for (auto &rec : dasEventStats)
  {
    totalBytes += rec.bytes;
    totalPackets += rec.packets;
    const auto name = g_entity_mgr->getEventsDb().findEventName(rec.type);
    maxNameLen = eastl::max(strlen(name), maxNameLen);
  }
  eastl::sort(dasEventStats.begin(), dasEventStats.end(),
    [](const DasEventStat &a, const DasEventStat &b) { return a.bytes > b.bytes; });
  const int limitLog = eastl::min(30, (int)dasEventStats.size());
  debug("das_net: dumping %s %d dasevents of total %d records, %d KBytes, %d packets:", prefix, limitLog, dasEventStats.size(),
    totalBytes >> 10, totalPackets);
  debug("  #N  %*s msg_size_bytes(avg/min/max) kilobytes(%%) packets(%%)", maxNameLen, "name");
  for (int i = 0; i < limitLog; ++i)
  {
    const DasEventStat &r = dasEventStats[i];
    const size_t ebytes = r.bytes;
    debug("  #%2d %*s %4u %4u %5u %7u(%5.2f%%) %7u(%5.2f%%)", i + 1, maxNameLen, g_entity_mgr->getEventsDb().findEventName(r.type),
      ebytes / r.packets, r.minBytes, r.maxBytes, ebytes >> 10, double(ebytes) / totalBytes * 100., r.packets,
      double(r.packets) / totalPackets * 100.);
  }
}

#define TRACE_TX_DAS_EVENT_STAT(event_type, bs, packages_count) \
  trace_das_event(txDasEventsStats, event_type, (bs).GetNumberOfBytesUsed(), packages_count)

#define TRACE_RX_DAS_EVENT_STAT(event_type, bs) trace_das_event(rxDasEventsStats, event_type, (bs).GetNumberOfBytesUsed())

#define DUMP_DAS_EVENTS_STAT()                                      \
  net::dump_das_event_stat(net::txDasEventsStats, "tx (outgoing)"); \
  net::dump_das_event_stat(net::rxDasEventsStats, "rx (incoming)"); \
  net::txDasEventsStats.clear();                                    \
  net::rxDasEventsStats.clear()

#else

#define TRACE_TX_DAS_EVENT_STAT(...) ((void)0)
#define TRACE_RX_DAS_EVENT_STAT(...) ((void)0)
#define DUMP_DAS_EVENTS_STAT()       ((void)0)

#endif

enum
{
  DAS_EVENT_HASH_BITS = 22,
  DAS_EVENT_HASH_MASK = (1 << DAS_EVENT_HASH_BITS) - 1,
  EVENT_STACK_SIZE = 256,
};
static constexpr int SERVER_CONN_ID = 0;
static uint64_t lastDasEventsGeneration = 0;

static ska::flat_hash_map<ecs::event_type_t, /*conn_id:send*/ eastl::bitset<DAS_NET_EVENTS_MAX_PLAYERS>,
  ska::power_of_two_std_hash<ecs::EventsDB::event_id_t>>
  remoteEventWithError;

static void invalidate_das_events_now(const char *reason)
{
  debug("das_net: invalidate das events now `%s`", reason);
  lastDasEventsGeneration = bind_dascript::get_dasevents_generation();
  remoteEventWithError.clear();
}

static void invalidate_das_events_for_connection(int connId)
{
  debug("das_net: invalidate das events for conn #%d", connId);
  for (auto &errors : remoteEventWithError)
    errors.second.set(connId, false);
}

static void invalidate_das_events_gen(const char *reason)
{
  if (lastDasEventsGeneration != bind_dascript::get_dasevents_generation())
    invalidate_das_events_now(reason);
}

static int get_dasevent_error_level(ecs::event_type_t type)
{
  const auto netLiable = bind_dascript::get_dasevent_net_liable(type);
  return netLiable == bind_dascript::DascriptEventDesc::NetLiable::Ignore ? LOGLEVEL_WARN : LOGLEVEL_ERR;
}

static bind_dascript::DasEvent *deserialize_das_event(const danet::BitStream &bs, const net::IMessage *msg, char *buf, int buf_size)
{
  const int connectionId = (int)msg->connection->getId();
  ecs::event_type_t eventType;
  if (EASTL_UNLIKELY(!bs.Read(eventType)))
  {
    logerr("das_net: failed to read event received from connection '%d'", connectionId);
    return nullptr;
  }

  const auto remoteEventErrors = remoteEventWithError.find(eventType);
  if (remoteEventErrors != remoteEventWithError.end() && remoteEventErrors->second.test(connectionId))
    return nullptr; // error was already shown, just ignore this event

  const ecs::EventsDB::event_id_t eventId = g_entity_mgr->getEventsDb().findEvent(eventType);
  if (EASTL_UNLIKELY(eventId == ecs::EventsDB::invalid_event_id))
  {
    remoteEventWithError[eventType].set(connectionId);
    logmessage(get_dasevent_error_level(eventType), "das_net: unknown event type <0x%X> received from connection '%d'", eventType,
      connectionId);
    return nullptr;
  }

  const ecs::EventsDB::event_scheme_hash_t schemeHash = g_entity_mgr->getEventsDb().getEventSchemeHash(eventId);
  if (EASTL_UNLIKELY(schemeHash == ecs::EventsDB::invalid_event_scheme_hash))
  {
    remoteEventWithError[eventType].set(connectionId);
    logmessage(get_dasevent_error_level(eventType), "das_net: no scheme to deserialize event '%s' <0x%X>",
      g_entity_mgr->getEventsDb().getEventName(eventId), eventType);
    return nullptr;
  }

  const ecs::event_scheme_t &scheme = g_entity_mgr->getEventsDb().getEventScheme(eventId);
  const ecs::event_size_t eventSize = g_entity_mgr->getEventsDb().getEventSize(eventId);

  char *data = eventSize <= buf_size ? buf : (char *)framemem_ptr()->alloc(eventSize);
  const uint8_t *offsets = scheme.get<uint8_t>();
  const ecs::component_type_t *types = scheme.get<ecs::component_type_t>();

  net::BitstreamDeserializer deserializer(bs);
  for (int i = 0, n = (int)scheme.size(); i < n; ++i)
  {
    if (types[i] == ecs::ComponentTypeInfo<ecs::string>::type)
    {
      Tab<char> tmp(framemem_ptr());
      tmp.resize(ecs::MAX_STRING_LENGTH);
      ecs::read_string(deserializer, tmp.data(), tmp.size());
      *(char **)(data + offsets[i]) = str_dup(tmp.data(), strmem);
    }
    else if (types[i] == ecs::ComponentTypeInfo<ecs::Object>::type)
    {
      ecs::Object *obj = new ecs::Object();
      ecs::deserialize_component_typeless(obj, types[i], deserializer);
      obj->addMember("fromconnid", connectionId);
      *(ecs::Object **)(data + offsets[i]) = obj;
    }
    else if (types[i] == ECS_HASH("danet::BitStream").hash)
    {
      danet::BitStream *stream = new danet::BitStream();
      bs.Read(*stream);
      *(danet::BitStream **)(data + offsets[i]) = stream;
    }

#define DECL_LIST_TYPE(t, tn)                                         \
  else if (types[i] == ecs::ComponentTypeInfo<t>::type)               \
  {                                                                   \
    t *obj = new t();                                                 \
    ecs::deserialize_component_typeless(obj, types[i], deserializer); \
    *(t **)(data + offsets[i]) = obj;                                 \
  }

    DAS_EVENT_ECS_CONT_LIST_TYPES
#undef DECL_LIST_TYPE
    else
    {
      ecs::deserialize_component_typeless(data + offsets[i], types[i], deserializer);
    }
  }
  bind_dascript::DasEvent *res = (bind_dascript::DasEvent *)data;
  const ecs::event_flags_t flags = g_entity_mgr->getEventsDb().getEventFlags(eventId);
  res->set(eventType, eventSize, flags);
  return res;
}

static bool serialize_das_event(ecs::Event &evt, danet::BitStream &bs)
{
  const ecs::event_type_t eventType = evt.getType();
  const auto eventId = g_entity_mgr->getEventsDb().findEvent(eventType);
  if (EASTL_UNLIKELY(eventId == ecs::EventsDB::invalid_event_id))
  {
    logwarn("das_net: unknown event type <0x%X>", eventType);
    return false;
  }
  const ecs::EventsDB::event_scheme_hash_t schemeHash = g_entity_mgr->getEventsDb().getEventSchemeHash(eventId);
  if (EASTL_UNLIKELY(schemeHash == ecs::EventsDB::invalid_event_scheme_hash))
  {
    logwarn("das_net: no event <0x%X> scheme to serialize event", eventType);
    return false;
  }
  const ecs::event_scheme_t &scheme = g_entity_mgr->getEventsDb().getEventScheme(eventId);

  bs.Write(eventType);

  const uint8_t *offsets = scheme.get<uint8_t>();
  const ecs::component_type_t *types = scheme.get<ecs::component_type_t>();

  net::BitstreamSerializer serializer(bs);
  char *data = (char *)&evt;
  for (int i = 0, n = (int)scheme.size(); i < n; ++i)
  {
    if (types[i] == ecs::ComponentTypeInfo<ecs::string>::type)
    {
      char **strPtr = (char **)(data + offsets[i]);
      if (*strPtr)
        ecs::write_string(serializer, *strPtr, strlen(*strPtr));
      else
        ecs::write_string(serializer, "", 0);
      continue;
    }
    if (types[i] == ECS_HASH("danet::BitStream").hash)
    {
      danet::BitStream *stream = *(danet::BitStream **)(data + offsets[i]);
      bs.Write(*stream);
      continue;
    }

#define DECL_LIST_TYPE(t, tn)                                                \
  if (types[i] == ecs::ComponentTypeInfo<t>::type)                           \
  {                                                                          \
    t *obj = *(t **)(data + offsets[i]);                                     \
    ecs::serialize_entity_component_ref_typeless(obj, types[i], serializer); \
    continue;                                                                \
  }

    DAS_EVENT_ECS_CONT_TYPES
#undef DECL_LIST_TYPE

    ecs::serialize_entity_component_ref_typeless(data + offsets[i], types[i], serializer);
  }
  return true;
}

void all_client_connections_filter(Tab<net::IConnection *> &out_conns)
{
  auto connections = get_client_connections();
  for (auto connection : connections)
    if (connection)
      out_conns.push_back(connection);
}

void send_dasevent(ecs::EntityManager *mgr, bool broadcast, const ecs::EntityId eid, bind_dascript::DasEvent *evt,
  const char *event_name, eastl::optional<dag::ConstSpan<net::IConnection *>> explicitConnections,
  eastl::fixed_function<sizeof(void *), eastl::string()> debug_msg)
{
  if (has_network())
  {
    const ecs::event_type_t eventType = evt->getType();
    const ecs::EventsDB::event_id_t eventId = g_entity_mgr->getEventsDb().findEvent(eventType);
    if (EASTL_UNLIKELY(eventId == ecs::EventsDB::invalid_event_id))
    {
      logerr("das_net: unregistered event '%s' <0x%X>", evt->getName(), eventType);
      return;
    }
    invalidate_das_events_gen("send event gen");
    const auto routingAndReliability = bind_dascript::get_dasevent_routing_reliability(eventType);
    const uint8_t routing = eastl::get<0>(routingAndReliability);
    const PacketReliability reliability = eastl::get<1>(routingAndReliability);
    if (explicitConnections && routing != net::ROUTING_SERVER_TO_CLIENT)
      logerr("%s: explicit filters only work with ROUTING_SERVER_TO_CLIENT '%s' <0x%X>", debug_msg(), event_name, eventType);
    if (routing != bind_dascript::DASEVENT_NO_ROUTING && routing != bind_dascript::DASEVENT_NATIVE_ROUTING)
    {
      danet::BitStream bs;
      const ecs::EntityId sendToEid = broadcast ? net::get_msg_sink() : eid;

      if (routing == net::ROUTING_SERVER_TO_CLIENT)
      {
        if (serialize_das_event(*evt, bs))
        {
          Tab<net::IConnection *> connections{framemem_ptr()};
          if (!explicitConnections)
          {
            // If there are no explicit connections provided, send event to all connections available
            all_client_connections_filter(connections);
          }
          else
          {
            connections = *eastl::move(explicitConnections);
            if (auto conn = net::get_replay_connection())
            {
#if DAGOR_DBGLEVEL > 0
              if (find_value_idx(connections, conn) >= 0)
                logerr("Replay connection is already exist in recipients."
                       "Event '%s', remove replay connection from event filter.",
                  event_name);
              else
                connections.push_back(conn);
#else
              connections.push_back(conn);
#endif
            }
          }
          DasEventMsg evMsg(eastl::move(bs));
          net::MessageNetDesc md = evMsg.getMsgClass();
          md.rcptFilter = &net::direct_connection_rcptf;
          md.reliability = reliability;
          int sendCount = 0;
          for (auto conn : connections)
          {
            evMsg.connection = conn; // conn can be nullptr here, it's okay for send_net_msg
            // Note: yes, move is called within loop, it's ugly but it works (in network)
            sendCount += send_net_msg(sendToEid, eastl::move(evMsg), &md);
          }
          G_UNUSED(sendCount);
          TRACE_TX_DAS_EVENT_STAT(eventType, evMsg.get<0>(), sendCount);
        }
        else
          logerr("%s: unable to serialize event '%s' <0x%X>", debug_msg(), event_name, eventType);
      }
      else if (routing == net::ROUTING_CLIENT_TO_SERVER)
      {
        if (serialize_das_event(*evt, bs))
        {
          ClientDasEventMsg evMsg(eastl::move(bs));
          net::MessageNetDesc md = evMsg.getMsgClass();
          md.reliability = reliability;
          const int sendCount = send_net_msg(sendToEid, eastl::move(evMsg), &md);
          G_UNUSED(sendCount);
          TRACE_TX_DAS_EVENT_STAT(eventType, evMsg.get<0>(), sendCount);
        }
        else
          logerr("%s: unable to serialize event '%s' <0x%X>", debug_msg(), event_name, eventType);
      }
      else // if (eventDesc->second.routing == net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER)
      {
        auto robj = g_entity_mgr->getNullable<net::Object>(eid, ECS_HASH("replication"));
        if (!robj || robj->getControlledBy() == SERVER_CONN_ID)
        {
          if (serialize_das_event(*evt, bs))
          {
            ClientControlledEntityDasEventMsg evMsg(eastl::move(bs));
            net::MessageNetDesc md = evMsg.getMsgClass();
            md.reliability = reliability;
            const int sendCount = send_net_msg(sendToEid, eastl::move(evMsg), &md);
            G_UNUSED(sendCount);
            TRACE_TX_DAS_EVENT_STAT(eventType, evMsg.get<0>(), sendCount);
          }
          else
            logerr("%s: unable to serialize event '%s' <0x%X>", debug_msg(), event_name, eventType);
        }
        else
          logwarn("%s: failed to send controlled by entity event '%s' <0x%X>", debug_msg(), event_name, eventType);
      }
    }
    else
      logerr("%s: event '%s' <0x%X> without specified routing. Use sendEvent/broadcastEvent for this event", debug_msg(), event_name,
        eventType);
  }

  if (!broadcast)
    bind_dascript::_builtin_send_blobevent2(mgr, eid, (char *)evt, event_name);
  else
    bind_dascript::_builtin_broadcast_blobevent2(mgr, (char *)evt, event_name);
}

template <class EventName, typename CB>
static void base_das_event_msg_handler(const EventName *msg, CB send_event)
{
  G_ASSERT(msg);
  invalidate_das_events_gen("event gen");
  alignas(16) char buf[EVENT_STACK_SIZE];
  const danet::BitStream bs = msg->template get<0>();
  if (bind_dascript::DasEvent *evt = deserialize_das_event(bs, msg, buf, EVENT_STACK_SIZE))
  {
    TRACE_RX_DAS_EVENT_STAT(evt->getType(), bs);
    send_event((ecs::Event &)*evt);
    if (EASTL_UNLIKELY(evt->getFlags() & ecs::EVFLG_DESTROY)) // we have to do it, as it can be that there is immediate strategy.
      g_entity_mgr->getEventsDb().destroy(*evt);
    if ((char *)evt != buf)
      framemem_ptr()->free(evt);
  }
}

template <class EventName>
static void broadcast_das_event_msg_handler(const net::IMessage *msg_)
{
  base_das_event_msg_handler(msg_->cast<EventName>(), [](ecs::Event &evt) { g_entity_mgr->broadcastEvent(evt); });
}

template <class EventName>
static void unicast_das_event_msg_handler(const ecs::EntityId &eid, const EventName *msg_)
{
  base_das_event_msg_handler(msg_, [&](ecs::Event &evt) { g_entity_mgr->sendEvent(eid, evt); });
}


template <class EventName, typename CB>
static void base_client_das_event_msg_handler(const EventName *msg, CB send_event)
{
  G_ASSERT(msg);
  invalidate_das_events_gen("client event gen");
  alignas(16) char buf[EVENT_STACK_SIZE];
  const danet::BitStream bs = msg->template get<0>();
  if (bind_dascript::DasEvent *evt = deserialize_das_event(bs, msg, buf, EVENT_STACK_SIZE))
  {
    TRACE_RX_DAS_EVENT_STAT(evt->getType(), bs);
    send_event((ecs::Event &)*evt);
    if (EASTL_UNLIKELY(evt->getFlags() & ecs::EVFLG_DESTROY)) // we have to do it, as it can be that there is immediate strategy.
      g_entity_mgr->getEventsDb().destroy(*evt);
    if ((char *)evt != buf)
      framemem_ptr()->free(evt);
  }
}

template <class EventName>
static void broadcast_client_das_event_msg_handler(const net::IMessage *msg_)
{
  base_client_das_event_msg_handler(msg_->cast<EventName>(), [](ecs::Event &evt) { g_entity_mgr->broadcastEvent(evt); });
}

template <class EventName>
static void unicast_client_das_event_msg_handler(const ecs::EntityId &eid, const EventName *msg_)
{
  base_client_das_event_msg_handler(msg_, [&](ecs::Event &evt) { g_entity_mgr->sendEvent(eid, evt); });
}

template <class EventName, typename CB>
static void base_client_controlled_das_event_msg_handler(const EventName *msg, CB send_event)
{
  G_ASSERT(msg);
  invalidate_das_events_gen("ctrl event gen");
  alignas(16) char buf[EVENT_STACK_SIZE];
  const danet::BitStream bs = msg->template get<0>();
  if (bind_dascript::DasEvent *evt = deserialize_das_event(bs, msg, buf, EVENT_STACK_SIZE))
  {
    TRACE_RX_DAS_EVENT_STAT(evt->getType(), bs);
    send_event((ecs::Event &)*evt);
    if (EASTL_UNLIKELY(evt->getFlags() & ecs::EVFLG_DESTROY)) // we have to do it, as it can be that there is immediate strategy.
      g_entity_mgr->getEventsDb().destroy(*evt);
    if ((char *)evt != buf)
      framemem_ptr()->free(evt);
  }
}

template <class EventName>
static void broadcast_client_controlled_das_event_msg_handler(const net::IMessage *msg_)
{
  base_client_controlled_das_event_msg_handler(msg_->cast<EventName>(), [](ecs::Event &evt) { g_entity_mgr->broadcastEvent(evt); });
}

template <class EventName>
static void unicast_client_controlled_das_event_msg_handler(const ecs::EntityId &eid, const EventName *msg_)
{
  base_client_controlled_das_event_msg_handler(msg_, [&](ecs::Event &evt) { g_entity_mgr->sendEvent(eid, evt); });
}

static void das_event_handshake_msg_handler(const net::IMessage *msg_)
{
  auto msg = msg_->cast<DasEventHandshakeMsg>();
  G_ASSERT(msg);
  const uint32_t clientVersion = msg->get<0>();
  const uint32_t serverVersion = bind_dascript::lock_dasevent_net_version();
  if (clientVersion == serverVersion)
  {
    debug("das_net: client #%d has correct dasevents version protocol 0x%x", (int)msg_->connection->getId(), serverVersion);
  }
  else
  {
    logwarn("das_net: Invalid client #%d dasevents version 0x%x (vs 0x%x)", (int)msg_->connection->getId(), clientVersion,
      serverVersion);
    msg_->connection->disconnect(DC_NET_PROTO_MISMATCH);
  }
}

} // namespace net

ECS_NET_IMPL_MSG(DasEventMsg, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf, RELIABLE_ORDERED, DAS_NET_EVENTS_DEF_NET_CHANNEL,
  net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &net::broadcast_das_event_msg_handler<DasEventMsg>);

ECS_NET_IMPL_MSG(ClientDasEventMsg, net::ROUTING_CLIENT_TO_SERVER, ECS_NET_NO_RCPTF, RELIABLE_ORDERED, DAS_NET_EVENTS_DEF_NET_CHANNEL,
  net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &net::broadcast_client_das_event_msg_handler<ClientDasEventMsg>);

ECS_NET_IMPL_MSG(ClientControlledEntityDasEventMsg, net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER, ECS_NET_NO_RCPTF,
  RELIABLE_ORDERED, DAS_NET_EVENTS_DEF_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP,
  &net::broadcast_client_controlled_das_event_msg_handler<ClientControlledEntityDasEventMsg>);

ECS_NET_IMPL_MSG(DasEventHandshakeMsg, net::ROUTING_CLIENT_TO_SERVER, &net::broadcast_rcptf, RELIABLE_ORDERED,
  DAS_NET_EVENTS_DEF_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &net::das_event_handshake_msg_handler);

ECS_TAG(netClient)
ECS_REQUIRE_NOT(ecs::Tag msg_sink)
static void global_unicast_dasevent_client_es_event_handler(const ecs::EventNetMessage &evt, ecs::EntityId eid)
{
  const net::IMessage *msg = evt.get<0>().get();
  if (auto dasEvent = msg->cast<DasEventMsg>())
  {
    net::unicast_das_event_msg_handler<DasEventMsg>(eid, dasEvent);
    return;
  }
}

ECS_TAG(server, net)
ECS_REQUIRE_NOT(ecs::Tag msg_sink)
static void global_unicast_dasevent_es_event_handler(const ecs::EventNetMessage &evt, ecs::EntityId eid)
{
  const net::IMessage *msg = evt.get<0>().get();
  if (auto clientDasEvent = msg->cast<ClientDasEventMsg>())
  {
    net::unicast_client_das_event_msg_handler(eid, clientDasEvent);
    return;
  }
  if (auto clientControlledDasEvent = msg->cast<ClientControlledEntityDasEventMsg>())
  {
    net::unicast_client_controlled_das_event_msg_handler(eid, clientControlledDasEvent);
    return;
  }
}

ECS_TAG(netClient)
static void invalidate_dasevents_cache_on_connect_to_server_es_event_handler(const EventOnConnectedToServer &)
{
  const uint32_t netVersion = bind_dascript::lock_dasevent_net_version();
  debug("das_net: we just connected to server. Lock dasevents registration. Send net version 0x%X", netVersion);
  net::invalidate_das_events_gen("connected gen");
  DasEventHandshakeMsg evMsg(netVersion);
  send_net_msg(net::get_msg_sink(), eastl::move(evMsg));
}

static void invalidate_dasevents_on_network_destroyed_es_event_handler(const EventOnNetworkDestroyed &)
{
  debug("das_net: we just destroyed network. Invalidate cache, unlock dasevents registration");
  bind_dascript::unlock_dasevent_net_version();
  net::invalidate_das_events_now("network destroy");
  DUMP_DAS_EVENTS_STAT();
}

ECS_TAG(server)
static void invalidate_dasevents_cache_on_client_change_es_event_handler(const EventOnClientConnected &evt)
{
  const int connId = evt.get<0>();
  if (connId != net::INVALID_CONNECTION_ID)
  {
    debug("das_net: new client #%d just connected to server", connId);
    net::invalidate_das_events_for_connection(connId);
  }
}
