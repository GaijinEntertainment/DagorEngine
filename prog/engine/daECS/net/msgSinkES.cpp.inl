// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/msgSink.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/net/message.h>
#include <ska_hash_map/flat_hash_map2.hpp>

namespace net
{

ecs::EntityId msg_sink_eid = ecs::INVALID_ENTITY_ID;
static ska::flat_hash_map<uint32_t, net::msg_handler_t, ska::power_of_two_std_hash<uint32_t>> msg_handlers;

msg_sink_created_cb_t on_msg_sink_created_cb;
void set_msg_sink_created_cb(msg_sink_created_cb_t cb) { on_msg_sink_created_cb = eastl::move(cb); }

ecs::EntityId get_msg_sink() { return msg_sink_eid; }

void clear_net_msg_handlers() { decltype(msg_handlers)().swap(msg_handlers); }

void register_net_msg_handler(const net::MessageClass &klass, net::msg_handler_t handler)
{
  G_ASSERT(handler);
  auto ins = msg_handlers.emplace(typename decltype(msg_handlers)::value_type(klass.classHash, handler));
  G_ASSERTF(ins.second || (ins.first->second == handler), "Duplicate net msg handler for message class %s/%x", klass.debugClassName,
    klass.classHash);
  ins.first->second = handler;
}

ECS_AUTO_REGISTER_COMPONENT(ecs::Tag, "msg_sink", nullptr, 0);

ECS_REQUIRE(ecs::Tag msg_sink)
static inline void msg_sink_es_event_handler(const ecs::EventEntityCreated &, ecs::EntityId eid)
{
  G_ASSERT(!msg_sink_eid);
  msg_sink_eid = eid;
  if (on_msg_sink_created_cb)
    on_msg_sink_created_cb(eid);
}

ECS_REQUIRE(ecs::Tag msg_sink)
static inline void msg_sink_es_event_handler(const ecs::EventEntityDestroyed &)
{
  G_ASSERT(msg_sink_eid);
  msg_sink_eid.reset();
}

ECS_REQUIRE(ecs::Tag msg_sink)
static inline void msg_sink_es_event_handler(const ecs::EventNetMessage &evt)
{
  const net::IMessage *msg = evt.get<0>().get();
  const net::MessageClass &mcls = msg->getMsgClass();
  auto it = msg_handlers.find(mcls.classHash);
  if (it != msg_handlers.end())
    it->second(msg);
}

} // namespace net
