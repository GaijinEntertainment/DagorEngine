#include "ecsDebug.cpp.inl"
ECS_DEF_PULL_VAR(ecsDebug);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc ecs_debug_entity_clear_es_comps[] ={};
static void ecs_debug_entity_clear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  ecs_debug_entity_clear_es(evt
        );
}
static ecs::EntitySystemDesc ecs_debug_entity_clear_es_es_desc
(
  "ecs_debug_entity_clear_es",
  "prog/gameLibs/ecs/debug/ecsDebug.cpp.inl",
  ecs::EntitySystemOps(nullptr, ecs_debug_entity_clear_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityManagerAfterClear,
                       ecs::EventEntityManagerBeforeClear>::build(),
  0
,"ecsDebug",nullptr,"__first_sync_point");
static constexpr ecs::ComponentDesc ecs_debug_entity_created_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("position"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [3]
  {ECS_HASH("noECSDebug"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void ecs_debug_entity_created_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ecs_debug_entity_created_es_event_handler(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(ecs_debug_entity_created_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP_PTR(ecs_debug_entity_created_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP_PTR(ecs_debug_entity_created_es_event_handler_comps, "position", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ecs_debug_entity_created_es_event_handler_es_desc
(
  "ecs_debug_entity_created_es",
  "prog/gameLibs/ecs/debug/ecsDebug.cpp.inl",
  ecs::EntitySystemOps(nullptr, ecs_debug_entity_created_es_event_handler_all_events),
  empty_span(),
  make_span(ecs_debug_entity_created_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  make_span(ecs_debug_entity_created_es_event_handler_comps+3, 1)/*no*/,
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build(),
  0
,"ecsDebug",nullptr,"__first_sync_point");
static constexpr ecs::ComponentDesc ecs_debug_entity_recreated_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 no components at [1]
  {ECS_HASH("noECSDebug"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void ecs_debug_entity_recreated_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityRecreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ecs_debug_entity_recreated_es_event_handler(static_cast<const ecs::EventEntityRecreated&>(evt)
        , ECS_RO_COMP(ecs_debug_entity_recreated_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ecs_debug_entity_recreated_es_event_handler_es_desc
(
  "ecs_debug_entity_recreated_es",
  "prog/gameLibs/ecs/debug/ecsDebug.cpp.inl",
  ecs::EntitySystemOps(nullptr, ecs_debug_entity_recreated_es_event_handler_all_events),
  empty_span(),
  make_span(ecs_debug_entity_recreated_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  make_span(ecs_debug_entity_recreated_es_event_handler_comps+1, 1)/*no*/,
  ecs::EventSetBuilder<ecs::EventEntityRecreated>::build(),
  0
,"ecsDebug",nullptr,"__first_sync_point");
static constexpr ecs::ComponentDesc ecs_debug_entity_destroyed_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 no components at [1]
  {ECS_HASH("noECSDebug"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void ecs_debug_entity_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ecs_debug_entity_destroyed_es_event_handler(static_cast<const ecs::EventEntityDestroyed&>(evt)
        , ECS_RO_COMP(ecs_debug_entity_destroyed_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ecs_debug_entity_destroyed_es_event_handler_es_desc
(
  "ecs_debug_entity_destroyed_es",
  "prog/gameLibs/ecs/debug/ecsDebug.cpp.inl",
  ecs::EntitySystemOps(nullptr, ecs_debug_entity_destroyed_es_event_handler_all_events),
  empty_span(),
  make_span(ecs_debug_entity_destroyed_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  make_span(ecs_debug_entity_destroyed_es_event_handler_comps+1, 1)/*no*/,
  ecs::EventSetBuilder<ecs::EventEntityDestroyed>::build(),
  0
,"ecsDebug",nullptr,"__first_sync_point");
