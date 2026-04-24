// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/coreEvents.h>
#include <ecs/phys/physVars.h>
#include <math/random/dag_random.h>

ECS_REGISTER_RELOCATABLE_TYPE(PhysVars, nullptr);
ECS_AUTO_REGISTER_COMPONENT(PhysVars, "phys_vars", nullptr, 0);
