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
// ChildComponent
#define TYPE(type)                                                                                                                    \
  das::addExtern<DAS_BIND_FUN(setChildComponent##type)>(*this, lib, "set", das::SideEffects::modifyArgument,                          \
    "bind_dascript::setChildComponent" #type);                                                                                        \
  das::addExtern<DAS_BIND_FUN(getChildComponent##type)>(*this, lib, "get_" #type, das::SideEffects::none,                             \
    "bind_dascript::getChildComponent" #type);                                                                                        \
  das::addExtern<DAS_BIND_FUN(getChildComponentRW##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyArgument,               \
    "bind_dascript::getChildComponentRW" #type);                                                                                      \
  das::addExtern<DAS_BIND_FUN(getChildComponentPtr##type)>(*this, lib, "get_" #type, das::SideEffects::none,                          \
    "bind_dascript::getChildComponentPtr" #type);                                                                                     \
  das::addExtern<DAS_BIND_FUN(getChildComponentPtrRW##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyArgumentAndExternal, \
    "bind_dascript::getChildComponentPtrRW" #type);
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE
  das::addExtern<DAS_BIND_FUN(setChildComponentStr)>(*this, lib, "set", das::SideEffects::modifyArgument,
    "bind_dascript::setChildComponentStr");
}
} // namespace bind_dascript
