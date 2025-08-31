// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <render/screenDroplets.h>
#include <render/resolution.h>
#include <render/renderEvent.h>
#include <ecs/render/updateStageRender.h>
#include <ioSys/dag_dataBlock.h>
#include <main/water.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderer.h>
#include <render/weather/rain.h>
#include <render/renderSettings.h>
#include <gamePhys/collision/collisionLib.h>
#include <render/rendererFeatures.h>
#include "screenDroplets.h"
#include "render/renderLibsAllowed.h"
#include <render/world/wrDispatcher.h>

static const Point3 RAIN_DIR = Point3(0, -1, 0);

static bool is_supported()
{
  return is_render_lib_allowed("screen_droplets") && renderer_has_feature(FeatureRenderFlags::POSTFX) && WRDispatcher::isReadyToUse();
}

ECS_TRACK(isInVehicle, bindedCamera)
ECS_REQUIRE(ecs::EntityId bindedCamera)
static void on_vehicle_change_es_event_handler(const ecs::Event &, bool isInVehicle)
{
  if (get_screen_droplets_mgr())
    get_screen_droplets_mgr()->setInVehicle(isInVehicle);
}

ECS_TRACK(camera__active)
ECS_ON_EVENT(on_appear)
static void on_camera_changed_es(const ecs::Event &, bool camera__active)
{
  if (camera__active && get_screen_droplets_mgr())
    get_screen_droplets_mgr()->abortDrops();
}

ECS_ON_EVENT(on_appear, ScreenDropletsReset)
ECS_TRACK(screen_droplets__has_leaks,
  screen_droplets__on_rain_fadeout_radius,
  screen_droplets__screen_time,
  screen_droplets__rain_cone_max,
  screen_droplets__rain_cone_off,
  screen_droplets__intensity_direct_control,
  screen_droplets__intensity_change_rate,
  screen_droplets__intensity_change_rate,
  screen_droplets__grid_x)
ECS_TAG(render)
static void screen_droplets_settings_es_event_handler(const ecs::Event &,
  bool screen_droplets__has_leaks,
  float screen_droplets__on_rain_fadeout_radius,
  float screen_droplets__screen_time,
  float screen_droplets__rain_cone_max,
  float screen_droplets__rain_cone_off,
  bool screen_droplets__intensity_direct_control,
  float screen_droplets__intensity_change_rate,
  float screen_droplets__grid_x)
{
  if (ScreenDroplets *screenDroplets = get_screen_droplets_mgr())
  {
    ScreenDroplets::ConvarParams convarParams;
    convarParams.hasLeaks = screen_droplets__has_leaks;
    convarParams.onRainFadeoutRadius = screen_droplets__on_rain_fadeout_radius;
    convarParams.screenTime = screen_droplets__screen_time;
    convarParams.rainConeMax = screen_droplets__rain_cone_max;
    convarParams.rainConeOff = screen_droplets__rain_cone_off;
    convarParams.intensityDirectControl = screen_droplets__intensity_direct_control;
    convarParams.intensityChangeRate = screen_droplets__intensity_change_rate;
    convarParams.gridX = screen_droplets__grid_x;
    screenDroplets->setConvars(convarParams);
  }
}

template <typename Callable>
static void is_in_vehicle_on_screen_droplets_reset_ecs_query(Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady, SetResolutionEvent, ChangeRenderFeatures)
ECS_TRACK(render_settings__screenSpaceWeatherEffects, render_settings__bare_minimum)
static void reset_screen_droplets_es(
  const ecs::Event &, bool render_settings__screenSpaceWeatherEffects, bool render_settings__bare_minimum)
{
  close_screen_droplets_mgr();
  if (render_settings__screenSpaceWeatherEffects && !render_settings__bare_minimum && is_supported())
  {
    int w, h;
    get_rendering_resolution(w, h);
    IPoint2 resolution = ScreenDroplets::getSuggestedResolution(w, h);
    init_screen_droplets_mgr(resolution.x, resolution.y, true, 0.5f, -0.25f);
    get_screen_droplets_mgr()->setEnabled(true);

    bool cameraInsideVehicle = false;
    is_in_vehicle_on_screen_droplets_reset_ecs_query(
      [&cameraInsideVehicle](ECS_REQUIRE(ecs::EntityId bindedCamera) bool isInVehicle) { cameraInsideVehicle = isInVehicle; });
    get_screen_droplets_mgr()->setInVehicle(cameraInsideVehicle);
  }
  g_entity_mgr->broadcastEventImmediate(ScreenDropletsReset{});
}

ECS_TAG(render)
ECS_ON_EVENT(UnloadLevel)
static void disable_screen_droplets_es(const ecs::Event &)
{
  if (get_screen_droplets_mgr())
    get_screen_droplets_mgr()->setEnabled(false);
}

ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
static void reset_screen_droplets_for_new_level_es(const ecs::Event &)
{
  if (get_screen_droplets_mgr())
  {
    get_screen_droplets_mgr()->abortDrops();
    get_screen_droplets_mgr()->setEnabled(true);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(dafg::NodeHandle &water_droplets_node)
static void destroy_screen_droplets_es(const ecs::Event &) { close_screen_droplets_mgr(); }

ECS_TAG(render)
ECS_REQUIRE(const dafg::NodeHandle &water_droplets_node)
static void update_screen_droplets_es(const UpdateStageInfoBeforeRender &evt)
{
  if (get_screen_droplets_mgr())
  {
    TIME_D3D_PROFILE(screen_droplets_update)
    get_screen_droplets_mgr()->update(is_underwater(), evt.viewItm, evt.dt);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(Rain &rain)
static void disable_rain_for_screen_droplets_es_event_handler(const ecs::Event &)
{
  if (get_screen_droplets_mgr())
    get_screen_droplets_mgr()->setRain(0.0, RAIN_DIR);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, UpdateStageInfoBeforeRender)
ECS_TRACK(*)
static void update_rain_density_for_screen_droplets_es(const ecs::Event &, float far_rain__density)
{
  if (get_screen_droplets_mgr())
    get_screen_droplets_mgr()->setRain(far_rain__density, RAIN_DIR);
}

/* static*/
bool ScreenDroplets::rayHitNormalized(const Point3 &p, const Point3 &dir, float t) { return dacoll::rayhit_normalized_ri(p, dir, t); }
