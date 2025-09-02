// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collisionTracesES.cpp.inl"
ECS_DEF_PULL_VAR(collisionTraces);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc entity_collres_eid_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc entity_collres_eid_ecs_query_desc
(
  "entity_collres_eid_ecs_query",
  empty_span(),
  make_span(entity_collres_eid_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void entity_collres_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, entity_collres_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(entity_collres_eid_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(entity_collres_eid_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP_PTR(entity_collres_eid_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc capsule_approximation_eid_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("capsule_approximation"), ecs::ComponentTypeInfo<ecs::SharedComponent<CapsuleApproximation>>()}
};
static ecs::CompileTimeQueryDesc capsule_approximation_eid_ecs_query_desc
(
  "capsule_approximation_eid_ecs_query",
  empty_span(),
  make_span(capsule_approximation_eid_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void capsule_approximation_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, capsule_approximation_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(capsule_approximation_eid_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP(capsule_approximation_eid_ecs_query_comps, "capsule_approximation", ecs::SharedComponent<CapsuleApproximation>)
            );

        }
    }
  );
}
