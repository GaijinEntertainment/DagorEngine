// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include "render/fx/effectEntity.h"
#include "render/fx/effectManager.h"

ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
static void dust_es_event_handler(const ecs::Event &,
  TMatrix &transform,
  float dust__speed,
  float dust__width,
  float dust__density,
  float dust__alpha,
  float dust__length,
  TheEffect &effect,
  float dust__maxDensity = 10)
{
  float spawnRate = dust__density / dust__maxDensity;

  TMatrix fxTm = TMatrix::IDENT;
  fxTm.setcol(3, (transform.getcol(3)));
  fxTm.setcol(0, fxTm.getcol(0) * dust__width);
  fxTm.setcol(1, fxTm.getcol(1) * dust__length);
  fxTm.setcol(2, fxTm.getcol(2) * dust__width);
  transform = fxTm;

  for (auto &fx : effect.getEffects())
  {
    fx.fx->setSpawnRate(min(spawnRate, 1.0f));
    fx.fx->setColorMult(Color4(1.0f, 1.0f, 1.0f, dust__alpha));
    fx.fx->setVelocity(Point3(0, -dust__speed / dust__length, 0));
  }
}