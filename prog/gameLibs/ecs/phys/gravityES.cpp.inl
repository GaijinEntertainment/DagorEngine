// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include <gamePhys/props/atmosphere.h>

ECS_ON_EVENT(on_appear)
ECS_TRACK(phys_props__gravity)
static __forceinline void gravity_controller_appear_es_event_handler(const ecs::Event &, float phys_props__gravity)
{
  gamephys::atmosphere::set_g(phys_props__gravity);
}

ECS_REQUIRE(float phys_props__gravity)
ECS_ON_EVENT(on_disappear)
static __forceinline void gravity_controller_disappear_es_event_handler(const ecs::Event &) { gamephys::atmosphere::reset_g(); }
