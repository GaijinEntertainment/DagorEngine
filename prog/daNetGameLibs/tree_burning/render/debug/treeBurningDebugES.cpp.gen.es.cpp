// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "treeBurningDebugES.cpp.inl"
ECS_DEF_PULL_VAR(treeBurningDebug);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc tree_burning_debug_visualization_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("tree_burning_manager"), ecs::ComponentTypeInfo<TreeBurningManager>()}
};
static void tree_burning_debug_visualization_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    tree_burning_debug_visualization_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(tree_burning_debug_visualization_es_comps, "tree_burning_manager", TreeBurningManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tree_burning_debug_visualization_es_es_desc
(
  "tree_burning_debug_visualization_es",
  "prog/daNetGameLibs/tree_burning/render/debug/treeBurningDebugES.cpp.inl",
  ecs::EntitySystemOps(tree_burning_debug_visualization_es_all),
  empty_span(),
  make_span(tree_burning_debug_visualization_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
