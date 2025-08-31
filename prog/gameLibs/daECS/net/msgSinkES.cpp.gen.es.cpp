// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "msgSinkES.cpp.inl"
ECS_DEF_PULL_VAR(msgSink);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc msg_sink_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void msg_sink_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<ecs::EventNetMessage>()) {
    net::msg_sink_es_event_handler(static_cast<const ecs::EventNetMessage&>(evt)
            );
} else if (evt.is<ecs::EventEntityDestroyed>()) {
    net::msg_sink_es_event_handler(static_cast<const ecs::EventEntityDestroyed&>(evt)
            );
} else if (evt.is<ecs::EventEntityCreated>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      net::msg_sink_es_event_handler(static_cast<const ecs::EventEntityCreated&>(evt)
            , ECS_RO_COMP(msg_sink_es_event_handler_comps, "eid", ecs::EntityId)
      );
    while (++comp != compE);
    } else {G_ASSERTF(0, "Unexpected event type <%s> in msg_sink_es_event_handler", evt.getName());}
}
static ecs::EntitySystemDesc msg_sink_es_event_handler_es_desc
(
  "msg_sink_es",
  "prog/gameLibs/daECS/net/msgSinkES.cpp.inl",
  ecs::EntitySystemOps(nullptr, msg_sink_es_event_handler_all_events),
  empty_span(),
  make_span(msg_sink_es_event_handler_comps+0, 1)/*ro*/,
  make_span(msg_sink_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventEntityDestroyed,
                       ecs::EventNetMessage>::build(),
  0
);
