// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <dasModules/dasMacro.h>
#include <dasModules/dasEvent.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityManager.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>

namespace bind_dascript
{
void ECS::addObjectRWBase(das::ModuleLibrary &lib)
{
#define DAS_ECS_BIND_OBJECT_RW
#define ECS_BIND_TYPE_LIST ECS_BASE_TYPE_LIST
#include "das_ecs_type_registration.inl"
}
} // namespace bind_dascript
