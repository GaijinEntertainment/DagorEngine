#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t node_attached__entity_get_type();
static ecs::LTComponentList node_attached__entity_component(ECS_HASH("node_attached__entity"), node_attached__entity_get_type(), "prog/daNetGame/render/fx/animCharEffectES.cpp.inl", "start_animchar_effect_es_event_handler", 0);
static constexpr ecs::component_t node_attached__nodeId_get_type();
static ecs::LTComponentList node_attached__nodeId_component(ECS_HASH("node_attached__nodeId"), node_attached__nodeId_get_type(), "prog/daNetGame/render/fx/animCharEffectES.cpp.inl", "start_animchar_effect_es_event_handler", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGame/render/fx/animCharEffectES.cpp.inl", "start_animchar_effect_es_event_handler", 0);
#include "animCharEffectES.cpp.inl"
ECS_DEF_PULL_VAR(animCharEffect);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc start_animchar_effect_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_effect"), ecs::ComponentTypeInfo<AnimCharEffectNode>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_effect__effect"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static void start_animchar_effect_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    start_animchar_effect_es_event_handler(evt
        , ECS_RO_COMP(start_animchar_effect_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(start_animchar_effect_es_event_handler_comps, "animchar_effect", AnimCharEffectNode)
    , ECS_RO_COMP(start_animchar_effect_es_event_handler_comps, "animchar_effect__effect", ecs::string)
    , ECS_RO_COMP(start_animchar_effect_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc start_animchar_effect_es_event_handler_es_desc
(
  "start_animchar_effect_es",
  "prog/daNetGame/render/fx/animCharEffectES.cpp.inl",
  ecs::EntitySystemOps(nullptr, start_animchar_effect_es_event_handler_all_events),
  make_span(start_animchar_effect_es_event_handler_comps+0, 1)/*rw*/,
  make_span(start_animchar_effect_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc end_animchar_effect_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("animchar_effect"), ecs::ComponentTypeInfo<AnimCharEffectNode>()}
};
static void end_animchar_effect_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    end_animchar_effect_es_event_handler(evt
        , ECS_RO_COMP(end_animchar_effect_es_event_handler_comps, "animchar_effect", AnimCharEffectNode)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc end_animchar_effect_es_event_handler_es_desc
(
  "end_animchar_effect_es",
  "prog/daNetGame/render/fx/animCharEffectES.cpp.inl",
  ecs::EntitySystemOps(nullptr, end_animchar_effect_es_event_handler_all_events),
  empty_span(),
  make_span(end_animchar_effect_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::component_t node_attached__entity_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t node_attached__nodeId_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
