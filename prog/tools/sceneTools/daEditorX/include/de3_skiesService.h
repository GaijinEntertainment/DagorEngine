//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
#include <supp/dag_math.h>
class String;
class DataBlock;


class ISkiesService
{
public:
  static constexpr unsigned HUID = 0x2EF40209u; // ISkiesService

  static constexpr float DEFAULT_LONGITUDE = -1.14f;     // normandy, D-Day
  static constexpr float DEFAULT_LATITUDE = 49.4163601f; // normandy D-Day
  static constexpr int DEFAULT_YEAR = 1944;
  static constexpr int DEFAULT_MONTH = 6;
  static constexpr int DEFAULT_DAY = 6;
  static constexpr float DEFAULT_LOCALTIME = 12.0f;

  virtual void setup(const char *app_dir, const DataBlock &env_blk) = 0;

  virtual void init() = 0;
  virtual void term() = 0;

  virtual void fillPresets(Tab<String> &out_presets, Tab<String> &out_env, Tab<String> &out_weather) = 0;

  virtual void setWeather(const char *preset_fn, const char *env, const char *weather) = 0;
  virtual void overrideShaderVarsFromLevelBlk(const DataBlock *b) = 0;
  virtual void overrideWeather(int prio, const char *env, const char *weather, int global_seed, const DataBlock *custom_w,
    const DataBlock *stars) = 0;
  virtual void setZnZfScale(float znzf_scale) = 0;

  virtual void getZRange(float &out_znear, float &out_zfar) = 0;
  virtual const DataBlock *getWeatherTypeBlk() = 0;
  virtual const DataBlock &getAppliedWeatherDesc() = 0;

  virtual void updateSkies(float dt) = 0;
  virtual void beforeRenderSky() = 0;
  virtual void beforeRenderSkyOrtho() = 0;
  virtual void renderSky() = 0;
  virtual void renderClouds() = 0;
  virtual bool areCloudTexturesReady() = 0;

  virtual void afterD3DReset(bool full_reset) = 0;

public:
  static inline float gmt_to_localtime(float gmt_time, float longitude)
  {
    float ret = fmodf(gmt_time + longitude * (12.f / 180.f), 24.f);
    return (ret < 0) ? (ret + 24.f) : ret;
  }
  static inline float localtime_to_gmt(float local_time, float longitude)
  {
    float ret = fmodf(local_time - longitude * (12.f / 180.f), 24.f);
    return (ret < 0) ? (ret + 24.f) : ret;
  }
  static inline float julian_day_to_gmt(double jd) { return fmodf(float((jd - floor(jd)) * 24.f + 12.f), 24.f); }
};
