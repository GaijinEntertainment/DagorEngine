// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/phys/animPhys.h>

#include <ecs/phys/physVars.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animchar_visbits.h>

ECS_REGISTER_RELOCATABLE_TYPE(AnimatedPhys, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(AnimatedPhys, "anim_phys", nullptr, 0, "phys_vars", "animchar");

ECS_ON_EVENT(on_appear, InvalidateAnimatedPhys)
static __forceinline void anim_phys_init_es_event_handler(const ecs::Event &, const AnimV20::AnimcharBaseComponent &animchar,
  const PhysVars &phys_vars, AnimatedPhys &anim_phys)
{
  anim_phys.init(animchar, phys_vars);
}
