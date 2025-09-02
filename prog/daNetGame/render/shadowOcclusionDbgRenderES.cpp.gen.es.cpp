#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGame/render/shadowOcclusionDbgRenderES.cpp.inl", "shadow_occlusion_render_debug_es", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "shadowOcclusionDbgRenderES.cpp.inl"
ECS_DEF_PULL_VAR(shadowOcclusionDbgRender);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc shadow_occlusion_render_debug_es_comps[] ={};
static void shadow_occlusion_render_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    shadow_occlusion_render_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc shadow_occlusion_render_debug_es_es_desc
(
  "shadow_occlusion_render_debug_es",
  "prog/daNetGame/render/shadowOcclusionDbgRenderES.cpp.inl",
  ecs::EntitySystemOps(shadow_occlusion_render_debug_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc animchar_shadow_cull_bounds_debug_render_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("animchar_shadow_cull_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc animchar_shadow_cull_bounds_debug_render_ecs_query_desc
(
  "animchar_shadow_cull_bounds_debug_render_ecs_query",
  empty_span(),
  make_span(animchar_shadow_cull_bounds_debug_render_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void animchar_shadow_cull_bounds_debug_render_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_shadow_cull_bounds_debug_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(animchar_shadow_cull_bounds_debug_render_ecs_query_comps, "animchar_render__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(animchar_shadow_cull_bounds_debug_render_ecs_query_comps, "animchar_shadow_cull_bbox", bbox3f)
            , ECS_RO_COMP(animchar_shadow_cull_bounds_debug_render_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
