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
void ECS::addEntityWrite(das::ModuleLibrary &lib)
{
#define TYPE(type)                                                                                                             \
  das::addExtern<DAS_BIND_FUN(entitySetHint##type)>(*this, lib, "set", das::SideEffects::modifyExternal,                       \
    "bind_dascript::entitySetHint" #type)                                                                                      \
    ->annotations.push_back(annotation_declaration(das::make_smart<EcsSetAnnotation<1, 3, /*optional*/ false>>(#type)));       \
  auto entitySetExt##type = das::addExtern<DAS_BIND_FUN(entitySet##type)>(*this, lib, "set", das::SideEffects::modifyExternal, \
    "bind_dascript::entitySet" #type);                                                                                         \
  entitySetExt##type->annotations.push_back(                                                                                   \
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE

  das::addExtern<DAS_BIND_FUN(entitySetStrHint)>(*this, lib, "set", das::SideEffects::modifyExternal,
    "bind_dascript::entitySetStrHint");
  auto entitySetStrExt =
    das::addExtern<DAS_BIND_FUN(entitySetStr)>(*this, lib, "set", das::SideEffects::modifyExternal, "bind_dascript::entitySetStr");
  entitySetStrExt->annotations.push_back(
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));

  das::addExtern<DAS_BIND_FUN(setEntityChildComponentHint)>(*this, lib, "set", das::SideEffects::modifyExternal,
    "bind_dascript::setEntityChildComponentHint")
    ->annotations.push_back(annotation_declaration(das::make_smart<EcsSetAnnotation<1, 3, /*optional*/ false>>("ChildComponent")));
  auto setEntityChildComponentExt = das::addExtern<DAS_BIND_FUN(setEntityChildComponent)>(*this, lib, "set",
    das::SideEffects::modifyExternal, "bind_dascript::setEntityChildComponent");
  setEntityChildComponentExt->annotations.push_back(
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
}
} // namespace bind_dascript
