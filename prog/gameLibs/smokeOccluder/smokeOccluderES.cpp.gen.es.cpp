#include "smokeOccluderES.cpp.inl"
ECS_DEF_PULL_VAR(smokeOccluder);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc smoke_occluders_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("smoke_occluder__sphere"), ecs::ComponentTypeInfo<Point4>()}
};
static ecs::CompileTimeQueryDesc smoke_occluders_ecs_query_desc
(
  "smoke_occluders_ecs_query",
  empty_span(),
  make_span(smoke_occluders_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult smoke_occluders_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, smoke_occluders_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(smoke_occluders_ecs_query_comps, "smoke_occluder__sphere", Point4)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
