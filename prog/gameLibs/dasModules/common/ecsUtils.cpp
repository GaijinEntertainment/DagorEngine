// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsUtils.h>
#include <dasModules/aotEcs.h>
#include <daScript/include/daScript/simulate/aot_builtin_ast.h>
#include <daScript/include/daScript/ast/ast_typefactory.h>
#include <daScript/src/builtin/module_builtin_ast.h>

DAS_BASE_BIND_ENUM_98(ecs::ComponentTypeFlags, ComponentTypeFlags, COMPONENT_TYPE_TRIVIAL, COMPONENT_TYPE_NON_TRIVIAL_CREATE,
  COMPONENT_TYPE_NON_TRIVIAL_MOVE, COMPONENT_TYPE_BOXED, COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE, COMPONENT_TYPE_NEED_RESOURCES,
  COMPONENT_TYPE_HAS_IO, COMPONENT_TYPE_TRIVIAL_MASK, COMPONENT_TYPE_IS_POD)

namespace bind_dascript
{

struct ComponentTypesAnnotation final : das::ManagedStructureAnnotation<ecs::ComponentTypes, false>
{
  ComponentTypesAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ComponentTypes", ml)
  {
    cppName = " ::ecs::ComponentTypes";
  }
};

struct ComponentTypeAnnotation final : das::ManagedStructureAnnotation<ecs::ComponentType, false>
{
  ComponentTypeAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ComponentType", ml)
  {
    cppName = " ::ecs::ComponentType";
    addField<DAS_BIND_MANAGED_FIELD(size)>("size");
    addField<DAS_BIND_MANAGED_FIELD(flags)>("flags");
  }
};

struct DataComponentsAnnotation final : das::ManagedStructureAnnotation<ecs::DataComponents, false>
{
  DataComponentsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DataComponents", ml)
  {
    cppName = " ::ecs::DataComponents";
  }
};

struct DataComponentAnnotation final : das::ManagedStructureAnnotation<ecs::DataComponent, false>
{
  DataComponentAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DataComponent", ml)
  {
    cppName = " ::ecs::DataComponent";
    addField<DAS_BIND_MANAGED_FIELD(componentType)>("componentType");
    addField<DAS_BIND_MANAGED_FIELD(flags)>("flags");
    addField<DAS_BIND_MANAGED_FIELD(componentTypeName)>("componentTypeName");
  }
};

struct ComponentsListAnnotation final : das::ManagedStructureAnnotation<ecs::ComponentsList, false>
{
  ComponentsListAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ComponentsList", ml)
  {
    cppName = " ::ecs::ComponentsList";
    addProperty<DAS_BIND_MANAGED_PROP(empty)>("empty", "empty");
  }
};

struct TemplateDBAnnotation final : das::ManagedStructureAnnotation<ecs::TemplateDB, false>
{
  TemplateDBAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TemplateDB", ml) { cppName = " ::ecs::TemplateDB"; }
};

struct SceneAnnotation final : das::ManagedStructureAnnotation<ecs::Scene, false>
{
  SceneAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Scene", ml) { cppName = " ::ecs::Scene"; }
};

struct EntitySystemDescAnnotation final : das::ManagedStructureAnnotation<ecs::EntitySystemDesc, false>
{
  EntitySystemDescAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("EntitySystemDesc", ml)
  {
    cppName = " ::ecs::EntitySystemDesc";

    addProperty<DAS_BIND_MANAGED_PROP(getQuant)>("quant", "getQuant");
    addProperty<DAS_BIND_MANAGED_PROP(isDynamic)>("isDynamic");
    addProperty<DAS_BIND_MANAGED_PROP(isEmpty)>("isEmpty");
    addProperty<DAS_BIND_MANAGED_PROP(getBefore)>("before", "getBefore");
    addProperty<DAS_BIND_MANAGED_PROP(getAfter)>("after", "getAfter");
    addProperty<DAS_BIND_MANAGED_PROP(getTags)>("tags", "getTags");
    addProperty<DAS_BIND_MANAGED_PROP(getCompSet)>("compSet", "getCompSet");
    addProperty<DAS_BIND_MANAGED_PROP(getModuleName)>("moduleName", "getModuleName");
  }
};


struct ComponentDescAnnotation final : das::ManagedStructureAnnotation<ecs::ComponentDesc, false>
{
  ComponentDescAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ComponentDesc", ml)
  {
    cppName = " ::ecs::ComponentDesc";

    addField<DAS_BIND_MANAGED_FIELD(name)>("name");
    addField<DAS_BIND_MANAGED_FIELD(type)>("type");
    addField<DAS_BIND_MANAGED_FIELD(flags)>("flags");
  }
};

bool das_get_underlying_ecs_type(das::TypeDeclPtr info, bool with_module_name,
  const das::TBlock<void, das::TTemporary<const char *>> &block, das::Context *context, das::LineInfoArg *at)
{
  das::string typeName;
  das::Type baseType;
  const bool res = get_underlying_ecs_type(*info.get(), typeName, baseType, with_module_name);
  vec4f args = das::cast<const char *>::from(typeName.c_str());
  context->invoke(block, &args, nullptr, at);
  return res;
}

static bool read_component(const char *name, Tab<ecs::ComponentDesc> &out_comps)
{
  if (name == nullptr || name[0] == '\0')
    return true; // empty component name, mark is valid for dynamic query
  const ecs::DataComponents &dataComponents = g_entity_mgr->getDataComponents();
  const ecs::ComponentTypes &componentTypes = g_entity_mgr->getComponentTypes();
  auto nameHash = ECS_HASH_SLOW(name);
  auto compId = dataComponents.findComponentId(nameHash.hash);
  if (compId != ecs::INVALID_COMPONENT_INDEX)
  {
    ecs::DataComponent comp = dataComponents.getComponentById(compId);
    out_comps.emplace_back(nameHash, componentTypes.getTypeById(comp.componentType));
    return true;
  }
  return false;
}

static bool ecs_dynamic_query(das::TArray<char *> const &comps_name_rq, das::TArray<char *> const &comps_name_no,
  const das::TBlock<void, ecs::EntityId> &block, das::Context *context, das::LineInfoArg *at)
{
  TIME_PROFILE(ecs_dynamic_query);
  Tab<ecs::ComponentDesc> comps_rq;
  comps_rq.reserve(comps_name_rq.size);
  for (int i = 0; i < comps_name_rq.size; i++)
    if (!read_component(comps_name_rq[i], comps_rq))
      return false;

  Tab<ecs::ComponentDesc> comps_no;
  comps_no.reserve(comps_name_no.size);
  for (int i = 0; i < comps_name_no.size; i++)
    read_component(comps_name_no[i], comps_no); // if have invalid component do not exit, because this component is not required

  Tab<ecs::ComponentDesc> comps_ro(1, {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()});
  ecs::NamedQueryDesc desc{
    "ecs_dynamic_query",
    dag::ConstSpan<ecs::ComponentDesc>(),
    make_span_const(comps_ro),
    make_span_const(comps_rq),
    make_span_const(comps_no),
  };

  // for bypass eastl error: fixed_function local buffer is not large enough
  auto callback = [&](ecs::EntityId eid) {
    vec4f args = das::cast<ecs::EntityId>::from(eid);
    context->invoke(block, &args, nullptr, at);
  };

  ecs::QueryId qid = g_entity_mgr->createQuery(desc);
  ecs::perform_query(g_entity_mgr.get(), qid, [&callback](const ecs::QueryView &qv) {
    for (auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
      callback(qv.getComponentRO<ecs::EntityId>(0, it));
  });
  g_entity_mgr->destroyQuery(qid);
  return true;
}

class EcsUtilsModule final : public das::Module
{
public:
  EcsUtilsModule() : das::Module("EcsUtils")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("ast"));
    addBuiltinDependency(lib, require("DagorDataBlock"));
    addEnumeration(das::make_smart<EnumerationComponentTypeFlags>());
    addAnnotation(das::make_smart<ComponentTypeAnnotation>(lib));
    addAnnotation(das::make_smart<ComponentTypesAnnotation>(lib));
    addAnnotation(das::make_smart<DataComponentAnnotation>(lib));
    addAnnotation(das::make_smart<DataComponentsAnnotation>(lib));
    addAnnotation(das::make_smart<ComponentsListAnnotation>(lib));
    addAnnotation(das::make_smart<TemplateDBAnnotation>(lib));
    addAnnotation(das::make_smart<SceneAnnotation>(lib));
    addAnnotation(das::make_smart<EntitySystemDescAnnotation>(lib));
    addAnnotation(das::make_smart<ComponentDescAnnotation>(lib));

    das::addUsing<ecs::ComponentsList>(*this, lib, "ecs::ComponentsList");

    das::addExternTempRef<DAS_BIND_FUN(getComponentTypes), das::SimNode_ExtFuncCallRef>(*this, lib, "getComponentTypes",
      das::SideEffects::accessExternal, "bind_dascript::getComponentTypes");
    das::addExternTempRef<DAS_BIND_FUN(getDataComponents), das::SimNode_ExtFuncCallRef>(*this, lib, "getDataComponents",
      das::SideEffects::accessExternal, "bind_dascript::getDataComponents");
    das::addExternTempRef<DAS_BIND_FUN(getTemplateDB), das::SimNode_ExtFuncCallRef>(*this, lib, "getTemplateDB",
      das::SideEffects::accessExternal, "bind_dascript::getTemplateDB");

    das::addExtern<DAS_BIND_FUN(components_to_blk)>(*this, lib, "components_to_blk", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::components_to_blk");
    das::addExtern<DAS_BIND_FUN(components_list_to_blk)>(*this, lib, "components_to_blk", das::SideEffects::modifyArgument,
      "bind_dascript::components_list_to_blk");
    das::addExtern<DAS_BIND_FUN(component_to_blk)>(*this, lib, "component_to_blk", das::SideEffects::modifyArgument,
      "bind_dascript::component_to_blk");
    das::addExtern<DAS_BIND_FUN(component_to_blk_param)>(*this, lib, "component_to_blk_param", das::SideEffects::modifyArgument,
      "bind_dascript::component_to_blk_param");

    das::addExtern<DAS_BIND_FUN(find_component)>(*this, lib, "find_component", das::SideEffects::accessExternal,
      "bind_dascript::find_component");

    das::addExtern<DAS_BIND_FUN(find_component_eid)>(*this, lib, "find_component", das::SideEffects::accessExternal,
      "bind_dascript::find_component_eid");
    das::addExtern<DAS_BIND_FUN(ecs_dynamic_query)>(*this, lib, "ecs_dynamic_query", das::SideEffects::accessExternal,
      "bind_dascript::ecs_dynamic_query");

    das::addExternTempRef<DAS_BIND_FUN(bind_dascript::get_active_scene), das::SimNode_ExtFuncCallRef>(*this, lib, "get_active_scene",
      das::SideEffects::modifyExternal, "bind_dascript::get_active_scene");

    das::addExtern<DAS_BIND_FUN(bind_dascript::getEvSet)>(*this, lib, "getEvSet", das::SideEffects::invoke, "bind_dascript::getEvSet");

    using method_insert_empty_entity_record = DAS_CALL_MEMBER(::ecs::Scene::insertEmptyEntityRecord);
    das::addExtern<DAS_CALL_METHOD(method_insert_empty_entity_record)>(*this, lib, "scene_insert_empty_entity_record",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::ecs::Scene::insertEmptyEntityRecord));

    using method_insert_entity_record = DAS_CALL_MEMBER(::ecs::Scene::insertEntityRecord);
    das::addExtern<DAS_CALL_METHOD(method_insert_entity_record)>(*this, lib, "scene_insert_entity_record",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::ecs::Scene::insertEntityRecord));

    using method_erase_entity_record = DAS_CALL_MEMBER(::ecs::Scene::eraseEntityRecord);
    das::addExtern<DAS_CALL_METHOD(method_erase_entity_record)>(*this, lib, "scene_erase_entity_record",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::ecs::Scene::eraseEntityRecord));

    using method_findType = DAS_CALL_MEMBER(ecs::ComponentTypes::findType);
    das::addExtern<DAS_CALL_METHOD(method_findType)>(*this, lib, "component_types_findType", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::ComponentTypes::findType));

    using method_findTypeInfo = DAS_CALL_MEMBER(ecs::ComponentTypes::findTypeInfo);
    das::addExtern<DAS_CALL_METHOD(method_findTypeInfo), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "component_types_findTypeInfo", das::SideEffects::none, DAS_CALL_MEMBER_CPP(ecs::ComponentTypes::findTypeInfo));

    using method_getTypeInfo = DAS_CALL_MEMBER(ecs::ComponentTypes::getTypeInfo);
    das::addExtern<DAS_CALL_METHOD(method_getTypeInfo), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "component_types_getTypeInfo", das::SideEffects::none, DAS_CALL_MEMBER_CPP(ecs::ComponentTypes::getTypeInfo));

    using method_getTypeById = DAS_CALL_MEMBER(ecs::ComponentTypes::getTypeById);
    das::addExtern<DAS_CALL_METHOD(method_getTypeById)>(*this, lib, "component_types_getTypeById", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::ComponentTypes::getTypeById));

    using method_getTypeNameById = DAS_CALL_MEMBER(ecs::ComponentTypes::getTypeNameById);
    das::addExtern<DAS_CALL_METHOD(method_getTypeNameById)>(*this, lib, "component_types_getTypeNameById", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::ComponentTypes::getTypeNameById));

    using method_getTypeCount = DAS_CALL_MEMBER(ecs::ComponentTypes::getTypeCount);
    das::addExtern<DAS_CALL_METHOD(method_getTypeCount)>(*this, lib, "component_types_getTypeCount", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::ComponentTypes::getTypeCount));

    using method_findTypeName = DAS_CALL_MEMBER(ecs::ComponentTypes::findTypeName);
    das::addExtern<DAS_CALL_METHOD(method_findTypeName)>(*this, lib, "component_types_findTypeName", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::ComponentTypes::findTypeName));

    using method_findComponentId = DAS_CALL_MEMBER(ecs::DataComponents::findComponentId);
    das::addExtern<DAS_CALL_METHOD(method_findComponentId)>(*this, lib, "data_components_findComponentId", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::DataComponents::findComponentId));

    using method_findComponentName = DAS_CALL_MEMBER(ecs::DataComponents::findComponentName);
    das::addExtern<DAS_CALL_METHOD(method_findComponentName)>(*this, lib, "data_components_findComponentName", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::DataComponents::findComponentName));

    using method_getComponentById = DAS_CALL_MEMBER(ecs::DataComponents::getComponentById);
    das::addExtern<DAS_CALL_METHOD(method_getComponentById), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "data_components_getComponentById", das::SideEffects::none, DAS_CALL_MEMBER_CPP(ecs::DataComponents::getComponentById));

    using method_getComponentNameById = DAS_CALL_MEMBER(ecs::DataComponents::getComponentNameById);
    das::addExtern<DAS_CALL_METHOD(method_getComponentNameById)>(*this, lib, "data_components_getComponentNameById",
      das::SideEffects::none, DAS_CALL_MEMBER_CPP(ecs::DataComponents::getComponentNameById));

    using method_findComponentsList = DAS_CALL_MEMBER(ecs::Scene::findComponentsList);
    das::addExtern<DAS_CALL_METHOD(method_findComponentsList)>(*this, lib, "scene_findComponentsList", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::Scene::findComponentsList));

    using method_getEntityComponentRef = DAS_CALL_MEMBER(ecs::EntityManager::getEntityComponentRef);
    das::addExtern<DAS_CALL_METHOD(method_getEntityComponentRef), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "getEntityComponentRef", das::SideEffects::none, DAS_CALL_MEMBER_CPP(ecs::EntityManager::getEntityComponentRef));

    using method_getEntityComponentRefRW = DAS_CALL_MEMBER(ecs::EntityManager::getComponentRefRW);
    das::addExtern<DAS_CALL_METHOD(method_getEntityComponentRefRW), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "getComponentRefRW", das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(ecs::EntityManager::getComponentRefRW));

    using method_getNumComponents = DAS_CALL_MEMBER(ecs::EntityManager::getNumComponents);
    das::addExtern<DAS_CALL_METHOD(method_getNumComponents)>(*this, lib, "getNumComponents", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(ecs::EntityManager::getNumComponents));

    das::addExtern<DAS_BIND_FUN(bind_dascript::das_get_underlying_ecs_type)>(*this, lib, "get_underlying_ecs_type",
      das::SideEffects::accessExternal, "bind_dascript::das_get_underlying_ecs_type");

    das::addExtern<DAS_BIND_FUN(bind_dascript::load_comp_list_from_blk)>(*this, lib, "load_comp_list_from_blk",
      das::SideEffects::modifyArgument, "::bind_dascript::load_comp_list_from_blk");

    das::addExtern<DAS_BIND_FUN(find_templateDB)>(*this, lib, "find_templateDB", das::SideEffects::accessExternal,
      "bind_dascript::find_templateDB");

    das::addExtern<DAS_BIND_FUN(find_systemDB)>(*this, lib, "find_systemDB", das::SideEffects::accessExternal,
      "bind_dascript::find_systemDB");

    das::addExtern<DAS_BIND_FUN(component_desc_name)>(*this, lib, "component_desc_name", das::SideEffects::none,
      "bind_dascript::component_desc_name");

    das::addExtern<DAS_BIND_FUN(query_componentsRW)>(*this, lib, "query_componentsRW", das::SideEffects::invoke,
      "bind_dascript::query_componentsRW");
    das::addExtern<DAS_BIND_FUN(query_componentsRO)>(*this, lib, "query_componentsRO", das::SideEffects::invoke,
      "bind_dascript::query_componentsRO");
    das::addExtern<DAS_BIND_FUN(query_componentsRQ)>(*this, lib, "query_componentsRQ", das::SideEffects::invoke,
      "bind_dascript::query_componentsRQ");
    das::addExtern<DAS_BIND_FUN(query_componentsNO)>(*this, lib, "query_componentsNO", das::SideEffects::invoke,
      "bind_dascript::query_componentsNO");
    das::addExtern<DAS_BIND_FUN(create_entities_blk)>(*this, lib, "create_entities_blk", das::SideEffects::modifyExternal,
      "bind_dascript::create_entities_blk");
    das::addExtern<DAS_BIND_FUN(getComponentRef), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "getComponentRef",
      das::SideEffects::none, "bind_dascript::getComponentRef");

    das::addConstant(*this, "DONT_REPLICATE", (ecs::component_flags_t)ecs::DataComponent::DONT_REPLICATE);
    das::addConstant(*this, "HAS_SERIALIZER", (ecs::component_flags_t)ecs::DataComponent::HAS_SERIALIZER);
    das::addConstant(*this, "IS_COPY", (ecs::component_flags_t)ecs::DataComponent::IS_COPY);
    das::addConstant(*this, "INVALID_COMPONENT_TYPE_INDEX", (ecs::type_index_t)ecs::INVALID_COMPONENT_TYPE_INDEX);
    das::addConstant(*this, "INVALID_COMPONENT_INDEX", (ecs::component_index_t)ecs::INVALID_COMPONENT_INDEX);
    das::addConstant(*this, "INVALID_TEMPLATE_INDEX", (ecs::template_t)ecs::INVALID_TEMPLATE_INDEX);
    das::addConstant(*this, "FLAG_CHANGE_EVENT", (uint32_t)ecs::FLAG_CHANGE_EVENT);
    das::addConstant(*this, "FLAG_REPLICATED", (uint32_t)ecs::FLAG_REPLICATED);

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotEcsUtils.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(EcsUtilsModule, bind_dascript);
