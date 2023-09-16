#include <ecs/scripts/netsqevent.h>
#include <sqModules/sqModules.h>
#include <daECS/net/schemelessEventSerialize.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/netbase.h>
#include <sqrat.h>
#include <ecs/core/entityManager.h>
#include <daECS/net/msgDecl.h>
#include <daNet/bitStream.h>

static constexpr int NET_SQ_EVENTS_DEFAULT_NET_CHANNEL = 0; // Note: this might be unresolved link time dependency if apps really need
                                                            // to configure that

// TODO: unreliable events
ECS_NET_DECL_MSG(SQEventMsg, bool /*bcast*/, ecs::EntityId /*to_eid*/, danet::BitStream /*payload*/);
ECS_NET_DECL_MSG(ClientSQEventMsg, bool /*bcast*/, ecs::EntityId /*to_eid*/, danet::BitStream /*payload*/);

template <typename F>
static inline void for_each_conn_do(const Sqrat::Object &to_whom, const F &func)
{
  auto ot = to_whom.GetType();
  if (ot == OT_ARRAY)
  {
    Sqrat::Array arr(to_whom);
    for (SQInteger i = 0, sz = arr.Length(); i < sz; ++i)
    {
      HSQOBJECT slot = arr.RawGetSlot(i).GetObject();
      int connId = sq_isinteger(slot) ? (int)slot._unVal.nInteger : -1;
      if (net::IConnection *conn = (connId >= 0) ? get_client_connection(connId) : nullptr)
        func(*conn);
    }
  }
  else
    G_ASSERTF(0, "Type %d/%x is not supported", (int)ot, (int)ot);
}

template <typename F>
static inline void for_each_conn_do(dag::Span<net::IConnection *> *to_whom, const F &func)
{
  for (auto conn : *to_whom)
    if (conn)
      func(*conn);
}

static inline bool is_null_array(const dag::Span<net::IConnection *> *arr) { return arr == nullptr; }
static inline bool is_null_array(const dag::ConstSpan<net::IConnection *> *arr) { return arr == nullptr; }
static inline bool is_null_array(const Sqrat::Object &arr) { return arr.IsNull(); }

template <typename T>
static void server_send_sqevent(ecs::EntityId to_eid, const ecs::SchemelessEvent &evt, T to_whom)
{
  bool bcast = !to_eid;
  SQEventMsg evMsg(bcast, to_eid, danet::BitStream{});
  ecs::serialize_to(evt, evMsg.get<2>());
  if (is_null_array(to_whom)) // broadcast to all connections
    send_net_msg(net::get_msg_sink(), eastl::move(evMsg));
  else
  {
    net::MessageNetDesc md = evMsg.getMsgClass();
    md.rcptFilter = &net::direct_connection_rcptf;
    for_each_conn_do(to_whom, [&](net::IConnection &conn) {
      evMsg.connection = &conn;
      // Note: yes, move is called within loop, it's ugly but it works (in network)
      send_net_msg(net::get_msg_sink(), eastl::move(evMsg), &md);
    });
  }
}

template <typename T>
static void do_send_sqevent_server(ecs::EntityId to_eid, ecs::SchemelessEvent &evt, T to_whom)
{
  server_send_sqevent(to_eid, evt, to_whom);
  g_entity_mgr->dispatchEvent(to_eid, eastl::move(evt));
}

void client_send_net_sqevent(ecs::EntityId to_eid, const ecs::SchemelessEvent &evt, bool bcastevt)
{
  danet::BitStream bs;
  ecs::serialize_to(evt, bs); // assume single player if there is no server connection
  send_net_msg(net::get_msg_sink(), ClientSQEventMsg(bcastevt, to_eid, eastl::move(bs)));
}

void server_send_net_sqevent(ecs::EntityId to_eid, ecs::SchemelessEvent &&evt, dag::Span<net::IConnection *> *to_whom)
{
  if (to_eid && is_server())
    do_send_sqevent_server(to_eid, evt, to_whom);
}
void server_broadcast_net_sqevent(ecs::SchemelessEvent &&evt, dag::Span<net::IConnection *> *to_whom)
{
  if (is_server())
    do_send_sqevent_server(ecs::INVALID_ENTITY_ID, evt, to_whom);
}

static SQInteger server_send_net_sqevent_sq(HSQUIRRELVM vm) // (ecs::EntityId to_eid, ecs::SQEvent &evt, (Sqrat::Array<int>|null)
                                                            // to_whom = null)
{
  if (!Sqrat::check_signature<ecs::entity_id_t, ecs::SchemelessEvent *>(vm, 2))
    return SQ_ERROR;

  SQInteger nargs = sq_gettop(vm);
  ecs::EntityId to_eid(Sqrat::Var<ecs::entity_id_t>(vm, 2).value);
  Sqrat::Var<ecs::SchemelessEvent *> evt(vm, 3);
  G_ASSERT_RETURN(evt.value, 0);
  Sqrat::Object toWhom(vm);
  if (nargs > 3)
    toWhom = Sqrat::Var<Sqrat::Object>(vm, 4).value;
  if (to_eid && is_server())
    do_send_sqevent_server(to_eid, *evt.value, toWhom);
  return 0;
}

static SQInteger server_broadcast_net_sqevent_sq(HSQUIRRELVM vm) // (ecs::SQEvent &evt, (Sqrat::Array<int>|null) to_whom = null)
{
  if (is_server())
  {
    if (!Sqrat::check_signature<ecs::SchemelessEvent *>(vm, 2))
      return SQ_ERROR;

    SQInteger nargs = sq_gettop(vm);
    Sqrat::Var<ecs::SchemelessEvent *> evt(vm, 2);
    G_ASSERT_RETURN(evt.value, 0);
    Sqrat::Object toWhom(vm);
    if (nargs > 2)
      toWhom = Sqrat::Var<Sqrat::Object>(vm, 3).value;
    do_send_sqevent_server(ecs::INVALID_ENTITY_ID, *evt.value, toWhom);
  }
  return 0;
}

static SQInteger client_request_unicast_net_sqevent_sq(HSQUIRRELVM vm) // (ecs::EntityId to_eid, ecs::SQEvent &evt)
{
  if (!Sqrat::check_signature<ecs::entity_id_t, ecs::SchemelessEvent *>(vm, 2))
    return SQ_ERROR;

  ecs::EntityId to_eid(Sqrat::Var<ecs::entity_id_t>(vm, 2).value);
  Sqrat::Var<ecs::SchemelessEvent *> evt(vm, 3);
  G_ASSERT_RETURN(evt.value, 0);
  client_send_net_sqevent(to_eid, *evt.value);
  return 0;
}

static SQInteger client_request_broadcast_net_sqevent_sq(HSQUIRRELVM vm) // (ecs::SQEvent &evt)
{
  if (!Sqrat::check_signature<ecs::SchemelessEvent *>(vm, 2))
    return SQ_ERROR;

  Sqrat::Var<ecs::SchemelessEvent *> evt(vm, 2);
  G_ASSERT_RETURN(evt.value, 0);
  client_send_net_sqevent(ecs::INVALID_ENTITY_ID, *evt.value, /*bcastevt*/ true);
  return 0;
}


template <typename T>
static void sq_event_msg_handler(const net::IMessage *msg_)
{
  net::IConnection *conn = msg_->connection;
  auto msg = msg_->cast<T>();
  G_ASSERT(msg);
  if (ecs::MaybeSchemelessEvent maybeEvent = ecs::deserialize_from(msg->template get<2>()))
  {
    // To consider: add non-payload field to native SchemelessEvent instead?
    maybeEvent->getRWData().addMember(ECS_HASH("fromconnid"), conn ? (int)conn->getId() : -1);
    if (msg->template get<0>()) // bcast
      g_entity_mgr->broadcastEvent(eastl::move(eastl::move(maybeEvent).value()));
    else
    {
      if (ecs::EntityId to_eid = msg->template get<1>())
        g_entity_mgr->sendEvent(to_eid, eastl::move(eastl::move(maybeEvent).value()));
      else
        logwarn("failed to resolve destination entity for SQEvent '%s'", maybeEvent.value().getName());
    }
  }
  else
    logerr("Failed to parse SQEventMsg from conn #%d", conn ? (int)conn->getId() : -1);
}

ECS_NET_IMPL_MSG(SQEventMsg, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf, RELIABLE_ORDERED, NET_SQ_EVENTS_DEFAULT_NET_CHANNEL,
  net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &sq_event_msg_handler<SQEventMsg>);
ECS_NET_IMPL_MSG(ClientSQEventMsg, net::ROUTING_CLIENT_TO_SERVER, ECS_NET_NO_RCPTF, RELIABLE_ORDERED,
  NET_SQ_EVENTS_DEFAULT_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &sq_event_msg_handler<ClientSQEventMsg>);

void register_net_sqevent(SqModules *module_mgr)
{
  Sqrat::Table exports(module_mgr->getVM());
  ///@module ecs.netevent
  exports.SquirrelFunc("server_send_net_sqevent", server_send_net_sqevent_sq, -3, ".ixa|o")
    .SquirrelFunc("server_broadcast_net_sqevent", server_broadcast_net_sqevent_sq, -2, ".xa|o")
    .SquirrelFunc("client_request_unicast_net_sqevent", client_request_unicast_net_sqevent_sq, -3, ".ix|o")
    .SquirrelFunc("client_request_broadcast_net_sqevent", client_request_broadcast_net_sqevent_sq, -2, ".x|o");
  module_mgr->addNativeModule("ecs.netevent", exports);
}
