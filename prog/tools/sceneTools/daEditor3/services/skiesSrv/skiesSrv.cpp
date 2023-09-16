#include <de3_skiesService.h>
#include <de3_lightService.h>
#include <de3_lightProps.h>
#include <de3_interface.h>
#include <de3_dynRenderService.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <daSkies2/daSkies.h>
#include <daSkies2/daSkiesToBlk.h>
#include <astronomy/daLocalTime.h>
#include <gameRes/dag_gameResSystem.h>
#include <render/fx/dag_demonPostFx.h>
#include <render/noiseTex.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_render.h>
#include <math/dag_mathAng.h>
#include <astronomy/astronomy.h>
#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>
#include <shaders/dag_shaderBlock.h>
#include <debug/dag_debug.h>
#include <render/sphHarmCalc.h>
#include <startup/dag_globalSettings.h>

static const Color4 HORIZONT_COLOR = Color4(0.2f, 0.2f, 0.2f, 0.f);
static const int SPH_HARM_FACE_WIDTH = 64;
static const int DYNAMIC_CUBE_HDR_OVERBRIGHT = 2.f;
static carray<int, SphHarmCalc::SH3_COUNT> sphharm_values_varids;
static carray<int, 3> sphharm_comp_values_varids;

extern const char *daeditor3_get_appblk_fname();
void interpolate_datablock(const DataBlock &from, DataBlock &to, float t);

static void set_sphharm_values_mono(const SphHarmCalc::values_t &v)
{
  Color4 normAmbient = v[SphHarmCalc::SH3_SH00].a > 0 ? v[SphHarmCalc::SH3_SH00] / v[SphHarmCalc::SH3_SH00].a : Color4(1, 1, 1, 1);

  ShaderGlobal::set_color4(sphharm_comp_values_varids[0],
    Color4(v[SphHarmCalc::SH3_SH00].a, v[SphHarmCalc::SH3_SH1P1].a, v[SphHarmCalc::SH3_SH10].a, v[SphHarmCalc::SH3_SH1M1].a));

  ShaderGlobal::set_color4(sphharm_comp_values_varids[1],
    Color4(v[SphHarmCalc::SH3_SH2P1].a, v[SphHarmCalc::SH3_SH2M1].a, v[SphHarmCalc::SH3_SH2M2].a, v[SphHarmCalc::SH3_SH20].a));

  ShaderGlobal::set_color4(sphharm_comp_values_varids[2],
    Color4(normAmbient.r, normAmbient.g, normAmbient.b, v[SphHarmCalc::SH3_SH2P2].a));
}


class GenericSkiesService : public ISkiesService
{
  DaSkies *daSkies;
  SimpleString weatherTypesFn;
  SimpleString daSkiesDepthTex;
  Tab<SimpleString> weatherPresetFn;

  SimpleString basePath;
  SimpleString weatherName, timeOfDay;
  SimpleString weatherPreset;
  String seqFileName;
  String globalFileName;
  FastNameMapEx enviNames, weatherNames;

  Point2 zRange;
  float znzfScale;
  float skyOffset;
  DataBlock interpolatedTimedBlk;
  int postFxLutTexVarId;
  TEXTUREID postFxLutTexId;

  Color3 skyColor;
  Point3 sunDir0;
  Color3 sunColor0;
  // DemonPostFxSettings initialPostFx;
  ISceneLightService *ltService;
  DataBlock loadedWeatherType;
  float time;
  float lastBrTime;
  DataBlock appliedWeather;

  static const int PRIO_COUNT = 4;
  struct WeatherOverride
  {
    SimpleString env, weather;
    DataBlock customWeather, stars;
    int seed;

    WeatherOverride() : seed(-1) {}
  };
  WeatherOverride weatherOverride[PRIO_COUNT];
  int last_target_w, last_target_h;
  TEXTUREID targetDepthId;


public:
  GenericSkiesService() : weatherPresetFn(midmem)
  {
    daSkies = NULL;
    ltService = NULL;
    zRange.set(0.1, 20000.0);
    znzfScale = 1.0f;
    skyOffset = 0;
    postFxLutTexVarId = -1;
    postFxLutTexId = BAD_TEXTUREID;
    time = 0;
    cube_pov_data = main_pov_data = refl_pov_data = NULL;
    last_target_w = last_target_h = 0;
    targetDepthId = BAD_TEXTUREID;

    appliedWeather.setStr("preset", "");
    appliedWeather.setStr("env", "");
    appliedWeather.setStr("weather", "");
    appliedWeather.setReal("lat", 0);
    appliedWeather.setReal("lon", 0);
    appliedWeather.setInt("year", 1941);
    appliedWeather.setInt("month", 6);
    appliedWeather.setInt("day", 22);
    appliedWeather.setReal("gmtTime", 4.0);
    appliedWeather.setReal("locTime", 4.0);
    appliedWeather.setInt("seed", -1);
    appliedWeather.setBool("customWeather", false);
  }

  virtual void setup(const char *app_dir, const DataBlock &env_blk)
  {
    basePath = String(260, "%s/%s/", app_dir, env_blk.getStr("skiesFilePathPrefix", "."));
    weatherTypesFn = String(260, "%s%s", basePath.str(), env_blk.getStr("skiesWeatherTypes", "?"));
    const char *skies_glob_fn = env_blk.getStr("skiesGlobal", "gameData/environments/global.blk");
    globalFileName = skies_glob_fn;
    if (!dd_file_exists(globalFileName))
      globalFileName.printf(0, "%s/develop/%s", app_dir, skies_glob_fn);
    if (!dd_file_exists(globalFileName))
      globalFileName.printf(0, "%s/develop/gameBase/%s", app_dir, skies_glob_fn);
    if (!dd_file_exists(globalFileName))
      globalFileName.printf(0, "%s%s", basePath, skies_glob_fn);
    debug("Skies: globalFileName=<%s> exists=%d", globalFileName, dd_file_exists(globalFileName));

    enviNames.reset();
    const DataBlock &b = *env_blk.getBlockByNameEx("enviNames");
    for (int i = 0; i < b.paramCount(); i++)
      if (b.getParamType(i) == DataBlock::TYPE_BOOL && b.getBool(i))
        enviNames.addNameId(b.getParamName(i));

    String mask(260, "%s/%s", basePath.str(), env_blk.getStr("skiesPresets", "*.blk"));
    char dir[260];
    dd_get_fname_location(dir, mask);
    alefind_t ff;

    debug("mask <%s> dir=%s", mask.str(), dir);
    if (::dd_find_first(mask, 0, &ff))
    {
      do
      {
        if (ff.name[0] == '.')
          continue;
        String fn(260, "%s/%s", dir, ff.name);
        dd_simplify_fname_c(fn);
        strlwr(fn);
        weatherPresetFn.push_back() = fn;
      } while (dd_find_next(&ff));
      dd_find_close(&ff);
    }
    if (!weatherPresetFn.size() && dd_file_exist(mask))
      weatherPresetFn.push_back() = mask;
    for (int i = 0; i < weatherPresetFn.size(); i++)
    {
      simplify_fname(weatherPresetFn[i]);
      strlwr(weatherPresetFn[i]);
    }

    DataBlock wblk(weatherTypesFn);
    for (int i = 0; i < wblk.blockCount(); i++)
      weatherNames.addNameId(wblk.getBlock(i)->getBlockName());

    debug("weatherNames=%d enviNames=%d presets=%d", weatherNames.nameCount(), enviNames.nameCount(), weatherPresetFn.size());
  }
  SkiesData *cube_pov_data, *main_pov_data, *refl_pov_data;

  virtual void init()
  {
    if (daSkies)
      return;

    DataBlock appBlk(daeditor3_get_appblk_fname());
    const_cast<DataBlock *>(::dgs_get_settings())->removeBlock("skies");
    const_cast<DataBlock *>(::dgs_get_settings())
      ->addNewBlock(appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("envi")->getBlockByNameEx("skies"), "skies");

    daSkiesDepthTex = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("envi")->getStr("daSkiesDepthTex", "depth_tex");

    ltService = EDITORCORE->queryEditorInterface<ISceneLightService>();
    daSkies = new DaSkies();
    daSkies->initSky();

    Point3 minR = Point3(-0.28, -0.23, -0.24);
    Point3 maxR = Point3(0.29, 0.27, 0.24);
    init_and_get_perlin_noise_3d(minR, maxR);

    int fog_quality = 128;
    main_pov_data = daSkies->createSkiesData("main", fog_quality);
    cube_pov_data = daSkies->createSkiesData("cube", fog_quality / 2);
    refl_pov_data = daSkies->createSkiesData("water", fog_quality / 2);

    sphharm_values_varids[SphHarmCalc::SH3_SH00] = get_shader_variable_id("SH3_sh00", true);
    sphharm_values_varids[SphHarmCalc::SH3_SH1M1] = get_shader_variable_id("SH3_sh1m1", true);
    sphharm_values_varids[SphHarmCalc::SH3_SH10] = get_shader_variable_id("SH3_sh10", true);
    sphharm_values_varids[SphHarmCalc::SH3_SH1P1] = get_shader_variable_id("SH3_sh1p1", true);
    sphharm_values_varids[SphHarmCalc::SH3_SH2M2] = get_shader_variable_id("SH3_sh2m2", true);
    sphharm_values_varids[SphHarmCalc::SH3_SH2M1] = get_shader_variable_id("SH3_sh2m1", true);
    sphharm_values_varids[SphHarmCalc::SH3_SH20] = get_shader_variable_id("SH3_sh20", true);
    sphharm_values_varids[SphHarmCalc::SH3_SH2P1] = get_shader_variable_id("SH3_sh2p1", true);
    sphharm_values_varids[SphHarmCalc::SH3_SH2P2] = get_shader_variable_id("SH3_sh2p2", true);

    sphharm_comp_values_varids[0] = get_shader_variable_id("SH3_04", true);
    sphharm_comp_values_varids[1] = get_shader_variable_id("SH3_58", true);
    sphharm_comp_values_varids[2] = get_shader_variable_id("SH3_color_9", true);
    // fixme:    setup earttexture with 0
  }
  virtual void term()
  {
    if (!daSkies)
      return;
    release_perline_noise_3d();
    ShaderGlobal::reset_textures(true);
    daSkies->destroy_skies_data(main_pov_data);
    daSkies->destroy_skies_data(cube_pov_data);
    daSkies->destroy_skies_data(refl_pov_data);
    del_it(daSkies);
    ltService = NULL;
  }

  virtual void fillPresets(Tab<String> &out_presets, Tab<String> &out_env, Tab<String> &out_weather)
  {
    out_presets.clear();
    out_env.clear();
    out_weather.clear();

    for (int i = 0; i < weatherPresetFn.size(); i++)
      out_presets.push_back() = weatherPresetFn[i];

    for (int i = 0; i < enviNames.nameCount(); i++)
      out_env.push_back() = enviNames.getName(i);

    for (int i = 0; i < weatherNames.nameCount(); i++)
      out_weather.push_back() = weatherNames.getName(i);
  }

  virtual void setWeather(const char *preset_fn, const char *env, const char *weather)
  {
    if (!daSkies)
      return;
    time = 0;
    lastBrTime = 0;
    if (!preset_fn && !env && !weather)
    {
      setEnvironment();
      return;
    }
    weatherPreset = preset_fn ? preset_fn : weatherPresetFn[0];
    timeOfDay = env;
    weatherName = weather;
    reloadWeather();
  }
  void reloadWeather()
  {
    const char *env = timeOfDay;
    const char *weather = weatherName;
    DataBlock stars;
    for (int i = 0; i < PRIO_COUNT; i++)
    {
      if (!weatherOverride[i].env.empty())
        env = weatherOverride[i].env;
      if (!weatherOverride[i].weather.empty())
        weather = weatherOverride[i].weather;
      if (!weatherOverride[i].stars.isEmpty())
      {
        if (weatherOverride[i].stars.getBool("#exclusive", false))
          stars.clearData();
        else if (weatherOverride[i].stars.getBool("#del:localTime", false))
          stars.removeParam("localTime");
        merge_data_block(stars, weatherOverride[i].stars);
      }
    }
    appliedWeather.setStr("preset", weatherPreset);
    appliedWeather.setStr("env", env);
    appliedWeather.setStr("weather", weather);
    appliedWeather.removeBlock("stars");
    if (!stars.isEmpty())
      appliedWeather.addNewBlock(&stars, "stars");

    setTime(env, stars.isEmpty() ? NULL : &stars);
    loadWeather(weatherPreset, weather);

    updateSkies(0.1);
    setEnvironment();
    beforeRenderSky();
    if (IDynRenderService *srv = EDITORCORE->queryEditorInterface<IDynRenderService>())
      srv->onLightingSettingsChanged();
  }

  virtual void overrideWeather(int prio, const char *env, const char *weather, int global_seed, const DataBlock *custom_w,
    const DataBlock *stars)
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

    if (daSkies && !weatherPreset.empty())
      reloadWeather();
  }
  virtual void setZnZfScale(float znzf_scale) { znzfScale = clamp(znzf_scale, 0.1f, 100.0f); }

  virtual void getZRange(float &out_znear, float &out_zfar)
  {
    out_znear = zRange[0] * znzfScale;
    out_zfar = zRange[1] * znzfScale;
  }
  virtual const DataBlock *getWeatherTypeBlk() { return &loadedWeatherType; }
  virtual const DataBlock &getAppliedWeatherDesc() { return appliedWeather; }

  virtual void updateSkies(float dt) { time += dt; }
  virtual void beforeRenderSky()
  {
    if (!daSkies)
      return;
    daSkies->prepare(daSkies->getSunDir(), false, time - lastBrTime);
    lastBrTime = time;

    if (IDynRenderService *srv = EDITORCORE->queryEditorInterface<IDynRenderService>())
    {
      static const float VOL_FOG_POS_ALT = 100;
      static const float VOL_FOG_POS_DIST = 50000;

      DemonPostFxSettings set;
      srv->getPostFxSettings(set);
      Point3 volFogPos(::grs_cur_view.pos.x, max(::grs_cur_view.pos.y, VOL_FOG_POS_ALT), ::grs_cur_view.pos.z);
      Color3 mieColor = daSkies->getMieColor(volFogPos, -sunDir0, VOL_FOG_POS_DIST);
      if (lengthSq(set.volfogColor - mieColor) > 1e-8)
      {
        set.volfogColor = mieColor;
        srv->setPostFxSettings(set);
      }
    }
  }
  virtual void beforeRenderSkyOrtho() {}
  virtual void renderSky()
  {
    if (!daSkies)
      return;
    int l, t, w, h;
    float minZ, maxZ;
    d3d::getview(l, t, w, h, minZ, maxZ);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    if (get_managed_texture_refcount(targetDepthId) < 0)
      targetDepthId = get_managed_texture_id(daSkiesDepthTex);
    if (last_target_w != w || last_target_h != h)
    {
      if (targetDepthId != BAD_TEXTUREID)
      {
        debug("sky: adopt new RT size %dx%d -> %dx%d", last_target_w, last_target_h, w, h);
        last_target_w = w;
        last_target_h = h;
        if (Texture *t = (Texture *)acquire_managed_tex(targetDepthId))
        {
          TextureInfo tinfo;
          t->getinfo(tinfo);
          if (tinfo.w != w || tinfo.h != h)
            last_target_w = last_target_h = 0;
        }
        release_managed_tex(targetDepthId);
        // when depth and target coinside, setup 3D clouds (level=1)
        daSkies->changeSkiesData(0, last_target_w > 0 ? 1 : 0, 0, w, h, main_pov_data);
      }
      else
        daSkies->changeSkiesData(0, 0, 0, w, h, main_pov_data);
    }

    TMatrix viewTm;
    TMatrix4 projTm;
    d3d::gettm(TM_VIEW, viewTm);
    d3d::gettm(TM_PROJ, &projTm);
    if (targetDepthId == BAD_TEXTUREID)
      daSkies->renderEnvi(false, dpoint3(::grs_cur_view.pos), dpoint3(::grs_cur_view.itm.getcol(2)), 0xFF,
        EDITORCORE->queryEditorInterface<IDynRenderService>()->getDownsampledFarDepth(),
        EDITORCORE->queryEditorInterface<IDynRenderService>()->getDownsampledFarDepth(), BAD_TEXTUREID, main_pov_data, viewTm, projTm);
    else
    {
      daSkies->prepareSkyAndClouds(false, dpoint3(::grs_cur_view.pos), dpoint3(::grs_cur_view.itm.getcol(2)), 0xFF,
        EDITORCORE->queryEditorInterface<IDynRenderService>()->getDownsampledFarDepth(),
        EDITORCORE->queryEditorInterface<IDynRenderService>()->getDownsampledFarDepth(), main_pov_data, viewTm, projTm, true, false);
      daSkies->renderSky(main_pov_data, viewTm, projTm);
    }

    d3d::setview(l, t, w, h, minZ, maxZ);
  }
  virtual void renderClouds()
  {
    if (targetDepthId == BAD_TEXTUREID)
      return;

    int l, t, w, h;
    float minZ, maxZ;
    d3d::getview(l, t, w, h, minZ, maxZ);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    TMatrix viewTm;
    TMatrix4 projTm;
    d3d::gettm(TM_VIEW, viewTm);
    d3d::gettm(TM_PROJ, &projTm);
    daSkies->renderClouds(false,
      TextureIDPair(EDITORCORE->queryEditorInterface<IDynRenderService>()->getDownsampledFarDepth().getTex2D(),
        EDITORCORE->queryEditorInterface<IDynRenderService>()->getDownsampledFarDepth().getTexId()),
      targetDepthId, main_pov_data, viewTm, projTm);

    d3d::setview(l, t, w, h, minZ, maxZ);
  }
  virtual void afterD3DReset(bool /*full_reset*/)
  {
    if (daSkies)
    {
      daSkies->reset();
      reloadWeather();
    }
  }

public:
  void setWeather(const DataBlock &weatherTypeBlk, int global_seed)
  {
    G_ASSERT(daSkies);

    static int weird_seed = 12345678;
    if (global_seed == -1)
      global_seed = weird_seed;

    float waterLevel = ShaderGlobal::get_real_fast(get_shader_variable_id("water_level", true));
    // float averageGroundLevel = levelBlk.getReal("average_ground_level", waterLevel);
    float averageGroundLevel = waterLevel; // fixme; we should read average_ground_level on locations without water
    DataBlock groundBlk;
    DataBlock *ground = groundBlk.addBlock("ground");
    ground->setReal("min_ground_offset", averageGroundLevel);
    ground->setReal("average_ground_albedo", 0.1); // levelBlk.getReal("average_ground_albedo", 0.1)//fixme:

    skies_utils::load_daSkies(*daSkies, weatherTypeBlk, global_seed, *ground);

    DynamicMemGeneralSaveCB debugSaveWeather(tmpmem, 8 << 10);
    DataBlock resultWeather;
    skies_utils::save_daSkies(*daSkies, resultWeather, 0, resultWeather);

    resultWeather.saveToTextStream(debugSaveWeather);
    char eof = 0;
    debugSaveWeather.write(&eof, 1);
    debug("result weather=\n%s\nweather end\n", (const char *)debugSaveWeather.data());

    daSkies->setCloudsOrigin(0, 0);
  }

  void loadWeather(const char *weatherPresetFileName, const char *weather)
  {
    G_ASSERT(daSkies);
    DataBlock weatherPresetBlk(weatherPresetFileName);
    G_ASSERTF(weatherPresetBlk.isValid(), "Invalid weather preset '%s'", weatherPresetFileName);

    DataBlock weatherTypesBlk(weatherTypesFn);
    G_ASSERT(weatherTypesBlk.isValid());

    DataBlock weatherTypeBlk;
    weatherTypeBlk = *weatherTypesBlk.getBlockByNameEx(weather);
    int seed = -1;
    bool custom = false;
    for (int i = 0; i < PRIO_COUNT; i++)
    {
      if (!weatherOverride[i].customWeather.isEmpty())
      {
        if (weatherOverride[i].customWeather.getBool("#exclusive", false))
        {
          weatherTypeBlk = *weatherTypesBlk.getBlockByNameEx(weather);
          custom = false;
        }
        merge_data_block(weatherTypeBlk, weatherOverride[i].customWeather);
        if (weatherOverride[i].customWeather.paramCount() > (weatherOverride[i].customWeather.findParam("#exclusive") < 0 ? 0 : 1))
          custom = true;
      }
      if (weatherOverride[i].seed != -1)
        seed = weatherOverride[i].seed;
    }

    G_ASSERTF(!weatherTypeBlk.isEmpty(), "No block for weather type '%s' in '%s'", weather, weatherTypesBlk.resolveFilename());

    DataBlock globalBlk(globalFileName);

    loadEnvironment(globalBlk, weatherPresetBlk, weatherTypeBlk);

    loadedWeatherType = weatherTypeBlk;
    appliedWeather.setBool("customWeather", custom);
    appliedWeather.setInt("seed", seed);
    setWeather(loadedWeatherType, seed);
  }

  void setTime(const char *currentEnvironment, const DataBlock *stars)
  {
    float parsedTime = 8.0f;
    float longitude = -1.14f;    // normandy, d-day
    float latitude = 49.4163601; // normandy D-Day
    int dd = 6, mm = 6, yy = 1944;
    if (stars)
    {
      longitude = stars->getReal("longitude", 0.0f); // Moscow time!
      latitude = stars->getReal("latitude", 21.3f);  // honolulu latitude. previous simul default
      yy = stars->getInt("year", yy);
      mm = stars->getInt("month", mm);
      dd = stars->getInt("day", 6);
    }

    if (currentEnvironment && atof(currentEnvironment) > 0)
      parsedTime = atof(currentEnvironment);
    else
      parsedTime = get_local_time_of_day_exact(currentEnvironment, 0.5f, latitude, yy, mm, dd);

    float gmtTime = parsedTime - longitude * (12. / 180);
    if (stars)
    {
      parsedTime = stars->getReal("localTime", parsedTime);
      gmtTime = stars->getReal("gmtTime", parsedTime - longitude * (12. / 180));
    }
    appliedWeather.setReal("lat", latitude);
    appliedWeather.setReal("lon", longitude);
    appliedWeather.setInt("year", yy);
    appliedWeather.setInt("month", mm);
    appliedWeather.setInt("day", dd);
    appliedWeather.setReal("gmtTime", gmtTime);
    appliedWeather.setReal("locTime", gmtTime + longitude * (12. / 180));

    daSkies->setStarsLatLon(latitude, longitude);
    daSkies->setStarsJulianDay(julian_day(yy, mm, dd, gmtTime));
    daSkies->setAstronomicalSunMoon();
  }

  void loadEnvironment(const DataBlock &global_blk, DataBlock &weather_preset_blk, const DataBlock &weather_type_blk)
  {
    G_ASSERT(weather_preset_blk.isValid());
    G_ASSERT(weather_type_blk.isValid());
    G_ASSERT(daSkies);


    // Parse global.blk.

    ShaderGlobal::set_vars_from_blk(global_blk);

    const DataBlock *sceneBlk = global_blk.getBlockByNameEx("scene");
    zRange[0] = sceneBlk->getReal("z_near", 0.1f);
    zRange[1] = sceneBlk->getReal("z_far", 20000.f);
    debug("loaded zrange: %.2f .. %.2f", zRange[0], zRange[1]);


    // Parse time independent part of weather preset.

    DataBlock *commonBlk = weather_preset_blk.addBlock("common");
    const DataBlock *waterBlk = commonBlk->getBlockByNameEx("water");

    ShaderGlobal::set_vars_from_blk(*waterBlk);


    // Set static HDR params to demonpostfx.

    if (true /*demonPostFx*/)
    {
      if (IDynRenderService *srv = EDITORCORE->queryEditorInterface<IDynRenderService>())
        srv->restartPostfx(*commonBlk);
      //::app->renderer->demonPostFx->getSettings(initialPostFx);
    }


    // Create weather.
    ShaderGlobal::set_vars_from_blk(*weather_type_blk.getBlockByNameEx("shader_vars"));


    // Postfx LUT.

    if (postFxLutTexId != BAD_TEXTUREID)
      release_managed_tex(postFxLutTexId);
    const char *postFxLutTexName = interpolatedTimedBlk.getStr("lut", NULL);
    postFxLutTexId = (postFxLutTexName && postFxLutTexName[0]) ? ::get_tex_gameres(postFxLutTexName) : BAD_TEXTUREID;
    if (postFxLutTexId != BAD_TEXTUREID)
    {
      VolTexture *tex = (VolTexture *)acquire_managed_tex(postFxLutTexId);
      TextureInfo info;
      if (tex)
        tex->getinfo(info);
      else
      {
        info.w = info.h = info.d = 2;
        logerr("failed to get LUT tex <%s>, tid=%d", postFxLutTexName, postFxLutTexId);
      }
      G_ASSERT(info.w >= 2 && info.w == info.h && info.w == info.d);
      ShaderGlobal::set_color4(get_shader_variable_id("postfx_lut_scale"), (info.w - 1) / float(info.w), 0.5f / info.w, 0.f, 0.f);
      release_managed_tex(postFxLutTexId);
    }

    ShaderGlobal::set_texture(postFxLutTexVarId, postFxLutTexId);
  }
  void getColors()
  {
    float sunCos, moonCos;
    Color3 sun_color, sun_sky_color, moon_color, moon_sky_color;
    if (daSkies->currentGroundSunSkyColor(sunCos, moonCos, sun_color, sun_sky_color, moon_color, moon_sky_color))
    {
      if (lengthSq(sun_color) + lengthSq(sun_sky_color) + lengthSq(moon_color) + lengthSq(moon_sky_color) < 1e-6)
      {
        logwarn("bad sun/sky colors (for %.3f, %.3f): %@ %@ %@ %@, recalc on CPU", sunCos, moonCos, sun_color, sun_sky_color,
          moon_color, moon_sky_color);
        goto calc_on_cpu;
      }
    }
    else
    {
      logerr("no sky colors gpu data available!");
    calc_on_cpu:
      Point3 origin(0, 10, 0);
      sun_color = daSkies->getCpuSun(origin, daSkies->getSunDir());
      moon_color = daSkies->getCpuMoon(origin, daSkies->getMoonDir());

      sun_sky_color = daSkies->getCpuSunSky(origin, daSkies->getSunDir());
      moon_sky_color = daSkies->getCpuMoonSky(origin, daSkies->getMoonDir());
    }

    skyColor = sun_sky_color + moon_sky_color;
    if (brightness(sun_color) < 0.1)
    {
      sunColor0 = moon_color / PI;
      sunDir0 = -daSkies->getMoonDir();
    }
    else
    {
      sunColor0 = sun_color / PI;
      sunDir0 = -daSkies->getSunDir();
    }
  }

  void setEnvironment()
  {
    G_ASSERT(daSkies);

    ltService->setSunCount(1);
    ltService->setSyncLtColors(false);
    ltService->setSyncLtDirs(false);

    // Get environment settings from daSkies.
    beforeRenderSky();
    getColors();

    Point2 s0a = dir_to_sph_ang(-sunDir0);
    debug("sunDir0=" FMT_P3 " %.3f, %.3f ->" FMT_P3 "", P3D(sunDir0), RadToDeg(s0a.x), RadToDeg(s0a.y), P3D(-sph_ang_to_dir(s0a)));
    ltService->getSky().ambCol = skyColor;
    ltService->getSky().ambColUser = e3dcolor(0.2f * skyColor);
    ltService->getSky().ambColMulUser = 5.0f;

    ltService->getSun(0)->ltDir = -sunDir0;
    ltService->getSun(0)->zenith = s0a.y;
    ltService->getSun(0)->azimuth = s0a.x;
    ltService->getSun(0)->ltCol = sunColor0;
    float maxRGB = max(max(sunColor0.r, sunColor0.g), sunColor0.b);
    ltService->getSun(0)->ltColUser = e3dcolor(sunColor0 / (0.0001 + maxRGB));
    ltService->getSun(0)->ltColMulUser = maxRGB;

    debug("HMAP sun az=%.5f zen=%.5f", RadToDeg(PI + atan2f(sunDir0.z, sunDir0.x)),
      -RadToDeg(atan2f(sunDir0.y, sqrtf(sunDir0.x * sunDir0.x + sunDir0.z * sunDir0.z))));

    // Update lmesh settings.
    if (!DAGORED2)
    {
      ltService->updateShaderVars();
      return;
    }

    IGenEditorPlugin *hmapPlugin = DAGORED2->getPluginByName("HeightmapLand");
    if (!hmapPlugin)
      hmapPlugin = DAGORED2->getPluginByName("bin_scene_view");
    IEnvironmentSettings *settings = hmapPlugin ? hmapPlugin->queryInterface<IEnvironmentSettings>() : NULL;

    ltService->updateShaderVars();
  }
};

static GenericSkiesService srv;

void *get_generic_skies_service() { return &srv; }
