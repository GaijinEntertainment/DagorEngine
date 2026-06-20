// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/msgSink.h>
#include <daECS/net/msgDispatch.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/net/message.h>

namespace net
{

ecs::EntityId msg_sink_eid = ecs::INVALID_ENTITY_ID;

msg_sink_created_cb_t on_msg_sink_created_cb;
void set_msg_sink_created_cb(msg_sink_created_cb_t cb) { on_msg_sink_created_cb = eastl::move(cb); }

ecs::EntityId get_msg_sink() { return msg_sink_eid; }

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
static inline void msg_sink_es_event_handler(const ecs::EventNetMessage &evt) { dispatch_net_msg_handler(evt.get<0>().get()); }

} // namespace net
