#include "componentFiltersES.cpp.inl"
ECS_DEF_PULL_VAR(componentFilters);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc mark_all_dirty_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<net::Object>()}
};
static ecs::CompileTimeQueryDesc mark_all_dirty_ecs_query_desc
(
  "net::mark_all_dirty_ecs_query",
  make_span(mark_all_dirty_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void net::mark_all_dirty_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, mark_all_dirty_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(mark_all_dirty_ecs_query_comps, "replication", net::Object)
            );

        }while (++comp != compE);
    }
  );
}
