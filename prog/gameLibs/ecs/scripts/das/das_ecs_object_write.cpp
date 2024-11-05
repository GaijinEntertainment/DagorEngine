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
void ECS::addObjectWrite(das::ModuleLibrary &lib)
{
#define TYPE(type)                                                                                                                   \
  das::addExtern<DAS_BIND_FUN(setObjectHint##type), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "set",                \
    das::SideEffects::modifyArgument, "bind_dascript::setObjectHint" #type);                                                         \
  auto setObjectExt##type = das::addExtern<DAS_BIND_FUN(setObject##type), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, \
    "set", das::SideEffects::modifyArgument, "bind_dascript::setObject" #type);                                                      \
  setObjectExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE

  das::addUsing<ecs::Object>(*this, lib, " ::ecs::Object");
  das::addExtern<DAS_BIND_FUN(setObjectStrHint), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "set",
    das::SideEffects::modifyArgument, "bind_dascript::setObjectStrHint");
  auto setObjectStrExt = das::addExtern<DAS_BIND_FUN(setObjectStr), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "set",
    das::SideEffects::modifyArgument, "bind_dascript::setObjectStr");
  setObjectStrExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(setObjectChildComponentHint), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "set",
    das::SideEffects::modifyArgument, "bind_dascript::setObjectChildComponentHint");
  auto setObjectChildComponentExt =
    das::addExtern<DAS_BIND_FUN(setObjectChildComponent), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "set",
      das::SideEffects::modifyArgument, "bind_dascript::setObjectChildComponent");
  setObjectChildComponentExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(insertHint), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "insert",
    das::SideEffects::modifyArgument, "bind_dascript::insertHint");
  auto insertExt = das::addExtern<DAS_BIND_FUN(insert), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "insert",
    das::SideEffects::modifyArgument, "bind_dascript::insert");
  insertExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(clearObject), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "clear",
    das::SideEffects::modifyArgument, "bind_dascript::clearObject");

  das::addExtern<DAS_BIND_FUN(eraseObjectHint), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "erase",
    das::SideEffects::modifyArgument, "bind_dascript::eraseObjectHint");
  auto eraseObjectExt = das::addExtern<DAS_BIND_FUN(eraseObject), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "erase",
    das::SideEffects::modifyArgument, "bind_dascript::eraseObject");
  eraseObjectExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(moveObject), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "move",
    das::SideEffects::modifyArgument, "bind_dascript::moveObject");

  das::addExtern<DAS_BIND_FUN(swapObjects), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "swap",
    das::SideEffects::modifyArgument, "bind_dascript::swapObjects");
}
} // namespace bind_dascript
