// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "dasEventES.cpp.inl"
ECS_DEF_PULL_VAR(dasEvent);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc global_unicast_dasevent_client_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 no components at [1]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void global_unicast_dasevent_client_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventNetMessage>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    global_unicast_dasevent_client_es_event_handler(static_cast<const ecs::EventNetMessage&>(evt)
        , ECS_RO_COMP(global_unicast_dasevent_client_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc global_unicast_dasevent_client_es_event_handler_es_desc
(
  "global_unicast_dasevent_client_es",
  "prog/gameLibs/daECS/net/dasevents/dasEventES.cpp.inl",
  ecs::EntitySystemOps(nullptr, global_unicast_dasevent_client_es_event_handler_all_events),
  empty_span(),
  make_span(global_unicast_dasevent_client_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  make_span(global_unicast_dasevent_client_es_event_handler_comps+1, 1)/*no*/,
  ecs::EventSetBuilder<ecs::EventNetMessage>::build(),
  0
,"netClient");
static constexpr ecs::ComponentDesc global_unicast_dasevent_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 no components at [1]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void global_unicast_dasevent_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventNetMessage>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    global_unicast_dasevent_es_event_handler(static_cast<const ecs::EventNetMessage&>(evt)
        , ECS_RO_COMP(global_unicast_dasevent_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc global_unicast_dasevent_es_event_handler_es_desc
(
  "global_unicast_dasevent_es",
  "prog/gameLibs/daECS/net/dasevents/dasEventES.cpp.inl",
  ecs::EntitySystemOps(nullptr, global_unicast_dasevent_es_event_handler_all_events),
  empty_span(),
  make_span(global_unicast_dasevent_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  make_span(global_unicast_dasevent_es_event_handler_comps+1, 1)/*no*/,
  ecs::EventSetBuilder<ecs::EventNetMessage>::build(),
  0
,"net,server");
//static constexpr ecs::ComponentDesc invalidate_dasevents_cache_on_connect_to_server_es_event_handler_comps[] ={};
static void invalidate_dasevents_cache_on_connect_to_server_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnConnectedToServer>());
  invalidate_dasevents_cache_on_connect_to_server_es_event_handler(static_cast<const EventOnConnectedToServer&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc invalidate_dasevents_cache_on_connect_to_server_es_event_handler_es_desc
(
  "invalidate_dasevents_cache_on_connect_to_server_es",
  "prog/gameLibs/daECS/net/dasevents/dasEventES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_dasevents_cache_on_connect_to_server_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnConnectedToServer>::build(),
  0
,"netClient");
//static constexpr ecs::ComponentDesc invalidate_dasevents_on_network_destroyed_es_event_handler_comps[] ={};
static void invalidate_dasevents_on_network_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnNetworkDestroyed>());
  invalidate_dasevents_on_network_destroyed_es_event_handler(static_cast<const EventOnNetworkDestroyed&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc invalidate_dasevents_on_network_destroyed_es_event_handler_es_desc
(
  "invalidate_dasevents_on_network_destroyed_es",
  "prog/gameLibs/daECS/net/dasevents/dasEventES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_dasevents_on_network_destroyed_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnNetworkDestroyed>::build(),
  0
);
//static constexpr ecs::ComponentDesc invalidate_dasevents_cache_on_client_change_es_event_handler_comps[] ={};
static void invalidate_dasevents_cache_on_client_change_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnClientConnected>());
  invalidate_dasevents_cache_on_client_change_es_event_handler(static_cast<const EventOnClientConnected&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc invalidate_dasevents_cache_on_client_change_es_event_handler_es_desc
(
  "invalidate_dasevents_cache_on_client_change_es",
  "prog/gameLibs/daECS/net/dasevents/dasEventES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_dasevents_cache_on_client_change_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnClientConnected>::build(),
  0
,"server");
