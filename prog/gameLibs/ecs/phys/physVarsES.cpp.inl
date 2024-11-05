// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include <ecs/phys/physVars.h>
#include <math/random/dag_random.h>

ECS_REGISTER_RELOCATABLE_TYPE(PhysVars, nullptr);
ECS_AUTO_REGISTER_COMPONENT(PhysVars, "phys_vars", nullptr, 0);
