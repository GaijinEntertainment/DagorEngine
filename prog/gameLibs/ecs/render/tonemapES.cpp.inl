// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <memory/dag_framemem.h>
#include <math/dag_color.h>
#include <util/dag_simpleString.h>
#include <shaders/dag_shaders.h>
#include <util/dag_console.h>

#include "shader_var_from_component.h"

inline void setObjectVars(const ecs::Object *objectData, ecs::Object *save)
{
  if (!objectData)
    return;
  for (auto &var : *objectData)
    set_shader_var(var.first.data(), var.second.getEntityComponentRef());

  if (save)
  {
    for (auto &var : *objectData)
      save->addMember(ECS_HASH_SLOW(var.first.data()), var.second);
  }
}

ECS_REQUIRE(ecs::Tag tonemapper)
ECS_TRACK(white_balance)
static __forceinline void tonemapper_white_balance_es_event_handler(const ecs::Event &, const ecs::Object &white_balance)
{
  setObjectVars(&white_balance, nullptr);
}

ECS_REQUIRE(ecs::Tag tonemapper)
ECS_TRACK(color_grading)
static __forceinline void tonemapper_color_grading_es_event_handler(const ecs::Event &, const ecs::Object &color_grading)
{
  setObjectVars(&color_grading, nullptr);
}

ECS_REQUIRE(ecs::Tag tonemapper)
ECS_TRACK(tonemap) static __forceinline void tonemapper_tonemap_es_event_handler(const ecs::Event &, const ecs::Object &tonemap)
{
  setObjectVars(&tonemap, nullptr);
}


ECS_REQUIRE(ecs::Tag tonemapper)
ECS_ON_EVENT(on_appear)
static __forceinline void tonemapper_appear_es_event_handler(const ecs::Event &, const ecs::Object *white_balance,
  const ecs::Object *color_grading, const ecs::Object *tonemap, ecs::Object *tonemap_save)
{
  if (tonemap_save)
    tonemap_save->clear();
  setObjectVars(white_balance, tonemap_save);
  setObjectVars(color_grading, tonemap_save);
  setObjectVars(tonemap, tonemap_save);
}

ECS_REQUIRE(ecs::Tag tonemapper)
ECS_ON_EVENT(on_disappear)
static __forceinline void tonemapper_disapear_es_event_handler(const ecs::Event &, ecs::Object *tonemap_save)
{
  setObjectVars(tonemap_save, nullptr);
}
