// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "waterShoreES.cpp.inl"
ECS_DEF_PULL_VAR(waterShore);
#include <daECS/core/internal/performQuery.h>
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
  "prog/daNetGame/render/waterShoreES.cpp.inl",
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
//start of 9 ro components at [0]
  {ECS_HASH("shore__wave_height_to_amplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__amplitude_to_length"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__parallelism_to_wind"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__width_k"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__waves_dist"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("shore__waves_depth_min"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__waves_depth_fade_interval"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__wave_gspeed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shore__rivers_wave_multiplier"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [9]
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
    , ECS_RO_COMP(water_shore_surf_setup_es_event_handler_comps, "shore__rivers_wave_multiplier", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc water_shore_surf_setup_es_event_handler_es_desc
(
  "water_shore_surf_setup_es",
  "prog/daNetGame/render/waterShoreES.cpp.inl",
  ecs::EntitySystemOps(nullptr, water_shore_surf_setup_es_event_handler_all_events),
  empty_span(),
  make_span(water_shore_surf_setup_es_event_handler_comps+0, 9)/*ro*/,
  make_span(water_shore_surf_setup_es_event_handler_comps+9, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"shore__amplitude_to_length,shore__parallelism_to_wind,shore__rivers_wave_multiplier,shore__wave_gspeed,shore__wave_height_to_amplitude,shore__waves_depth_fade_interval,shore__waves_depth_min,shore__waves_dist,shore__width_k");
