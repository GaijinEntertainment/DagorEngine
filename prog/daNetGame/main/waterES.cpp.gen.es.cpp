#include "waterES.cpp.inl"
ECS_DEF_PULL_VAR(water);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc water_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void water_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    water_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(water_es_comps, "water", FFTWater)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_es_es_desc
(
  "water_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(water_es_all),
  make_span(water_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
);
static constexpr ecs::ComponentDesc set_water_on_level_loaded_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void set_water_on_level_loaded_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_water_on_level_loaded_es(evt
        , ECS_RW_COMP(set_water_on_level_loaded_es_comps, "water", FFTWater)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_water_on_level_loaded_es_es_desc
(
  "set_water_on_level_loaded_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_water_on_level_loaded_es_all_events),
  make_span(set_water_on_level_loaded_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"gameClient",nullptr,"water_strength_es");
static constexpr ecs::ComponentDesc water_level_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()},
//start of 1 ro components at [1]
  {ECS_HASH("water__level"), ecs::ComponentTypeInfo<float>()}
};
static void water_level_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    water_level_es_event_handler(evt
        , ECS_RW_COMP(water_level_es_event_handler_comps, "water", FFTWater)
    , ECS_RO_COMP(water_level_es_event_handler_comps, "water__level", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_level_es_event_handler_es_desc
(
  "water_level_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_level_es_event_handler_all_events),
  make_span(water_level_es_event_handler_comps+0, 1)/*rw*/,
  make_span(water_level_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"water__level",nullptr,"set_water_on_level_loaded_es");
static constexpr ecs::ComponentDesc water_fft_resolution_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()},
//start of 1 ro components at [1]
  {ECS_HASH("water__fft_resolution"), ecs::ComponentTypeInfo<int>()}
};
static void water_fft_resolution_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    water_fft_resolution_es_event_handler(evt
        , ECS_RW_COMP(water_fft_resolution_es_event_handler_comps, "water", FFTWater)
    , ECS_RO_COMP(water_fft_resolution_es_event_handler_comps, "water__fft_resolution", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_fft_resolution_es_event_handler_es_desc
(
  "water_fft_resolution_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_fft_resolution_es_event_handler_all_events),
  make_span(water_fft_resolution_es_event_handler_comps+0, 1)/*rw*/,
  make_span(water_fft_resolution_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"water__fft_resolution");
static constexpr ecs::ComponentDesc water_flowmap_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()},
  {ECS_HASH("water__flowmap_tex"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 14 ro components at [2]
  {ECS_HASH("water__flowmap"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("water__flowmap_area"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("water__wind_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_range"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_fading"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_damping"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_prebaked_speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_prebaked_foam_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_prebaked_foam_power"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_prebaked_foamfx"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_min_distance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_max_distance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_simulated_speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_simulated_foam_scale"), ecs::ComponentTypeInfo<float>()}
};
static void water_flowmap_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    water_flowmap_es_event_handler(evt
        , ECS_RW_COMP(water_flowmap_es_event_handler_comps, "water", FFTWater)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap", bool)
    , ECS_RW_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_tex", ecs::string)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_area", Point4)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__wind_strength", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_range", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_fading", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_damping", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_prebaked_speed", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_prebaked_foam_scale", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_prebaked_foam_power", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_prebaked_foamfx", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_min_distance", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_max_distance", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_simulated_speed", float)
    , ECS_RO_COMP(water_flowmap_es_event_handler_comps, "water__flowmap_simulated_foam_scale", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_flowmap_es_event_handler_es_desc
(
  "water_flowmap_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_flowmap_es_event_handler_all_events),
  make_span(water_flowmap_es_event_handler_comps+0, 2)/*rw*/,
  make_span(water_flowmap_es_event_handler_comps+2, 14)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"water__flowmap,water__flowmap_area,water__flowmap_damping,water__flowmap_fading,water__flowmap_max_distance,water__flowmap_min_distance,water__flowmap_prebaked_foam_power,water__flowmap_prebaked_foam_scale,water__flowmap_prebaked_foamfx,water__flowmap_prebaked_speed,water__flowmap_range,water__flowmap_simulated_foam_scale,water__flowmap_simulated_speed,water__flowmap_tex,water__wind_strength");
static constexpr ecs::ComponentDesc water_flowmap_foam_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()},
//start of 14 ro components at [1]
  {ECS_HASH("water__flowmap_foam_power"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_foam_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_foam_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_foam_reflectivity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_foam_reflectivity_min"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_foam_color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("water__flowmap_foam_tiling"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_speed_depth_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_foam_speed_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_speed_depth_max"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_foam_depth_max"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__flowmap_slope"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__has_slopes"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("water__flowmap_detail"), ecs::ComponentTypeInfo<bool>()}
};
static void water_flowmap_foam_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    water_flowmap_foam_es_event_handler(evt
        , ECS_RW_COMP(water_flowmap_foam_es_event_handler_comps, "water", FFTWater)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_power", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_scale", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_threshold", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_reflectivity", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_reflectivity_min", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_color", Point3)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_tiling", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_speed_depth_scale", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_speed_scale", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_speed_depth_max", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_foam_depth_max", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_slope", float)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__has_slopes", bool)
    , ECS_RO_COMP(water_flowmap_foam_es_event_handler_comps, "water__flowmap_detail", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_flowmap_foam_es_event_handler_es_desc
(
  "water_flowmap_foam_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_flowmap_foam_es_event_handler_all_events),
  make_span(water_flowmap_foam_es_event_handler_comps+0, 1)/*rw*/,
  make_span(water_flowmap_foam_es_event_handler_comps+1, 14)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"water__flowmap_detail,water__flowmap_foam_color,water__flowmap_foam_depth_max,water__flowmap_foam_power,water__flowmap_foam_reflectivity,water__flowmap_foam_reflectivity_min,water__flowmap_foam_scale,water__flowmap_foam_speed_scale,water__flowmap_foam_threshold,water__flowmap_foam_tiling,water__flowmap_slope,water__flowmap_speed_depth_max,water__flowmap_speed_depth_scale,water__has_slopes");
static constexpr ecs::ComponentDesc water_shore_setup_es_event_handler_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("shore__texture_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("shore__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("shore__hmap_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__rivers_width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__significant_wave_threshold"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [5]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void water_shore_setup_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    water_shore_setup_es_event_handler(evt
        , ECS_RO_COMP(water_shore_setup_es_event_handler_comps, "shore__texture_size", int)
    , ECS_RO_COMP(water_shore_setup_es_event_handler_comps, "shore__enabled", bool)
    , ECS_RO_COMP(water_shore_setup_es_event_handler_comps, "shore__hmap_size", float)
    , ECS_RO_COMP(water_shore_setup_es_event_handler_comps, "shore__rivers_width", float)
    , ECS_RO_COMP(water_shore_setup_es_event_handler_comps, "shore__significant_wave_threshold", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_shore_setup_es_event_handler_es_desc
(
  "water_shore_setup_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_shore_setup_es_event_handler_all_events),
  empty_span(),
  make_span(water_shore_setup_es_event_handler_comps+0, 5)/*ro*/,
  make_span(water_shore_setup_es_event_handler_comps+5, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,",shore__enabled,shore__hmap_size,shore__rivers_width,shore__significant_wave_threshold,shore__texture_size");
static constexpr ecs::ComponentDesc water_shore_surf_setup_es_event_handler_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("shore__wave_height_to_amplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__amplitude_to_length"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__parallelism_to_wind"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__width_k"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__waves_dist"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("shore__waves_depth_min"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__waves_depth_fade_interval"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__wave_gspeed"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [8]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void water_shore_surf_setup_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    water_shore_surf_setup_es_event_handler(evt
        , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__wave_height_to_amplitude", float)
    , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__amplitude_to_length", float)
    , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__parallelism_to_wind", float)
    , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__width_k", float)
    , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__waves_dist", Point4)
    , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__waves_depth_min", float)
    , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__waves_depth_fade_interval", float)
    , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__wave_gspeed", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_shore_surf_setup_es_event_handler_es_desc
(
  "water_shore_surf_setup_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_shore_surf_setup_es_event_handler_all_events),
  empty_span(),
  make_span(water_shore_surf_setup_es_event_handler_comps+0, 8)/*ro*/,
  make_span(water_shore_surf_setup_es_event_handler_comps+8, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"shore__amplitude_to_length,shore__parallelism_to_wind,shore__wave_gspeed,shore__wave_height_to_amplitude,shore__waves_depth_fade_interval,shore__waves_depth_min,shore__waves_dist,shore__width_k");
static constexpr ecs::ComponentDesc water_strength_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()},
//start of 4 ro components at [1]
  {ECS_HASH("water__wind_dir"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("water__max_tessellation"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("water__oneToFourFFTPeriodicDivisor"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void water_strength_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    water_strength_es_event_handler(evt
        , ECS_RW_COMP(water_strength_es_event_handler_comps, "water", FFTWater)
    , ECS_RO_COMP(water_strength_es_event_handler_comps, "water__wind_dir", float)
    , ECS_RO_COMP(water_strength_es_event_handler_comps, "water__strength", float)
    , ECS_RO_COMP(water_strength_es_event_handler_comps, "water__max_tessellation", int)
    , ECS_RO_COMP_OR(water_strength_es_event_handler_comps, "water__oneToFourFFTPeriodicDivisor", float(1.0f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_strength_es_event_handler_es_desc
(
  "water_strength_es",
  "prog/daNetGame/main/waterES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_strength_es_event_handler_all_events),
  make_span(water_strength_es_event_handler_comps+0, 1)/*rw*/,
  make_span(water_strength_es_event_handler_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"water__oneToFourFFTPeriodicDivisor,water__strength,water__wind_dir");
