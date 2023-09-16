#include "animCharDbgRenderES.cpp.inl"
ECS_DEF_PULL_VAR(animCharDbgRender);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc animchar_render_debug_es_comps[] ={};
static void animchar_render_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    animchar_render_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc animchar_render_debug_es_es_desc
(
  "animchar_render_debug_es",
  "prog/gameLibs/ecs/render/animCharDbgRenderES.cpp.inl",
  ecs::EntitySystemOps(animchar_render_debug_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc animchar_bounds_debug_render_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>()},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc animchar_bounds_debug_render_ecs_query_desc
(
  "animchar_bounds_debug_render_ecs_query",
  empty_span(),
  make_span(animchar_bounds_debug_render_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void animchar_bounds_debug_render_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_bounds_debug_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(animchar_bounds_debug_render_ecs_query_comps, "animchar_render__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(animchar_bounds_debug_render_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP(animchar_bounds_debug_render_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(animchar_bounds_debug_render_ecs_query_comps, "animchar_node_wtm", AnimcharNodesMat44)
            , ECS_RO_COMP(animchar_bounds_debug_render_ecs_query_comps, "animchar_bsph", vec4f)
            , ECS_RO_COMP(animchar_bounds_debug_render_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP(animchar_bounds_debug_render_ecs_query_comps, "animchar_visbits", uint8_t)
            );

        }while (++comp != compE);
      }
  );
}
