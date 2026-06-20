// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/netEvent.h>
#include <daECS/net/object.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <EASTL/vector_map.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/bitvector.h>
#include <memory/dag_framemem.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/network.h>
#include <osApiWrappers/dag_miscApi.h>

ECS_REGISTER_EVENT_NS(ecs, EventNetMessage);
ECS_UNICAST_EVENT_TYPE(EventNetEventDummy); // Dummy event type to ensure that EventSet is not empty
ECS_REGISTER_EVENT(EventNetEventDummy);     // Dummy event type to ensure that EventSet is not empty

inline void entity_events(ecs::EntityManager &mgr, const ecs::Event &event, ecs::EntityId eid)
{
  eastl::unique_ptr<net::IMessage, framememDeleter> msg(net::event::create_message_by_event(event, net::Er::Unicast, framemem_ptr()));
  if (msg)
    send_net_msg(mgr, eid, eastl::move(*msg));
}

inline void broadcast_events(ecs::EntityManager &mgr, const ecs::Event &event)
{
  eastl::unique_ptr<net::IMessage, framememDeleter> msg(
    net::event::create_message_by_event(event, net::Er::Broadcast, framemem_ptr()));
  if (msg)
    send_net_msg(mgr, net::get_msg_sink(), eastl::move(*msg));
}

ECS_ON_EVENT(EventNetEventDummy) // we actually won't receive this event
inline void net_unicast_events_es_event_handler(const ecs::Event &event, ecs::EntityManager &manager, ecs::EntityId eid)
{
  entity_events(manager, event, eid);
}
ECS_ON_EVENT(EventNetEventDummy) // we actually won't receive this event
inline void net_broadcast_events_es_event_handler(const ecs::Event &event, ecs::EntityManager &manager)
{
  broadcast_events(manager, event);
}

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

template <typename F>
static void for_each_net_tx_handler(F fn)
{
  ecs::iterate_systems([&fn](ecs::EntitySystemDesc *esd) {
    const auto evtfn = esd->getOps().onEvent;
    const bool unicast = evtfn == &net_unicast_events_es_event_handler_all_events;
    if (unicast || evtfn == &net_broadcast_events_es_event_handler_all_events)
      fn(esd, unicast);
  });
}

static void make_net_tx_handler_inert(ecs::EntitySystemDesc *esd)
{
  esd->setOwner(nullptr);
  esd->setEvSet(ecs::EventSetBuilder<EventNetEventDummy>::build());
}

static void claim_for(ecs::EntityManager *mgr, dag::ConstSpan<MessageRouting> tx_routings)
{
  const int64_t dbgTid = get_current_thread_id();
  debug("[net::event::claim_for] tid=%lld mgr=%p tx_routings.size=%u", dbgTid, mgr, (uint32_t)tx_routings.size());
  do_init_indexes(tx_routings);
  bool didRebuild = false;
  {
    ecs::EsListLock::ScopedLock lock;
    int unicastCount = 0;
    int broadcastCount = 0;
    for_each_net_tx_handler([mgr, tx_routings, &unicastCount, &broadcastCount](ecs::EntitySystemDesc *esd, bool unicast) {
      ecs::EventSet evs = build_event_set_for_tx_routing(tx_routings, unicast ? net::Er::Unicast : net::Er::Broadcast);
      esd->setEvSet(evs.empty() ? ecs::EventSetBuilder<EventNetEventDummy>::build() : eastl::move(evs));
      esd->setOwner(mgr);
      (unicast ? unicastCount : broadcastCount)++;
    });
    debug("[net::event::claim_for] tid=%lld mgr=%p claimed unicast=%d broadcast=%d", dbgTid, mgr, unicastCount, broadcastCount);
    if (mgr)
      didRebuild = mgr->resetEsOrderLocked();
  }
  if (didRebuild && mgr)
    mgr->broadcastEventImmediate(ecs::EventEntityManagerEsOrderSet());
  debug("[net::event::claim_for] tid=%lld mgr=%p DONE", dbgTid, mgr);
}

void init_server(ecs::EntityManager *mgr)
{
  debug("[net::event::init_server] tid=%lld mgr=%p", get_current_thread_id(), mgr);
  MessageRouting srvr = ROUTING_SERVER_TO_CLIENT;
  claim_for(mgr, make_span_const(&srvr, 1));
}

void init_client(ecs::EntityManager *mgr)
{
  debug("[net::event::init_client] tid=%lld mgr=%p", get_current_thread_id(), mgr);
  MessageRouting clr[] = {ROUTING_CLIENT_TO_SERVER, ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER};
  claim_for(mgr, make_span_const(clr));
}

void shutdown()
{
  const int64_t dbgTid = get_current_thread_id();
  debug("[net::event::shutdown] tid=%lld ENTER", dbgTid);
  ecs::EsListLock::ScopedLock lock;
  int releasedCount = 0;
  for_each_net_tx_handler([&releasedCount](ecs::EntitySystemDesc *esd, bool) {
    make_net_tx_handler_inert(esd);
    releasedCount++;
  });
  tx_events_index.clear();
  rx_msg_bitmap.clear();
  rx_msg_index.clear();
  debug("[net::event::shutdown] tid=%lld DONE releasedHandlers=%d", dbgTid, releasedCount);
}

void release_claim(ecs::EntityManager *mgr)
{
  if (!mgr)
    return;
  const int64_t dbgTid = get_current_thread_id();
  ecs::EsListLock::ScopedLock lock;
  int releasedCount = 0;
  for_each_net_tx_handler([mgr, &releasedCount](ecs::EntitySystemDesc *esd, bool) {
    if (esd->getOwner() != mgr)
      return;
    make_net_tx_handler_inert(esd);
    releasedCount++;
  });
  debug("[net::event::release_claim] tid=%lld mgr=%p released=%d", dbgTid, mgr, releasedCount);
}

bool try_receive(const net::IMessage &msg, ecs::EntityManager &mgr, ecs::EntityId toeid)
{
  const MessageClass &msgClass = msg.getMsgClass();
  if (!rx_msg_bitmap.test(msgClass.classId, false))
    return false;
  const EventRegRecord *err = rx_msg_index[msgClass.classId];
  G_FAST_ASSERT(err);
  err->msg2evt(mgr, (err->er == net::Er::Broadcast) ? ecs::INVALID_ENTITY_ID : toeid, msg);
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
