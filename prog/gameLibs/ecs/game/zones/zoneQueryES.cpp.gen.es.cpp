// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "zoneQueryES.cpp.inl"
ECS_DEF_PULL_VAR(zoneQuery);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc active_sphere_capzones_ecs_query_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("sphere_zone__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sphere_zone__inverted"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("active"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [5]
  {ECS_HASH("capzone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc active_sphere_capzones_ecs_query_desc
(
  "active_sphere_capzones_ecs_query",
  empty_span(),
  make_span(active_sphere_capzones_ecs_query_comps+0, 5)/*ro*/,
  make_span(active_sphere_capzones_ecs_query_comps+5, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void active_sphere_capzones_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, active_sphere_capzones_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP_OR(active_sphere_capzones_ecs_query_comps, "active", bool( true))) )
            continue;
          function(
              ECS_RO_COMP(active_sphere_capzones_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(active_sphere_capzones_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(active_sphere_capzones_ecs_query_comps, "sphere_zone__radius", float)
            , ECS_RO_COMP_OR(active_sphere_capzones_ecs_query_comps, "sphere_zone__inverted", bool(false))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc active_box_capzones_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("box_zone__inverted"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("active"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 2 rq components at [4]
  {ECS_HASH("box_zone"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("capzone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc active_box_capzones_ecs_query_desc
(
  "active_box_capzones_ecs_query",
  empty_span(),
  make_span(active_box_capzones_ecs_query_comps+0, 4)/*ro*/,
  make_span(active_box_capzones_ecs_query_comps+4, 2)/*rq*/,
  empty_span());
template<typename Callable>
inline void active_box_capzones_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, active_box_capzones_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP_OR(active_box_capzones_ecs_query_comps, "active", bool( true))) )
            continue;
          function(
              ECS_RO_COMP(active_box_capzones_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(active_box_capzones_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP_OR(active_box_capzones_ecs_query_comps, "box_zone__inverted", bool(false))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc active_relative_poly_capzones_ecs_query_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("capzone__areaPoints"), ecs::ComponentTypeInfo<ecs::Point2List>()},
  {ECS_HASH("capzone__minHeight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("capzone__maxHeight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("poly_zone__inverted"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("active"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [6]
  {ECS_HASH("capzone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc active_relative_poly_capzones_ecs_query_desc
(
  "active_relative_poly_capzones_ecs_query",
  empty_span(),
  make_span(active_relative_poly_capzones_ecs_query_comps+0, 6)/*ro*/,
  make_span(active_relative_poly_capzones_ecs_query_comps+6, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void active_relative_poly_capzones_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, active_relative_poly_capzones_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP_OR(active_relative_poly_capzones_ecs_query_comps, "active", bool( true))) )
            continue;
          function(
              ECS_RO_COMP(active_relative_poly_capzones_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(active_relative_poly_capzones_ecs_query_comps, "capzone__areaPoints", ecs::Point2List)
            , ECS_RO_COMP(active_relative_poly_capzones_ecs_query_comps, "capzone__minHeight", float)
            , ECS_RO_COMP(active_relative_poly_capzones_ecs_query_comps, "capzone__maxHeight", float)
            , ECS_RO_COMP_OR(active_relative_poly_capzones_ecs_query_comps, "poly_zone__inverted", bool(false))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc box_zone_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("box_zone__inverted"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [2]
  {ECS_HASH("box_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc box_zone_ecs_query_desc
(
  "box_zone_ecs_query",
  empty_span(),
  make_span(box_zone_ecs_query_comps+0, 2)/*ro*/,
  make_span(box_zone_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline bool box_zone_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, box_zone_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(box_zone_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP_OR(box_zone_ecs_query_comps, "box_zone__inverted", bool(false))
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc sphere_zone_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("sphere_zone__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sphere_zone__inverted"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc sphere_zone_ecs_query_desc
(
  "sphere_zone_ecs_query",
  empty_span(),
  make_span(sphere_zone_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool sphere_zone_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, sphere_zone_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(sphere_zone_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(sphere_zone_ecs_query_comps, "sphere_zone__radius", float)
            , ECS_RO_COMP_OR(sphere_zone_ecs_query_comps, "sphere_zone__inverted", bool(false))
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc poly_capzone_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("capzone__areaPoints"), ecs::ComponentTypeInfo<ecs::Point2List>()},
  {ECS_HASH("capzone__minHeight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("capzone__maxHeight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("poly_zone__inverted"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc poly_capzone_ecs_query_desc
(
  "poly_capzone_ecs_query",
  empty_span(),
  make_span(poly_capzone_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool poly_capzone_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, poly_capzone_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(poly_capzone_ecs_query_comps, "capzone__areaPoints", ecs::Point2List)
            , ECS_RO_COMP(poly_capzone_ecs_query_comps, "capzone__minHeight", float)
            , ECS_RO_COMP(poly_capzone_ecs_query_comps, "capzone__maxHeight", float)
            , ECS_RO_COMP_OR(poly_capzone_ecs_query_comps, "poly_zone__inverted", bool(false))
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc poly_battle_area_zone_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("battleAreaPoints"), ecs::ComponentTypeInfo<ecs::Point2List>()},
  {ECS_HASH("poly_zone__inverted"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc poly_battle_area_zone_ecs_query_desc
(
  "poly_battle_area_zone_ecs_query",
  empty_span(),
  make_span(poly_battle_area_zone_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool poly_battle_area_zone_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, poly_battle_area_zone_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(poly_battle_area_zone_ecs_query_comps, "battleAreaPoints", ecs::Point2List)
            , ECS_RO_COMP_OR(poly_battle_area_zone_ecs_query_comps, "poly_zone__inverted", bool(false))
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_transform_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc get_transform_ecs_query_desc
(
  "get_transform_ecs_query",
  empty_span(),
  make_span(get_transform_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_transform_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_transform_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_transform_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
