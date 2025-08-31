// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/core/entityManager.h>

struct ECSEntityCreateData
{
  eastl::string templName;
  ecs::ComponentsInitializer attrs;
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  ECSEntityCreateData(ecs::EntityId e, const char *template_name = NULL);
  ECSEntityCreateData(const char *template_name, const TMatrix *tm = NULL);
  ECSEntityCreateData(const char *template_name, const TMatrix *tm, const char *riex_name);

  static void createCb(ECSEntityCreateData *data);
};
