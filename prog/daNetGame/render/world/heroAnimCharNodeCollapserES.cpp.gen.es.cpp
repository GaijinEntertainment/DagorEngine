// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "heroAnimCharNodeCollapserES.cpp.inl"
ECS_DEF_PULL_VAR(heroAnimCharNodeCollapser);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc hero_animchar_node_hider_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("hero_animchar_node_hider__nodeIndices"), ecs::ComponentTypeInfo<ecs::IntList>()},
//start of 4 ro components at [1]
  {ECS_HASH("hero_animchar_node_hider__rootNode"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("hero_animchar_node_hider__endNodes"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()}
};
static void hero_animchar_node_hider_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    hero_animchar_node_hider_init_es_event_handler(evt
        , ECS_RO_COMP(hero_animchar_node_hider_init_es_event_handler_comps, "hero_animchar_node_hider__rootNode", ecs::string)
    , ECS_RO_COMP(hero_animchar_node_hider_init_es_event_handler_comps, "hero_animchar_node_hider__endNodes", ecs::StringList)
    , ECS_RO_COMP(hero_animchar_node_hider_init_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(hero_animchar_node_hider_init_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RW_COMP(hero_animchar_node_hider_init_es_event_handler_comps, "hero_animchar_node_hider__nodeIndices", ecs::IntList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc hero_animchar_node_hider_init_es_event_handler_es_desc
(
  "hero_animchar_node_hider_init_es",
  "prog/daNetGame/render/world/heroAnimCharNodeCollapserES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hero_animchar_node_hider_init_es_event_handler_all_events),
  make_span(hero_animchar_node_hider_init_es_event_handler_comps+0, 1)/*rw*/,
  make_span(hero_animchar_node_hider_init_es_event_handler_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc hero_animchar_node_hider_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("human_net_phys__isZoomingRenderData"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("hero_animchar_node_hider__nodeIndices"), ecs::ComponentTypeInfo<ecs::IntList>()},
//start of 1 rq components at [3]
  {ECS_HASH("hero"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void hero_animchar_node_hider_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<HideNodesEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    hero_animchar_node_hider_es_event_handler(static_cast<const HideNodesEvent&>(evt)
        , ECS_RW_COMP(hero_animchar_node_hider_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(hero_animchar_node_hider_es_event_handler_comps, "human_net_phys__isZoomingRenderData", bool)
    , ECS_RO_COMP(hero_animchar_node_hider_es_event_handler_comps, "hero_animchar_node_hider__nodeIndices", ecs::IntList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc hero_animchar_node_hider_es_event_handler_es_desc
(
  "hero_animchar_node_hider_es",
  "prog/daNetGame/render/world/heroAnimCharNodeCollapserES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hero_animchar_node_hider_es_event_handler_all_events),
  make_span(hero_animchar_node_hider_es_event_handler_comps+0, 1)/*rw*/,
  make_span(hero_animchar_node_hider_es_event_handler_comps+1, 2)/*ro*/,
  make_span(hero_animchar_node_hider_es_event_handler_comps+3, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<HideNodesEvent>::build(),
  0
);
