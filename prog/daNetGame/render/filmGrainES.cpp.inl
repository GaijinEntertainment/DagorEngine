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
#include <util/dag_console.h>
#include "render/renderEvent.h"

#include "filmGrain.h"
#include "renderSettings.h"

static ShaderVariableInfo film_grain_lut_paramsVarId("film_grain_lut_params", true);
static ShaderVariableInfo film_grain_gen_paramsVarId("film_grain_gen_params", true);
static ShaderVariableInfo film_grain_gen_sizeVarId("film_grain_gen_size", true);

FilmGrainLutHolder::FilmGrainLutHolder() { setFilmGrainReady(false); }

FilmGrainLutHolder::~FilmGrainLutHolder() { setFilmGrainReady(false); }

void FilmGrainLutHolder::setFilmGrainReady(bool is_ready)
{
  if (is_ready)
    PriorityShadervar::clear((int)film_grain_lut_paramsVarId, GEN_PRIORITY);
  else
    PriorityShadervar::set_float4((int)film_grain_lut_paramsVarId, GEN_PRIORITY, Point4(0, 0, 0, 0));
}

void FilmGrainLutHolder::requestRebuild()
{
  setFilmGrainReady(false);
  rebuildRequested = true;
  // restart generation from scratch if already in progress
  if (genSlice >= 0)
    genSlice = -1;
}

void FilmGrainLutHolder::setParamsFromSettings(const Point4 &value)
{
  PriorityShadervar::set_float4((int)film_grain_lut_paramsVarId, SETTINGS_PRIORITY, value);
  enabledFromSettings = true;
  if (lut.getBaseTex() == nullptr)
    requestRebuild();
}

void FilmGrainLutHolder::resetParamsFromSettings()
{
  PriorityShadervar::clear((int)film_grain_lut_paramsVarId, SETTINGS_PRIORITY);
  enabledFromSettings = false;
  if (!isLutNeeded())
    resetLut();
}

void FilmGrainLutHolder::setExternalParams(const Point4 &value)
{
  PriorityShadervar::set_float4((int)film_grain_lut_paramsVarId, EXTERNAL_PRIORITY, value);
  enabledFromExternalModifier = true;
  if (lut.getBaseTex() == nullptr)
    requestRebuild();
}

void FilmGrainLutHolder::resetExternalParams()
{
  PriorityShadervar::clear((int)film_grain_lut_paramsVarId, EXTERNAL_PRIORITY);
  enabledFromExternalModifier = false;
  if (!isLutNeeded())
    resetLut();
}

void FilmGrainLutHolder::setGenParamsSettings(const Point4 &gen_params)
{
  const bool hasLut = lut.getBaseTex() != nullptr;
  if (gen_params == genParams && hasLut == isLutNeeded())
    return;
  ShaderGlobal::set_float4(film_grain_gen_paramsVarId, gen_params);
  genParams = gen_params;
  if (isLutNeeded())
    requestRebuild();
}

void FilmGrainLutHolder::setLutResolution(int wh, int d)
{
  const bool hasLut = lut.getBaseTex() != nullptr;
  if (wh == lutWH && d == lutD && hasLut == isLutNeeded())
    return;
  lutWH = wh;
  lutD = d;
  if (isLutNeeded())
    requestRebuild();
}

void FilmGrainLutHolder::resetLut()
{
  lut.close();
  genCs.reset();
  genSlice = -1;
  rebuildRequested = false;
}

bool FilmGrainLutHolder::generate()
{
  if (!rebuildRequested)
    return true;

  TIME_D3D_PROFILE(generateFilmGrainLut);

  // First call (or restart after requestRebuild during generation)
  if (genSlice < 0)
  {
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
    rebuildRequested = false;
    setFilmGrainReady(true);
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
ECS_ON_EVENT(on_appear)
ECS_TRACK(film_grain_lut__gen_params)
static void film_grain_lut_params_change_es_event_handler(
  const ecs::Event &, FilmGrainLutHolder &film_grain_lut, const Point4 &film_grain_lut__gen_params = Point4(0, 0, 0, 0))
{
  film_grain_lut.setGenParamsSettings(film_grain_lut__gen_params);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, OnRenderSettingsUpdated)
static void film_grain_lut_settings_change_es_event_handler(const ecs::Event &evt, FilmGrainLutHolder &film_grain_lut)
{
  const bool bareMinimum = g_entity_mgr->getOr(get_render_settings(evt), ECS_HASH("render_settings__bare_minimum"), false);
  const bool needsFilmGrain = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("filmGrain", false) && !bareMinimum;
  const IPoint2 filmGrainRes = dgs_get_settings()->getBlockByNameEx("graphics")->getIPoint2("filmGrainLutRes", IPoint2(256, 64));
  film_grain_lut.setLutResolution(filmGrainRes.x, filmGrainRes.y);
  if (needsFilmGrain)
  {
    Point4 value = dgs_get_settings()->getBlockByNameEx("cinematicEffects")->getPoint4("filmGrain", Point4(0.2, 0.1, 1, 0));
    film_grain_lut.setParamsFromSettings(value);
  }
  else
    film_grain_lut.resetParamsFromSettings();
}

ECS_TAG(render)
static void film_grain_lut_generate_es(const UpdateStageInfoBeforeRender &, FilmGrainLutHolder &film_grain_lut)
{
  film_grain_lut.generate();
}

template <typename Callable>
static void film_grain_lut_ecs_query(ecs::EntityManager &manager, Callable c);

void FilmGrainLutHolder::enable_external_modifier(const Point4 &value)
{
  film_grain_lut_ecs_query(*g_entity_mgr, [value](FilmGrainLutHolder &film_grain_lut) { film_grain_lut.setExternalParams(value); });
}

void FilmGrainLutHolder::disable_external_modifier()
{
  film_grain_lut_ecs_query(*g_entity_mgr, [](FilmGrainLutHolder &film_grain_lut) { film_grain_lut.resetExternalParams(); });
}

static bool film_grain_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;

  int found = 0;
  CONSOLE_CHECK_NAME("render", "set_film_grain_lut_resolution", 3, 3)
  {
    const IPoint2 resFromSettings = dgs_get_settings()->getBlockByNameEx("graphics")->getIPoint2("filmGrainLutRes", IPoint2(256, 64));
    int wh = console::to_int(argv[1]);
    int d = console::to_int(argv[2]);
    const IPoint2 requestedRes = wh > 0 && d > 0 ? IPoint2(wh, d) : resFromSettings;
    film_grain_lut_ecs_query(*g_entity_mgr,
      [requestedRes](FilmGrainLutHolder &film_grain_lut) { film_grain_lut.setLutResolution(requestedRes.x, requestedRes.y); });
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(film_grain_console_handler);
