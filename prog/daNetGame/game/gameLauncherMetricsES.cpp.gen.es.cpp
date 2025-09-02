// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gameLauncherMetricsES.cpp.inl"
ECS_DEF_PULL_VAR(gameLauncherMetrics);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc game_launcher_metrics_es_event_handler_comps[] ={};
static void game_launcher_metrics_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
if (evt.is<EventUserMMQueueJoined>()) {
    gamelauncher::game_launcher_metrics_es_event_handler(static_cast<const EventUserMMQueueJoined&>(evt)
            );
} else if (evt.is<EventGameSessionFinished>()) {
    gamelauncher::game_launcher_metrics_es_event_handler(static_cast<const EventGameSessionFinished&>(evt)
            );
} else if (evt.is<EventGameSessionStarted>()) {
    gamelauncher::game_launcher_metrics_es_event_handler(static_cast<const EventGameSessionStarted&>(evt)
            );
  } else {G_ASSERTF(0, "Unexpected event type <%s> in game_launcher_metrics_es_event_handler", evt.getName());}
}
static ecs::EntitySystemDesc game_launcher_metrics_es_event_handler_es_desc
(
  "game_launcher_metrics_es",
  "prog/daNetGame/game/gameLauncherMetricsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, game_launcher_metrics_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameSessionStarted,
                       EventGameSessionFinished,
                       EventUserMMQueueJoined>::build(),
  0
);
