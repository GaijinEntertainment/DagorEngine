#include "das_ecs.h"
#include <dasModules/dasMacro.h>
#include <dasModules/dasEvent.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>

namespace bind_dascript
{
struct ComponentsInitializerAnnotation final : das::ManagedStructureAnnotation<ecs::ComponentsInitializer, false>
{
  ComponentsInitializerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ComponentsInitializer", ml)
  {
    cppName = " ::ecs::ComponentsInitializer";
  }
};


#define ADD_EXTERN(fun, side_effects) das::addExtern<DAS_BIND_FUN(fun)>(*this, lib, #fun, side_effects, "bind_dascript::" #fun)
#define ADD_EXTERN_NAME(fun, name, side_effects) \
  das::addExtern<DAS_BIND_FUN(fun)>(*this, lib, name, side_effects, "bind_dascript::" #fun)

void ECS::addInitializers(das::ModuleLibrary &lib)
{
  addAnnotation(das::make_smart<ComponentsInitializerAnnotation>(lib));

  das::addUsing<ecs::ComponentsInitializer>(*this, lib, "ecs::ComponentsInitializer");

// initializer
#define TYPE(type)                                                                                                                    \
  das::addExtern<DAS_BIND_FUN(setInitHint##type)>(*this, lib, "set", das::SideEffects::modifyArgument,                                \
    "bind_dascript::setInitHint" #type)                                                                                               \
    ->annotations.push_back(annotation_declaration(das::make_smart<EcsInitSetAnnotation<1, 3>>(#type)));                              \
  auto setInitExt##type =                                                                                                             \
    das::addExtern<DAS_BIND_FUN(setInit##type)>(*this, lib, "set", das::SideEffects::modifyArgument, "bind_dascript::setInit" #type); \
  setInitExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE
  das::addExtern<DAS_BIND_FUN(setInitStrHint)>(*this, lib, "set", das::SideEffects::modifyArgument, "bind_dascript::setInitStrHint")
    ->annotations.push_back(annotation_declaration(das::make_smart<EcsInitSetAnnotation<1, 3>>("Str")));
  auto setInitStrExt =
    das::addExtern<DAS_BIND_FUN(setInitStr)>(*this, lib, "set", das::SideEffects::modifyArgument, "bind_dascript::setInitStr");
  setInitStrExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));

  das::addExtern<DAS_BIND_FUN(setVec4f)>(*this, lib, "setVec4f", das::SideEffects::modifyArgument, "bind_dascript::setVec4f");

  das::addExtern<DAS_BIND_FUN(setInitChildComponentHint)>(*this, lib, "set", das::SideEffects::modifyArgument,
    "bind_dascript::setInitChildComponentHint")
    ->annotations.push_back(annotation_declaration(das::make_smart<EcsInitSetAnnotation<1, 3>>("ChildComp")));
  auto setInitChildComponentExt = das::addExtern<DAS_BIND_FUN(setInitChildComponent)>(*this, lib, "set",
    das::SideEffects::modifyArgument, "bind_dascript::setInitChildComponent");
  setInitChildComponentExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
}
} // namespace bind_dascript
