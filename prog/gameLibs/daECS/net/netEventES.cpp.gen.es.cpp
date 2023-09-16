#include "netEventES.cpp.inl"
ECS_DEF_PULL_VAR(netEvent);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc net_unicast_events_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void net_unicast_events_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    net_unicast_events_es_event_handler(evt
        , ECS_RO_COMP(net_unicast_events_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc net_unicast_events_es_event_handler_es_desc
(
  "net_unicast_events_es",
  "prog/gameLibs/daECS/net/netEventES.cpp.inl",
  ecs::EntitySystemOps(nullptr, net_unicast_events_es_event_handler_all_events),
  empty_span(),
  make_span(net_unicast_events_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventNetEventDummy>::build(),
  0
);
//static constexpr ecs::ComponentDesc net_broadcast_events_es_event_handler_comps[] ={};
static void net_broadcast_events_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  net_broadcast_events_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc net_broadcast_events_es_event_handler_es_desc
(
  "net_broadcast_events_es",
  "prog/gameLibs/daECS/net/netEventES.cpp.inl",
  ecs::EntitySystemOps(nullptr, net_broadcast_events_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventNetEventDummy>::build(),
  0
);
