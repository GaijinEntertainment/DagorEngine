// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "lensFlareCommonES.cpp.inl"
ECS_DEF_PULL_VAR(lensFlareCommon);
#include <daECS/core/internal/performQuery.h>
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
  "prog/gameLibs/render/lensFlare/render/lensFlareCommonES.cpp.inl",
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
  "prog/gameLibs/render/lensFlare/render/lensFlareCommonES.cpp.inl",
  ecs::EntitySystemOps(nullptr, point_light_flare_provider_invalidate_es_all_events),
  make_span(point_light_flare_provider_invalidate_es_comps+0, 2)/*rw*/,
  empty_span(),
  make_span(point_light_flare_provider_invalidate_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"dynamic_light_lens_flare__flare_config");
static constexpr ecs::ComponentDesc point_lens_flare_provider_changed_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("point_lens_flare_provider__cached_params"), ecs::ComponentTypeInfo<Point2>()},
//start of 2 ro components at [1]
  {ECS_HASH("point_lens_flare_provider__angle_cutoff_deg"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("point_lens_flare_provider__angle_fade_start_deg"), ecs::ComponentTypeInfo<float>()}
};
static void point_lens_flare_provider_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    point_lens_flare_provider_changed_es(evt
        , ECS_RW_COMP(point_lens_flare_provider_changed_es_comps, "point_lens_flare_provider__cached_params", Point2)
    , ECS_RO_COMP(point_lens_flare_provider_changed_es_comps, "point_lens_flare_provider__angle_cutoff_deg", float)
    , ECS_RO_COMP(point_lens_flare_provider_changed_es_comps, "point_lens_flare_provider__angle_fade_start_deg", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc point_lens_flare_provider_changed_es_es_desc
(
  "point_lens_flare_provider_changed_es",
  "prog/gameLibs/render/lensFlare/render/lensFlareCommonES.cpp.inl",
  ecs::EntitySystemOps(nullptr, point_lens_flare_provider_changed_es_all_events),
  make_span(point_lens_flare_provider_changed_es_comps+0, 1)/*rw*/,
  make_span(point_lens_flare_provider_changed_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","point_lens_flare_provider__angle_cutoff_deg,point_lens_flare_provider__angle_fade_start_deg");
static constexpr ecs::ComponentDesc lens_flare_config_on_appear_es_comps[] =
{
//start of 9 rq components at [0]
  {ECS_HASH("lens_flare_config__use_occlusion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lens_flare_config__scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lens_flare_config__elements"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("lens_flare_config__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("lens_flare_config__depth_bias"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__exposure_reduction"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__smooth_screen_fadeout_distance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__spotlight_cone_angle_deg"), ecs::ComponentTypeInfo<float>()}
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
  "prog/gameLibs/render/lensFlare/render/lensFlareCommonES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_config_on_appear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(lens_flare_config_on_appear_es_comps+0, 9)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","lens_flare_config__depth_bias,lens_flare_config__elements,lens_flare_config__exposure_reduction,lens_flare_config__intensity,lens_flare_config__name,lens_flare_config__scale,lens_flare_config__smooth_screen_fadeout_distance,lens_flare_config__spotlight_cone_angle_deg,lens_flare_config__use_occlusion");
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
  "prog/gameLibs/render/lensFlare/render/lensFlareCommonES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_config_on_disappear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(lens_flare_config_on_disappear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
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
static constexpr ecs::ComponentDesc prepare_point_flares_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("point_lens_flare_provider__cached_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("point_lens_flare_provider__cached_flare_config_id"), ecs::ComponentTypeInfo<int>()},
//start of 11 ro components at [2]
  {ECS_HASH("enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("point_lens_flare_provider__flare_config"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("point_lens_flare_provider__color"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("point_lens_flare_provider__intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("point_lens_flare_provider__distance_attenuation"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("point_lens_flare_provider__distance_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("point_lens_flare_provider__distance_cutoff"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("point_lens_flare_provider__angle_attenuation"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("point_lens_flare_provider__cached_params"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("point_lens_flare_provider__dir"), ecs::ComponentTypeInfo<Point3>()}
};
static ecs::CompileTimeQueryDesc prepare_point_flares_ecs_query_desc
(
  "prepare_point_flares_ecs_query",
  make_span(prepare_point_flares_ecs_query_comps+0, 2)/*rw*/,
  make_span(prepare_point_flares_ecs_query_comps+2, 11)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void prepare_point_flares_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, prepare_point_flares_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "enabled", bool)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__flare_config", ecs::string)
            , ECS_RW_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__cached_id", int)
            , ECS_RW_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__cached_flare_config_id", int)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__color", E3DCOLOR)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__intensity", float)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__distance_attenuation", bool)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__distance_offset", float)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__distance_cutoff", float)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__angle_attenuation", bool)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__cached_params", Point2)
            , ECS_RO_COMP(prepare_point_flares_ecs_query_comps, "point_lens_flare_provider__dir", Point3)
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
//start of 9 ro components at [0]
  {ECS_HASH("lens_flare_config__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("lens_flare_config__smooth_screen_fadeout_distance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lens_flare_config__intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__use_occlusion"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lens_flare_config__depth_bias"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__spotlight_cone_angle_deg"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__exposure_reduction"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lens_flare_config__elements"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static ecs::CompileTimeQueryDesc gather_flare_configs_ecs_query_desc
(
  "gather_flare_configs_ecs_query",
  empty_span(),
  make_span(gather_flare_configs_ecs_query_comps+0, 9)/*ro*/,
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
            , ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__depth_bias", float)
            , ECS_RO_COMP(gather_flare_configs_ecs_query_comps, "lens_flare_config__spotlight_cone_angle_deg", float)
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
