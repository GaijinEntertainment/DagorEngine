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
#include <dasModules/aotEcsEnum.h>
#include <dasModules/dasCreatingTemplate.h>

namespace das
{
inline das::TypeDeclPtr make_vec4()
{
  auto t = das::make_smart<das::TypeDecl>(das::Type::tFloat4);
  t->alias = "vec4f";
  // t->aotAlias = true;
  return t;
}

IMPLEMENT_OP1_EVAL_POLICY(BoolNot, ecs::EntityId);
IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ, ecs::EntityId);
IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu, ecs::EntityId);

template <>
struct typeFactory<vec4f>
{
  static ___noinline das::TypeDeclPtr make(const das::ModuleLibrary &) { return make_vec4(); }
};
}; // namespace das

// not working correctly in AOT, as AOT name is not possible
DAS_BASE_BIND_ENUM(ecs::QueryCbResult, QueryCbResult, Stop, Continue)

IMPLEMENT_EXTERNAL_TYPE_FACTORY(EntityId, ecs::EntityId)

#define MAKE_ECS_TYPE IMPLEMENT_EXTERNAL_TYPE_FACTORY
MAKE_ECS_TYPES
#undef MAKE_ECS_TYPE

namespace bind_dascript
{
struct EntityIdAnnotation final : das::ManagedValueAnnotation<ecs::EntityId>
{
  EntityIdAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "EntityId", " ::ecs::EntityId") {}
  virtual void walk(das::DataWalker &walker, void *data) override
  {
    if (!walker.reading)
    {
      const ecs::EntityId *t = (ecs::EntityId *)data;
      int32_t eidV = int32_t(ecs::entity_id_t(*t));
      walker.Int(eidV);
    }
  }
  bool canBePlacedInContainer() const override { return true; }
  virtual bool hasNonTrivialCtor() const override { return false; } // Warning: workaround, do not copy!
};

struct TagAnnotation final : das::ManagedStructureAnnotation<ecs::Tag, false>
{
  TagAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Tag", ml) { cppName = " ::ecs::Tag"; }
};

struct EntityComponentRefAnnotation final : das::ManagedStructureAnnotation<ecs::EntityComponentRef, false>
{
  EntityComponentRefAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("EntityComponentRef", ml)
  {
    cppName = " ::ecs::EntityComponentRef";

    addProperty<DAS_BIND_MANAGED_PROP(isNull)>("isNull");
    addProperty<DAS_BIND_MANAGED_PROP(getUserType)>("userType", "getUserType");
    addProperty<DAS_BIND_MANAGED_PROP(getTypeId)>("typeId", "getTypeId");
    addProperty<DAS_BIND_MANAGED_PROP(getComponentId)>("componentId", "getComponentId");
    addPropertyExtConst<void *(ecs::EntityComponentRef::*)(), &ecs::EntityComponentRef::getRawData,
      const void *(ecs::EntityComponentRef::*)() const, &ecs::EntityComponentRef::getRawData>("rawData", "getRawData");
  }
  bool hasNonTrivialCtor() const override { return false; }
  bool isLocal() const override { return true; }
  bool canMove() const override { return true; }
  bool canBePlacedInContainer() const override { return true; }
};


#define ADD_EXTERN(fun, side_effects) das::addExtern<DAS_BIND_FUN(fun)>(*this, lib, #fun, side_effects, "bind_dascript::" #fun)
#define ADD_EXTERN_NAME(fun, name, side_effects) \
  das::addExtern<DAS_BIND_FUN(fun)>(*this, lib, name, side_effects, "bind_dascript::" #fun)

static char ecs_das[] =
#include "ecs.das.inl"
  ;

void ECS::addPrerequisits(das::ModuleLibrary &) const {}

ECS::ECS() : das::Module("ecs")
{
  das::ModuleLibrary lib(this);

  lib.addBuiltInModule();
  addBuiltinDependency(lib, require("math"));
  addBuiltinDependency(lib, require("DagorMath"));

  addEnumeration(das::make_smart<EnumerationQueryCbResult>()); // not working correctly in AOT
  addAnnotation(das::make_smart<TagAnnotation>(lib));
  addAnnotation(das::make_smart<EntityComponentRefAnnotation>(lib));
  addAnnotation(das::make_smart<BakeHashFunctionAnnotation<0>>());
  addAnnotation(das::make_smart<BakeHashFunctionAnnotation<1>>());
  addAnnotation(das::make_smart<EntityIdAnnotation>(lib));

  // G_VERIFY(addAlias(das::typeFactory<vec4f>::make(lib)));
  G_VERIFY(addAlias(das::make_vec4()));

  // addFunction( das::make_smart<das::BuiltInFn<das::Sim_Equ<ecs::EntityId>,         bool, ecs::EntityId,  ecs::EntityId>  >("==",
  // lib, "Equ") ); addFunction( das::make_smart<das::BuiltInFn<das::Sim_NotEqu<ecs::EntityId>,      bool, ecs::EntityId,
  // ecs::EntityId>  >("!=",     lib, "NotEqu") );
  das::addFunctionBasic<ecs::EntityId>(*this, lib);
  das::addConstant<uint32_t>(*this, "INVALID_ENTITY_ID_VAL", ecs::ECS_INVALID_ENTITY_ID_VAL);
  das::addConstant<ecs::template_t>(*this, "INVALID_TEMPLATE_INDEX", ecs::INVALID_TEMPLATE_INDEX);
  addFunction(das::make_smart<das::BuiltInFn<das::Sim_BoolNot<ecs::EntityId>, bool, ecs::EntityId>>("!", lib, "BoolNot"));
  G_STATIC_ASSERT((eastl::is_same<das::string, ecs::string>::value));
  das::addExtern<DAS_BIND_FUN(castEid)>(*this, lib, "uint", das::SideEffects::none, "bind_dascript::castEid");
  das::addExtern<DAS_BIND_FUN(eidCast)>(*this, lib, "EntityId", das::SideEffects::none, "bind_dascript::eidCast");
  das::addExtern<DAS_BIND_FUN(cloneEntityComponentRef), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "clone",
    das::SideEffects::none, "bind_dascript::cloneEntityComponentRef");

  addES(lib);
  addContainerAnnotations(lib);
  addList(lib);
  addListFn(lib);
  addChildComponent(lib);
  addArray(lib);
  addObjectRead(lib);
  addObjectWrite(lib);
  addObjectRW(lib);
  addCompMap(lib);
  addTemplates(lib);
  addInitializers(lib);
  addEvents(lib);
  addEntityRead(lib);
  addEntityWrite(lib);
  addEntityWriteOptional(lib);
  addEntityRW(lib);

  ADD_EXTERN(createEntity, das::SideEffects::modifyExternal);
  ADD_EXTERN(createEntitySync, das::SideEffects::modifyExternal);
  ADD_EXTERN(reCreateEntityFrom, das::SideEffects::modifyExternal);

  ADD_EXTERN(destroyEntity, das::SideEffects::modifyExternal);
  ADD_EXTERN(doesEntityExist, das::SideEffects::accessExternal);
  ADD_EXTERN(isLoadingEntity, das::SideEffects::accessExternal);
  ADD_EXTERN(getEntityTemplateId, das::SideEffects::accessExternal);
  ADD_EXTERN(getEntityTemplateName, das::SideEffects::accessExternal);
  ADD_EXTERN(getEntityFutureTemplateName, das::SideEffects::accessExternal);

  ADD_EXTERN_NAME(getSingletonEntityHint, "getSingletonEntity", das::SideEffects::accessExternal);
  auto getSingletonEntityExt = ADD_EXTERN(getSingletonEntity, das::SideEffects::accessExternal);
  getSingletonEntityExt->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<0>>()));

  ADD_EXTERN(is_entity_mgr_exists, das::SideEffects::accessExternal);

  das::addExtern<DAS_BIND_FUN(createEntityWithComp)>(*this, lib, "createEntity", das::SideEffects::modifyExternal,
    "bind_dascript::createEntityWithComp");
  das::addExtern<DAS_BIND_FUN(createEntitySyncWithComp)>(*this, lib, "createEntitySync", das::SideEffects::modifyExternal,
    "bind_dascript::createEntitySyncWithComp");
  das::addExtern<DAS_BIND_FUN(createInstantiatedEntitySyncWithComp)>(*this, lib, "createInstantiatedEntitySync",
    das::SideEffects::modifyExternal, "bind_dascript::createInstantiatedEntitySyncWithComp");
  das::addExtern<DAS_BIND_FUN(_builtin_create_entity2)>(*this, lib, "createEntity", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_create_entity2");
  das::addExtern<DAS_BIND_FUN(_builtin_create_entity_sync2)>(*this, lib, "createEntitySync", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_create_entity_sync2");
  das::addExtern<DAS_BIND_FUN(_builtin_create_instantiated_entity_sync)>(*this, lib, "createInstantiatedEntitySync",
    das::SideEffects::modifyExternal, "bind_dascript::_builtin_create_instantiated_entity_sync");
  das::addExtern<DAS_BIND_FUN(reCreateEntityFromWithComp)>(*this, lib, "reCreateEntityFrom", das::SideEffects::modifyExternal,
    "bind_dascript::reCreateEntityFromWithComp");
  das::addExtern<DAS_BIND_FUN(_builtin_create_entity_lambda)>(*this, lib, "createEntity", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_create_entity_lambda");
  das::addExtern<DAS_BIND_FUN(_builtin_recreate_entity_lambda)>(*this, lib, "reCreateEntityFrom", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_recreate_entity_lambda");
  das::addExtern<DAS_BIND_FUN(_builtin_add_sub_template_lambda)>(*this, lib, "addSubTemplate", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_add_sub_template_lambda");

  das::addExtern<DAS_BIND_FUN(_builtin_add_sub_template_name)>(*this, lib, "add_sub_template_name", das::SideEffects::none,
    "bind_dascript::_builtin_add_sub_template_name");
  das::addExtern<DAS_BIND_FUN(_builtin_remove_sub_template_name)>(*this, lib, "remove_sub_template_name", das::SideEffects::none,
    "bind_dascript::_builtin_remove_sub_template_name");
  das::addExtern<DAS_BIND_FUN(_builtin_add_sub_template_name_str)>(*this, lib, "add_sub_template_name", das::SideEffects::none,
    "bind_dascript::_builtin_add_sub_template_name_str");
  das::addExtern<DAS_BIND_FUN(_builtin_remove_sub_template_name_str)>(*this, lib, "remove_sub_template_name", das::SideEffects::none,
    "bind_dascript::_builtin_remove_sub_template_name_str");

  // modifiers
  das::addExtern<DAS_BIND_FUN(_builtin_add_sub_template)>(*this, lib, "addSubTemplate", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_add_sub_template");
  das::addExtern<DAS_BIND_FUN(_builtin_remove_sub_template)>(*this, lib, "removeSubTemplate", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_remove_sub_template");

  das::addExtern<DAS_BIND_FUN(ecs_hash)>(*this, lib, "ecs_hash", das::SideEffects::none, "bind_dascript::ecs_hash");

  das::addExtern<DAS_BIND_FUN(load_das)>(*this, lib, "load_das", das::SideEffects::modifyExternal, "bind_dascript::load_das");
  das::addExtern<DAS_BIND_FUN(load_das_debugger)>(*this, lib, "load_das_debugger", das::SideEffects::modifyExternal,
    "bind_dascript::load_das_debugger");

  das::addExtern<DAS_BIND_FUN(load_das_with_debugcode)>(*this, lib, "load_das_with_debugcode", das::SideEffects::modifyExternal,
    "bind_dascript::load_das_with_debugcode");

#if DAGOR_DBGLEVEL > 0
  das::addExtern<DAS_BIND_FUN(reload_das_debug)>(*this, lib, "reload_das_debug", das::SideEffects::modifyExternal,
    "bind_dascript::reload_das_debug");
  das::addExtern<DAS_BIND_FUN(find_loaded_das)>(*this, lib, "find_loaded_das", das::SideEffects::modifyExternal,
    "bind_dascript::find_loaded_das");
#endif

  // ecs enums API
  das::addExtern<DAS_BIND_FUN(ecs::is_type_ecs_enum)>(*this, lib, "is_type_ecs_enum", das::SideEffects::accessExternal,
    "ecs::is_type_ecs_enum");

  das::addExtern<DAS_BIND_FUN(get_ecs_enum_values)>(*this, lib, "get_ecs_enum_values", das::SideEffects::accessExternal,
    "bind_dascript::get_ecs_enum_values");

  das::addExtern<DAS_BIND_FUN(ecs::update_enum_value)>(*this, lib, "update_enum_value",
    das::SideEffects::modifyArgumentAndAccessExternal, "ecs::update_enum_value");

  das::addExtern<DAS_BIND_FUN(ecs::find_enum_idx)>(*this, lib, "find_enum_idx", das::SideEffects::modifyArgumentAndAccessExternal,
    "ecs::find_enum_idx");

  // and builtin module
  compileBuiltinModule("ecs.das", (unsigned char *)ecs_das, sizeof(ecs_das));

  verifyAotReady();
}

das::ModuleAotType ECS::aotRequire(das::TextWriter &tw) const
{
  tw << "#include <dasModules/aotEcs.h>\n";
  tw << "#include <dasModules/aotEcsContainer.h>\n";
  tw << "#include <dasModules/aotEcsEvents.h>\n";
  tw << "#include <dasModules/aotEcsEnum.h>\n";
  tw << "#include <dasModules/dasCreatingTemplate.h>\n";
  return das::ModuleAotType::cpp;
}

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(ECS, bind_dascript);
