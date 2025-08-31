// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "appProfileES.cpp.inl"
ECS_DEF_PULL_VAR(appProfile);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc app_profile_es_event_handler_comps[] ={};
static void app_profile_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
if (evt.is<EventUserLoggedOut>()) {
    app_profile::app_profile_es_event_handler(static_cast<const EventUserLoggedOut&>(evt)
            );
} else if (evt.is<EventUserLoggedIn>()) {
    app_profile::app_profile_es_event_handler(static_cast<const EventUserLoggedIn&>(evt)
            );
  } else {G_ASSERTF(0, "Unexpected event type <%s> in app_profile_es_event_handler", evt.getName());}
}
static ecs::EntitySystemDesc app_profile_es_event_handler_es_desc
(
  "app_profile_es",
  "prog/daNetGame/main/appProfileES.cpp.inl",
  ecs::EntitySystemOps(nullptr, app_profile_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventUserLoggedIn,
                       EventUserLoggedOut>::build(),
  0
);
