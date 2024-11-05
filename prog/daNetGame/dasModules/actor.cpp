// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/actor.h"


struct BasePhysActorAnnotation final : das::ManagedStructureAnnotation<BasePhysActor, false>
{
  BasePhysActorAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BasePhysActor", ml)
  {
    cppName = " ::BasePhysActor";
    addProperty<DAS_BIND_MANAGED_PROP(getPhysTypeStr)>("physTypeStr", "getPhysTypeStr");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getRole)>("role", "getRole");
    addProperty<DAS_BIND_MANAGED_PROP(isAsleep)>("isAsleep", "isAsleep");
    addField<DAS_BIND_MANAGED_FIELD(tickrateType)>("tickrateType");
  }
};

namespace bind_dascript
{
class DngActorModule final : public das::Module
{
public:
  DngActorModule() : das::Module("DngActor")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));

    addEnumeration(das::make_smart<EnumerationRoleFlags>());
    das::addEnumFlagOps<IPhysActor::RoleFlags>(*this, lib, "IPhysActor::RoleFlags");
    addEnumeration(das::make_smart<EnumerationNetRole>());
    das::addEnumFlagOps<IPhysActor::NetRole>(*this, lib, "IPhysActor::NetRole");
    addEnumeration(das::make_smart<EnumerationPhysTickRateType>());

    addAnnotation(das::make_smart<BasePhysActorAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(BasePhysActor::resizeSyncStates)>(*this, lib, "base_phys_actor_resizeSyncStates",
      das::SideEffects::modifyExternal, "BasePhysActor::resizeSyncStates");
    das::addExtern<DAS_BIND_FUN(base_phys_actor_set_role_and_tickrate_type)>(*this, lib, "base_phys_actor_setRoleAndTickrateType",
      das::SideEffects::modifyArgument, "bind_dascript::base_phys_actor_set_role_and_tickrate_type");
    das::addExtern<DAS_BIND_FUN(::get_phys_actor)>(*this, lib, "get_phys_actor", das::SideEffects::none, "::get_phys_actor");
    das::addExtern<DAS_BIND_FUN(base_phys_actor_init_role)>(*this, lib, "base_phys_actor_initRole", das::SideEffects::modifyArgument,
      "bind_dascript::base_phys_actor_init_role");
    das::addExtern<DAS_BIND_FUN(base_phys_actor_reset_aas)>(*this, lib, "base_phys_actor_resetAAS", das::SideEffects::modifyArgument,
      "bind_dascript::base_phys_actor_reset_aas");
    das::addExtern<DAS_BIND_FUN(base_phys_actor_reset)>(*this, lib, "base_phys_actor_reset", das::SideEffects::modifyArgument,
      "bind_dascript::base_phys_actor_reset");
    das::addExtern<DAS_BIND_FUN(::teleport_phys_actor)>(*this, lib, "teleport_phys_actor", das::SideEffects::modifyExternal,
      "::teleport_phys_actor");
    das::addExtern<DAS_BIND_FUN(::calc_phys_update_to_tick)>(*this, lib, "calc_phys_update_to_tick", das::SideEffects::none,
      "::calc_phys_update_to_tick");

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_actor_min_time_step)>(*this, lib, "get_actor_min_time_step",
      das::SideEffects::accessExternal, "bind_dascript::get_actor_min_time_step");
    das::addExtern<DAS_BIND_FUN(bind_dascript::get_actor_max_time_step)>(*this, lib, "get_actor_max_time_step",
      das::SideEffects::accessExternal, "bind_dascript::get_actor_max_time_step");
    das::addExtern<DAS_BIND_FUN(BasePhysActor::getDefTimeStepByTickrateType)>(*this, lib,
      "base_phys_actor_getDefTimeStepByTickrateType", das::SideEffects::accessExternal, "BasePhysActor::getDefTimeStepByTickrateType");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/actor.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngActorModule, bind_dascript);
