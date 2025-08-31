// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animIrqES.cpp.inl"
ECS_DEF_PULL_VAR(animIrq);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc anim_init_irq_listener_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("anim_irq__eventNames"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("anim_irq_to_event_converter"), ecs::ComponentTypeInfo<AnimIrqToEventComponent>()}
};
static void anim_init_irq_listener_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    anim_init_irq_listener_es_event_handler(evt
        , ECS_RW_COMP(anim_init_irq_listener_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP(anim_init_irq_listener_es_event_handler_comps, "anim_irq__eventNames", ecs::Object)
    , ECS_RW_COMP(anim_init_irq_listener_es_event_handler_comps, "anim_irq_to_event_converter", AnimIrqToEventComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc anim_init_irq_listener_es_event_handler_es_desc
(
  "anim_init_irq_listener_es",
  "prog/gameLibs/ecs/anim/animIrqES.cpp.inl",
  ecs::EntitySystemOps(nullptr, anim_init_irq_listener_es_event_handler_all_events),
  make_span(anim_init_irq_listener_es_event_handler_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
