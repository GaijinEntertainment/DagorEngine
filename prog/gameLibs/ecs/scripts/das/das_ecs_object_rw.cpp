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
void ECS::addObjectRW(das::ModuleLibrary &lib)
{
  addObjectRWBase(lib);
  addObjectRWList(lib);

  das::addExtern<DAS_BIND_FUN(objectGetRWChildHint)>(*this, lib, "getRW_child", das::SideEffects::none,
    "bind_dascript::objectGetRWChildHint");
  auto objectGetRWChildExt = das::addExtern<DAS_BIND_FUN(objectGetRWChild)>(*this, lib, "getRW_child",
    das::SideEffects::accessExternal, "bind_dascript::objectGetRWChild");
  objectGetRWChildExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(objectPtrGetRWChildHint)>(*this, lib, "getRW_child", das::SideEffects::none,
    "bind_dascript::objectPtrGetRWChildHint");
  auto objectPtrGetRWChildExt = das::addExtern<DAS_BIND_FUN(objectPtrGetRWChild)>(*this, lib, "getRW_child",
    das::SideEffects::accessExternal, "bind_dascript::objectPtrGetRWChild");
  objectPtrGetRWChildExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
}
} // namespace bind_dascript
