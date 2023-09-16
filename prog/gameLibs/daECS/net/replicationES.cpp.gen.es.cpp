#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t replication_get_type();
static ecs::LTComponentList replication_component(ECS_HASH("replication"), replication_get_type(), "prog/gameLibs/daECS/net/replicationES.cpp.inl", "", 0);
#include "replicationES.cpp.inl"
ECS_DEF_PULL_VAR(replication);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc reset_replication_es_event_handler_comps[] ={};
static void reset_replication_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  net::reset_replication_es_event_handler(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc reset_replication_es_event_handler_es_desc
(
  "reset_replication_es",
  "prog/gameLibs/daECS/net/replicationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_replication_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerBeforeClear,
                       ecs::EventEntityManagerEsOrderSet>::build(),
  0
,"net,server");
//static constexpr ecs::ComponentDesc client_validation_es_event_handler_comps[] ={};
static void client_validation_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityManagerEsOrderSet>());
  net::client_validation_es_event_handler(static_cast<const ecs::EventEntityManagerEsOrderSet&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc client_validation_es_event_handler_es_desc
(
  "client_validation_es",
  "prog/gameLibs/daECS/net/replicationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, client_validation_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerEsOrderSet>::build(),
  0
,"dev,netClient",nullptr,nullptr,"reset_replication_es");
//static constexpr ecs::ComponentDesc server_replication_es_event_handler_comps[] ={};
static void server_replication_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityManagerEsOrderSet>());
  net::server_replication_es_event_handler(static_cast<const ecs::EventEntityManagerEsOrderSet&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc server_replication_es_event_handler_es_desc
(
  "server_replication_es",
  "prog/gameLibs/daECS/net/replicationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, server_replication_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerEsOrderSet>::build(),
  0
,"net,server",nullptr,nullptr,"reset_replication_es");
static constexpr ecs::ComponentDesc replication_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<net::Object>()}
};
static void replication_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityRecreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    net::replication_es_event_handler(static_cast<const ecs::EventEntityRecreated&>(evt)
        , ECS_RW_COMP(replication_es_event_handler_comps, "replication", net::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc replication_es_event_handler_es_desc
(
  "replication_es",
  "prog/gameLibs/daECS/net/replicationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, replication_es_event_handler_all_events),
  make_span(replication_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityRecreated>::build(),
  0
,"net,server");
static constexpr ecs::ComponentDesc replication_validation_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<net::Object>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void replication_validation_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<ecs::EventEntityDestroyed>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      net::replication_validation_es_event_handler(static_cast<const ecs::EventEntityDestroyed&>(evt)
            , ECS_RO_COMP(replication_validation_es_event_handler_comps, "eid", ecs::EntityId)
      );
    while (++comp != compE);
  } else {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      net::replication_validation_es_event_handler(evt
            , ECS_RW_COMP(replication_validation_es_event_handler_comps, "replication", net::Object)
      );
    while (++comp != compE);
  }
}
static ecs::EntitySystemDesc replication_validation_es_event_handler_es_desc
(
  "replication_validation_es",
  "prog/gameLibs/daECS/net/replicationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, replication_validation_es_event_handler_all_events),
  make_span(replication_validation_es_event_handler_comps+0, 1)/*rw*/,
  make_span(replication_validation_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"dev,netClient");
//static constexpr ecs::ComponentDesc replication_destruction_es_event_handler_comps[] ={};
static void replication_destruction_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
if (evt.is<ecs::EventEntityManagerAfterClear>()) {
    net::replication_destruction_es_event_handler(static_cast<const ecs::EventEntityManagerAfterClear&>(evt)
            );
} else if (evt.is<ecs::EventEntityManagerBeforeClear>()) {
    net::replication_destruction_es_event_handler(static_cast<const ecs::EventEntityManagerBeforeClear&>(evt)
            );
  } else {G_ASSERTF(0, "Unexpected event type <%s> in replication_destruction_es_event_handler", evt.getName());}
}
static ecs::EntitySystemDesc replication_destruction_es_event_handler_es_desc
(
  "replication_destruction_es",
  "prog/gameLibs/daECS/net/replicationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, replication_destruction_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerBeforeClear,
                       ecs::EventEntityManagerAfterClear>::build(),
  0
,"netClient");
static constexpr ecs::ComponentDesc replication_destruction_logerr_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<net::Object>()},
//start of 1 no components at [2]
  {ECS_HASH("client__canDestroyServerEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void replication_destruction_logerr_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    net::replication_destruction_logerr_es_event_handler(static_cast<const ecs::EventEntityDestroyed&>(evt)
        , ECS_RO_COMP(replication_destruction_logerr_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(replication_destruction_logerr_es_event_handler_comps, "replication", net::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc replication_destruction_logerr_es_event_handler_es_desc
(
  "replication_destruction_logerr_es",
  "prog/gameLibs/daECS/net/replicationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, replication_destruction_logerr_es_event_handler_all_events),
  empty_span(),
  make_span(replication_destruction_logerr_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  make_span(replication_destruction_logerr_es_event_handler_comps+2, 1)/*no*/,
  ecs::EventSetBuilder<ecs::EventEntityDestroyed>::build(),
  0
,"netClient");
static constexpr ecs::component_t replication_get_type(){return ecs::ComponentTypeInfo<net::Object>::type; }
