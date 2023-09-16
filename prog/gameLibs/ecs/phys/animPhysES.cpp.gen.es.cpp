#include "animPhysES.cpp.inl"
ECS_DEF_PULL_VAR(animPhys);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc anim_phys_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("anim_phys"), ecs::ComponentTypeInfo<AnimatedPhys>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("phys_vars"), ecs::ComponentTypeInfo<PhysVars>()}
};
static void anim_phys_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    anim_phys_init_es_event_handler(evt
        , ECS_RO_COMP(anim_phys_init_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(anim_phys_init_es_event_handler_comps, "phys_vars", PhysVars)
    , ECS_RW_COMP(anim_phys_init_es_event_handler_comps, "anim_phys", AnimatedPhys)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc anim_phys_init_es_event_handler_es_desc
(
  "anim_phys_init_es",
  "prog/gameLibs/ecs/phys/animPhysES.cpp.inl",
  ecs::EntitySystemOps(nullptr, anim_phys_init_es_event_handler_all_events),
  make_span(anim_phys_init_es_event_handler_comps+0, 1)/*rw*/,
  make_span(anim_phys_init_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<InvalidateAnimatedPhys,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
