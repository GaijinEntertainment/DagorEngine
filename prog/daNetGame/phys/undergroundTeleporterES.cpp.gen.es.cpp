#include "undergroundTeleporterES.cpp.inl"
ECS_DEF_PULL_VAR(undergroundTeleporter);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc lanmesh_holes_search_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [1]
  {ECS_HASH("underground_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc lanmesh_holes_search_ecs_query_desc
(
  "lanmesh_holes_search_ecs_query",
  empty_span(),
  make_span(lanmesh_holes_search_ecs_query_comps+0, 1)/*ro*/,
  make_span(lanmesh_holes_search_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void lanmesh_holes_search_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, lanmesh_holes_search_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(lanmesh_holes_search_ecs_query_comps, "transform", TMatrix)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
