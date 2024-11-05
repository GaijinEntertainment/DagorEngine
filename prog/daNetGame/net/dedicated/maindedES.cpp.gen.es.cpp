#include "maindedES.cpp.inl"
ECS_DEF_PULL_VAR(mainded);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc dedicated_init_on_appstart_es_event_handler_comps[] ={};
static void dedicated_init_on_appstart_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameAppStarted>());
  dedicated::dedicated_init_on_appstart_es_event_handler(static_cast<const EventOnGameAppStarted&>(evt)
        );
}
static ecs::EntitySystemDesc dedicated_init_on_appstart_es_event_handler_es_desc
(
  "dedicated_init_on_appstart_es",
  "prog/daNetGame/net/dedicated/maindedES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dedicated_init_on_appstart_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameAppStarted>::build(),
  0
,nullptr,nullptr,nullptr,"on_gameapp_started_es");
