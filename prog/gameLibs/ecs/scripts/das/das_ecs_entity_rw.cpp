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
#define TYPE(type)                                                                                                           \
  das::addExtern<DAS_BIND_FUN(entityGetNullableRWHint_##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyExternal, \
    "bind_dascript::entityGetNullableRWHint_" #type);                                                                        \
  auto entityGetNullableRWExt##type = das::addExtern<DAS_BIND_FUN(entityGetNullableRW_##type)>(*this, lib, "getRW_" #type,   \
    das::SideEffects::modifyExternal, "bind_dascript::entityGetNullableRW_" #type);                                          \
  entityGetNullableRWExt##type->annotations.push_back(                                                                       \
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE
}
} // namespace bind_dascript
