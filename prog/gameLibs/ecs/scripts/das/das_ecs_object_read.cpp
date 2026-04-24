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
void ECS::addObjectRead(das::ModuleLibrary &lib)
{
  addObjectReadBase(lib);
  addObjectReadList(lib);

  das::addExtern<DAS_BIND_FUN(is_object_empty)>(*this, lib, "empty", das::SideEffects::none, "bind_dascript::is_object_empty");
  das::addExtern<DAS_BIND_FUN(get_object_length)>(*this, lib, "length", das::SideEffects::none, "bind_dascript::get_object_length");

  // TODO: change side effect to none, issue with annotation
  das::addExtern<DAS_BIND_FUN(objectHasHint)>(*this, lib, "has", das::SideEffects::accessExternal, "bind_dascript::objectHasHint");
  auto objectHasExt =
    das::addExtern<DAS_BIND_FUN(objectHas)>(*this, lib, "has", das::SideEffects::accessExternal, "bind_dascript::objectHas");
  objectHasExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(objectGetStringHint)>(*this, lib, "get_string", das::SideEffects::none,
    "bind_dascript::objectGetStringHint");
  auto objectGetStringExt = das::addExtern<DAS_BIND_FUN(objectGetString)>(*this, lib, "get_string", das::SideEffects::accessExternal,
    "bind_dascript::objectGetString");
  objectGetStringExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(objectPtrGetStringHint)>(*this, lib, "get_string", das::SideEffects::none,
    "bind_dascript::objectPtrGetStringHint");
  auto objectPtrGetStringExt = das::addExtern<DAS_BIND_FUN(objectPtrGetString)>(*this, lib, "get_string",
    das::SideEffects::accessExternal, "bind_dascript::objectPtrGetString");
  objectPtrGetStringExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(objectGetChildHint)>(*this, lib, "get_child", das::SideEffects::none,
    "bind_dascript::objectGetChildHint");
  auto objectGetChildExt = das::addExtern<DAS_BIND_FUN(objectGetChild)>(*this, lib, "get_child", das::SideEffects::accessExternal,
    "bind_dascript::objectGetChild");
  objectGetChildExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(objectPtrGetChildHint)>(*this, lib, "get_child", das::SideEffects::none,
    "bind_dascript::objectPtrGetChildHint");
  auto objectPtrGetChildExt = das::addExtern<DAS_BIND_FUN(objectPtrGetChild)>(*this, lib, "get_child",
    das::SideEffects::accessExternal, "bind_dascript::objectPtrGetChild");
  objectPtrGetChildExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
}
} // namespace bind_dascript
