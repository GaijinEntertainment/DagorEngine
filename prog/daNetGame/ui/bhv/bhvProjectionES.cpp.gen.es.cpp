#include "bhvProjectionES.cpp.inl"
ECS_DEF_PULL_VAR(bhvProjection);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc visibility_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("item__visibleCached"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("item__visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc visibility_ecs_query_desc
(
  "visibility_ecs_query",
  empty_span(),
  make_span(visibility_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void visibility_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP_PTR(visibility_ecs_query_comps, "item__visibleCached", bool)
            , ECS_RO_COMP_PTR(visibility_ecs_query_comps, "item__visible", bool)
            , ECS_RO_COMP_OR(visibility_ecs_query_comps, "animchar__visible", bool(true))
            );

        }
    }
  );
}
