// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <main/water.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentType.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>

#include <fftWater/fftWater.h>

namespace var
{
static ShaderVariableInfo cpu_viewpos_water_level("cpu_viewpos_water_level", true);
} // namespace var

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
static void retrieve_dof_entity_for_underwater_dof_ecs_query(Callable);

template <typename Callable>
static void toggle_underwater_dof_ecs_query(Callable);

static void toggle_underwater_dof(bool is_underwater)
{
  retrieve_dof_entity_for_underwater_dof_ecs_query(
    [is_underwater](bool &dof__on, bool &dof__is_filmic, float &dof__nearDofStart, float &dof__nearDofEnd,
      float &dof__nearDofAmountPercent, float &dof__farDofStart, float &dof__farDofEnd, float &dof__farDofAmountPercent) {
      toggle_underwater_dof_ecs_query(
        [is_underwater, &dof__on, &dof__is_filmic, &dof__nearDofStart, &dof__nearDofEnd, &dof__nearDofAmountPercent, &dof__farDofStart,
          &dof__farDofEnd, &dof__farDofAmountPercent](bool &savedDof__on, bool &savedDof__is_filmic, float &savedDof__nearDofStart,
          float &savedDof__nearDofEnd, float &savedDof__nearDofAmountPercent, float &savedDof__farDofStart, float &savedDof__farDofEnd,
          float &savedDof__farDofAmountPercent, float underwaterDof__nearDofStart, float underwaterDof__nearDofEnd,
          float underwaterDof__nearDofAmountPercent, float underwaterDof__farDofStart, float underwaterDof__farDofEnd,
          float underwaterDof__farDofAmountPercent) {
          if (is_underwater)
          {
            savedDof__on = dof__on;
            savedDof__is_filmic = dof__is_filmic;
            savedDof__nearDofAmountPercent = dof__nearDofAmountPercent;
            savedDof__nearDofStart = dof__nearDofStart;
            savedDof__nearDofEnd = dof__nearDofEnd;
            savedDof__farDofStart = dof__farDofStart;
            savedDof__farDofEnd = savedDof__farDofEnd;
            savedDof__farDofAmountPercent = savedDof__farDofAmountPercent;

            dof__on = true;
            dof__is_filmic = false;
            dof__nearDofAmountPercent = underwaterDof__nearDofAmountPercent;
            dof__nearDofStart = underwaterDof__nearDofStart;
            dof__nearDofEnd = underwaterDof__nearDofEnd;
            dof__farDofStart = underwaterDof__farDofStart;
            dof__farDofEnd = underwaterDof__farDofEnd;
            dof__farDofAmountPercent = underwaterDof__farDofAmountPercent;
          }
          else
          {
            dof__on = savedDof__on;
            dof__is_filmic = savedDof__is_filmic;
            dof__nearDofAmountPercent = savedDof__nearDofAmountPercent;
            dof__nearDofStart = savedDof__nearDofStart;
            dof__nearDofEnd = savedDof__nearDofEnd;
            dof__farDofStart = savedDof__farDofStart;
            dof__farDofEnd = savedDof__farDofEnd;
            dof__farDofAmountPercent = savedDof__farDofAmountPercent;
          }
        });
    });
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // required to increase parallelity (start `animchar_before_render_es` as early as possible)
static void update_water_level_values_es(
  const UpdateStageInfoBeforeRender &evt, FFTWater &water, bool &is_underwater, float &water_level)
{
  water_level = 0;
  fft_water::setRenderParamsToPhysics(&water);
  fft_water::getHeightAboveWater(&water, evt.camPos, water_level, true);
  bool wasUnderwater = is_underwater;
  is_underwater = water_level < 0 && !is_water_hidden();
  if (wasUnderwater != is_underwater)
    toggle_underwater_dof(is_underwater);
  ShaderGlobal::set_real(var::cpu_viewpos_water_level, evt.camPos.y - water_level);
}
