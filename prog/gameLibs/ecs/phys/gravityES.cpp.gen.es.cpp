#include "gravityES.cpp.inl"
ECS_DEF_PULL_VAR(gravity);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc gravity_controller_appear_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("phys_props__gravity"), ecs::ComponentTypeInfo<float>()}
};
static void gravity_controller_appear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gravity_controller_appear_es_event_handler(evt
        , ECS_RO_COMP(gravity_controller_appear_es_event_handler_comps, "phys_props__gravity", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gravity_controller_appear_es_event_handler_es_desc
(
  "gravity_controller_appear_es",
  "prog/gameLibs/ecs/phys/gravityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gravity_controller_appear_es_event_handler_all_events),
  empty_span(),
  make_span(gravity_controller_appear_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"phys_props__gravity");
static constexpr ecs::ComponentDesc gravity_controller_disappear_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("phys_props__gravity"), ecs::ComponentTypeInfo<float>()}
};
static void gravity_controller_disappear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  gravity_controller_disappear_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc gravity_controller_disappear_es_event_handler_es_desc
(
  "gravity_controller_disappear_es",
  "prog/gameLibs/ecs/phys/gravityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gravity_controller_disappear_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(gravity_controller_disappear_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
