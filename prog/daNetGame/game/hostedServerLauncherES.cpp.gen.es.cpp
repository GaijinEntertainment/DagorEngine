// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "hostedServerLauncherES.cpp.inl"
ECS_DEF_PULL_VAR(hostedServerLauncher);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc event_start_internal_server_es_event_handler_comps[] ={};
static void event_start_internal_server_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventHostedInternalServerToStart>());
  event_start_internal_server_es_event_handler(static_cast<const EventHostedInternalServerToStart&>(evt)
        );
}
static ecs::EntitySystemDesc event_start_internal_server_es_event_handler_es_desc
(
  "event_start_internal_server_es",
  "prog/daNetGame/game/hostedServerLauncherES.cpp.inl",
  ecs::EntitySystemOps(nullptr, event_start_internal_server_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventHostedInternalServerToStart>::build(),
  0
);
//static constexpr ecs::ComponentDesc event_stop_internal_server_es_event_handler_comps[] ={};
static void event_stop_internal_server_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  event_stop_internal_server_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc event_stop_internal_server_es_event_handler_es_desc
(
  "event_stop_internal_server_es",
  "prog/daNetGame/game/hostedServerLauncherES.cpp.inl",
  ecs::EntitySystemOps(nullptr, event_stop_internal_server_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventHostedInternalServerToStop>::build(),
  0
);
