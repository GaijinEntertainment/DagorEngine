//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>

struct HealAnim
{
  int enumValue;
  int enumTargetValue;

  HealAnim(const ecs::EntityManager &mgr, ecs::EntityId eid);
};

ECS_DECLARE_RELOCATABLE_TYPE(HealAnim);
