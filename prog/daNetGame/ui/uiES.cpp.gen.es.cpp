// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "uiES.cpp.inl"
ECS_DEF_PULL_VAR(ui);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc ui_rebind_das_events_es_comps[] ={};
static void ui_rebind_das_events_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventGameModeLoaded>());
  ui_rebind_das_events_es(static_cast<const EventGameModeLoaded&>(evt)
        );
}
static ecs::EntitySystemDesc ui_rebind_das_events_es_es_desc
(
  "ui_rebind_das_events_es",
  "prog/daNetGame/ui/uiES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ui_rebind_das_events_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameModeLoaded>::build(),
  0
,nullptr,nullptr,"*");
