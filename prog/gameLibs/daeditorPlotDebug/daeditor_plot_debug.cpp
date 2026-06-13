// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daeditor_plot_debug.h"

#if DAGOR_DBGLEVEL > 0

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>

void daeditor_plot_debug_signal(float value, int slot)
{
  if (slot < 0)
    return;

  ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("daeditor_plot_debug"));
  if (eid == ecs::INVALID_ENTITY_ID)
    return;

  ecs::FloatList *signals = g_entity_mgr->getNullableRW<ecs::FloatList>(eid, ECS_HASH("daeditor_plot_debug__signals"));
  if (!signals || slot >= (int)signals->size())
    return;

  (*signals)[slot] = value;
}

float daeditor_plot_debug_get_signal(int slot)
{
  if (slot < 0)
    return 0.0f;

  ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("daeditor_plot_debug"));
  if (eid == ecs::INVALID_ENTITY_ID)
    return 0.0f;

  const ecs::FloatList *signals = g_entity_mgr->getNullable<ecs::FloatList>(eid, ECS_HASH("daeditor_plot_debug__signals"));
  if (!signals || slot >= (int)signals->size())
    return 0.0f;

  return (*signals)[slot];
}

#endif
