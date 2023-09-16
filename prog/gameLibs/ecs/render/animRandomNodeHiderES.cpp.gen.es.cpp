#include "animRandomNodeHiderES.cpp.inl"
ECS_DEF_PULL_VAR(animRandomNodeHider);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc anim_random_node_hider_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("animchar_random_node_hider__nodes"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void anim_random_node_hider_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    anim_random_node_hider_es_event_handler(evt
        , ECS_RW_COMP(anim_random_node_hider_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(anim_random_node_hider_es_event_handler_comps, "animchar_random_node_hider__nodes", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc anim_random_node_hider_es_event_handler_es_desc
(
  "anim_random_node_hider_es",
  "prog/gameLibs/ecs/render/animRandomNodeHiderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, anim_random_node_hider_es_event_handler_all_events),
  make_span(anim_random_node_hider_es_event_handler_comps+0, 1)/*rw*/,
  make_span(anim_random_node_hider_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
