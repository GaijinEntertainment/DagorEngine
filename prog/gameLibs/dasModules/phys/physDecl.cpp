// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotPhysDecl.h>
#include <dasModules/aotGeomNodeTree.h>

struct PhysSystemInstanceAnnotation : das::ManagedStructureAnnotation<PhysSystemInstance, false>
{
  PhysSystemInstanceAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysSystemInstance", ml)
  {
    cppName = " ::PhysSystemInstance";
    addProperty<DAS_BIND_MANAGED_PROP(getBodyCount)>("bodyCount", "getBodyCount");
  }
};

struct PhysRagdollAnnotation : das::ManagedStructureAnnotation<PhysRagdoll, false>
{
  PhysRagdollAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysRagdoll", ml)
  {
    cppName = " ::PhysRagdoll";
    addProperty<DAS_BIND_MANAGED_PROP(isCanApplyNodeImpulse)>("isCanApplyNodeImpulse");
    addProperty<DAS_BIND_MANAGED_PROP(overridesBlender)>("overridesBlender");
  }
};

struct PhysBodyAnnotation : das::ManagedStructureAnnotation<PhysBody, false>
{
  PhysBodyAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysBody", ml) { cppName = " ::PhysBody"; }
};

struct ProjectileImpulseAnnotation : das::ManagedStructureAnnotation<ProjectileImpulse, false>
{
  ProjectileImpulseAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ProjectileImpulse", ml)
  {
    cppName = " ::ProjectileImpulse";
  }
};

struct ImpulseDataAnnotation : das::ManagedStructureAnnotation<ProjectileImpulse::ImpulseData, false>
{
  ImpulseDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ImpulseData", ml)
  {
    cppName = " ProjectileImpulse::ImpulseData";
    addField<DAS_BIND_MANAGED_FIELD(nodeId)>("nodeId");
    addField<DAS_BIND_MANAGED_FIELD(pos)>("pos");
    addField<DAS_BIND_MANAGED_FIELD(impulse)>("impulse");
  }
};

struct ProjectileImpulseDataPairAnnotation : das::ManagedStructureAnnotation<das::ProjectileImpulseDataPair, false>
{
  ProjectileImpulseDataPairAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ProjectileImpulseDataPair", ml)
  {
    cppName = " das::ProjectileImpulseDataPair";
    addField<DAS_BIND_MANAGED_FIELD(time)>("time");
    addField<DAS_BIND_MANAGED_FIELD(data)>("data");
  }
};

namespace bind_dascript
{
class PhysDeclModule final : public das::Module
{
public:
  PhysDeclModule() : das::Module("PhysDecl")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("GeomNodeTree"));

    addAnnotation(das::make_smart<PhysSystemInstanceAnnotation>(lib));
    addAnnotation(das::make_smart<PhysRagdollAnnotation>(lib));
    addAnnotation(das::make_smart<PhysBodyAnnotation>(lib));
    addAnnotation(das::make_smart<ProjectileImpulseAnnotation>(lib));
    addAnnotation(das::make_smart<ImpulseDataAnnotation>(lib));
    addAnnotation(das::make_smart<ProjectileImpulseDataPairAnnotation>(lib));

    using method_startRagdoll = DAS_CALL_MEMBER(PhysRagdoll::startRagdoll);
    das::addExtern<DAS_CALL_METHOD(method_startRagdoll)>(*this, lib, "ragdoll_startRagdoll", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysRagdoll::startRagdoll));

    using method_endRagdoll = DAS_CALL_MEMBER(PhysRagdoll::endRagdoll);
    das::addExtern<DAS_CALL_METHOD(method_endRagdoll)>(*this, lib, "ragdoll_endRagdoll", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysRagdoll::endRagdoll));

    using method_setOverrideVel = DAS_CALL_MEMBER(PhysRagdoll::setOverrideVel);
    das::addExtern<DAS_CALL_METHOD(method_setOverrideVel)>(*this, lib, "ragdoll_setOverrideVel", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysRagdoll::setOverrideVel));

    using method_setDriveBodiesToAnimchar = DAS_CALL_MEMBER(PhysRagdoll::setDriveBodiesToAnimchar);
    das::addExtern<DAS_CALL_METHOD(method_setDriveBodiesToAnimchar)>(*this, lib, "ragdoll_setDriveBodiesToAnimchar",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysRagdoll::setDriveBodiesToAnimchar));

    using method_setOverrideOmega = DAS_CALL_MEMBER(PhysRagdoll::setOverrideOmega);
    das::addExtern<DAS_CALL_METHOD(method_setOverrideOmega)>(*this, lib, "ragdoll_setOverrideOmega", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysRagdoll::setOverrideOmega));

    using method_setContinuousCollisionMode = DAS_CALL_MEMBER(PhysRagdoll::setContinuousCollisionMode);
    das::addExtern<DAS_CALL_METHOD(method_setContinuousCollisionMode)>(*this, lib, "ragdoll_setContinuousCollisionMode",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysRagdoll::setContinuousCollisionMode));

    using method_setDynamicClipout = DAS_CALL_MEMBER(PhysRagdoll::setDynamicClipout);
    das::addExtern<DAS_CALL_METHOD(method_setDynamicClipout)>(*this, lib, "ragdoll_setDynamicClipout",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysRagdoll::setDynamicClipout));

    using method_wakeUp = DAS_CALL_MEMBER(PhysRagdoll::wakeUp);
    das::addExtern<DAS_CALL_METHOD(method_wakeUp)>(*this, lib, "ragdoll_wakeUp", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysRagdoll::wakeUp));

    using method_applyImpulse = DAS_CALL_MEMBER(PhysRagdoll::applyImpulse);
    das::addExtern<DAS_CALL_METHOD(method_applyImpulse)>(*this, lib, "ragdoll_applyImpulse", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysRagdoll::applyImpulse));

    using method_setStartAddLinVel = DAS_CALL_MEMBER(PhysRagdoll::setStartAddLinVel);
    das::addExtern<DAS_CALL_METHOD(method_setStartAddLinVel)>(*this, lib, "ragdoll_setStartAddLinVel",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysRagdoll::setStartAddLinVel));

    das::addExtern<DAS_BIND_FUN(ragdoll_getPhysSys)>(*this, lib, "ragdoll_getPhysSys", das::SideEffects::modifyArgument,
      "bind_dascript::ragdoll_getPhysSys");

    using method_setInteractionLayerAndGroup = DAS_CALL_MEMBER(PhysSystemInstance::setGroupAndLayerMask);
    das::addExtern<DAS_CALL_METHOD(method_setInteractionLayerAndGroup)>(*this, lib, "phys_system_instance_setGroupAndLayerMask",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysSystemInstance::setGroupAndLayerMask));

    using method_setJointsMotorSettings = DAS_CALL_MEMBER(PhysSystemInstance::setJointsMotorSettings);
    das::addExtern<DAS_CALL_METHOD(method_setJointsMotorSettings)>(*this, lib, "phys_system_instance_setJointsMotorSettings",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysSystemInstance::setJointsMotorSettings));

    using method_findBodyIdByName = DAS_CALL_MEMBER(PhysSystemInstance::findBodyIdByName);
    das::addExtern<DAS_CALL_METHOD(method_findBodyIdByName)>(*this, lib, "findBodyIdByName", das::SideEffects::accessExternal,
      DAS_CALL_MEMBER_CPP(PhysSystemInstance::findBodyIdByName));

    das::addExtern<DAS_BIND_FUN(phys_system_instance_getBody)>(*this, lib, "phys_system_instance_getBody",
      das::SideEffects::modifyArgument, "bind_dascript::phys_system_instance_getBody");

    using method_getTm = DAS_CALL_MEMBER(PhysBody::getTm);
    das::addExtern<DAS_CALL_METHOD(method_getTm)>(*this, lib, "phys_body_getTm", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysBody::getTm));

    using method_setTm = DAS_CALL_MEMBER(PhysBody::setTm);
    das::addExtern<DAS_CALL_METHOD(method_setTm)>(*this, lib, "phys_body_setTm", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysBody::setTm));

    using method_getMassMatrix = DAS_CALL_MEMBER(PhysBody::getMassMatrix);
    das::addExtern<DAS_CALL_METHOD(method_getMassMatrix)>(*this, lib, "phys_body_getMassMatrix", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysBody::getMassMatrix));

    using method_setVelocity = DAS_CALL_MEMBER(PhysBody::setVelocity);
    das::addExtern<DAS_CALL_METHOD(method_setVelocity)>(*this, lib, "phys_body_setVelocity", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysBody::setVelocity));

    using method_getVelocity = DAS_CALL_MEMBER(PhysBody::getVelocity);
    das::addExtern<DAS_CALL_METHOD(method_getVelocity)>(*this, lib, "phys_body_getVelocity", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(PhysBody::getVelocity));

    using method_setGravity = DAS_CALL_MEMBER(PhysBody::setGravity);
    das::addExtern<DAS_CALL_METHOD(method_setGravity)>(*this, lib, "phys_body_setGravity", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysBody::setGravity));

    using method_activateBody = DAS_CALL_MEMBER(PhysBody::activateBody);
    das::addExtern<DAS_CALL_METHOD(method_activateBody)>(*this, lib, "phys_body_activateBody", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysBody::activateBody));

    using method_isActive = DAS_CALL_MEMBER(PhysBody::isActive);
    das::addExtern<DAS_CALL_METHOD(method_isActive)>(*this, lib, "phys_body_isActive", das::SideEffects::accessExternal,
      DAS_CALL_MEMBER_CPP(PhysBody::isActive));

    using method_disableDeactivation = DAS_CALL_MEMBER(PhysBody::disableDeactivation);
    das::addExtern<DAS_CALL_METHOD(method_disableDeactivation)>(*this, lib, "phys_body_disableDeactivation",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysBody::disableDeactivation));

    using method_setAngularVelocity = DAS_CALL_MEMBER(PhysBody::setAngularVelocity);
    das::addExtern<DAS_CALL_METHOD(method_setAngularVelocity)>(*this, lib, "phys_body_setAngularVelocity",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysBody::setAngularVelocity));

    das::addExtern<DAS_BIND_FUN(projectile_impulse_get_data)>(*this, lib, "projectile_impulse_get_data",
      das::SideEffects::accessExternal, "bind_dascript::projectile_impulse_get_data");

    das::addExtern<DAS_BIND_FUN(save_projectile_impulse)>(*this, lib, "save_projectile_impulse", das::SideEffects::modifyArgument,
      "save_projectile_impulse");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotPhysDecl.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PhysDeclModule, bind_dascript);
