// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "profileEventsES.cpp.inl"
ECS_DEF_PULL_VAR(profileEvents);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc unique_event_profile_render_es_event_handler_comps[] ={};
static void unique_event_profile_render_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  unique_event_profile_render_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc unique_event_profile_render_es_event_handler_es_desc
(
  "unique_event_profile_render_es",
  "prog/daNetGame/render/profileEventsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, unique_event_profile_render_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderEventDebugGUI>::build(),
  0
,"dev,render",nullptr,"*");
