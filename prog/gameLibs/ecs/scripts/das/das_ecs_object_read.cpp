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
#define TYPE(type)                                                                                                                \
  das::addExtern<DAS_BIND_FUN(getObjectHint##type)>(*this, lib, "get_" #type, das::SideEffects::none,                             \
    "bind_dascript::getObjectHint" #type);                                                                                        \
  auto getObjectExt##type = das::addExtern<DAS_BIND_FUN(getObject##type)>(*this, lib, "get_" #type, das::SideEffects::none,       \
    "bind_dascript::getObject" #type);                                                                                            \
  getObjectExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));            \
  das::addExtern<DAS_BIND_FUN(getObjectPtrHint##type)>(*this, lib, "get_" #type, das::SideEffects::none,                          \
    "bind_dascript::getObjectPtrHint" #type);                                                                                     \
  auto getObjectPtrExt##type = das::addExtern<DAS_BIND_FUN(getObjectPtr##type)>(*this, lib, "get_" #type, das::SideEffects::none, \
    "bind_dascript::getObjectPtr" #type);                                                                                         \
  getObjectPtrExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
  ECS_BASE_TYPE_LIST
  ECS_LIST_TYPE_LIST
#undef TYPE
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
