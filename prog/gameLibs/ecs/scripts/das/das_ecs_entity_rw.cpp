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
void ECS::addEntityRW(das::ModuleLibrary &lib)
{
  addEntityRWBase(lib);
  addEntityRWList(lib);
}
} // namespace bind_dascript
