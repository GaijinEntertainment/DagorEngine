#include "animCharFastPhysES.cpp.inl"
ECS_DEF_PULL_VAR(animCharFastPhys);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_fast_phys_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 rq components at [1]
  {ECS_HASH("animchar_fast_phys"), ecs::ComponentTypeInfo<FastPhysTag>()}
};
static void animchar_fast_phys_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_fast_phys_es_event_handler(evt
        , ECS_RW_COMP(animchar_fast_phys_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_fast_phys_es_event_handler_es_desc
(
  "animchar_fast_phys_es",
  "prog/gameLibs/ecs/phys/animCharFastPhysES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_fast_phys_es_event_handler_all_events),
  make_span(animchar_fast_phys_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(animchar_fast_phys_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc animchar_fast_phys_destroy_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 rq components at [1]
  {ECS_HASH("animchar_fast_phys"), ecs::ComponentTypeInfo<FastPhysTag>()}
};
static void animchar_fast_phys_destroy_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_fast_phys_destroy_es_event_handler(evt
        , ECS_RW_COMP(animchar_fast_phys_destroy_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_fast_phys_destroy_es_event_handler_es_desc
(
  "animchar_fast_phys_destroy_es",
  "prog/gameLibs/ecs/phys/animCharFastPhysES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_fast_phys_destroy_es_event_handler_all_events),
  make_span(animchar_fast_phys_destroy_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(animchar_fast_phys_destroy_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
