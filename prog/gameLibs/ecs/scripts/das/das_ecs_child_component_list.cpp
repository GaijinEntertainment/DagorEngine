// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <dasModules/dasEvent.h>
#include <dasModules/dasMacro.h>
#include <daECS/core/entityManager.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>

namespace bind_dascript
{
void ECS::addChildComponentList(das::ModuleLibrary &lib)
{
#define DAS_ECS_BIND_CHILD_COMPONENT
#define ECS_BIND_TYPE_LIST ECS_LIST_TYPE_LIST
#include "das_ecs_type_registration.inl"
}
} // namespace bind_dascript
