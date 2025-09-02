// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "renderOcclusionExclusionES.cpp.inl"
ECS_DEF_PULL_VAR(renderOcclusionExclusion);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc render_occlusion_exclusion_es_event_handler_comps[] ={};
static void render_occlusion_exclusion_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OcclusionExclusion>());
  render_occlusion_exclusion_es_event_handler(static_cast<const OcclusionExclusion&>(evt)
        );
}
static ecs::EntitySystemDesc render_occlusion_exclusion_es_event_handler_es_desc
(
  "render_occlusion_exclusion_es",
  "prog/daNetGame/render/world/renderOcclusionExclusionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, render_occlusion_exclusion_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OcclusionExclusion>::build(),
  0
);
static constexpr ecs::ComponentDesc process_animchar_eid_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc process_animchar_eid_ecs_query_desc
(
  "process_animchar_eid_ecs_query",
  make_span(process_animchar_eid_ecs_query_comps+0, 1)/*rw*/,
  make_span(process_animchar_eid_ecs_query_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void process_animchar_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, process_animchar_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(process_animchar_eid_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RW_COMP(process_animchar_eid_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP_PTR(process_animchar_eid_ecs_query_comps, "additional_data", ecs::Point4List)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc render_hero_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 3 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [4]
  {ECS_HASH("watchedByPlr"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static ecs::CompileTimeQueryDesc render_hero_ecs_query_desc
(
  "render_hero_ecs_query",
  make_span(render_hero_ecs_query_comps+0, 1)/*rw*/,
  make_span(render_hero_ecs_query_comps+1, 3)/*ro*/,
  make_span(render_hero_ecs_query_comps+4, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void render_hero_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, render_hero_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(render_hero_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(render_hero_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP_PTR(render_hero_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_PTR(render_hero_ecs_query_comps, "additional_data", ecs::Point4List)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc render_hero_vehicle_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [3]
  {ECS_HASH("vehicleWithWatched"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static ecs::CompileTimeQueryDesc render_hero_vehicle_ecs_query_desc
(
  "render_hero_vehicle_ecs_query",
  make_span(render_hero_vehicle_ecs_query_comps+0, 1)/*rw*/,
  make_span(render_hero_vehicle_ecs_query_comps+1, 2)/*ro*/,
  make_span(render_hero_vehicle_ecs_query_comps+3, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void render_hero_vehicle_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, render_hero_vehicle_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(render_hero_vehicle_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(render_hero_vehicle_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP_PTR(render_hero_vehicle_ecs_query_comps, "additional_data", ecs::Point4List)
            );

        }while (++comp != compE);
    }
  );
}
