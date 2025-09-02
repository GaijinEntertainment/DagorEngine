// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "visualConsoleES.cpp.inl"
ECS_DEF_PULL_VAR(visualConsole);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc visual_cnsl_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("visualConsoleObject"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void visual_cnsl_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  visual_cnsl_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc visual_cnsl_es_event_handler_es_desc
(
  "visual_cnsl_es",
  "prog/daNetGame/render/visualConsoleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, visual_cnsl_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(visual_cnsl_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
