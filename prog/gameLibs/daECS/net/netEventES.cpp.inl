#include <daECS/net/netEvent.h>
#include <daECS/net/object.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <EASTL/vector_map.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/bitvector.h>
#include <memory/dag_framemem.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/network.h>

ECS_REGISTER_EVENT_NS(ecs, EventNetMessage);
ECS_UNICAST_EVENT_TYPE(EventNetEventDummy); // Dummy event type to ensure that EventSet is not empty
ECS_REGISTER_EVENT(EventNetEventDummy);     // Dummy event type to ensure that EventSet is not empty

inline void entity_events(const ecs::Event &event, ecs::EntityId eid)
{
  eastl::unique_ptr<net::IMessage, framememDeleter> msg(net::event::create_message_by_event(event, net::Er::Unicast, framemem_ptr()));
  if (msg)
    send_net_msg(eid, eastl::move(*msg));
}

inline void broadcast_events(const ecs::Event &event)
{
  eastl::unique_ptr<net::IMessage, framememDeleter> msg(
    net::event::create_message_by_event(event, net::Er::Broadcast, framemem_ptr()));
  if (msg)
    send_net_msg(net::get_msg_sink(), eastl::move(*msg));
}

ECS_ON_EVENT(EventNetEventDummy) // we actually won't receive this event
inline void net_unicast_events_es_event_handler(const ecs::Event &event, ecs::EntityId eid) { entity_events(event, eid); }
ECS_ON_EVENT(EventNetEventDummy) // we actually won't receive this event
inline void net_broadcast_events_es_event_handler(const ecs::Event &event) { broadcast_events(event); }

static ecs::EventSet build_event_set_for_tx_routing(dag::ConstSpan<net::MessageRouting> tx_routings, net::Er er)
{
  ecs::EventSet evs;
  auto collectEvs = [&tx_routings, &evs, er](const net::EventRegRecord &err) {
    if (find_value_idx(tx_routings, err.msgClass.routing) >= 0 && err.er == er)
      evs.insert(err.eventType);
  };
  net::EventRegRecord::mapRecords(collectEvs);
  return evs;
}

static void net_unicast_events_es_event_handler_all_events(const ecs::Event &__restrict, const ecs::QueryView &__restrict);
static void net_broadcast_events_es_event_handler_all_events(const ecs::Event &__restrict, const ecs::QueryView &__restrict);

static void update_esd_event_set(dag::ConstSpan<net::MessageRouting> tx_routings)
{
  ecs::iterate_systems([&tx_routings](ecs::EntitySystemDesc *esd) {
    auto evtfn = esd->getOps().onEvent;
    if (evtfn == &net_unicast_events_es_event_handler_all_events)
    {
      ecs::EventSet evs = build_event_set_for_tx_routing(tx_routings, net::Er::Unicast);
      esd->setEvSet(evs.empty() ? ecs::EventSetBuilder<EventNetEventDummy>::build() : eastl::move(evs));
    }
    else if (evtfn == &net_broadcast_events_es_event_handler_all_events)
    {
      ecs::EventSet evs = build_event_set_for_tx_routing(tx_routings, net::Er::Broadcast);
      esd->setEvSet(evs.empty() ? ecs::EventSetBuilder<EventNetEventDummy>::build() : eastl::move(evs));
    }
  });
}
namespace ecs
{
extern void reset_es_order();
};
namespace net
{
/*static*/ EventRegRecord *EventRegRecord::netEventsRegList = nullptr;
// This is just cache of content EventRegRecord registry (for faster access)
static eastl::vector_map<ecs::event_type_t, const EventRegRecord *> tx_events_index;
static eastl::bitvector<> rx_msg_bitmap;                   // for faster negative tests
static eastl::vector<const EventRegRecord *> rx_msg_index; // To consider: use sorted array instead?

static void do_init_indexes(dag::ConstSpan<MessageRouting> tx_routings)
{
  tx_events_index.clear();
  rx_msg_bitmap.clear();
  rx_msg_index.clear();
  auto collectIndexes = [&tx_routings](const EventRegRecord &err) {
    if (find_value_idx(tx_routings, err.msgClass.routing) >= 0)
      tx_events_index.insert(decltype(tx_events_index)::value_type(err.eventType, &err));
    else
    {
      G_ASSERTF(err.msgClass.classId >= 0, "MessageClass::init() wasn't called?");
      rx_msg_bitmap.set(err.msgClass.classId, true);
      if (err.msgClass.classId >= rx_msg_index.size())
        rx_msg_index.resize(err.msgClass.classId + 1);
      rx_msg_index[err.msgClass.classId] = &err;
    }
  };
  EventRegRecord::mapRecords(collectIndexes);
}

namespace event
{

void init_server()
{
  MessageRouting srvr = ROUTING_SERVER_TO_CLIENT;
  do_init_indexes(make_span_const(&srvr, 1));
  update_esd_event_set(make_span_const(&srvr, 1));
  ecs::reset_es_order();
}

void init_client()
{
  MessageRouting clr[] = {ROUTING_CLIENT_TO_SERVER, ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER};
  do_init_indexes(make_span_const(clr));
  update_esd_event_set(make_span_const(clr));
  ecs::reset_es_order();
}

void shutdown()
{
  tx_events_index.clear();
  rx_msg_bitmap.clear();
  rx_msg_index.clear();
}

bool try_receive(const net::IMessage &msg, ecs::EntityId toeid)
{
  const MessageClass &msgClass = msg.getMsgClass();
  if (!rx_msg_bitmap.test(msgClass.classId, false))
    return false;
  const EventRegRecord *err = rx_msg_index[msgClass.classId];
  G_FAST_ASSERT(err);
  err->msg2evt((err->er == net::Er::Broadcast) ? ecs::INVALID_ENTITY_ID : toeid, msg);
  return true;
}

IMessage *create_message_by_event(const ecs::Event &evt, net::Er er, IMemAlloc *alloc)
{
  G_UNUSED(er);
  auto it = tx_events_index.find(evt.getType());
  if (it == tx_events_index.end())
    return nullptr;
  G_ASSERTF(it->second->er == er, "Message %s/%x was registered as broadcasted but sent as unicast (or vice versa)", evt.getName(),
    evt.getType());
  G_FAST_ASSERT(it->second->evt2msg);
  return it->second->evt2msg(evt, alloc);
}

} // namespace event
} // namespace net
