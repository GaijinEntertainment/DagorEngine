#include "skiesSettingsES.cpp.inl"
ECS_DEF_PULL_VAR(skiesSettings);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc skies_settings_weather_seed_created_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("skies_settings__weatherSeed"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void skies_settings_weather_seed_created_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    skies_settings_weather_seed_created_es_event_handler(evt
        , ECS_RW_COMP(skies_settings_weather_seed_created_es_event_handler_comps, "skies_settings__weatherSeed", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc skies_settings_weather_seed_created_es_event_handler_es_desc
(
  "skies_settings_weather_seed_created_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, skies_settings_weather_seed_created_es_event_handler_all_events),
  make_span(skies_settings_weather_seed_created_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(skies_settings_weather_seed_created_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,nullptr,"clouds_form_es_event_handler,clouds_weather_gen_es_event_handler,sky_atmosphere_es_event_handler,strata_clouds_es_event_handler");
static constexpr ecs::ComponentDesc skies_settings_weather_seed_changed_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("skies_settings__weatherSeed"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void skies_settings_weather_seed_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    skies_settings_weather_seed_changed_es_event_handler(evt
        , ECS_RO_COMP(skies_settings_weather_seed_changed_es_event_handler_comps, "skies_settings__weatherSeed", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc skies_settings_weather_seed_changed_es_event_handler_es_desc
(
  "skies_settings_weather_seed_changed_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, skies_settings_weather_seed_changed_es_event_handler_all_events),
  empty_span(),
  make_span(skies_settings_weather_seed_changed_es_event_handler_comps+0, 1)/*ro*/,
  make_span(skies_settings_weather_seed_changed_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"skies_settings__weatherSeed");
static constexpr ecs::ComponentDesc skies_settings_skies_loaded_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("skies_settings__weatherSeed"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void skies_settings_skies_loaded_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SkiesLoaded>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    skies_settings_skies_loaded_es_event_handler(static_cast<const SkiesLoaded&>(evt)
        , ECS_RW_COMP(skies_settings_skies_loaded_es_event_handler_comps, "skies_settings__weatherSeed", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc skies_settings_skies_loaded_es_event_handler_es_desc
(
  "skies_settings_skies_loaded_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, skies_settings_skies_loaded_es_event_handler_all_events),
  make_span(skies_settings_skies_loaded_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(skies_settings_skies_loaded_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded>::build(),
  0
,nullptr,nullptr,"clouds_form_es_event_handler,clouds_weather_gen_es_event_handler,sky_atmosphere_es_event_handler,strata_clouds_es_event_handler");
static constexpr ecs::ComponentDesc force_panoramic_sky_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("clouds_settings__force_panorama"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void force_panoramic_sky_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    force_panoramic_sky_es_event_handler(evt
        , ECS_RO_COMP(force_panoramic_sky_es_event_handler_comps, "clouds_settings__force_panorama", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc force_panoramic_sky_es_event_handler_es_desc
(
  "force_panoramic_sky_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, force_panoramic_sky_es_event_handler_all_events),
  empty_span(),
  make_span(force_panoramic_sky_es_event_handler_comps+0, 1)/*ro*/,
  make_span(force_panoramic_sky_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*","clouds_form_es_event_handler,clouds_settings_es_event_handler");
static constexpr ecs::ComponentDesc clouds_rendering_es_event_handler_comps[] =
{
//start of 9 ro components at [0]
  {ECS_HASH("clouds_rendering__forward_eccentricity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_rendering__back_eccentricity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_rendering__forward_eccentricity_weight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_rendering__erosion_noise_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_rendering__ambient_desaturation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_rendering__ms_contribution"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_rendering__ms_attenuation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_rendering__ms_ecc_attenuation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_rendering__erosionWindSpeed"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [9]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void clouds_rendering_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    clouds_rendering_es_event_handler(evt
        , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__forward_eccentricity", float)
    , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__back_eccentricity", float)
    , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__forward_eccentricity_weight", float)
    , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__erosion_noise_size", float)
    , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__ambient_desaturation", float)
    , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__ms_contribution", float)
    , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__ms_attenuation", float)
    , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__ms_ecc_attenuation", float)
    , ECS_RO_COMP(clouds_rendering_es_event_handler_comps, "clouds_rendering__erosionWindSpeed", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc clouds_rendering_es_event_handler_es_desc
(
  "clouds_rendering_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, clouds_rendering_es_event_handler_all_events),
  empty_span(),
  make_span(clouds_rendering_es_event_handler_comps+0, 9)/*ro*/,
  make_span(clouds_rendering_es_event_handler_comps+9, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc strata_clouds_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("strata_clouds__amount"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("strata_clouds__altitude"), ecs::ComponentTypeInfo<Point2>()},
//start of 1 rq components at [2]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void strata_clouds_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    strata_clouds_es_event_handler(evt
        , ECS_RO_COMP(strata_clouds_es_event_handler_comps, "strata_clouds__amount", Point2)
    , ECS_RO_COMP(strata_clouds_es_event_handler_comps, "strata_clouds__altitude", Point2)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc strata_clouds_es_event_handler_es_desc
(
  "strata_clouds_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, strata_clouds_es_event_handler_all_events),
  empty_span(),
  make_span(strata_clouds_es_event_handler_comps+0, 2)/*ro*/,
  make_span(strata_clouds_es_event_handler_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc strata_clouds_tex_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("strata_clouds__tex"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void strata_clouds_tex_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    strata_clouds_tex_es(evt
        , ECS_RO_COMP(strata_clouds_tex_es_comps, "strata_clouds__tex", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc strata_clouds_tex_es_es_desc
(
  "strata_clouds_tex_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, strata_clouds_tex_es_all_events),
  empty_span(),
  make_span(strata_clouds_tex_es_comps+0, 1)/*ro*/,
  make_span(strata_clouds_tex_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc clouds_form_es_event_handler_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("clouds_form__layers"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("clouds_form__extinction"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("clouds_form__turbulenceStrength"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("clouds_form__shapeNoiseScale"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("clouds_form__cumulonimbusShapeScale"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("clouds_form__turbulenceFreq"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [6]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void clouds_form_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    clouds_form_es_event_handler(evt
        , ECS_RO_COMP(clouds_form_es_event_handler_comps, "clouds_form__layers", ecs::Array)
    , ECS_RO_COMP(clouds_form_es_event_handler_comps, "clouds_form__extinction", Point2)
    , ECS_RO_COMP(clouds_form_es_event_handler_comps, "clouds_form__turbulenceStrength", Point2)
    , ECS_RO_COMP(clouds_form_es_event_handler_comps, "clouds_form__shapeNoiseScale", int)
    , ECS_RO_COMP(clouds_form_es_event_handler_comps, "clouds_form__cumulonimbusShapeScale", int)
    , ECS_RO_COMP(clouds_form_es_event_handler_comps, "clouds_form__turbulenceFreq", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc clouds_form_es_event_handler_es_desc
(
  "clouds_form_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, clouds_form_es_event_handler_all_events),
  empty_span(),
  make_span(clouds_form_es_event_handler_comps+0, 6)/*ro*/,
  make_span(clouds_form_es_event_handler_comps+6, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc clouds_settings_es_event_handler_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("clouds_settings__maximum_averaging_ratio"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_settings__quality"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("clouds_settings__target_quality"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("clouds_settings__competitive_advantage"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("clouds_settings__fastEvolution"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [5]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void clouds_settings_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    clouds_settings_es_event_handler(evt
        , ECS_RO_COMP(clouds_settings_es_event_handler_comps, "clouds_settings__maximum_averaging_ratio", float)
    , ECS_RO_COMP(clouds_settings_es_event_handler_comps, "clouds_settings__quality", int)
    , ECS_RO_COMP(clouds_settings_es_event_handler_comps, "clouds_settings__target_quality", int)
    , ECS_RO_COMP(clouds_settings_es_event_handler_comps, "clouds_settings__competitive_advantage", bool)
    , ECS_RO_COMP(clouds_settings_es_event_handler_comps, "clouds_settings__fastEvolution", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc clouds_settings_es_event_handler_es_desc
(
  "clouds_settings_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, clouds_settings_es_event_handler_all_events),
  empty_span(),
  make_span(clouds_settings_es_event_handler_comps+0, 5)/*ro*/,
  make_span(clouds_settings_es_event_handler_comps+5, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc clouds_weather_gen_es_event_handler_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("clouds_weather_gen__layers"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("clouds_weather_gen__epicness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("clouds_weather_gen__cumulonimbusCoverage"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("clouds_weather_gen__cumulonimbusSeed"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("clouds_weather_gen__worldSize"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [5]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void clouds_weather_gen_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    clouds_weather_gen_es_event_handler(evt
        , ECS_RO_COMP(clouds_weather_gen_es_event_handler_comps, "clouds_weather_gen__layers", ecs::Array)
    , ECS_RO_COMP(clouds_weather_gen_es_event_handler_comps, "clouds_weather_gen__epicness", float)
    , ECS_RO_COMP(clouds_weather_gen_es_event_handler_comps, "clouds_weather_gen__cumulonimbusCoverage", Point2)
    , ECS_RO_COMP(clouds_weather_gen_es_event_handler_comps, "clouds_weather_gen__cumulonimbusSeed", Point2)
    , ECS_RO_COMP(clouds_weather_gen_es_event_handler_comps, "clouds_weather_gen__worldSize", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc clouds_weather_gen_es_event_handler_es_desc
(
  "clouds_weather_gen_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, clouds_weather_gen_es_event_handler_all_events),
  empty_span(),
  make_span(clouds_weather_gen_es_event_handler_comps+0, 5)/*ro*/,
  make_span(clouds_weather_gen_es_event_handler_comps+5, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc sky_atmosphere_es_event_handler_comps[] =
{
//start of 29 ro components at [0]
  {ECS_HASH("sky_atmosphere__average_ground_albedo"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__min_ground_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_settings__haze_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_settings__haze_min_angle"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_settings__haze_peak_hour_offset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__ground_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("sky_atmosphere__mie2_thickness"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__mie2_altitude"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__mie2_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__mie_height"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__mie_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__mie_absorption_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__mie_scattering_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("sky_atmosphere__mie_absorption_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("sky_atmosphere__mie_assymetry"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__mie_back_assymetry"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__planet_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__atmosphere_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__rayleigh_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__rayleigh_alt_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__rayleigh_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("sky_atmosphere__multiple_scattering_factor"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__ozone_alt_dist"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__ozone_max_alt"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__ozone_scale"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("sky_atmosphere__sun_brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__moon_brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sky_atmosphere__moon_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("sky_disable_sky_influence"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [29]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void sky_atmosphere_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    sky_atmosphere_es_event_handler(evt
        , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__average_ground_albedo", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__min_ground_offset", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_settings__haze_strength", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_settings__haze_min_angle", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_settings__haze_peak_hour_offset", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__ground_color", Point3)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie2_thickness", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie2_altitude", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie2_scale", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie_height", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie_scale", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie_absorption_scale", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie_scattering_color", Point3)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie_absorption_color", Point3)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie_assymetry", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__mie_back_assymetry", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__planet_scale", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__atmosphere_scale", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__rayleigh_scale", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__rayleigh_alt_scale", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__rayleigh_color", Point3)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__multiple_scattering_factor", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__ozone_alt_dist", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__ozone_max_alt", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__ozone_scale", Point2)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__sun_brightness", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__moon_brightness", float)
    , ECS_RO_COMP(sky_atmosphere_es_event_handler_comps, "sky_atmosphere__moon_color", Point3)
    , ECS_RO_COMP_OR(sky_atmosphere_es_event_handler_comps, "sky_disable_sky_influence", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc sky_atmosphere_es_event_handler_es_desc
(
  "sky_atmosphere_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, sky_atmosphere_es_event_handler_all_events),
  empty_span(),
  make_span(sky_atmosphere_es_event_handler_comps+0, 29)/*ro*/,
  make_span(sky_atmosphere_es_event_handler_comps+29, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc sky_settings_altitude_ofs_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("sky_coord_frame__altitude_offset"), ecs::ComponentTypeInfo<float>()}
};
static void sky_settings_altitude_ofs_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    sky_settings_altitude_ofs_es_event_handler(evt
        , ECS_RO_COMP(sky_settings_altitude_ofs_es_event_handler_comps, "sky_coord_frame__altitude_offset", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc sky_settings_altitude_ofs_es_event_handler_es_desc
(
  "sky_settings_altitude_ofs_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, sky_settings_altitude_ofs_es_event_handler_all_events),
  empty_span(),
  make_span(sky_settings_altitude_ofs_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"sky_coord_frame__altitude_offset");
static constexpr ecs::ComponentDesc use_clouds_hole_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("use_clouds_hole"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void use_clouds_hole_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    use_clouds_hole_es_event_handler(evt
        , ECS_RO_COMP(use_clouds_hole_es_event_handler_comps, "use_clouds_hole", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc use_clouds_hole_es_event_handler_es_desc
(
  "use_clouds_hole_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, use_clouds_hole_es_event_handler_all_events),
  empty_span(),
  make_span(use_clouds_hole_es_event_handler_comps+0, 1)/*ro*/,
  make_span(use_clouds_hole_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"use_clouds_hole");
static constexpr ecs::ComponentDesc clouds_hole_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("density"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [2]
  {ECS_HASH("clouds_hole_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void clouds_hole_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    clouds_hole_es_event_handler(evt
        , ECS_RO_COMP(clouds_hole_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP(clouds_hole_es_event_handler_comps, "density", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc clouds_hole_es_event_handler_es_desc
(
  "clouds_hole_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, clouds_hole_es_event_handler_all_events),
  empty_span(),
  make_span(clouds_hole_es_event_handler_comps+0, 2)/*ro*/,
  make_span(clouds_hole_es_event_handler_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"density,transform");
static constexpr ecs::ComponentDesc moon_params_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("moon_res_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("moon_size"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [2]
  {ECS_HASH("moon_params"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void moon_params_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    moon_params_es_event_handler(evt
        , ECS_RO_COMP(moon_params_es_event_handler_comps, "moon_res_name", ecs::string)
    , ECS_RO_COMP(moon_params_es_event_handler_comps, "moon_size", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc moon_params_es_event_handler_es_desc
(
  "moon_params_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, moon_params_es_event_handler_all_events),
  empty_span(),
  make_span(moon_params_es_event_handler_comps+0, 2)/*ro*/,
  make_span(moon_params_es_event_handler_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<SkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc randomize_skies_setting_seed_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("skies_settings__weatherSeed"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [1]
  {ECS_HASH("randomize_seed_button"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void randomize_skies_setting_seed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    randomize_skies_setting_seed_es_event_handler(evt
        , ECS_RO_COMP(randomize_skies_setting_seed_es_event_handler_comps, "randomize_seed_button", bool)
    , ECS_RW_COMP(randomize_skies_setting_seed_es_event_handler_comps, "skies_settings__weatherSeed", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc randomize_skies_setting_seed_es_event_handler_es_desc
(
  "randomize_skies_setting_seed_es",
  "prog/daNetGame/render/skiesSettingsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, randomize_skies_setting_seed_es_event_handler_all_events),
  make_span(randomize_skies_setting_seed_es_event_handler_comps+0, 1)/*rw*/,
  make_span(randomize_skies_setting_seed_es_event_handler_comps+1, 1)/*ro*/,
  make_span(randomize_skies_setting_seed_es_event_handler_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"dev","randomize_seed_button");
static constexpr ecs::ComponentDesc is_cloud_hole_enabled_for_preset_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("level__cloudsHoleEnabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc is_cloud_hole_enabled_for_preset_ecs_query_desc
(
  "is_cloud_hole_enabled_for_preset_ecs_query",
  empty_span(),
  make_span(is_cloud_hole_enabled_for_preset_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void is_cloud_hole_enabled_for_preset_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_cloud_hole_enabled_for_preset_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(is_cloud_hole_enabled_for_preset_ecs_query_comps, "level__cloudsHoleEnabled", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc is_panorama_forced_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("clouds_settings__force_panorama"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc is_panorama_forced_ecs_query_desc
(
  "is_panorama_forced_ecs_query",
  empty_span(),
  make_span(is_panorama_forced_ecs_query_comps+0, 1)/*ro*/,
  make_span(is_panorama_forced_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void is_panorama_forced_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_panorama_forced_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(is_panorama_forced_ecs_query_comps, "clouds_settings__force_panorama", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc validate_volumetric_clouds_settings_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("clouds_form__layers"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("clouds_settings__force_panorama"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("skies_settings_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc validate_volumetric_clouds_settings_ecs_query_desc
(
  "validate_volumetric_clouds_settings_ecs_query",
  empty_span(),
  make_span(validate_volumetric_clouds_settings_ecs_query_comps+0, 2)/*ro*/,
  make_span(validate_volumetric_clouds_settings_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void validate_volumetric_clouds_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, validate_volumetric_clouds_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(validate_volumetric_clouds_settings_ecs_query_comps, "clouds_form__layers", ecs::Array)
            , ECS_RO_COMP(validate_volumetric_clouds_settings_ecs_query_comps, "clouds_settings__force_panorama", bool)
            );

        }while (++comp != compE);
    }
  );
}
