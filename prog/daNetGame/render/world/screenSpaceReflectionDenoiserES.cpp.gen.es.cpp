#include "screenSpaceReflectionDenoiserES.cpp.inl"
ECS_DEF_PULL_VAR(screenSpaceReflectionDenoiser);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc draw_validaton_es_event_handler_comps[] ={};
static void draw_validaton_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  draw_validaton_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc draw_validaton_es_event_handler_es_desc
(
  "draw_validaton_es",
  "prog/daNetGame/render/world/screenSpaceReflectionDenoiserES.cpp.inl",
  ecs::EntitySystemOps(nullptr, draw_validaton_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderEventDebugGUI>::build(),
  0
,"dev,render",nullptr,"*");
