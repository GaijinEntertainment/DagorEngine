// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "skies.h"
#include <daSkies2/daSkiesToBlk.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStates.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_resetDevice.h>
#include <webui/editVarPlugin.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>
#include <astronomy/astronomy.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <memory/dag_framemem.h>
#include "render/renderer.h"
#include <util/dag_console.h>
#include <perfMon/dag_statDrv.h>
#include "main/appProfile.h"
#include <render/renderEvent.h>
#include <main/weatherPreset.h>
#include <daECS/core/entityManager.h>
#include <util/dag_convar.h>
#include <ecs/weather/skiesSettings.h>
#include "main/level.h"
#if DAGOR_DBGLEVEL > 0
#include <osApiWrappers/dag_files.h>
#endif

#define INSIDE_RENDERER 1
#include "world/private_worldRenderer.h"

CONSOLE_BOOL_VAL("skies", cloudMovementEnabled, true);

static int skylight_paramsVarId = -1;
static int skylight_sky_attenVarId = -1;
static int skies_scattering_effectVarId = -1;
static int last_skies_scattering_effectVarId = -1;
static int daskies_timeVarId = -1;

#if DAGOR_DBGLEVEL > 0
static eastl::unique_ptr<DataBlock> last_level_blk;
static eastl::unique_ptr<DataBlock> last_weather_blk;
static eastl::string weather_file_name;
static int64_t weather_mtime = 0;
#endif

static InitOnDemand<DngSkies, false> daSkies;
void init_daskies() { daSkies.demandInit(); }
void term_daskies()
{
  daSkies.demandDestroy();
#if DAGOR_DBGLEVEL > 0
  last_level_blk.reset();
  last_weather_blk.reset();
#endif
}

DngSkies *get_daskies()
{
  if (daSkies)
    return daSkies.get();
  return nullptr;
}

DaSkies *get_daskies_impl() { return get_daskies(); }

DngSkies::DngSkies() : DaSkies(/*cpuOnly=*/false)
{
  minCloudThickness = 2.9f;

  initSky();

  skylight_paramsVarId = ::get_shader_variable_id("skylight_params", true);
  skylight_sky_attenVarId = ::get_shader_variable_id("skylight_sky_atten", true);
  skies_scattering_effectVarId = ::get_shader_variable_id("skies_scattering_effect", true);
  last_skies_scattering_effectVarId = ::get_shader_variable_id("last_skies_scattering_effect", true);
  daskies_timeVarId = ::get_shader_variable_id("daskies_time", true);
}

DngSkies::~DngSkies() { closeSky(); }

void DngSkies::enableBlackSkyRendering(bool black_sky_on) { renderBlackSky = black_sky_on; }

bool DngSkies::updateSkyLightParams()
{
  if (renderBlackSky)
  {
    ShaderGlobal::set_color4(skylight_paramsVarId, 0, 0, 0, 0);
    return true;
  }

  ShaderGlobal::set_color4(skylight_paramsVarId,
    skyLightProgress,           // skylight_progress
    skyLightEnviAttenuation,    // skylight_ambient_atten
    skyLightSunAttenuation,     // skylight_sun_attenuation
    skyLightGiWeightAttenuation // skylight_gi_weight_atten
  );

  ShaderGlobal::set_color4(skylight_sky_attenVarId, skyLightBaseSkyAttenuation, skyLightSkyAttenuation, 0, 0);

  return true;
}

void DngSkies::useFog(const Point3 &origin,
  SkiesData *data,
  const TMatrix &view_tm,
  const TMatrix4 &proj_tm,
  UpdateSky update_sky,
  float altitude_tolerance)
{
  ShaderGlobal::set_real(skies_scattering_effectVarId, scatteringEffect);
  ShaderGlobal::set_real(last_skies_scattering_effectVarId, lastScatteringEffect);
  DaSkies::useFog(calcViewPos(origin), data, calcViewTm(view_tm), proj_tm, update_sky, altitude_tolerance);
}

void DngSkies::useFogNoScattering(const Point3 &origin,
  SkiesData *data,
  const TMatrix &view_tm,
  const TMatrix4 &proj_tm,
  UpdateSky update_sky,
  float altitude_tolerance)
{
  ShaderGlobal::set_real(skies_scattering_effectVarId, 1.0f);
  ShaderGlobal::set_real(last_skies_scattering_effectVarId, 1.0f);
  DaSkies::useFog(calcViewPos(origin), data, calcViewTm(view_tm), proj_tm, update_sky, altitude_tolerance);
}

void DngSkies::prepare(const Point3 &dir_to_sun, bool force_update_cpu, float dt)
{
  lastScatteringEffect = scatteringEffect;
  scatteringEffect = skyLightSunAttenuation;
  DaSkies::prepare(dir_to_sun, force_update_cpu, dt);
}

void DngSkies::unsetSkyLightParams()
{
  if (renderBlackSky)
  {
    ShaderGlobal::set_color4(skylight_paramsVarId, 0, 0, 0, 0);
    return;
  }

  ShaderGlobal::set_color4(skylight_paramsVarId,
    0, // skylight_progress
    1, // skylight_ambient_atten
    1, // skylight_sun_attenuation
    1  // skylight_gi_weight_atten
  );
}

void DngSkies::setSkyLightParams(
  float progress, float sun_atten, float envi_atten, float gi_atten, float sky_atten, float base_sky_atten)
{
  skyLightProgress = progress;
  skyLightSunAttenuation = clamp(sun_atten, 0.0001f, 1.0f);
  skyLightEnviAttenuation = clamp(envi_atten, 0.0001f, 1.0f);
  skyLightGiWeightAttenuation = clamp(gi_atten, 0.0001f, 1.0f);
  skyLightSkyAttenuation = clamp(sky_atten, 0.0001f, 1.0f);
  skyLightBaseSkyAttenuation = clamp(base_sky_atten, 0.0001f, 1.0f);
}

static struct SkiesPanel
{
  float time = 9.f;
  int day = 21;
  int month = 6;
  int year = 1941;

  float latitude = 55.f;
  float longtitude = 37.f;

  int seed = 1;
#if DAGOR_DBGLEVEL > 0
  bool reload = false;
#endif

  bool operator==(const SkiesPanel &rhs) const
  {
    return
#if DAGOR_DBGLEVEL > 0
      reload == rhs.reload &&
#endif
      time == rhs.time && day == rhs.day && month == rhs.month && year == rhs.year && latitude == rhs.latitude &&
      longtitude == rhs.longtitude && seed == rhs.seed;
  }
  bool operator!=(const SkiesPanel &rhs) const { return !(*this == rhs); }
} skies_panel, prev_skies;

static void set_dir_to_sun(int year, int month, int day, float time)
{
  float lat, lon;
  daSkies->getStarsLatLon(lat, lon);
  float gmtTime = time - lon * (12. / 180); // local time
  double jd = julian_day(year, month, day, gmtTime);

  daSkies->setStarsJulianDay(jd);
  daSkies->setAstronomicalSunMoon();
}

void save_daskies(DataBlock &blk)
{
  if (auto sk = get_daskies())
  {
    float skiesTime = get_daskies_time();
    blk.addReal("skiesTime", skiesTime);
    skies_utils::save_daSkies(*sk, blk, false, blk);
  }
}

void load_daskies_int(
  const DataBlock &blk, float time_of_day, const char *weather_blk, int year, int month, int day, float lat, float lon, int seed)
{
  if (!daSkies)
  {
    logwarn("daSkies is not inited, load_daskies() call is skipped");
    return;
  }

  daSkies->setAltitudeOffset(blk.getReal("sky_coord_frame__altitude_offset", 0.0f));
  daSkies->setCloudsOrigin(0, 0);
  daSkies->setStrataCloudsOrigin(0, 0);

  const char *weatherToLoad = NULL;
#if DAGOR_DBGLEVEL > 0
  last_weather_blk.reset(new DataBlock);
  DataBlock &weather = *last_weather_blk;
#else
  DataBlock weather(midmem);
#endif
  if (weather_blk && *weather_blk)
    weatherToLoad = weather_blk;
  else
    weatherToLoad = blk.getStr("weatherName", NULL);
#if DAGOR_DBGLEVEL > 0
  weather_file_name = "";
#endif
  if (weatherToLoad)
  {
    debug("loading weather %s", weatherToLoad);
#if DAGOR_DBGLEVEL > 0
    const char *realName = df_get_real_name(weatherToLoad);
    DagorStat stat;
    if (realName && df_stat(weatherToLoad, &stat) != -1)
    {
      weather_file_name = realName;
      weather_mtime = stat.mtime;
    }
#endif
    if (!dblk::load(weather, weatherToLoad, dblk::ReadFlag::ROBUST))
      logerr("can't loading weather blk. Blk path = %s", weatherToLoad);
  }

  if (seed < 0)
  {
    seed = blk.getInt("weatherSeed", 13);
    if (seed < 0)
      seed = grnd();
  }

#if DAGOR_DBGLEVEL > 0
  // So skies_utils::load_daSkies can be called on WebUi skies_panel
  // change, or when skies.setSeed console cmd is used.
  last_level_blk.reset(new DataBlock(*blk.getBlockByNameEx("level")));
#endif

  debug("weather seed %d", seed);
  skies_utils::load_daSkies(*daSkies, weather, seed, *blk.getBlockByNameEx("level"));

  const GuiControlDescWebUi skyGuiDesc[] = {
    DECLARE_INT_SLIDER(skies_panel, year, 1940, 1946, blk.getInt("year", year)),
    DECLARE_INT_SLIDER(skies_panel, month, 1, 12, blk.getInt("month", month)),
    DECLARE_INT_SLIDER(skies_panel, day, 1, 31, blk.getInt("day", day)),
    DECLARE_FLOAT_SLIDER(skies_panel, time, 0, 24.0, blk.getReal("time", time_of_day), 0.01),
    DECLARE_FLOAT_SLIDER(skies_panel, latitude, -90, 90, lat >= -90 ? lat : blk.getReal("latitude", 55), 0.01),
    DECLARE_FLOAT_SLIDER(skies_panel, longtitude, -180, 180, lon >= -180 ? lon : blk.getReal("longtitude", 37), 0.01),
    DECLARE_INT_SLIDER(skies_panel, seed, 0, 131072, seed),
  };
  de3_webui_clear(skyGuiDesc);
  de3_webui_build(skyGuiDesc);

  daSkies->setStarsLatLon(skies_panel.latitude, skies_panel.longtitude);
  set_dir_to_sun(skies_panel.year, skies_panel.month, skies_panel.day, skies_panel.time);
  prev_skies = skies_panel;
  daSkies->invalidate();
}

void load_daskies(
  const DataBlock &blk, float time_of_day, const char *weather_blk, int year, int month, int day, float lat, float lon, int seed)
{
  d3d::GpuAutoLock gpuLock;
  load_daskies_int(blk, time_of_day, weather_blk, year, month, day, lat, lon, seed);
}

static String delayed_select_preset_name;

void select_weather_preset_delayed(const char *preset_name) { delayed_select_preset_name = preset_name; }

void update_delayed_weather_selection()
{
  if (!delayed_select_preset_name.empty())
  {
    select_weather_preset(delayed_select_preset_name.c_str());
    delayed_select_preset_name = "";
  }
}

void before_render_daskies()
{
  if (!daSkies)
    return;
#if DAGOR_DBGLEVEL > 0
  static int frame = 0;
  if (!weather_file_name.empty() && ((frame++ & 255) == 0) &&
      ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("useAddonVromSrc", false))
  {
    DagorStat stat;
    if (df_stat(weather_file_name.c_str(), &stat) != -1)
    {
      if (weather_mtime != stat.mtime)
      {
        skies_panel.reload = true;
        weather_mtime = stat.mtime;
      }
    }
  }
#endif

  if (skies_panel != prev_skies)
  {
#if DAGOR_DBGLEVEL > 0
    if (skies_panel.reload)
      last_weather_blk->load(weather_file_name.c_str());

    const DataBlock *paramsBlk = ::dgs_get_game_params()->getBlockByNameEx("debugWeather");
    if (!paramsBlk->getBool("allowDynamicWeatherAndTime", false) || skies_panel.reload)
    {
      skies_utils::load_daSkies(*daSkies, *last_weather_blk, skies_panel.seed, *last_level_blk);
    }
    skies_panel.reload = false;
#endif
    daSkies->setStarsLatLon(skies_panel.latitude, skies_panel.longtitude);
    set_dir_to_sun(skies_panel.year, skies_panel.month, skies_panel.day, skies_panel.time);
    daSkies->reset();
    get_world_renderer()->reloadCube(true);
    prev_skies = skies_panel;

    g_entity_mgr->broadcastEventImmediate(EventSkiesLoaded{});
  }

  ShaderGlobal::set_real(daskies_timeVarId, get_daskies_time());
}

float get_daskies_time() { return skies_panel.time; }

void set_daskies_time(float time_of_day)
{
  skies_panel.time = time_of_day;
  g_entity_mgr->set(get_current_level_eid(), ECS_HASH("level__timeOfDay"), time_of_day);
  debug("set time %f", time_of_day);
}

void set_daskies_latitude(float latitude)
{
  skies_panel.latitude = latitude;
  debug("set latitude %f", latitude);
}
void move_cumulus_clouds(const Point2 &amount)
{
  if (!daSkies || !cloudMovementEnabled)
    return;
  daSkies->setCloudsOrigin(daSkies->getCloudsOrigin().x + amount.x, daSkies->getCloudsOrigin().y + amount.y);
}
void move_strata_clouds(const Point2 &amount)
{
  if (!daSkies || !cloudMovementEnabled)
    return;
  daSkies->setStrataCloudsOrigin(daSkies->getStrataCloudsOrigin().x + amount.x, daSkies->getStrataCloudsOrigin().y + amount.y);
}
void move_skies(const Point2 &amount)
{
  move_cumulus_clouds(amount);
  move_strata_clouds(amount);
}

int get_daskies_seed() { return skies_panel.seed; }

void set_daskies_seed(int seed) { skies_panel.seed = seed; }

DPoint2 get_clouds_origin() { return daSkies->getCloudsOrigin(); }

DPoint2 get_strata_clouds_origin() { return daSkies->getStrataCloudsOrigin(); }

void set_clouds_origin(DPoint2 pos) { daSkies->setCloudsOrigin(pos.x, pos.y); }

void set_strata_clouds_origin(DPoint2 pos) { daSkies->setStrataCloudsOrigin(pos.x, pos.y); }

using namespace console;

static bool skies_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("skies", "setTime", 2, 2) { set_daskies_time(to_real(argv[1])); }
  CONSOLE_CHECK_NAME("skies", "getTime", 1, 1)
  {
    float skiesTime = get_daskies_time();
    console::print_d("skies time: %f", skiesTime);
  }
  CONSOLE_CHECK_NAME("skies", "setRandomTime", 2, 20)
  {
    static int seed = 1121213;
    set_daskies_time(to_real(argv[1 + (_rnd(seed) % (argc - 1))]));
  }
  CONSOLE_CHECK_NAME("skies", "setLatitude", 2, 2) { set_daskies_latitude(to_real(argv[1])); }
  CONSOLE_CHECK_NAME("skies", "moveClouds", 3, 3) { move_skies(Point2(to_real(argv[1]), to_real(argv[2]))); }
  CONSOLE_CHECK_NAME("skies", "setCloudsOrigin", 3, 3)
  {
    if (daSkies && cloudMovementEnabled)
      daSkies->setCloudsOrigin(console::to_real(argv[1]), console::to_real(argv[2]));
  }
  CONSOLE_CHECK_NAME("skies", "setStrataCloudsOrigin", 3, 3)
  {
    if (daSkies && cloudMovementEnabled)
      daSkies->setStrataCloudsOrigin(console::to_real(argv[1]), console::to_real(argv[2]));
  }
  CONSOLE_CHECK_NAME("skies", "setSeed", 2, 2) { skies_panel.seed = to_int(argv[1]); }
  CONSOLE_CHECK_NAME("skies", "getSeed", 1, 1) { console::print_d("skies import seed: %d", skies_panel.seed); }
  return found;
}

REGISTER_CONSOLE_HANDLER(skies_console_handler);
