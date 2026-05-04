// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <dasModules/dasMacro.h>
#include <dasModules/dasEvent.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <dasModules/dasModulesCommon.h>
#include <daScript/ast/ast_policy_types.h>
#include <daScript/simulate/sim_policy.h>
#include <dasModules/aotEcs.h>

namespace bind_dascript
{
void ECS::addEntityWriteOptionalBase(das::ModuleLibrary &lib)
{
#define DAS_ECS_BIND_ENTITY_WRITE_OPTIONAL
#define ECS_BIND_TYPE_LIST ECS_BASE_TYPE_LIST
#include "das_ecs_type_registration.inl"
}
} // namespace bind_dascript
