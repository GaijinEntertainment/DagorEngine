#include "randomAnimStarterES.cpp.inl"
ECS_DEF_PULL_VAR(randomAnimStarter);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc amimchar_entities_with_random_starter_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("randomAnimStart"), ecs::ComponentTypeInfo<bool>()}
};
static void amimchar_entities_with_random_starter_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(amimchar_entities_with_random_starter_es_event_handler_comps, "randomAnimStart", bool)) )
      continue;
    amimchar_entities_with_random_starter_es_event_handler(evt
          , ECS_RW_COMP(amimchar_entities_with_random_starter_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc amimchar_entities_with_random_starter_es_event_handler_es_desc
(
  "amimchar_entities_with_random_starter_es",
  "prog/gameLibs/ecs/anim/randomAnimStarterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, amimchar_entities_with_random_starter_es_event_handler_all_events),
  make_span(amimchar_entities_with_random_starter_es_event_handler_comps+0, 1)/*rw*/,
  make_span(amimchar_entities_with_random_starter_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"gameClient");
