#include "ssssES.cpp.inl"
ECS_DEF_PULL_VAR(ssss);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc ssss_settings_tracking_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("render_settings__ssssQuality"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("render_settings__forwardRendering"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__combinedShadows"), ecs::ComponentTypeInfo<bool>()}
};
static void ssss_settings_tracking_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ssss_settings_tracking_es(evt
        , ECS_RO_COMP(ssss_settings_tracking_es_comps, "render_settings__ssssQuality", ecs::string)
    , ECS_RO_COMP(ssss_settings_tracking_es_comps, "render_settings__forwardRendering", bool)
    , ECS_RO_COMP(ssss_settings_tracking_es_comps, "render_settings__combinedShadows", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ssss_settings_tracking_es_es_desc
(
  "ssss_settings_tracking_es",
  "prog/daNetGame/render/world/ssssES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ssss_settings_tracking_es_all_events),
  empty_span(),
  make_span(ssss_settings_tracking_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,nullptr,"render_settings__combinedShadows,render_settings__forwardRendering,render_settings__ssssQuality");
static constexpr ecs::ComponentDesc ssss_created_or_params_changed_es_event_handler_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("transmittance__thickness_bias"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__thickness_min"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__thickness_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__translucency_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__blur_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__sample_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("transmittance__normal_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("reflectance__blur_width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("reflectance__blur_quality"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("reflectance__blur_follow_surface"), ecs::ComponentTypeInfo<bool>()}
};
static void ssss_created_or_params_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ssss_created_or_params_changed_es_event_handler(evt
        , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "transmittance__thickness_bias", float)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "transmittance__thickness_min", float)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "transmittance__thickness_scale", float)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "transmittance__translucency_scale", float)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "transmittance__blur_scale", float)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "transmittance__sample_count", int)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "transmittance__normal_offset", float)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "reflectance__blur_width", float)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "reflectance__blur_quality", int)
    , ECS_RO_COMP(ssss_created_or_params_changed_es_event_handler_comps, "reflectance__blur_follow_surface", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ssss_created_or_params_changed_es_event_handler_es_desc
(
  "ssss_created_or_params_changed_es",
  "prog/daNetGame/render/world/ssssES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ssss_created_or_params_changed_es_event_handler_all_events),
  empty_span(),
  make_span(ssss_created_or_params_changed_es_event_handler_comps+0, 10)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc ssss_enabled_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("render_settings__ssssQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc ssss_enabled_ecs_query_desc
(
  "ssss_enabled_ecs_query",
  make_span(ssss_enabled_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void ssss_enabled_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, ssss_enabled_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(ssss_enabled_ecs_query_comps, "render_settings__ssssQuality", ecs::string)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc ssss_init_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("ssss__horizontal_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("ssss__vertical_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc ssss_init_ecs_query_desc
(
  "ssss_init_ecs_query",
  make_span(ssss_init_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void ssss_init_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, ssss_init_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(ssss_init_ecs_query_comps, "ssss__horizontal_node", dabfg::NodeHandle)
            , ECS_RW_COMP(ssss_init_ecs_query_comps, "ssss__vertical_node", dabfg::NodeHandle)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc ssss_params_ecs_query_comps[] =
{
//start of 10 rw components at [0]
  {ECS_HASH("transmittance__thickness_bias"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__thickness_min"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__thickness_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__translucency_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__blur_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transmittance__sample_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("transmittance__normal_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("reflectance__blur_width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("reflectance__blur_quality"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("reflectance__blur_follow_surface"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc ssss_params_ecs_query_desc
(
  "ssss_params_ecs_query",
  make_span(ssss_params_ecs_query_comps+0, 10)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void ssss_params_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, ssss_params_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(ssss_params_ecs_query_comps, "transmittance__thickness_bias", float)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "transmittance__thickness_min", float)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "transmittance__thickness_scale", float)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "transmittance__translucency_scale", float)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "transmittance__blur_scale", float)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "transmittance__sample_count", int)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "transmittance__normal_offset", float)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "reflectance__blur_width", float)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "reflectance__blur_quality", int)
            , ECS_RW_COMP(ssss_params_ecs_query_comps, "reflectance__blur_follow_surface", bool)
            );

        }
    }
  );
}
