// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <main/water.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentType.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>

#include <fftWater/fftWater.h>

template <typename Callable>
static void get_is_underwater_ecs_query(Callable);
template <typename Callable>
static void get_waterlevel_for_camera_pos_ecs_query(Callable);

bool is_underwater()
{
  bool isUnderWater = false;
  get_is_underwater_ecs_query([&](bool is_underwater) { isUnderWater = is_underwater; });
  return isUnderWater;
}

template <typename T>
static inline void is_water_hidden_ecs_query(T cb);

bool is_water_hidden()
{
  bool waterHidden = false;
  is_water_hidden_ecs_query([&](bool water_hidden) { waterHidden = water_hidden; });
  return waterHidden;
}

float get_waterlevel_for_camera_pos()
{
  float height = 10000;
  get_waterlevel_for_camera_pos_ecs_query([&](float water_level) { height = water_level; });
  return height;
}

template <typename Callable>
static void get_water_ecs_query(Callable);

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // required to increase parallelity (start `animchar_before_render_es` as early as possible)
static void update_water_level_values_es(
  const UpdateStageInfoBeforeRender &evt, FFTWater &water, bool &is_underwater, float &water_level)
{
  water_level = 0;
  fft_water::setRenderParamsToPhysics(&water);
  fft_water::getHeightAboveWater(&water, evt.camPos, water_level, true);
  is_underwater = water_level < 0 && !is_water_hidden();
}
