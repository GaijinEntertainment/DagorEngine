//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>

struct HealAnim
{
  int enumValue;
  int enumTargetValue;

  HealAnim(const ecs::EntityManager &mgr, ecs::EntityId eid);
};

ECS_DECLARE_RELOCATABLE_TYPE(HealAnim);
