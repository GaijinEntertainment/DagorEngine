#include "distantHazeES.cpp.inl"
ECS_DEF_PULL_VAR(distantHaze);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_distant_haze_manager_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("distant_haze__manager"), ecs::ComponentTypeInfo<DistantHazeManager>()}
};
static void init_distant_haze_manager_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_distant_haze_manager_es_event_handler(evt
        , ECS_RW_COMP(init_distant_haze_manager_es_event_handler_comps, "distant_haze__manager", DistantHazeManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_distant_haze_manager_es_event_handler_es_desc
(
  "init_distant_haze_manager_es",
  "prog/daNetGameLibs/distant_haze/render/distantHazeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_distant_haze_manager_es_event_handler_all_events),
  make_span(init_distant_haze_manager_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc distant_haze_settings_tracking_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__haze"), ecs::ComponentTypeInfo<bool>()}
};
static void distant_haze_settings_tracking_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    distant_haze_settings_tracking_es(evt
        , ECS_RO_COMP(distant_haze_settings_tracking_es_comps, "render_settings__haze", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc distant_haze_settings_tracking_es_es_desc
(
  "distant_haze_settings_tracking_es",
  "prog/daNetGameLibs/distant_haze/render/distantHazeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, distant_haze_settings_tracking_es_all_events),
  empty_span(),
  make_span(distant_haze_settings_tracking_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__haze");
static constexpr ecs::ComponentDesc set_shader_params_ecs_query_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("distant_haze__is_center_fixed"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("distant_haze__center"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("distant_haze__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__fade_in_bottom"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__fade_out_top"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__total_height"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__blur"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__speed"), ecs::ComponentTypeInfo<Point3>()}
};
static ecs::CompileTimeQueryDesc set_shader_params_ecs_query_desc
(
  "set_shader_params_ecs_query",
  empty_span(),
  make_span(set_shader_params_ecs_query_comps+0, 10)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_shader_params_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_shader_params_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__is_center_fixed", bool)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__center", Point2)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__radius", float)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__fade_in_bottom", float)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__fade_out_top", float)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__total_height", float)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__size", float)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__strength", float)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__blur", float)
            , ECS_RO_COMP(set_shader_params_ecs_query_comps, "distant_haze__speed", Point3)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc is_active_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("distant_haze__strength"), ecs::ComponentTypeInfo<float>()},
//start of 9 rq components at [1]
  {ECS_HASH("distant_haze__center"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("distant_haze__speed"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("distant_haze__is_center_fixed"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("distant_haze__blur"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__fade_in_bottom"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__fade_out_top"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("distant_haze__total_height"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc is_active_ecs_query_desc
(
  "is_active_ecs_query",
  empty_span(),
  make_span(is_active_ecs_query_comps+0, 1)/*ro*/,
  make_span(is_active_ecs_query_comps+1, 9)/*rq*/,
  empty_span());
template<typename Callable>
inline void is_active_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_active_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(is_active_ecs_query_comps, "distant_haze__strength", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_heat_haze_lod_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("heat_haze__lod"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc get_heat_haze_lod_ecs_query_desc
(
  "get_heat_haze_lod_ecs_query",
  empty_span(),
  make_span(get_heat_haze_lod_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_heat_haze_lod_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_heat_haze_lod_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_heat_haze_lod_ecs_query_comps, "heat_haze__lod", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc distant_haze_node_init_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("distant_haze__manager"), ecs::ComponentTypeInfo<DistantHazeManager>()},
  {ECS_HASH("distant_haze__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("distant_haze__color_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc distant_haze_node_init_ecs_query_desc
(
  "distant_haze_node_init_ecs_query",
  make_span(distant_haze_node_init_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void distant_haze_node_init_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, distant_haze_node_init_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(distant_haze_node_init_ecs_query_comps, "distant_haze__manager", DistantHazeManager)
            , ECS_RW_COMP(distant_haze_node_init_ecs_query_comps, "distant_haze__node", dabfg::NodeHandle)
            , ECS_RW_COMP(distant_haze_node_init_ecs_query_comps, "distant_haze__color_node", dabfg::NodeHandle)
            );

        }
    }
  );
}
