#include "dargPanelAnchorResolveES.cpp.inl"
ECS_DEF_PULL_VAR(dargPanelAnchorResolve);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc get_entity_node_transform_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static ecs::CompileTimeQueryDesc get_entity_node_transform_ecs_query_desc
(
  "get_entity_node_transform_ecs_query",
  empty_span(),
  make_span(get_entity_node_transform_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_entity_node_transform_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_entity_node_transform_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_entity_node_transform_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_entity_transform_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc get_entity_transform_ecs_query_desc
(
  "get_entity_transform_ecs_query",
  empty_span(),
  make_span(get_entity_transform_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_entity_transform_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_entity_transform_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_entity_transform_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
