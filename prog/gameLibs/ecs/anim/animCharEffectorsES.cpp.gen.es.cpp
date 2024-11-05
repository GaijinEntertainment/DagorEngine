#include "animCharEffectorsES.cpp.inl"
ECS_DEF_PULL_VAR(animCharEffectors);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_effectors_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_effectors__effectorsState"), ecs::ComponentTypeInfo<ecs::Object>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_effectors__effectorsList"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static void animchar_effectors_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_effectors_init_es_event_handler(evt
        , ECS_RO_COMP(animchar_effectors_init_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(animchar_effectors_init_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(animchar_effectors_init_es_event_handler_comps, "animchar_effectors__effectorsList", ecs::Array)
    , ECS_RW_COMP(animchar_effectors_init_es_event_handler_comps, "animchar_effectors__effectorsState", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_effectors_init_es_event_handler_es_desc
(
  "animchar_effectors_init_es",
  "prog/gameLibs/ecs/anim/animCharEffectorsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_effectors_init_es_event_handler_all_events),
  make_span(animchar_effectors_init_es_event_handler_comps+0, 1)/*rw*/,
  make_span(animchar_effectors_init_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<InvalidateEffectorData,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc animchar_effectors_update_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 4 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar_effectors__effectorsState"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("animchar__updatable"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("animchar__visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [5]
  {ECS_HASH("animchar__actOnDemand"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void animchar_effectors_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateAnimcharEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(animchar_effectors_update_es_comps, "animchar__updatable", bool) && ECS_RO_COMP_OR(animchar_effectors_update_es_comps, "animchar__visible", bool( true))) )
      continue;
    animchar_effectors_update_es(static_cast<const UpdateAnimcharEvent&>(evt)
          , ECS_RW_COMP(animchar_effectors_update_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
      , ECS_RO_COMP(animchar_effectors_update_es_comps, "transform", TMatrix)
      , ECS_RO_COMP(animchar_effectors_update_es_comps, "animchar_effectors__effectorsState", ecs::Object)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_effectors_update_es_es_desc
(
  "animchar_effectors_update_es",
  "prog/gameLibs/ecs/anim/animCharEffectorsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_effectors_update_es_all_events),
  make_span(animchar_effectors_update_es_comps+0, 1)/*rw*/,
  make_span(animchar_effectors_update_es_comps+1, 4)/*ro*/,
  empty_span(),
  make_span(animchar_effectors_update_es_comps+5, 1)/*no*/,
  ecs::EventSetBuilder<UpdateAnimcharEvent>::build(),
  0
,"render",nullptr,"animchar__updater_es","before_animchar_update_sync");
