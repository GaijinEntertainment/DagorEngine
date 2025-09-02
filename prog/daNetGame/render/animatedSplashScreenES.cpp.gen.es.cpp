// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animatedSplashScreenES.cpp.inl"
ECS_DEF_PULL_VAR(animatedSplashScreen);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc stop_splash_on_disconnect_es_event_handler_comps[] ={};
static void stop_splash_on_disconnect_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnDisconnectedFromServer>());
  stop_splash_on_disconnect_es_event_handler(static_cast<const EventOnDisconnectedFromServer&>(evt)
        );
}
static ecs::EntitySystemDesc stop_splash_on_disconnect_es_event_handler_es_desc
(
  "stop_splash_on_disconnect_es",
  "prog/daNetGame/render/animatedSplashScreenES.cpp.inl",
  ecs::EntitySystemOps(nullptr, stop_splash_on_disconnect_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnDisconnectedFromServer>::build(),
  0
);
