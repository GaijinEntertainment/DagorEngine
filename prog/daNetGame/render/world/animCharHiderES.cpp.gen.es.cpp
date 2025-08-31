// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animCharHiderES.cpp.inl"
ECS_DEF_PULL_VAR(animCharHider);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_hider_at_node_created_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_hider__nodeIdx"), ecs::ComponentTypeInfo<int>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimCharV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_hider__nodeName"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void animchar_hider_at_node_created_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_hider_at_node_created_es_event_handler(evt
        , ECS_RO_COMP(animchar_hider_at_node_created_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(animchar_hider_at_node_created_es_event_handler_comps, "animchar", AnimCharV20::AnimcharBaseComponent)
    , ECS_RO_COMP(animchar_hider_at_node_created_es_event_handler_comps, "animchar_hider__nodeName", ecs::string)
    , ECS_RW_COMP(animchar_hider_at_node_created_es_event_handler_comps, "animchar_hider__nodeIdx", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_hider_at_node_created_es_event_handler_es_desc
(
  "animchar_hider_at_node_created_es",
  "prog/daNetGame/render/world/animCharHiderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_hider_at_node_created_es_event_handler_all_events),
  make_span(animchar_hider_at_node_created_es_event_handler_comps+0, 1)/*rw*/,
  make_span(animchar_hider_at_node_created_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc animchar_hider_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_hider__camThreshold"), ecs::ComponentTypeInfo<float>()},
//start of 1 no components at [3]
  {ECS_HASH("animchar_hider__nodeIdx"), ecs::ComponentTypeInfo<int>()}
};
static void animchar_hider_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<HideNodesEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_hider_es_event_handler(static_cast<const HideNodesEvent&>(evt)
        , ECS_RW_COMP(animchar_hider_es_event_handler_comps, "animchar_visbits", animchar_visbits_t)
    , ECS_RO_COMP(animchar_hider_es_event_handler_comps, "animchar_bsph", vec4f)
    , ECS_RO_COMP(animchar_hider_es_event_handler_comps, "animchar_hider__camThreshold", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_hider_es_event_handler_es_desc
(
  "animchar_hider_es",
  "prog/daNetGame/render/world/animCharHiderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_hider_es_event_handler_all_events),
  make_span(animchar_hider_es_event_handler_comps+0, 1)/*rw*/,
  make_span(animchar_hider_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  make_span(animchar_hider_es_event_handler_comps+3, 1)/*no*/,
  ecs::EventSetBuilder<HideNodesEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc animchar_hider_at_node_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
//start of 3 ro components at [1]
  {ECS_HASH("animchar_hider__nodeIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("animchar_hider__camThreshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimCharV20::AnimcharBaseComponent>()}
};
static void animchar_hider_at_node_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<HideNodesEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_hider_at_node_es_event_handler(static_cast<const HideNodesEvent&>(evt)
        , ECS_RW_COMP(animchar_hider_at_node_es_event_handler_comps, "animchar_visbits", animchar_visbits_t)
    , ECS_RO_COMP(animchar_hider_at_node_es_event_handler_comps, "animchar_hider__nodeIdx", int)
    , ECS_RO_COMP(animchar_hider_at_node_es_event_handler_comps, "animchar_hider__camThreshold", float)
    , ECS_RO_COMP(animchar_hider_at_node_es_event_handler_comps, "animchar", AnimCharV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_hider_at_node_es_event_handler_es_desc
(
  "animchar_hider_at_node_es",
  "prog/daNetGame/render/world/animCharHiderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_hider_at_node_es_event_handler_all_events),
  make_span(animchar_hider_at_node_es_event_handler_comps+0, 1)/*rw*/,
  make_span(animchar_hider_at_node_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<HideNodesEvent>::build(),
  0
,"render");
