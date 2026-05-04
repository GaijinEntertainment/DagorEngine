// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "daECS/scene/scene.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>

struct ECSEntityCreateData
{
  eastl::string templName;
  ecs::ComponentsInitializer attrs;
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  ecs::Scene::SceneId scene = ecs::Scene::C_INVALID_SCENE_ID;
  uint32_t orderInScene = 0;

  ECSEntityCreateData(ecs::EntityId e, const char *template_name = NULL);
  ECSEntityCreateData(const char *template_name, const TMatrix *tm = NULL);
  ECSEntityCreateData(const char *template_name, const TMatrix *tm, const char *riex_name);

  static void createCb(ECSEntityCreateData *data);
};
