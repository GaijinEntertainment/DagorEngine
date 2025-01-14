#include "lensFlareES.cpp.inl"
ECS_DEF_PULL_VAR(lensFlare);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc lens_flare_renderer_on_appear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()}
};
static void lens_flare_renderer_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lens_flare_renderer_on_appear_es(evt
        , ECS_RW_COMP(lens_flare_renderer_on_appear_es_comps, "lens_flare_renderer", LensFlareRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lens_flare_renderer_on_appear_es_es_desc
(
  "lens_flare_renderer_on_appear_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_renderer_on_appear_es_all_events),
  make_span(lens_flare_renderer_on_appear_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc lens_flare_renderer_on_settings_changed_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__lensFlares"), ecs::ComponentTypeInfo<bool>()}
};
static void lens_flare_renderer_on_settings_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lens_flare_renderer_on_settings_changed_es(evt
        , ECS_RO_COMP(lens_flare_renderer_on_settings_changed_es_comps, "render_settings__lensFlares", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lens_flare_renderer_on_settings_changed_es_es_desc
(
  "lens_flare_renderer_on_settings_changed_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_renderer_on_settings_changed_es_all_events),
  empty_span(),
  make_span(lens_flare_renderer_on_settings_changed_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__lensFlares");
static constexpr ecs::ComponentDesc sun_flare_provider_invalidate_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("sun_flare_provider__cached_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("sun_flare_provider__cached_flare_config_id"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [2]
  {ECS_HASH("sun_flare_provider__flare_config"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void sun_flare_provider_invalidate_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    sun_flare_provider_invalidate_es(evt
        , ECS_RW_COMP(sun_flare_provider_invalidate_es_comps, "sun_flare_provider__cached_id", int)
    , ECS_RW_COMP(sun_flare_provider_invalidate_es_comps, "sun_flare_provider__cached_flare_config_id", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc sun_flare_provider_invalidate_es_es_desc
(
  "sun_flare_provider_invalidate_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, sun_flare_provider_invalidate_es_all_events),
  make_span(sun_flare_provider_invalidate_es_comps+0, 2)/*rw*/,
  empty_span(),
  make_span(sun_flare_provider_invalidate_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","sun_flare_provider__flare_config");
static constexpr ecs::ComponentDesc point_light_flare_provider_invalidate_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("dynamic_light_lens_flare__cached_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dynamic_light_lens_flare__cached_flare_config_id"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [2]
  {ECS_HASH("dynamic_light_lens_flare__flare_config"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void point_light_flare_provider_invalidate_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    point_light_flare_provider_invalidate_es(evt
        , ECS_RW_COMP(point_light_flare_provider_invalidate_es_comps, "dynamic_light_lens_flare__cached_id", int)
    , ECS_RW_COMP(point_light_flare_provider_invalidate_es_comps, "dynamic_light_lens_flare__cached_flare_config_id", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc point_light_flare_provider_invalidate_es_es_desc
(
  "point_light_flare_provider_invalidate_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, point_light_flare_provider_invalidate_es_all_events),
  make_span(point_light_flare_provider_invalidate_es_comps+0, 2)/*rw*/,
  empty_span(),
  make_span(point_light_flare_provider_invalidate_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"dynamic_light_lens_flare__flare_config");
static constexpr ecs::ComponentDesc lens_flare_config_on_appear_es_comps[] =
{
//start of 7 rq components at [0]
  {ECS_HASH("lens_flare_config__use_occlusion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lens_flare_config__scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lens_flare_config__elements"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("lens_flare_config__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("lens_flare_config__exposure_reduction"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__smooth_screen_fadeout_distance"), ecs::ComponentTypeInfo<float>()}
};
static void lens_flare_config_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  lens_flare_config_on_appear_es(evt
        );
}
static ecs::EntitySystemDesc lens_flare_config_on_appear_es_es_desc
(
  "lens_flare_config_on_appear_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_config_on_appear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(lens_flare_config_on_appear_es_comps+0, 7)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","lens_flare_config__elements,lens_flare_config__exposure_reduction,lens_flare_config__intensity,lens_flare_config__name,lens_flare_config__scale,lens_flare_config__smooth_screen_fadeout_distance,lens_flare_config__use_occlusion");
static constexpr ecs::ComponentDesc lens_flare_config_on_disappear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("lens_flare_config__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void lens_flare_config_on_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  lens_flare_config_on_disappear_es(evt
        );
}
static ecs::EntitySystemDesc lens_flare_config_on_disappear_es_es_desc
(
  "lens_flare_config_on_disappear_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_config_on_disappear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(lens_flare_config_on_disappear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc register_lens_flare_for_postfx_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()}
};
static void register_lens_flare_for_postfx_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RegisterPostfxResources>());
  register_lens_flare_for_postfx_es(static_cast<const RegisterPostfxResources&>(evt)
        );
}
static ecs::EntitySystemDesc register_lens_flare_for_postfx_es_es_desc
(
  "register_lens_flare_for_postfx_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_lens_flare_for_postfx_es_all_events),
  empty_span(),
  empty_span(),
  make_span(register_lens_flare_for_postfx_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<RegisterPostfxResources>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_lens_flare_nodes_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()},
  {ECS_HASH("lens_flare_renderer__render_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("lens_flare_renderer__prepare_lights_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc init_lens_flare_nodes_ecs_query_desc
(
  "init_lens_flare_nodes_ecs_query",
  make_span(init_lens_flare_nodes_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void init_lens_flare_nodes_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, init_lens_flare_nodes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(init_lens_flare_nodes_ecs_query_comps, "lens_flare_renderer", LensFlareRenderer)
            , ECS_RW_COMP(init_lens_flare_nodes_ecs_query_comps, "lens_flare_renderer__render_node", dabfg::NodeHandle)
            , ECS_RW_COMP(init_lens_flare_nodes_ecs_query_comps, "lens_flare_renderer__prepare_lights_node", dabfg::NodeHandle)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc prepare_sun_flares_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("sun_flare_provider__cached_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("sun_flare_provider__cached_flare_config_id"), ecs::ComponentTypeInfo<int>()},
//start of 2 ro components at [2]
  {ECS_HASH("enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("sun_flare_provider__flare_config"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc prepare_sun_flares_ecs_query_desc
(
  "prepare_sun_flares_ecs_query",
  make_span(prepare_sun_flares_ecs_query_comps+0, 2)/*rw*/,
  make_span(prepare_sun_flares_ecs_query_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void prepare_sun_flares_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, prepare_sun_flares_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(prepare_sun_flares_ecs_query_comps, "enabled", bool)
            , ECS_RO_COMP(prepare_sun_flares_ecs_query_comps, "sun_flare_provider__flare_config", ecs::string)
            , ECS_RW_COMP(prepare_sun_flares_ecs_query_comps, "sun_flare_provider__cached_id", int)
            , ECS_RW_COMP(prepare_sun_flares_ecs_query_comps, "sun_flare_provider__cached_flare_config_id", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc prepare_point_light_flares_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("dynamic_light_lens_flare__cached_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dynamic_light_lens_flare__cached_flare_config_id"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [2]
  {ECS_HASH("dynamic_light_lens_flare__flare_config"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc prepare_point_light_flares_ecs_query_desc
(
  "prepare_point_light_flares_ecs_query",
  make_span(prepare_point_light_flares_ecs_query_comps+0, 2)/*rw*/,
  make_span(prepare_point_light_flares_ecs_query_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void prepare_point_light_flares_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, prepare_point_light_flares_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(prepare_point_light_flares_ecs_query_comps, "dynamic_light_lens_flare__flare_config", ecs::string)
            , ECS_RW_COMP(prepare_point_light_flares_ecs_query_comps, "dynamic_light_lens_flare__cached_id", int)
            , ECS_RW_COMP(prepare_point_light_flares_ecs_query_comps, "dynamic_light_lens_flare__cached_flare_config_id", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_flare_configs_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("lens_flare_config__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("lens_flare_config__smooth_screen_fadeout_distance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lens_flare_config__intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__use_occlusion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lens_flare_config__exposure_reduction"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__elements"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static ecs::CompileTimeQueryDesc gather_flare_configs_ecs_query_desc
(
  "gather_flare_configs_ecs_query",
  empty_span(),
  make_span(gather_flare_configs_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_flare_configs_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_flare_configs_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__name", ecs::string)
            , ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__smooth_screen_fadeout_distance", float)
            , ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__scale", Point2)
            , ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__intensity", float)
            , ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__use_occlusion", bool)
            , ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__exposure_reduction", float)
            , ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__elements", ecs::Array)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc schedule_update_flares_renderer_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()}
};
static ecs::CompileTimeQueryDesc schedule_update_flares_renderer_ecs_query_desc
(
  "schedule_update_flares_renderer_ecs_query",
  make_span(schedule_update_flares_renderer_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void schedule_update_flares_renderer_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, schedule_update_flares_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(schedule_update_flares_renderer_ecs_query_comps, "lens_flare_renderer", LensFlareRenderer)
            );

        }while (++comp != compE);
    }
  );
}
