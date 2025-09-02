// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include "render/fx/effectEntity.h"
#include <effectManager/effectManager.h>


template <typename Callable>
inline void get_wind_strength_ecs_query(Callable c);

ECS_BEFORE(bound_camera_effect_es)
ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
static void storm_es_event_handler(const ecs::Event &,
  TMatrix &transform,
  float storm__speed,
  float storm__width,
  float storm__density,
  float storm__alpha,
  float storm__length,
  Point3 &effect__offset,
  TheEffect &effect,
  float storm__maxDensity = 10)
{
  float spawnRate = storm__density / storm__maxDensity;
  float windPower;
  Point3 windDir;
  get_wind_strength_ecs_query([&](float wind__strength, float wind__dir) {
    windPower = wind__strength;
    windDir = Point3(cosf(DegToRad(wind__dir)), 0.0f, sinf(DegToRad(wind__dir)));
  });

  effect__offset = effect__offset - windDir * (windPower * 0.5);
  TMatrix fxTm = TMatrix::IDENT;
  fxTm.setcol(3, (transform.getcol(3)));
  fxTm.setcol(0, fxTm.getcol(0) * storm__width);
  fxTm.setcol(1, fxTm.getcol(1) * storm__length);
  fxTm.setcol(2, fxTm.getcol(2) * storm__width);
  transform = fxTm;

  for (auto &fx : effect.getEffects())
  {
    fx.fx->setSpawnRate(min(spawnRate, 1.0f));
    fx.fx->setColorMult(Color4(1.0f, 1.0f, 1.0f, storm__alpha));
    fx.fx->setVelocity(Point3(0, -storm__speed / storm__length, 0));
  }
}