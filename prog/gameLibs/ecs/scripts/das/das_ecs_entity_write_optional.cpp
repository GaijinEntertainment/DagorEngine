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
void ECS::addEntityWriteOptional(das::ModuleLibrary &lib)
{
#define TYPE(type)                                                                                                       \
  das::addExtern<DAS_BIND_FUN(entitySetOptionalHint##type)>(*this, lib, "setOptional", das::SideEffects::modifyExternal, \
    "bind_dascript::entitySetOptionalHint" #type)                                                                        \
    ->annotations.push_back(annotation_declaration(das::make_smart<EcsSetAnnotation<1, 3, /*optional*/ true>>(#type)));  \
  auto entitySetOptionalExt##type = das::addExtern<DAS_BIND_FUN(entitySetOptional##type)>(*this, lib, "setOptional",     \
    das::SideEffects::modifyExternal, "bind_dascript::entitySetOptional" #type);                                         \
  entitySetOptionalExt##type->annotations.push_back(                                                                     \
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE

  das::addExtern<DAS_BIND_FUN(entitySetOptionalStrHint)>(*this, lib, "setOptional", das::SideEffects::modifyExternal,
    "bind_dascript::entitySetOptionalStrHint");
  auto entitySetOptionalStrExt = das::addExtern<DAS_BIND_FUN(entitySetOptionalStr)>(*this, lib, "setOptional",
    das::SideEffects::modifyExternal, "bind_dascript::entitySetOptionalStr");
  entitySetOptionalStrExt->annotations.push_back(
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
}
} // namespace bind_dascript
