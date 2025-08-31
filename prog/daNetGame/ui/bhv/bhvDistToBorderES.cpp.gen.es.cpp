// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bhvDistToBorderES.cpp.inl"
ECS_DEF_PULL_VAR(bhvDistToBorder);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc get_poly_capzone_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("capzone__areaPoints"), ecs::ComponentTypeInfo<ecs::Point2List>()},
  {ECS_HASH("capzone__minHeight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("capzone__maxHeight"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_poly_capzone_ecs_query_desc
(
  "get_poly_capzone_ecs_query",
  empty_span(),
  make_span(get_poly_capzone_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_poly_capzone_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_poly_capzone_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_poly_capzone_ecs_query_comps, "capzone__areaPoints", ecs::Point2List)
            , ECS_RO_COMP(get_poly_capzone_ecs_query_comps, "capzone__minHeight", float)
            , ECS_RO_COMP(get_poly_capzone_ecs_query_comps, "capzone__maxHeight", float)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_box_capzone_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [1]
  {ECS_HASH("box_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc get_box_capzone_ecs_query_desc
(
  "get_box_capzone_ecs_query",
  empty_span(),
  make_span(get_box_capzone_ecs_query_comps+0, 1)/*ro*/,
  make_span(get_box_capzone_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline bool get_box_capzone_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_box_capzone_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_box_capzone_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_sphere_capzone_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("sphere_zone__radius"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_sphere_capzone_ecs_query_desc
(
  "get_sphere_capzone_ecs_query",
  empty_span(),
  make_span(get_sphere_capzone_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_sphere_capzone_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_sphere_capzone_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_sphere_capzone_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(get_sphere_capzone_ecs_query_comps, "sphere_zone__radius", float)
            );

        }
    }
  );
}
