// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "heightmapQueryDebugES.cpp.inl"
ECS_DEF_PULL_VAR(heightmapQueryDebug);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc debug_draw_heightmap_queries_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static void debug_draw_heightmap_queries_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    debug_draw_heightmap_queries_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(debug_draw_heightmap_queries_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(debug_draw_heightmap_queries_es_comps, "camera__active", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc debug_draw_heightmap_queries_es_es_desc
(
  "debug_draw_heightmap_queries_es",
  "prog/daNetGame/render/heightmapQueryDebugES.cpp.inl",
  ecs::EntitySystemOps(debug_draw_heightmap_queries_es_all),
  empty_span(),
  make_span(debug_draw_heightmap_queries_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
