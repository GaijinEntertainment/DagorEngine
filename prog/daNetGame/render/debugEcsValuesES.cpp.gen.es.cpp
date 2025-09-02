// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "debugEcsValuesES.cpp.inl"
ECS_DEF_PULL_VAR(debugEcsValues);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc debug_draw_ecs_values_es_comps[] ={};
static void debug_draw_ecs_values_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    debug_draw_ecs_values_es(*info.cast<UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc debug_draw_ecs_values_es_es_desc
(
  "debug_draw_ecs_values_es",
  "prog/daNetGame/render/debugEcsValuesES.cpp.inl",
  ecs::EntitySystemOps(debug_draw_ecs_values_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc items_with_debug_values_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ecs_debug__debugValues"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("ecs_debug__debugValuesOffset"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc items_with_debug_values_ecs_query_desc
(
  "items_with_debug_values_ecs_query",
  empty_span(),
  make_span(items_with_debug_values_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void items_with_debug_values_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, items_with_debug_values_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(items_with_debug_values_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(items_with_debug_values_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(items_with_debug_values_ecs_query_comps, "ecs_debug__debugValues", ecs::Array)
            , ECS_RO_COMP_OR(items_with_debug_values_ecs_query_comps, "ecs_debug__debugValuesOffset", Point3(Point3(0.f, 1.f, 0.f)))
            );

        }while (++comp != compE);
    }
  );
}
