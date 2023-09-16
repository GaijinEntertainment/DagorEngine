//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/entityManager.h>
#include <daECS/net/msgDecl.h>

namespace net
{

typedef net::IMessage *(*event2msg_t)(const ecs::Event &evt, IMemAlloc *alloc);
typedef void (*msg2event_t)(ecs::EntityId eid, const net::IMessage &msg);

enum class Er // abbreviation from "Event Routing"
{
  Unicast,  // Sent to specific entity (i.e. via sendEvent)
  Broadcast // Sent to all entities (i.e. broadcastEvent)
};

struct EventRegRecord
{
  ecs::event_type_t eventType;
  Er er;
  event2msg_t evt2msg;
  msg2event_t msg2evt;
  const MessageClass &msgClass;

  EventRegRecord *next;
  static EventRegRecord *netEventsRegList;

  EventRegRecord(ecs::event_type_t etype, const MessageClass &mcls, Er er_, event2msg_t evt2msg_, msg2event_t msg2evt_) :
    eventType(etype), er(er_), msgClass(mcls), evt2msg(evt2msg_), msg2evt(msg2evt_)
  {
    next = netEventsRegList;
    netEventsRegList = this; // register itself
  }

  template <typename F>
  static void mapRecords(const F &fun)
  {
    for (const EventRegRecord *err = EventRegRecord::netEventsRegList; err; err = err->next)
      fun(*err);
  }
};

template <typename Evt, typename Msg, Er EvtRt>
struct EventRegRecordT : public EventRegRecord
{
  static IMessage *event2msg(const ecs::Event &evt, IMemAlloc *alloc)
  {
    const Evt *event = evt.cast<Evt>();
    G_ASSERT(event);
    return new (alloc) Msg(typename Evt::TupleType(*event));
  }
  static void msg2event(ecs::EntityId eid, const net::IMessage &message)
  {
    const Msg *msg = message.cast<Msg>();
    G_ASSERT(msg);
    Evt evt(typename Evt::TupleType(*static_cast<const typename Msg::Tuple *>(msg)));
    G_FAST_ASSERT(((EvtRt == net::Er::Unicast) && eid) || ((EvtRt != net::Er::Unicast) && !eid));
    g_entity_mgr->dispatchEvent(eid, eastl::move(evt));
  }
  EventRegRecordT(const MessageClass &mcls) : EventRegRecord(Evt::staticType(), mcls, EvtRt, event2msg, msg2event) {}
};

namespace event
{
void init_server();
void init_client();
void shutdown();
bool try_receive(const net::IMessage &msg, ecs::EntityId toeid); // return false if message wasn't event one
IMessage *create_message_by_event(const ecs::Event &evt, net::Er er, IMemAlloc *alloc);
}; // namespace event

} // namespace net

#define ECS_REGISTER_NET_EVENT(Klass, er, rout, ...)                   \
  ECS_NET_DECL_MSG_EX(Klass##NetMsg, net::IMessage, Klass::TupleType); \
  ECS_NET_IMPL_MSG(Klass##NetMsg, rout, ##__VA_ARGS__);                \
  static net::EventRegRecordT<Klass, Klass##NetMsg, er> Klass##_net_record(Klass##NetMsg::messageClass)
