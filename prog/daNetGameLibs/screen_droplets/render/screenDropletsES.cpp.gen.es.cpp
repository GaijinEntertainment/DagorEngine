#include "screenDropletsES.cpp.inl"
ECS_DEF_PULL_VAR(screenDroplets);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc on_vehicle_change_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("isInVehicle"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("bindedCamera"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void on_vehicle_change_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    on_vehicle_change_es_event_handler(evt
        , ECS_RO_COMP(on_vehicle_change_es_event_handler_comps, "isInVehicle", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_vehicle_change_es_event_handler_es_desc
(
  "on_vehicle_change_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_vehicle_change_es_event_handler_all_events),
  empty_span(),
  make_span(on_vehicle_change_es_event_handler_comps+0, 1)/*ro*/,
  make_span(on_vehicle_change_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"bindedCamera,isInVehicle");
static constexpr ecs::ComponentDesc screen_droplets_settings_es_event_handler_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("screen_droplets__has_leaks"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("screen_droplets__on_rain_fadeout_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_droplets__screen_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_droplets__rain_cone_max"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_droplets__rain_cone_off"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_droplets__intensity_direct_control"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("screen_droplets__intensity_change_rate"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_droplets__grid_x"), ecs::ComponentTypeInfo<float>()}
};
static void screen_droplets_settings_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    screen_droplets_settings_es_event_handler(evt
        , ECS_RO_COMP(screen_droplets_settings_es_event_handler_comps, "screen_droplets__has_leaks", bool)
    , ECS_RO_COMP(screen_droplets_settings_es_event_handler_comps, "screen_droplets__on_rain_fadeout_radius", float)
    , ECS_RO_COMP(screen_droplets_settings_es_event_handler_comps, "screen_droplets__screen_time", float)
    , ECS_RO_COMP(screen_droplets_settings_es_event_handler_comps, "screen_droplets__rain_cone_max", float)
    , ECS_RO_COMP(screen_droplets_settings_es_event_handler_comps, "screen_droplets__rain_cone_off", float)
    , ECS_RO_COMP(screen_droplets_settings_es_event_handler_comps, "screen_droplets__intensity_direct_control", bool)
    , ECS_RO_COMP(screen_droplets_settings_es_event_handler_comps, "screen_droplets__intensity_change_rate", float)
    , ECS_RO_COMP(screen_droplets_settings_es_event_handler_comps, "screen_droplets__grid_x", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc screen_droplets_settings_es_event_handler_es_desc
(
  "screen_droplets_settings_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_droplets_settings_es_event_handler_all_events),
  empty_span(),
  make_span(screen_droplets_settings_es_event_handler_comps+0, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ScreenDropletsReset,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","screen_droplets__grid_x,screen_droplets__has_leaks,screen_droplets__intensity_change_rate,screen_droplets__intensity_direct_control,screen_droplets__on_rain_fadeout_radius,screen_droplets__rain_cone_max,screen_droplets__rain_cone_off,screen_droplets__screen_time");
static constexpr ecs::ComponentDesc reset_screen_droplets_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__screenSpaceWeatherEffects"), ecs::ComponentTypeInfo<bool>()}
};
static void reset_screen_droplets_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    reset_screen_droplets_es(evt
        , ECS_RO_COMP(reset_screen_droplets_es_comps, "render_settings__screenSpaceWeatherEffects", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc reset_screen_droplets_es_es_desc
(
  "reset_screen_droplets_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_screen_droplets_es_all_events),
  empty_span(),
  make_span(reset_screen_droplets_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       OnRenderSettingsReady,
                       SetResolutionEvent>::build(),
  0
,"render","render_settings__screenSpaceWeatherEffects");
//static constexpr ecs::ComponentDesc disable_screen_droplets_es_comps[] ={};
static void disable_screen_droplets_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  disable_screen_droplets_es(evt
        );
}
static ecs::EntitySystemDesc disable_screen_droplets_es_es_desc
(
  "disable_screen_droplets_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_screen_droplets_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UnloadLevel>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc reset_screen_droplets_for_new_level_es_comps[] ={};
static void reset_screen_droplets_for_new_level_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  reset_screen_droplets_for_new_level_es(evt
        );
}
static ecs::EntitySystemDesc reset_screen_droplets_for_new_level_es_es_desc
(
  "reset_screen_droplets_for_new_level_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_screen_droplets_for_new_level_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc destroy_screen_droplets_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("water_droplets_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void destroy_screen_droplets_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  destroy_screen_droplets_es(evt
        );
}
static ecs::EntitySystemDesc destroy_screen_droplets_es_es_desc
(
  "destroy_screen_droplets_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_screen_droplets_es_all_events),
  empty_span(),
  empty_span(),
  make_span(destroy_screen_droplets_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_screen_droplets_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("water_droplets_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void update_screen_droplets_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  update_screen_droplets_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc update_screen_droplets_es_es_desc
(
  "update_screen_droplets_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_screen_droplets_es_all_events),
  empty_span(),
  empty_span(),
  make_span(update_screen_droplets_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc disable_rain_for_screen_droplets_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("rain"), ecs::ComponentTypeInfo<Rain>()}
};
static void disable_rain_for_screen_droplets_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  disable_rain_for_screen_droplets_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc disable_rain_for_screen_droplets_es_event_handler_es_desc
(
  "disable_rain_for_screen_droplets_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_rain_for_screen_droplets_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(disable_rain_for_screen_droplets_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_rain_density_for_screen_droplets_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("far_rain__density"), ecs::ComponentTypeInfo<float>()}
};
static void update_rain_density_for_screen_droplets_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_rain_density_for_screen_droplets_es(evt
        , ECS_RO_COMP(update_rain_density_for_screen_droplets_es_comps, "far_rain__density", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_rain_density_for_screen_droplets_es_es_desc
(
  "update_rain_density_for_screen_droplets_es",
  "prog/daNetGameLibs/screen_droplets/render/screenDropletsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_rain_density_for_screen_droplets_es_all_events),
  empty_span(),
  make_span(update_rain_density_for_screen_droplets_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
