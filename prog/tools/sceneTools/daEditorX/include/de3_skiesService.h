//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
class String;
class DataBlock;


class ISkiesService
{
public:
  static constexpr unsigned HUID = 0x2EF40209u; // ISkiesService

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
};
