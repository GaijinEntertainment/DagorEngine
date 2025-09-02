// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collResES.cpp.inl"
ECS_DEF_PULL_VAR(collRes);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc get_animchar_collision_transform_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc get_animchar_collision_transform_ecs_query_desc
(
  "get_animchar_collision_transform_ecs_query",
  empty_span(),
  make_span(get_animchar_collision_transform_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_animchar_collision_transform_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_animchar_collision_transform_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_animchar_collision_transform_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(get_animchar_collision_transform_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP_OR(get_animchar_collision_transform_ecs_query_comps, "transform", TMatrix(TMatrix::IDENT))
            );

        }
    }
  );
}
