// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "grassES.cpp.inl"
ECS_DEF_PULL_VAR(grass);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc grass_view_process_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__grass_manager"), ecs::ComponentTypeInfo<dagdp::GrassManager>()}
};
static void grass_view_process_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventViewProcess>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::grass_view_process_es(static_cast<const dagdp::EventViewProcess&>(evt)
        , ECS_RW_COMP(grass_view_process_es_comps, "dagdp__grass_manager", dagdp::GrassManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grass_view_process_es_es_desc
(
  "grass_view_process_es",
  "prog/daNetGameLibs/daGdp/render/placers/grassES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grass_view_process_es_all_events),
  make_span(grass_view_process_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventViewProcess>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc grass_view_finalize_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__grass_manager"), ecs::ComponentTypeInfo<dagdp::GrassManager>()}
};
static void grass_view_finalize_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventViewFinalize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::grass_view_finalize_es(static_cast<const dagdp::EventViewFinalize&>(evt)
        , ECS_RW_COMP(grass_view_finalize_es_comps, "dagdp__grass_manager", dagdp::GrassManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grass_view_finalize_es_es_desc
(
  "grass_view_finalize_es",
  "prog/daNetGameLibs/daGdp/render/placers/grassES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grass_view_finalize_es_all_events),
  make_span(grass_view_finalize_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventViewFinalize>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc dagdp_placer_grass_changed_es_comps[] =
{
//start of 4 rq components at [0]
  {ECS_HASH("dagdp__biomes"), ecs::ComponentTypeInfo<ecs::List<int>>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("dagdp_placer_grass"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("dagdp__density"), ecs::ComponentTypeInfo<float>()}
};
static void dagdp_placer_grass_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dagdp::dagdp_placer_grass_changed_es(evt
        );
}
static ecs::EntitySystemDesc dagdp_placer_grass_changed_es_es_desc
(
  "dagdp_placer_grass_changed_es",
  "prog/daNetGameLibs/daGdp/render/placers/grassES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_placer_grass_changed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dagdp_placer_grass_changed_es_comps+0, 4)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","dagdp__biomes,dagdp__density,dagdp__name");
static constexpr ecs::ComponentDesc grass_settings_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("dagdp__grass_max_range"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [1]
  {ECS_HASH("dagdp_level_settings"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc grass_settings_ecs_query_desc
(
  "dagdp::grass_settings_ecs_query",
  empty_span(),
  make_span(grass_settings_ecs_query_comps+0, 1)/*ro*/,
  make_span(grass_settings_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::grass_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, grass_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(grass_settings_ecs_query_comps, "dagdp__grass_max_range", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc grass_placers_ecs_query_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dagdp__biomes"), ecs::ComponentTypeInfo<ecs::List<int>>()},
  {ECS_HASH("dagdp__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("fast_grass__height"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("fast_grass__height_variance"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [6]
  {ECS_HASH("dagdp_placer_grass"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc grass_placers_ecs_query_desc
(
  "dagdp::grass_placers_ecs_query",
  empty_span(),
  make_span(grass_placers_ecs_query_comps+0, 6)/*ro*/,
  make_span(grass_placers_ecs_query_comps+6, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::grass_placers_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, grass_placers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(grass_placers_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(grass_placers_ecs_query_comps, "dagdp__biomes", ecs::List<int>)
            , ECS_RO_COMP(grass_placers_ecs_query_comps, "dagdp__density", float)
            , ECS_RO_COMP(grass_placers_ecs_query_comps, "dagdp__name", ecs::string)
            , ECS_RO_COMP_PTR(grass_placers_ecs_query_comps, "fast_grass__height", float)
            , ECS_RO_COMP_PTR(grass_placers_ecs_query_comps, "fast_grass__height_variance", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static ecs::CompileTimeQueryDesc manager_ecs_query_desc
(
  "dagdp::manager_ecs_query",
  make_span(manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(manager_ecs_query_comps, "dagdp__global_manager", dagdp::GlobalManager)
            );

        }while (++comp != compE);
    }
  );
}
