// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <dasModules/dasEvent.h>
#include <dasModules/dasMacro.h>
#include <daECS/core/entityManager.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>

namespace bind_dascript
{
void ECS::addChildComponent(das::ModuleLibrary &lib)
{
  addChildComponentBase(lib);
  addChildComponentList(lib);

  das::addExtern<DAS_BIND_FUN(setChildComponentStr)>(*this, lib, "set", das::SideEffects::modifyArgument,
    "bind_dascript::setChildComponentStr");
}
} // namespace bind_dascript
