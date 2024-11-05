// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/updateStageRender.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <render/renderEvent.h>
#include "render/fx/effectEntity.h"
#include "render/fx/effectManager.h"
#include "render/weather/rain.h"
#include <util/dag_convar.h>

static int wetness_paramsVarId = -1;
static int droplets_speedVarId = -1;
static int puddle_increaseVarId = -1;

Rain::Rain()
{
  parDensity = 15.0f;

  useDropSplashes = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("rain_dropSplashes_enabled", true);

  wetness_paramsVarId = ::get_shader_variable_id("wetness_params", true);
  droplets_speedVarId = ::get_shader_variable_id("droplets_speed", true);
  puddle_increaseVarId = ::get_shader_variable_id("puddle_increase", true);
}

Rain::~Rain()
{
  ShaderGlobal::set_color4(wetness_paramsVarId, Color4(0, 0, 0.65, 0));
  ShaderGlobal::set_real(puddle_increaseVarId, 0);
}

void Rain::update(float dt, float growthRate, float limit)
{
  growth = min(growth + parDensity * growthRate * dt, limit);
  ShaderGlobal::set_real(puddle_increaseVarId, growth);
}

ECS_REGISTER_RELOCATABLE_TYPE(Rain, nullptr);
ECS_AUTO_REGISTER_COMPONENT(Rain, "rain", nullptr, 0);

ECS_TAG(render)
ECS_NO_ORDER
static inline void update_rain_effects_es(
  const ecs::UpdateStageInfoBeforeRender &info, Rain &rain, float puddles__growthRate, float puddles__growthLimit)
{
  TIME_D3D_PROFILE(rain_update);
  rain.update(info.dt, puddles__growthRate, puddles__growthLimit);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
static void update_drop_effects_es(const ecs::Event &, const float &drop_splash_fx__spawnRate, TheEffect &effect)
{
  for (auto fx : effect.getEffects())
  {
    fx.fx->setSpawnRate(min(drop_splash_fx__spawnRate, 1.0f));
  }
}

template <typename Callable>
inline void set_params_for_splashes_ecs_query(ecs::EntityId, Callable c);
ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
static void rain_es_event_handler(const ecs::Event &,
  TMatrix &transform,
  Rain &rain,
  float far_rain__length,
  float far_rain__speed,
  float far_rain__width,
  float far_rain__density,
  float far_rain__alpha,
  float wetness__strength,
  float drop_splashes__spriteYPos,
  TheEffect &effect,
  const ecs::string &drop_fx_template,
  ecs::EntityId &drop_splashes__splashesFxId,
  float far_rain__maxDensity = 10)
{
  float spawnRate = far_rain__density / far_rain__maxDensity;
  TMatrix fxTm = TMatrix::IDENT;
  fxTm.setcol(3, (transform.getcol(3)));
  fxTm.setcol(0, fxTm.getcol(0) * far_rain__width);
  fxTm.setcol(1, fxTm.getcol(1) * far_rain__length);
  fxTm.setcol(2, fxTm.getcol(2) * far_rain__width);
  transform = fxTm;

  for (auto &fx : effect.getEffects())
  {
    fx.fx->setSpawnRate(min(spawnRate, 1.0f));
    fx.fx->setColorMult(Color4(1.0f, 1.0f, 1.0f, far_rain__alpha));
    fx.fx->setVelocity(Point3(0, -far_rain__speed / far_rain__length, 0));
  }

  Color4 wetnessParams = ShaderGlobal::get_color4(wetness_paramsVarId);
  wetnessParams.r = wetness__strength;
  wetnessParams.b = 1;
  ShaderGlobal::set_color4(wetness_paramsVarId, wetnessParams);
  ShaderGlobal::set_real(droplets_speedVarId, 1 / (max(wetness__strength, 0.f) + 0.5f));

  if (rain.getUseDropSplashes())
  {
    if (drop_splashes__splashesFxId == ecs::INVALID_ENTITY_ID)
    {
      ecs::ComponentsInitializer attrs;
      TMatrix effectTm = transform;
      effectTm.m[3][1] = drop_splashes__spriteYPos;
      attrs[ECS_HASH("transform")] = effectTm;
      attrs[ECS_HASH("drop_splash_fx__spawnRate")] = spawnRate;
      drop_splashes__splashesFxId = g_entity_mgr->createEntityAsync(drop_fx_template.c_str(), eastl::move(attrs));
    }
    else
    {
      set_params_for_splashes_ecs_query(drop_splashes__splashesFxId,
        [&](float &drop_splash_fx__spawnRate) { drop_splash_fx__spawnRate = spawnRate; });
    }
  }
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(Rain &rain)
static void detach_rain_es_event_handler(const ecs::Event &, ecs::EntityId &drop_splashes__splashesFxId)
{
  g_entity_mgr->destroyEntity(drop_splashes__splashesFxId);
}