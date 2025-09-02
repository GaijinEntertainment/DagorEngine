// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "capsuleApproximationCollisionsES.cpp.inl"
ECS_DEF_PULL_VAR(capsuleApproximationCollisions);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc capsules_collision_on_appear_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("capsule_approximation_collisions_names"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("animchar_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("capsule_approximation_collisions_ids"), ecs::ComponentTypeInfo<ecs::IntList>()}
};
static void capsules_collision_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    capsules_collision_on_appear_es(evt
        , ECS_RW_COMP(capsules_collision_on_appear_es_comps, "capsule_approximation_collisions_names", ecs::StringList)
    , ECS_RW_COMP(capsules_collision_on_appear_es_comps, "animchar_attach__attachedTo", ecs::EntityId)
    , ECS_RW_COMP(capsules_collision_on_appear_es_comps, "capsule_approximation_collisions_ids", ecs::IntList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc capsules_collision_on_appear_es_es_desc
(
  "capsules_collision_on_appear_es",
  "prog/daNetGame/game/capsuleApproximationCollisionsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, capsules_collision_on_appear_es_all_events),
  make_span(capsules_collision_on_appear_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","animchar_attach__attachedTo");
static constexpr ecs::ComponentDesc capsules_collisions_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("capsule_approximation_collisions_ids"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>()}
};
static void capsules_collisions_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    capsules_collisions_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(capsules_collisions_es_comps, "animchar_attach__attachedTo", ecs::EntityId)
    , ECS_RW_COMP(capsules_collisions_es_comps, "capsule_approximation_collisions_ids", ecs::IntList)
    , ECS_RW_COMP(capsules_collisions_es_comps, "additional_data", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc capsules_collisions_es_es_desc
(
  "capsules_collisions_es",
  "prog/daNetGame/game/capsuleApproximationCollisionsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, capsules_collisions_es_all_events),
  make_span(capsules_collisions_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc get_attached_to_capsules_preprocess_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
//start of 1 ro components at [1]
  {ECS_HASH("capsule_approximation"), ecs::ComponentTypeInfo<ecs::SharedComponent<CapsuleApproximation>>()}
};
static ecs::CompileTimeQueryDesc get_attached_to_capsules_preprocess_ecs_query_desc
(
  "get_attached_to_capsules_preprocess_ecs_query",
  make_span(get_attached_to_capsules_preprocess_ecs_query_comps+0, 1)/*rw*/,
  make_span(get_attached_to_capsules_preprocess_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_attached_to_capsules_preprocess_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_attached_to_capsules_preprocess_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(get_attached_to_capsules_preprocess_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(get_attached_to_capsules_preprocess_ecs_query_comps, "capsule_approximation", ecs::SharedComponent<CapsuleApproximation>)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_attached_to_capsules_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("capsule_approximation"), ecs::ComponentTypeInfo<ecs::SharedComponent<CapsuleApproximation>>()}
};
static ecs::CompileTimeQueryDesc get_attached_to_capsules_ecs_query_desc
(
  "get_attached_to_capsules_ecs_query",
  make_span(get_attached_to_capsules_ecs_query_comps+0, 1)/*rw*/,
  make_span(get_attached_to_capsules_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_attached_to_capsules_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_attached_to_capsules_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(get_attached_to_capsules_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP(get_attached_to_capsules_ecs_query_comps, "capsule_approximation", ecs::SharedComponent<CapsuleApproximation>)
            );

        }
    }
  );
}
