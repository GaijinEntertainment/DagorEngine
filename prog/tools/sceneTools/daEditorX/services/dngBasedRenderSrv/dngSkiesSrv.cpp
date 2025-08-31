// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_skiesService.h>
#include <de3_dynRenderService.h>
#include "render/skies.h"
#include <daSkies2/daSkiesToBlk.h>
#include <astronomy/daLocalTime.h>
#include <EditorCore/ec_interface.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_direct.h>

class DngBasedSkiesService : public ISkiesService
{
  static const int PRIO_COUNT = 4;
  struct WeatherOverride
  {
    SimpleString env, weather;
    DataBlock customWeather, stars;
    int seed = -1;
  };
  struct AstroTime
  {
    double jd = 0;
    float lat = 0, lon = 0;
  };
  WeatherOverride weatherOverride[PRIO_COUNT];
  DataBlock loadedWeatherType;
  DataBlock appliedWeather;
  DataBlock levelBlkShaderVars;
  Tab<SimpleString> weatherPresetFn;
  FastNameMapEx enviNames, weatherNames;
  String globalFileName;
  SimpleString weatherTypesFn, weatherPreset, weatherName, timeOfDay;
  AstroTime lastUpdatedTime;
  Point2 zRange = Point2(0.1, 20000.0);
  float znzfScale = 1.0f;

  void init() override
  {
    appliedWeather.setStr("preset", "");
    appliedWeather.setStr("env", "");
    appliedWeather.setStr("weather", "");
    appliedWeather.setReal("lat", 0);
    appliedWeather.setReal("lon", 0);
    appliedWeather.setInt("year", 1941);
    appliedWeather.setInt("month", 6);
    appliedWeather.setInt("day", 22);
    appliedWeather.setReal("gmtTime", 0);
    appliedWeather.setReal("locTime", 0);
    appliedWeather.setInt("seed", -1);
    appliedWeather.setBool("customWeather", false);
  }
  void term() override {}

  void setup(const char *app_dir, const DataBlock &env_blk) override {}

  void fillPresets(Tab<String> &out_presets, Tab<String> &out_env, Tab<String> &out_weather) override
  {
    out_presets.clear();
    out_env.clear();
    out_weather.clear();

    out_presets.reserve(weatherPresetFn.size());
    for (auto &fn : weatherPresetFn)
      out_presets.push_back() = fn;

    out_env.reserve(enviNames.nameCount());
    iterate_names(enviNames, [&out_env](int, const char *en) { out_env.push_back() = en; });

    out_weather.reserve(weatherNames.nameCount());
    iterate_names(weatherNames, [&out_weather](int, const char *wn) { out_weather.push_back() = wn; });
  }

  void setWeather(const char *preset_fn, const char *env, const char *weather) override
  {
    if (!preset_fn && !env && !weather)
      return;
    weatherPreset = preset_fn ? preset_fn : weatherPresetFn[0];
    timeOfDay = env;
    weatherName = weather;
    reapplyWeather();
  }
  void overrideShaderVarsFromLevelBlk(const DataBlock *b) override { levelBlkShaderVars = b ? *b : DataBlock::emptyBlock; }
  void overrideWeather(int prio, const char *env, const char *weather, int global_seed, const DataBlock *custom_w,
    const DataBlock *stars) override
  {
    if (prio < 0 || prio >= PRIO_COUNT)
      return;
    weatherOverride[prio].env = env;
    weatherOverride[prio].weather = weather;
    weatherOverride[prio].seed = global_seed;
    if (custom_w)
      weatherOverride[prio].customWeather = *custom_w;
    else
      weatherOverride[prio].customWeather.clearData();
    if (stars)
      weatherOverride[prio].stars = *stars;
    else
      weatherOverride[prio].stars.clearData();

    reapplyWeather();
  }
  void setZnZfScale(float znzf_scale) override { znzfScale = clamp(znzf_scale, 0.1f, 100.0f); }

  void getZRange(float &out_znear, float &out_zfar) override
  {
    out_znear = zRange[0] * znzfScale;
    out_zfar = zRange[1] * znzfScale;
  }

  const DataBlock *getWeatherTypeBlk() override { return &loadedWeatherType; }
  const DataBlock &getAppliedWeatherDesc() override { return appliedWeather; }

  void updateSkies(float dt) override
  {
    if (auto *daSkies = get_daskies())
    {
      float lat = 55.f, lon = 37.f;
      daSkies->getStarsLatLon(lat, lon);
      double jd = daSkies->getStarsJulianDay();
      if (fabs(lastUpdatedTime.jd - jd) > 1e-5 || fabsf(lastUpdatedTime.lat - lat) > 1e-3 || fabsf(lastUpdatedTime.lon - lon) > 1e-3)
      {
        lastUpdatedTime.jd = jd;
        lastUpdatedTime.lat = lat;
        lastUpdatedTime.lon = lon;

        float gmtTime = float(jd - floor(jd)) * 24.0f + 12.0f;
        float localTime = fmodf(gmtTime + lon * (12.f / 180.f), 24.0f);
        gmtTime = localTime - lon * (12.f / 180.f);

        appliedWeather.setReal("lat", lat);
        appliedWeather.setReal("lon", lon);
        appliedWeather.setReal("gmtTime", gmtTime);
        appliedWeather.setReal("locTime", localTime);
        debug("sync astro time: lat=%g lon=%g gmtTime=%g localTime=%g", lat, lon, gmtTime, localTime);
      }
    }
  }
  void beforeRenderSky() override {}
  void beforeRenderSkyOrtho() override {}
  void renderSky() override {}
  void renderClouds() override {}
  bool areCloudTexturesReady() override { return get_daskies() ? get_daskies()->isCloudsReady() : false; }

  void afterD3DReset(bool full_reset) override { reapplyWeather(); }

protected:
  void applyWeather(const char *weatherPresetFileName, const char *weather)
  {
    DataBlock weatherPresetBlk(weatherPresetFileName);
    G_ASSERTF(weatherPresetBlk.isValid(), "Invalid weather preset '%s'", weatherPresetFileName);

    DataBlock weatherTypesBlk(weatherTypesFn);
    G_ASSERT(weatherTypesBlk.isValid());

    DataBlock weatherTypeBlk;
    weatherTypeBlk = *weatherTypesBlk.getBlockByNameEx(weather);
    int seed = -1;
    bool custom = false;
    for (auto &ovr : weatherOverride)
    {
      if (!ovr.customWeather.isEmpty())
      {
        if (ovr.customWeather.getBool("#exclusive", false))
        {
          weatherTypeBlk = *weatherTypesBlk.getBlockByNameEx(weather);
          custom = false;
        }
        merge_data_block(weatherTypeBlk, ovr.customWeather);
        if (ovr.customWeather.paramCount() > (ovr.customWeather.findParam("#exclusive") < 0 ? 0 : 1))
          custom = true;
      }
      if (ovr.seed != -1)
        seed = ovr.seed;
    }

    G_ASSERTF(!weatherTypeBlk.isEmpty(), "No block for weather type '%s' in '%s'", weather, weatherTypesBlk.resolveFilename());

    DataBlock globalBlk(globalFileName);
    loadEnvironment(globalBlk, weatherPresetBlk, weatherTypeBlk);

    loadedWeatherType = weatherTypeBlk;
    appliedWeather.setBool("customWeather", custom);
    appliedWeather.setInt("seed", seed);
    setWeatherFinal(loadedWeatherType, seed);
  }
  void reapplyWeather()
  {
    auto *daSkies = get_daskies();
    if (!daSkies)
      return;

    const char *env = timeOfDay;
    const char *weather = weatherName;
    DataBlock stars;
    for (auto &ovr : weatherOverride)
    {
      if (!ovr.env.empty())
        env = ovr.env;
      if (!ovr.weather.empty())
        weather = ovr.weather;
      if (!ovr.stars.isEmpty())
      {
        if (ovr.stars.getBool("#exclusive", false))
          stars.clearData();
        else if (ovr.stars.getBool("#del:localTime", false))
          stars.removeParam("localTime");
        merge_data_block(stars, ovr.stars);
      }
    }

    if (weatherPreset.empty() && !stars.isEmpty())
    {
      float lat = 55.f, lon = 37.f;
      daSkies->getStarsLatLon(lat, lon);
      double jd = daSkies->getStarsJulianDay(), _jd0 = jd;
      float gmtTime = float(jd - floor(jd)) * 24.0f + 12.0f;
      if (gmtTime >= 24.f)
      {
        gmtTime -= 24.f;
        jd += 1.0;
      }
      float localTime = gmtTime + lon * (12.f / 180.f);
      float _lat0 = lat, _lon0 = lon, _gmtTime0 = gmtTime;

      lat = stars.getReal("latitude", lat);
      lon = stars.getReal("longitude", lon);
      localTime = stars.getReal("localTime", localTime);
      gmtTime = localTime - lon * (12.f / 180.f);

      stars.setReal("latitude", lat);
      stars.setReal("longitude", lon);
      stars.setReal("gmtTime", gmtTime);

      if (fabsf(lat - _lat0) > 1e-3f || fabsf(lon - _lon0) > 1e-3f || fabsf(gmtTime - _gmtTime0) > 1e-3f)
      {
        jd += fmodf(gmtTime - _gmtTime0, 24.0f) / 24.0;
        daSkies->setStarsLatLon(lat, lon);
        daSkies->setStarsJulianDay(jd);
        daSkies->setAstronomicalSunMoon();
        // debug_print_datablock("result weather time", &stars);
        lastUpdatedTime.jd = jd;
        lastUpdatedTime.lat = lat;
        lastUpdatedTime.lon = lon;
      }

      appliedWeather.setReal("lat", lat);
      appliedWeather.setReal("lon", lon);
      appliedWeather.setReal("gmtTime", gmtTime);
      appliedWeather.setReal("locTime", localTime);
    }

    appliedWeather.setStr("preset", weatherPreset);
    appliedWeather.setStr("env", env);
    appliedWeather.setStr("weather", weather);
    appliedWeather.removeBlock("stars");
    if (!stars.isEmpty())
      appliedWeather.addNewBlock(&stars, "stars");

    if (!weatherPreset.empty())
    {
      setTime(env, stars.isEmpty() ? NULL : &stars);
      applyWeather(weatherPreset, weather);
    }

    if (IDynRenderService *srv = EDITORCORE->queryEditorInterface<IDynRenderService>())
      srv->onLightingSettingsChanged();

    ShaderGlobal::set_vars_from_blk(levelBlkShaderVars);
  }
  void setWeatherFinal(const DataBlock &weatherTypeBlk, int global_seed)
  {
    static int weird_seed = 12345678;
    if (global_seed == -1)
      global_seed = weird_seed;

    float waterLevel = ShaderGlobal::get_real_fast(get_shader_variable_id("water_level", true));
    float averageGroundLevel = waterLevel; // fixme; we should read average_ground_level on locations without water
    DataBlock groundBlk;
    DataBlock *ground = groundBlk.addBlock("ground");
    ground->setReal("min_ground_offset", averageGroundLevel);
    ground->setReal("average_ground_albedo", 0.1); // levelBlk.getReal("average_ground_albedo", 0.1)//fixme:

    if (auto *daSkies = get_daskies())
    {
      skies_utils::load_daSkies(*daSkies, weatherTypeBlk, global_seed, *ground);

      DataBlock resultWeather;
      skies_utils::save_daSkies(*daSkies, resultWeather, 0, resultWeather);
      debug_print_datablock("result weather", &resultWeather);

      daSkies->setCloudsOrigin(0, 0);
    }
  }
  void setTime(const char *currentEnvironment, const DataBlock *stars)
  {
    int dd = 22, mm = 6, yy = 1941;
    float longitude = 37, latitude = 55;
    float parsedTime = 4.0f;
    if (stars)
    {
      longitude = stars->getReal("longitude", longitude);
      latitude = stars->getReal("latitude", latitude);
      yy = stars->getInt("year", yy);
      mm = stars->getInt("month", mm);
      dd = stars->getInt("day", dd);
    }

    if (currentEnvironment && atof(currentEnvironment) > 0)
      parsedTime = atof(currentEnvironment);
    else
      parsedTime = get_local_time_of_day_exact(currentEnvironment, 0.5f, latitude, yy, mm, dd);

    float gmtTime = parsedTime - longitude * (12.f / 180.f);
    if (stars)
    {
      parsedTime = stars->getReal("localTime", parsedTime);
      gmtTime = stars->getReal("gmtTime", parsedTime - longitude * (12.f / 180.f));
    }
    appliedWeather.setReal("lat", latitude);
    appliedWeather.setReal("lon", longitude);
    appliedWeather.setInt("year", yy);
    appliedWeather.setInt("month", mm);
    appliedWeather.setInt("day", dd);
    appliedWeather.setReal("gmtTime", gmtTime);
    appliedWeather.setReal("locTime", gmtTime + longitude * (12.f / 180.f));

    if (auto *daSkies = get_daskies())
    {
      daSkies->setStarsLatLon(latitude, longitude);
      daSkies->setStarsJulianDay(julian_day(yy, mm, dd, gmtTime));
      daSkies->setAstronomicalSunMoon();
    }
  }

  void loadEnvironment(const DataBlock &global_blk, DataBlock &weather_preset_blk, const DataBlock &weather_type_blk)
  {
    G_ASSERT(weather_preset_blk.isValid());
    G_ASSERT(weather_type_blk.isValid());

    // Parse global.blk.
    ShaderGlobal::set_vars_from_blk(global_blk);

    const DataBlock *sceneBlk = global_blk.getBlockByNameEx("scene");
    zRange[0] = sceneBlk->getReal("z_near", 0.1f);
    zRange[1] = sceneBlk->getReal("z_far", 20000.f);
    debug("loaded zrange: %.2f .. %.2f", zRange[0], zRange[1]);

    // ShaderGlobal::set_vars_from_blk(*weather_type_blk.getBlockByNameEx("shader_vars"));
  }
};

static DngBasedSkiesService srv;

void *get_dng_based_skies_service() { return &srv; }
