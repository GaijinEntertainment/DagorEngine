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
#define ADD_EXTERN(fun, side_effects) das::addExtern<DAS_BIND_FUN(fun)>(*this, lib, #fun, side_effects, "bind_dascript::" #fun)

void ECS::addEntityRead(das::ModuleLibrary &lib)
{
  das::addExtern<DAS_BIND_FUN(hasHint)>(*this, lib, "has", das::SideEffects::accessExternal, "bind_dascript::hasHint");
  auto hasExt = ADD_EXTERN(has, das::SideEffects::accessExternal);
  hasExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));

  // get
  //  da s::addExtern<DAS_BIND_FUN(entityGetHint_##type)>(*this, lib, "getValue_" #type,\
  //   das::SideEffects::accessExternal, "bind_dascript::entityGetHint_" #type);\
  // auto entityGetExt##type = das::addExtern<DAS_BIND_FUN(entityGet_##type)>(*this, lib, "getValue_" #type,\
  //   das::SideEffects::accessExternal, "bind_dascript::entityGet_" #type);\
  // entityGetExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast
  //  call*/true>>()));
#define TYPE(type)                                                                                                             \
  auto entityGetNullable##type = das::addExtern<DAS_BIND_FUN(entityGetNullableHint_##type)>(*this, lib, "get_" #type,          \
    das::SideEffects::accessExternal, "bind_dascript::entityGetNullableHint_" #type);                                          \
  entityGetNullable##type->annotations.push_back(annotation_declaration(das::make_smart<EcsGetOrFunctionAnnotation<1, 3>>())); \
  auto entityGetNullableExt##type = das::addExtern<DAS_BIND_FUN(entityGetNullable_##type)>(*this, lib, "get_" #type,           \
    das::SideEffects::accessExternal, "bind_dascript::entityGetNullable_" #type);                                              \
  entityGetNullableExt##type->annotations.push_back(                                                                           \
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE

  auto entityGetStringHintExt = das::addExtern<DAS_BIND_FUN(entityGetStringHint)>(*this, lib, "get_string",
    das::SideEffects::accessExternal, "bind_dascript::entityGetStringHint");
  entityGetStringHintExt->annotations.push_back(annotation_declaration(das::make_smart<EcsGetOrFunctionAnnotation<1, 4>>()));
  auto entityGetStringExt = das::addExtern<DAS_BIND_FUN(entityGetString)>(*this, lib, "get_string", das::SideEffects::accessExternal,
    "bind_dascript::entityGetString");
  entityGetStringExt->annotations.push_back(
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
}
} // namespace bind_dascript
