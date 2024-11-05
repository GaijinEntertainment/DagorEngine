// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/updateStageRender.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_convar.h>
#include "render/fx/effectEntity.h"
#include "render/fx/effectManager.h"

ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
static void snow_es_event_handler(const ecs::Event &,
  TMatrix &transform,
  float snow__length,
  float snow__speed,
  float snow__width,
  float snow__density,
  float snow__alpha,
  TheEffect *effect = nullptr,
  float snow__maxDensity = 10)
{
  float spawnRate = snow__density / snow__maxDensity;
  TMatrix fxTm = TMatrix::IDENT;
  fxTm.setcol(3, (transform.getcol(3)));
  fxTm.setcol(0, fxTm.getcol(0) * snow__width);
  fxTm.setcol(1, fxTm.getcol(1) * snow__length);
  fxTm.setcol(2, fxTm.getcol(2) * snow__width);
  transform = fxTm;

  for (auto &fx : effect->getEffects())
  {
    fx.fx->setSpawnRate(min(spawnRate, 1.0f));
    fx.fx->setColorMult(Color4(1.0f, 1.0f, 1.0f, snow__alpha));
    fx.fx->setVelocity(Point3(0, -snow__speed / snow__length, 0));
  }
}
