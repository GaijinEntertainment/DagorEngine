// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/core/entityManager.h>

class AcesEffect;

struct EffectArea
{
  EffectArea(const ecs::EntityManager &mgr, ecs::EntityId eid);
  void initPause(float cur_time, float pause_length_max);
  float timeOfUnpause = 0.0;
  Point3 (*getRandomPoint)() = nullptr;
  int fxType;
};
