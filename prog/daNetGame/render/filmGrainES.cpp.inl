// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <render/priorityManagedShadervar.h>
#include <perfMon/dag_statDrv.h>
#include "render/renderEvent.h"

#include "filmGrain.h"
#include "renderSettings.h"

static ShaderVariableInfo film_grain_lut_paramsVarId("film_grain_lut_params", true);
static ShaderVariableInfo film_grain_gen_paramsVarId("film_grain_gen_params", true);
static ShaderVariableInfo film_grain_gen_sizeVarId("film_grain_gen_size", true);

void FilmGrainLutHolder::requestRebuild()
{
  rebuildRequested = true;
  // restart generation from scratch if already in progress
  if (genSlice >= 0)
    genSlice = -1;
}

void FilmGrainLutHolder::setSettings(const Point4 &value, int prio, int wh, int d, const Point4 &gen_params)
{
  PriorityShadervar::set_float4((int)film_grain_lut_paramsVarId, prio, value);
  ShaderGlobal::set_float4(film_grain_gen_paramsVarId, gen_params);
  if (wh == lutWH && d == lutD && gen_params == genParams && lut.getBaseTex())
    return;
  genParams = gen_params;
  lutWH = wh;
  lutD = d;
  requestRebuild();
}

void FilmGrainLutHolder::disable()
{
  lut.close();
  genCs.reset();
  genSlice = -1;
  rebuildRequested = false;
  PriorityShadervar::clear((int)film_grain_lut_paramsVarId, SETTINGS_PRIORITY);
}

void FilmGrainLutHolder::reinitFromSettings(int es_wh, int es_d, const Point4 &gen_params)
{
  bool filmGrain = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("filmGrain", false);
  Point4 value = dgs_get_settings()->getBlockByNameEx("cinematicEffects")->getPoint4("filmGrain", Point4(0.2, 0.1, 1, 0));
  if (filmGrain)
    setSettings(value, SETTINGS_PRIORITY, es_wh, es_d, gen_params);
  else
    disable();
}

bool FilmGrainLutHolder::generate()
{
  if (!rebuildRequested)
    return true;

  TIME_D3D_PROFILE(generateFilmGrainLut);

  // First call (or restart after requestRebuild during generation)
  if (genSlice < 0)
  {
    PriorityShadervar::set_float4((int)film_grain_lut_paramsVarId, GEN_PRIORITY, Point4(0, 0, 0, 0));

    lut.close();
    uint32_t fmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(fmt) & d3d::USAGE_UNORDERED))
      fmt = TEXFMT_DEFAULT;
    lut = dag::create_voltex(lutWH, lutWH, lutD, fmt | TEXCF_UNORDERED, 1, "film_grain_lut");
    lut.setVar();

    genCs.reset(new_compute_shader("film_grain_generate_lut", true));
    if (!genCs)
    {
      lut.close();
      rebuildRequested = false;
      return true;
    }
    genSlice = 0;
  }

  // Dispatch a batch of Z slices
  int slices = min(lutD - genSlice, SLICES_PER_STEP);
  ShaderGlobal::set_float4(film_grain_gen_sizeVarId, lutWH, lutWH, lutD, genSlice);
  genCs->dispatchThreads(lutWH, lutWH, slices);
  genSlice += slices;

  if (genSlice >= lutD)
  {
    genCs.reset();
    genSlice = -1;
    PriorityShadervar::clear((int)film_grain_lut_paramsVarId, GEN_PRIORITY);
    rebuildRequested = false;
    return true;
  }
  return false;
}

void FilmGrainLutHolder::afterDeviceReset()
{
  if (lut)
    requestRebuild();
}

ECS_DECLARE_RELOCATABLE_TYPE(FilmGrainLutHolder);
ECS_REGISTER_RELOCATABLE_TYPE(FilmGrainLutHolder, nullptr);
ECS_AUTO_REGISTER_COMPONENT(FilmGrainLutHolder, "film_grain_lut", nullptr, 0);

ECS_TAG(render)
ECS_ON_EVENT(AfterDeviceReset)
static void film_grain_lut_after_device_reset_es_event_handler(const ecs::Event &, FilmGrainLutHolder &film_grain_lut)
{
  film_grain_lut.afterDeviceReset();
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear, OnRenderSettingsUpdated)
ECS_TRACK(film_grain_lut__wh, film_grain_lut__d, film_grain_lut__gen_params)
static void film_grain_lut_init_es_event_handler(const ecs::Event &,
  FilmGrainLutHolder &film_grain_lut,
  int film_grain_lut__wh = 256,
  int film_grain_lut__d = 64,
  const Point4 &film_grain_lut__gen_params = Point4(0, 0, 0, 0))
{
  film_grain_lut.reinitFromSettings(film_grain_lut__wh, film_grain_lut__d, film_grain_lut__gen_params);
}

ECS_TAG(render)
static void film_grain_lut_generate_es(const UpdateStageInfoBeforeRender &, FilmGrainLutHolder &film_grain_lut)
{
  film_grain_lut.generate();
}

template <typename Callable>
static void film_grain_lut_ecs_query(ecs::EntityManager &manager, Callable c);

void FilmGrainLutHolder::enableExternalModifier(const Point4 &value)
{
  film_grain_lut_ecs_query(*g_entity_mgr, [value](FilmGrainLutHolder &film_grain_lut, int film_grain_lut__wh, int film_grain_lut__d,
                                            const Point4 &film_grain_lut__gen_params) {
    film_grain_lut.setSettings(value, EXTERNAL_PRIORITY, film_grain_lut__wh, film_grain_lut__d, film_grain_lut__gen_params);
  });
}

void FilmGrainLutHolder::disableExternalModifier() { PriorityShadervar::clear((int)film_grain_lut_paramsVarId, EXTERNAL_PRIORITY); }
