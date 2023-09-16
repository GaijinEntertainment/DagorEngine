#include "das_ecs.h"
#include <dasModules/dasMacro.h>
#include <dasModules/dasEvent.h>
#include <daECS/core/entityManager.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>

namespace bind_dascript
{
void ECS::addArray(das::ModuleLibrary &lib)
{
#define TYPE(type)                                                                                                                    \
  das::addExtern<DAS_BIND_FUN(setArray##type)>(*this, lib, "set", das::SideEffects::modifyArgument, "bind_dascript::setArray" #type); \
  das::addExtern<DAS_BIND_FUN(getArray##type)>(*this, lib, "get_" #type, das::SideEffects::none, "bind_dascript::getArray" #type);    \
  das::addExtern<DAS_BIND_FUN(getArrayRW##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyArgument,                        \
    "bind_dascript::getArrayRW" #type);                                                                                               \
  das::addExtern<DAS_BIND_FUN(pushArray##type)>(*this, lib, "push", das::SideEffects::modifyArgument,                                 \
    "bind_dascript::pushArray" #type);                                                                                                \
  das::addExtern<DAS_BIND_FUN(pushAtArray##type)>(*this, lib, "push", das::SideEffects::modifyArgument,                               \
    "bind_dascript::pushAtArray" #type);
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE

  das::addUsing<ecs::Array>(*this, lib, " ::ecs::Array");
  das::addExtern<DAS_BIND_FUN(is_array_empty)>(*this, lib, "empty", das::SideEffects::none, "bind_dascript::is_array_empty");
  das::addExtern<DAS_BIND_FUN(get_array_length)>(*this, lib, "length", das::SideEffects::none, "bind_dascript::get_array_length");
  das::addExtern<DAS_BIND_FUN(pushArrayStr), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "push",
    das::SideEffects::modifyArgument, "bind_dascript::pushArrayStr");
  das::addExtern<DAS_BIND_FUN(pushAtArrayStr), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "push",
    das::SideEffects::modifyArgument, "bind_dascript::pushAtArrayStr");
  das::addExtern<DAS_BIND_FUN(pushChildComponent), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "push",
    das::SideEffects::modifyArgument, "bind_dascript::pushChildComponent");
  das::addExtern<DAS_BIND_FUN(popArray), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "pop",
    das::SideEffects::modifyArgument, "bind_dascript::popArray");
  das::addExtern<DAS_BIND_FUN(clearArray), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "clear",
    das::SideEffects::modifyArgument, "bind_dascript::clearArray");
  das::addExtern<DAS_BIND_FUN(reserveArray), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "reserve",
    das::SideEffects::modifyArgument, "bind_dascript::reserveArray");
  das::addExtern<DAS_BIND_FUN(moveArray), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "move",
    das::SideEffects::modifyArgument, "bind_dascript::moveArray");
  das::addExtern<DAS_BIND_FUN(eraseArray), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "erase",
    das::SideEffects::modifyArgument, "bind_dascript::eraseArray");
  das::addExtern<DAS_BIND_FUN(eraseCountArray), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "erase",
    das::SideEffects::modifyArgument, "bind_dascript::eraseCountArray");
  das::addExtern<DAS_BIND_FUN(das_Array_each), das::SimNode_ExtFuncCallAndCopyOrMove, das::explicitConstArgFn>(*this, lib, "each",
    das::SideEffects::none, "bind_dascript::das_Array_each");
  das::addExtern<DAS_BIND_FUN(das_Array_each_const), das::SimNode_ExtFuncCallAndCopyOrMove, das::explicitConstArgFn>(*this, lib,
    "each", das::SideEffects::none, "bind_dascript::das_Array_each_const");
}
} // namespace bind_dascript
