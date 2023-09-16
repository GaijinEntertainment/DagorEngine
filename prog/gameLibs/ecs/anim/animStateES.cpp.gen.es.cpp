#include "animStateES.cpp.inl"
ECS_DEF_PULL_VAR(animState);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc anim_state_debug_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar__animState"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void anim_state_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    anim_state_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(anim_state_debug_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(anim_state_debug_es_comps, "animchar__animState", ecs::Object)
    , ECS_RO_COMP(anim_state_debug_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc anim_state_debug_es_es_desc
(
  "anim_state_debug_es",
  "prog/gameLibs/ecs/anim/animStateES.cpp.inl",
  ecs::EntitySystemOps(anim_state_debug_es_all),
  empty_span(),
  make_span(anim_state_debug_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc restriction_parse_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("human_anim_restriction_ids__prohibited"), ecs::ComponentTypeInfo<ecs::Array>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("human_anim_restrictions__prohibited"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void restriction_parse_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    restriction_parse_es(evt
        , ECS_RO_COMP(restriction_parse_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(restriction_parse_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(restriction_parse_es_comps, "human_anim_restrictions__prohibited", ecs::Object)
    , ECS_RW_COMP(restriction_parse_es_comps, "human_anim_restriction_ids__prohibited", ecs::Array)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc restriction_parse_es_es_desc
(
  "restriction_parse_es",
  "prog/gameLibs/ecs/anim/animStateES.cpp.inl",
  ecs::EntitySystemOps(nullptr, restriction_parse_es_all_events),
  make_span(restriction_parse_es_comps+0, 1)/*rw*/,
  make_span(restriction_parse_es_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc human_anim_state_change_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar__animState"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("animchar__animStateDirty"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [2]
  {ECS_HASH("human_anim_restriction_ids__prohibited"), ecs::ComponentTypeInfo<ecs::Array>()},
//start of 1 no components at [3]
  {ECS_HASH("animchar__lockAnimStateChange"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static void human_anim_state_change_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventChangeAnimState>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    human_anim_state_change_es_event_handler(static_cast<const EventChangeAnimState&>(evt)
        , ECS_RW_COMP(human_anim_state_change_es_event_handler_comps, "animchar__animState", ecs::Object)
    , ECS_RO_COMP(human_anim_state_change_es_event_handler_comps, "human_anim_restriction_ids__prohibited", ecs::Array)
    , ECS_RW_COMP(human_anim_state_change_es_event_handler_comps, "animchar__animStateDirty", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc human_anim_state_change_es_event_handler_es_desc
(
  "human_anim_state_change_es",
  "prog/gameLibs/ecs/anim/animStateES.cpp.inl",
  ecs::EntitySystemOps(nullptr, human_anim_state_change_es_event_handler_all_events),
  make_span(human_anim_state_change_es_event_handler_comps+0, 2)/*rw*/,
  make_span(human_anim_state_change_es_event_handler_comps+2, 1)/*ro*/,
  empty_span(),
  make_span(human_anim_state_change_es_event_handler_comps+3, 1)/*no*/,
  ecs::EventSetBuilder<EventChangeAnimState>::build(),
  0
);
static constexpr ecs::ComponentDesc anim_state_change_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar__animState"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("animchar__animStateDirty"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [2]
  {ECS_HASH("human_anim_restriction_ids__prohibited"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static void anim_state_change_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventChangeAnimState>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    anim_state_change_es_event_handler(static_cast<const EventChangeAnimState&>(evt)
        , ECS_RW_COMP(anim_state_change_es_event_handler_comps, "animchar__animState", ecs::Object)
    , ECS_RW_COMP(anim_state_change_es_event_handler_comps, "animchar__animStateDirty", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc anim_state_change_es_event_handler_es_desc
(
  "anim_state_change_es",
  "prog/gameLibs/ecs/anim/animStateES.cpp.inl",
  ecs::EntitySystemOps(nullptr, anim_state_change_es_event_handler_all_events),
  make_span(anim_state_change_es_event_handler_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  make_span(anim_state_change_es_event_handler_comps+2, 1)/*no*/,
  ecs::EventSetBuilder<EventChangeAnimState>::build(),
  0
);
