// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <dasModules/dasMacro.h>
#include <dasModules/dasEvent.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>
#include "das_es.h"
#include <daScript/simulate/aot_builtin_rtti.h>
#include <dasModules/dasCreatingTemplate.h>

MAKE_TYPE_FACTORY(CreatingTemplate, bind_dascript::CreatingTemplate);

namespace bind_dascript
{
struct ComponentsInitializerAnnotation final : das::ManagedStructureAnnotation<ecs::ComponentsInitializer, false>
{
  ComponentsInitializerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ComponentsInitializer", ml)
  {
    cppName = " ::ecs::ComponentsInitializer";
  }
};

struct TemplateAnnotation final : das::ManagedStructureAnnotation<ecs::Template, false>
{
  TemplateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Template", ml)
  {
    cppName = " ::ecs::Template";

    addProperty<DAS_BIND_MANAGED_PROP(isSingleton)>("isSingleton");
    addProperty<DAS_BIND_MANAGED_PROP(getComponentsMap)>("componentsMap", "getComponentsMap");
  }
};

// bool createTemplate2(void *group, uint32_t tag, const char* tname, const das::TArray<char *> &parents,

bool createTemplate2(das::ModuleGroup &group, uint32_t tag, const char *tname, const das::TArray<char *> &parents,
  const das::TBlock<void, CreatingTemplate> &block, das::Context *context, das::LineInfoArg *at)
{
  G_UNUSED(tag);
  ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
  if (!mgd)
    return false;
  CreatingTemplate temp{mgd->trefs};
  temp.name = tname;
  vec4f arg = das::cast<char *>::from((char *)&temp);
  mgd->trefs.templatesIds.addNameId(tname); // add in same order as in postProcessModuleGroupUserData (for correct parents resolve)
  temp.parents.resize(parents.size);
  for (int i = 0, e = parents.size; i < e; ++i)
    temp.parents[i] = mgd->trefs.ensureTemplate(parents[i]);
  context->invoke(block, &arg, nullptr, at);
  temp.compTags.shrink_to_fit();
  mgd->templates.emplace_back(eastl::move(temp));
  return true;
}

void CreatingTemplate::process_flags(das::Bitfield flags, ecs::component_t hash)
{
  if (uint32_t(flags) & 1)
    tracked.insert(hash);
  if (uint32_t(flags) & 2)
    replicated.insert(hash);
  if (uint32_t(flags) & 4)
    ignored.insert(hash);
}

bool CreatingTemplate::addInfo(das::Bitfield flags, ecs::component_type_t tp, const ecs::HashedConstString comp_name)
{
  if (db.addComponent(comp_name.hash, tp, comp_name.str))
  {
    process_flags(flags, comp_name.hash);
    return true;
  }
  return false;
}

bool setCreatingTemplateTyped(CreatingTemplate &map, das::Bitfield flags, const char *key, const char *type)
{
  if (!key)
    return false;
  ecs::HashedConstString name = ECS_HASH_SLOW(key);
  ecs::component_type_t typeName = ECS_HASH_SLOW(type).hash;
  // ecs::type_index_t cmpType = g_entity_mgr->getComponentTypes().findType(typeName);
  // if (cmpType == ecs::INVALID_COMPONENT_TYPE_INDEX)
  //   return false;
  if (!map.addInfo(flags, typeName, name))
    return false;
  // fixme: DONT_REPLICATE
  // uint32_t flag = 0;//todo: server tag is magical, and should result in flag DONT_REPLICATE
  // if (g_entity_mgr->createComponent(name, cmpType, dag::Span<ecs::component_t>(), nullptr, flag) == ecs::INVALID_COMPONENT_INDEX)
  //   return false;
  map.map[name];
  return true;
}


#define ADD_EXTERN(fun, side_effects) das::addExtern<DAS_BIND_FUN(fun)>(*this, lib, #fun, side_effects, "bind_dascript::" #fun)
#define ADD_EXTERN_NAME(fun, name, side_effects) \
  das::addExtern<DAS_BIND_FUN(fun)>(*this, lib, name, side_effects, "bind_dascript::" #fun)

void ECS::addTemplates(das::ModuleLibrary &lib)
{
  addBuiltinDependency(lib, require("rtti"));
  addAnnotation(das::make_smart<TemplateAnnotation>(lib));
  addAnnotation(das::make_smart<das::DummyTypeAnnotation>("CreatingTemplate", " ::bind_dascript::CreatingTemplate", 1, 1));

  ADD_EXTERN(getTemplateByName, das::SideEffects::accessExternal);
  ADD_EXTERN(buildTemplateByName, das::SideEffects::modifyExternal);
  ADD_EXTERN_NAME(templateHasComponentHint, "templateHasComponent", das::SideEffects::accessExternal);
  ADD_EXTERN(getTemplatePath, das::SideEffects::none);
  auto templateHasComponentExt = ADD_EXTERN(templateHasComponent, das::SideEffects::accessExternal);
  templateHasComponentExt->annotations.push_back(
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));

  ADD_EXTERN_NAME(getTemplateComponentHint, "getTemplateComponent", das::SideEffects::accessExternal);
  auto getTemplateComponentExt = ADD_EXTERN(getTemplateComponent, das::SideEffects::accessExternal);
  getTemplateComponentExt->annotations.push_back(
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));

  das::addExtern<DAS_BIND_FUN(getRegExpInheritedFlags)>(*this, lib, "getRegExpInheritedFlags", das::SideEffects::none,
    "bind_dascript::getRegExpInheritedFlags");

  ADD_EXTERN(createTemplate2, das::SideEffects::modifyExternal);
  das::addExtern<DAS_BIND_FUN(setCreatingTemplateUndef)>(*this, lib, "set_undef", das::SideEffects::modifyArgumentAndExternal,
    "bind_dascript::setCreatingTemplateUndef");
#define TYPE(type)                                                                                             \
  das::addExtern<DAS_BIND_FUN(setCreatingTemplate##type)>(*this, lib, "set", das::SideEffects::modifyArgument, \
    "bind_dascript::setCreatingTemplate" #type);
  ECS_BASE_TYPE_LIST
#undef TYPE
  das::addExtern<DAS_BIND_FUN(setCreatingTemplateStr)>(*this, lib, "set", das::SideEffects::modifyArgument,
    "bind_dascript::setCreatingTemplateStr");

  using methodAddTag = DAS_CALL_MEMBER(CreatingTemplate::addTag);
  das::addExtern<DAS_CALL_METHOD(methodAddTag)>(*this, lib, "creating_template_addTag", das::SideEffects::modifyArgument,
    DAS_CALL_MEMBER_CPP(CreatingTemplate::addTag));

  using method_addCompTag = DAS_CALL_MEMBER(CreatingTemplate::addCompTag);
  das::addExtern<DAS_CALL_METHOD(method_addCompTag)>(*this, lib, "creating_template_addCompTag", das::SideEffects::modifyArgument,
    DAS_CALL_MEMBER_CPP(CreatingTemplate::addCompTag));
}

} // namespace bind_dascript
