// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "debugInputEventsES.cpp.inl"
ECS_DEF_PULL_VAR(debugInputEvents);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc debug_input_events_init_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("debugInputEventsTag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void debug_input_events_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  debug_input_events_init_es_event_handler(static_cast<const ecs::EventEntityCreated&>(evt)
        );
}
static ecs::EntitySystemDesc debug_input_events_init_es_event_handler_es_desc
(
  "debug_input_events_init_es",
  "prog/daNetGame/input/debugInputEventsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, debug_input_events_init_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(debug_input_events_init_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build(),
  0
,"dev,gameClient");
static constexpr ecs::ComponentDesc debug_input_events_close_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("debugInputEventsTag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void debug_input_events_close_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyed>());
  debug_input_events_close_es_event_handler(static_cast<const ecs::EventEntityDestroyed&>(evt)
        );
}
static ecs::EntitySystemDesc debug_input_events_close_es_event_handler_es_desc
(
  "debug_input_events_close_es",
  "prog/daNetGame/input/debugInputEventsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, debug_input_events_close_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(debug_input_events_close_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed>::build(),
  0
,"dev,gameClient");
