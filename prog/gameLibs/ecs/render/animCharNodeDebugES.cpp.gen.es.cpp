// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animCharNodeDebugES.cpp.inl"
ECS_DEF_PULL_VAR(animCharNodeDebug);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc debug_draw_animchar_node_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar__debugNodes"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("animchar__debugNodesPos"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void debug_draw_animchar_node_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    debug_draw_animchar_node_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(debug_draw_animchar_node_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(debug_draw_animchar_node_es_comps, "animchar__debugNodes", ecs::Array)
    , ECS_RO_COMP_OR(debug_draw_animchar_node_es_comps, "animchar__debugNodesPos", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc debug_draw_animchar_node_es_es_desc
(
  "debug_draw_animchar_node_es",
  "prog/gameLibs/ecs/render/animCharNodeDebugES.cpp.inl",
  ecs::EntitySystemOps(debug_draw_animchar_node_es_all),
  empty_span(),
  make_span(debug_draw_animchar_node_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
