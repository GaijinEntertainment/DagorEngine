// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gameLauncherES.cpp.inl"
ECS_DEF_PULL_VAR(gameLauncher);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc game_launcher_es_event_handler_comps[] ={};
static void game_launcher_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventUserLoggedOut>());
  gamelauncher::game_launcher_es_event_handler(static_cast<const EventUserLoggedOut&>(evt)
        );
}
static ecs::EntitySystemDesc game_launcher_es_event_handler_es_desc
(
  "game_launcher_es",
  "prog/daNetGame/game/gameLauncherES.cpp.inl",
  ecs::EntitySystemOps(nullptr, game_launcher_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventUserLoggedOut>::build(),
  0
);
