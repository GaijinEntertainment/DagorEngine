#include "animCharParamsDebugES.cpp.inl"
ECS_DEF_PULL_VAR(animCharParamsDebug);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc debug_draw_animchar_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_debug__varsList"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void debug_draw_animchar_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    debug_draw_animchar_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RW_COMP(debug_draw_animchar_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(debug_draw_animchar_es_comps, "animchar_debug__varsList", ecs::Array)
    , ECS_RO_COMP(debug_draw_animchar_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc debug_draw_animchar_es_es_desc
(
  "debug_draw_animchar_es",
  "prog/gameLibs/ecs/render/animCharParamsDebugES.cpp.inl",
  ecs::EntitySystemOps(debug_draw_animchar_es_all),
  make_span(debug_draw_animchar_es_comps+0, 1)/*rw*/,
  make_span(debug_draw_animchar_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc debug_draw_animchar_on_node_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_debug__varsList"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("animchar_debug__nodeName"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void debug_draw_animchar_on_node_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    debug_draw_animchar_on_node_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RW_COMP(debug_draw_animchar_on_node_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(debug_draw_animchar_on_node_es_comps, "animchar_debug__varsList", ecs::Array)
    , ECS_RO_COMP(debug_draw_animchar_on_node_es_comps, "animchar_debug__nodeName", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc debug_draw_animchar_on_node_es_es_desc
(
  "debug_draw_animchar_on_node_es",
  "prog/gameLibs/ecs/render/animCharParamsDebugES.cpp.inl",
  ecs::EntitySystemOps(debug_draw_animchar_on_node_es_all),
  make_span(debug_draw_animchar_on_node_es_comps+0, 1)/*rw*/,
  make_span(debug_draw_animchar_on_node_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
