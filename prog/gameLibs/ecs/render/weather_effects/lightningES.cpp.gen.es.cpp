#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t animchar__res_get_type();
static ecs::LTComponentList animchar__res_component(ECS_HASH("animchar__res"), animchar__res_get_type(), "prog/gameLibs/ecs/render/weather_effects/lightningES.cpp.inl", "lightning_manager_created_es", 0);
static constexpr ecs::component_t animchar_render__enabled_get_type();
static ecs::LTComponentList animchar_render__enabled_component(ECS_HASH("animchar_render__enabled"), animchar_render__enabled_get_type(), "prog/gameLibs/ecs/render/weather_effects/lightningES.cpp.inl", "lightning_manager_created_es", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "lightningES.cpp.inl"
ECS_DEF_PULL_VAR(lightning);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc lightning_update_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("lightning"), ecs::ComponentTypeInfo<LightningFX>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("lightning__animchars_eids"), ecs::ComponentTypeInfo<ecs::EidList>()},
//start of 38 ro components at [3]
  {ECS_HASH("lightning__is_volumetric"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lightning__base_strike_time_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__base_sleep_time_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__base_distance_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__base_azimuth_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__base_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__series_radius_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__series_strength_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__series_size_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__series_sleep_time_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__series_strike_time_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__series_distance_deviation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__series_azimuth_deviation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__series_fadeout_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__series_probability"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__series_create_bolt"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lightning__bolt_probability"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__bolt_strike_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__bolt_step_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("lightning__emissive_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__emissive_fadein_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__emissive_fadeout_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__vert_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__vert_noise_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__vert_noise_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__vert_noise_speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__point_light_fadeout_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__point_light_flickering_probability"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__point_light_flickering_speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__point_light_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__point_light_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__point_light_strength_interval"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("lightning__point_light_extinction_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__point_light_natural_fade"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lightning__point_light_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("lightning__scene_illumination_multiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("lightning__scene_illumination_enable_for_flash"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("lightning__scene_illumination_near_sun_threshold"), ecs::ComponentTypeInfo<float>()}
};
static void lightning_update_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    lightning_update_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(lightning_update_es_comps, "lightning", LightningFX)
    , ECS_RW_COMP(lightning_update_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__is_volumetric", bool)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__base_strike_time_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__base_sleep_time_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__base_distance_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__base_azimuth_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__base_offset", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_radius_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_strength_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_size_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_sleep_time_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_strike_time_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_distance_deviation", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_azimuth_deviation", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_fadeout_time", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_probability", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__series_create_bolt", bool)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__bolt_probability", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__bolt_strike_time", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__bolt_step_size", int)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__emissive_multiplier", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__emissive_fadein_time", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__emissive_fadeout_time", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__vert_noise_scale", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__vert_noise_strength", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__vert_noise_time", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__vert_noise_speed", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_fadeout_time", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_flickering_probability", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_flickering_speed", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_offset", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_radius", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_strength_interval", Point2)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_extinction_threshold", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_natural_fade", bool)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__point_light_color", Point3)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__scene_illumination_multiplier", float)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__scene_illumination_enable_for_flash", bool)
    , ECS_RO_COMP(lightning_update_es_comps, "lightning__scene_illumination_near_sun_threshold", float)
    , ECS_RW_COMP(lightning_update_es_comps, "lightning__animchars_eids", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lightning_update_es_es_desc
(
  "lightning_update_es",
  "prog/gameLibs/ecs/render/weather_effects/lightningES.cpp.inl",
  ecs::EntitySystemOps(lightning_update_es_all),
  make_span(lightning_update_es_comps+0, 3)/*rw*/,
  make_span(lightning_update_es_comps+3, 38)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc lightning_manager_destroy_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("lightning__animchars_eids_base"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static void lightning_manager_destroy_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lightning_manager_destroy_es(evt
        , ECS_RW_COMP(lightning_manager_destroy_es_comps, "lightning__animchars_eids_base", ecs::EidList)
    , components.manager()
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lightning_manager_destroy_es_es_desc
(
  "lightning_manager_destroy_es",
  "prog/gameLibs/ecs/render/weather_effects/lightningES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lightning_manager_destroy_es_all_events),
  make_span(lightning_manager_destroy_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc lightning_manager_created_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("lightning__animchars_eids_base"), ecs::ComponentTypeInfo<ecs::EidList>()},
//start of 1 ro components at [1]
  {ECS_HASH("lightning__animchars"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static void lightning_manager_created_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lightning_manager_created_es(evt
        , ECS_RO_COMP(lightning_manager_created_es_comps, "lightning__animchars", ecs::StringList)
    , ECS_RW_COMP(lightning_manager_created_es_comps, "lightning__animchars_eids_base", ecs::EidList)
    , components.manager()
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lightning_manager_created_es_es_desc
(
  "lightning_manager_created_es",
  "prog/gameLibs/ecs/render/weather_effects/lightningES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lightning_manager_created_es_all_events),
  make_span(lightning_manager_created_es_comps+0, 1)/*rw*/,
  make_span(lightning_manager_created_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc lightning_created_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("lightning"), ecs::ComponentTypeInfo<LightningFX>()},
  {ECS_HASH("lightning__animchars_eids"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static void lightning_created_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lightning_created_es(evt
        , ECS_RW_COMP(lightning_created_es_comps, "lightning", LightningFX)
    , ECS_RW_COMP(lightning_created_es_comps, "lightning__animchars_eids", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lightning_created_es_es_desc
(
  "lightning_created_es",
  "prog/gameLibs/ecs/render/weather_effects/lightningES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lightning_created_es_all_events),
  make_span(lightning_created_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_lightnings_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("lightning__animchars_eids"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static ecs::CompileTimeQueryDesc get_lightnings_ecs_query_desc
(
  "get_lightnings_ecs_query",
  make_span(get_lightnings_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_lightnings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_lightnings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_lightnings_ecs_query_comps, "lightning__animchars_eids", ecs::EidList)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_animchar_eids_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("lightning__animchars_eids_base"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static ecs::CompileTimeQueryDesc get_animchar_eids_ecs_query_desc
(
  "get_animchar_eids_ecs_query",
  make_span(get_animchar_eids_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_animchar_eids_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_animchar_eids_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_animchar_eids_ecs_query_comps, "lightning__animchars_eids_base", ecs::EidList)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc disable_lightning_animchar_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc disable_lightning_animchar_ecs_query_desc
(
  "disable_lightning_animchar_ecs_query",
  make_span(disable_lightning_animchar_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void disable_lightning_animchar_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, disable_lightning_animchar_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(disable_lightning_animchar_ecs_query_comps, "animchar_render__enabled", bool)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc enable_lightning_animchar_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("animchar_render__dist_sq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc enable_lightning_animchar_ecs_query_desc
(
  "enable_lightning_animchar_ecs_query",
  make_span(enable_lightning_animchar_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void enable_lightning_animchar_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, enable_lightning_animchar_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(enable_lightning_animchar_ecs_query_comps, "animchar_render__enabled", bool)
            , ECS_RW_COMP(enable_lightning_animchar_ecs_query_comps, "animchar_render__dist_sq", float)
            , ECS_RW_COMP(enable_lightning_animchar_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
static constexpr ecs::component_t animchar__res_get_type(){return ecs::ComponentTypeInfo<eastl::basic_string<char, eastl::allocator>>::type; }
static constexpr ecs::component_t animchar_render__enabled_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
