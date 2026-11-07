// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "lightingNodesES.cpp.inl"
ECS_DEF_PULL_VAR(lightingNodes);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_lighting_nodes_es_comps[] ={};
static void create_lighting_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_lighting_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_lighting_nodes_es_es_desc
(
  "create_lighting_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/lightingNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_lighting_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc gather_scene_shadow_animchars_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
//start of 8 ro components at [1]
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_attaches_bbox"), ecs::ComponentTypeInfo<bbox3f>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__nodeVisibleStgFilters"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("invisibleUpdatableAnimchar"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__renderPriority"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [9]
  {ECS_HASH("excludeFromAnimcharRender"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_scene_shadow_animchars_ecs_query_desc
(
  "gather_scene_shadow_animchars_ecs_query",
  make_span(gather_scene_shadow_animchars_ecs_query_comps+0, 1)/*rw*/,
  make_span(gather_scene_shadow_animchars_ecs_query_comps+1, 8)/*ro*/,
  empty_span(),
  make_span(gather_scene_shadow_animchars_ecs_query_comps+9, 1)/*no*/);
template<typename Callable>
inline void gather_scene_shadow_animchars_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, gather_scene_shadow_animchars_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(gather_scene_shadow_animchars_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP(gather_scene_shadow_animchars_ecs_query_comps, "animchar_bsph", vec4f)
            , ECS_RO_COMP(gather_scene_shadow_animchars_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP_PTR(gather_scene_shadow_animchars_ecs_query_comps, "animchar_attaches_bbox", bbox3f)
            , ECS_RO_COMP(gather_scene_shadow_animchars_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP_PTR(gather_scene_shadow_animchars_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_PTR(gather_scene_shadow_animchars_ecs_query_comps, "animchar_render__nodeVisibleStgFilters", ecs::UInt8List)
            , ECS_RO_COMP_PTR(gather_scene_shadow_animchars_ecs_query_comps, "invisibleUpdatableAnimchar", ecs::Tag)
            , ECS_RO_COMP_OR(gather_scene_shadow_animchars_ecs_query_comps, "animchar__renderPriority", bool(false))
            );

        }while (++comp != compE);
    }
  );
}
