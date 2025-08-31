// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotPhysObj.h>
#include <dasModules/dasManagedTab.h>
#include <ecs/phys/netPhysResync.h>

struct PhysObjStateAnnotation : das::ManagedStructureAnnotation<PhysObjState, false>
{
  PhysObjStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysObjState", ml)
  {
    cppName = " ::PhysObjState";

    addField<DAS_BIND_MANAGED_FIELD(atTick)>("atTick");
    addField<DAS_BIND_MANAGED_FIELD(lastAppliedControlsForTick)>("lastAppliedControlsForTick");
    addField<DAS_BIND_MANAGED_FIELD(canBeCheckedForSync)>("canBeCheckedForSync");

    addField<DAS_BIND_MANAGED_FIELD(location)>("location");
    addField<DAS_BIND_MANAGED_FIELD(velocity)>("velocity");
    addField<DAS_BIND_MANAGED_FIELD(omega)>("omega");
    addField<DAS_BIND_MANAGED_FIELD(alpha)>("alpha");
    addField<DAS_BIND_MANAGED_FIELD(acceleration)>("acceleration");
    addField<DAS_BIND_MANAGED_FIELD(addForce)>("addForce");
    addField<DAS_BIND_MANAGED_FIELD(addMoment)>("addMoment");
    addField<DAS_BIND_MANAGED_FIELD(contactPoint)>("contactPoint");
    addField<DAS_BIND_MANAGED_FIELD(gravDirection)>("gravDirection");
    addField<DAS_BIND_MANAGED_FIELD(logCCD)>("logCCD");
    addField<DAS_BIND_MANAGED_FIELD(isSleep)>("isSleep");
    addField<DAS_BIND_MANAGED_FIELD(hadContact)>("hadContact");
  }
};

struct PhysObjControlStateAnnotation : das::ManagedStructureAnnotation<PhysObjControlState, false>
{
  PhysObjControlStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysObjControlState", ml)
  {
    cppName = " ::PhysObjControlState";
    addField<DAS_BIND_MANAGED_FIELD(producedAtTick)>("producedAtTick");
  }
};

struct PhysObjAnnotation : das::ManagedStructureAnnotation<PhysObj, false>
{
  PhysObjAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysObj", ml)
  {
    cppName = " ::PhysObj";

    addField<DAS_BIND_MANAGED_FIELD(mass)>("mass");
    addField<DAS_BIND_MANAGED_FIELD(objDestroyMass)>("objDestroyMass");
    addField<DAS_BIND_MANAGED_FIELD(linearDamping)>("linearDamping");
    addField<DAS_BIND_MANAGED_FIELD(angularDamping)>("angularDamping");
    addField<DAS_BIND_MANAGED_FIELD(friction)>("friction");
    addField<DAS_BIND_MANAGED_FIELD(bouncing)>("bouncing");
    addField<DAS_BIND_MANAGED_FIELD(limitFriction)>("limitFriction");
    addField<DAS_BIND_MANAGED_FIELD(frictionGround)>("frictionGround");
    addField<DAS_BIND_MANAGED_FIELD(bouncingGround)>("bouncingGround");
    addField<DAS_BIND_MANAGED_FIELD(minSpeedForBounce)>("minSpeedForBounce");
    addField<DAS_BIND_MANAGED_FIELD(boundingRadius)>("boundingRadius");
    addField<DAS_BIND_MANAGED_FIELD(soundShockImpulse)>("soundShockImpulse");
    addField<DAS_BIND_MANAGED_FIELD(linearSlop)>("linearSlop");
    addField<DAS_BIND_MANAGED_FIELD(energyConservation)>("energyConservation");
    addField<DAS_BIND_MANAGED_FIELD(erp)>("erp");
    addField<DAS_BIND_MANAGED_FIELD(gravityMult)>("gravityMult");
    addField<DAS_BIND_MANAGED_FIELD(momentOfInertia)>("momentOfInertia");
    addField<DAS_BIND_MANAGED_FIELD(velocityLimit)>("velocityLimit");
    addField<DAS_BIND_MANAGED_FIELD(velocityLimitDampingMult)>("velocityLimitDampingMult");
    addField<DAS_BIND_MANAGED_FIELD(omegaLimit)>("omegaLimit");
    addField<DAS_BIND_MANAGED_FIELD(omegaLimitDampingMult)>("omegaLimitDampingMult");
    addField<DAS_BIND_MANAGED_FIELD(isVelocityLimitAbsolute)>("isVelocityLimitAbsolute");
    addField<DAS_BIND_MANAGED_FIELD(isOmegaLimitAbsolute)>("isOmegaLimitAbsolute");
    addField<DAS_BIND_MANAGED_FIELD(useFutureContacts)>("useFutureContacts");
    addField<DAS_BIND_MANAGED_FIELD(useMovementDamping)>("useMovementDamping");
    addField<DAS_BIND_MANAGED_FIELD(ignoreCollision)>("ignoreCollision");
    addField<DAS_BIND_MANAGED_FIELD(hasGroundCollisionPoint)>("hasGroundCollisionPoint");
    addField<DAS_BIND_MANAGED_FIELD(hasRiDestroyingCollision)>("hasRiDestroyingCollision");
    addField<DAS_BIND_MANAGED_FIELD(hasFrtCollision)>("hasFrtCollision");
    addField<DAS_BIND_MANAGED_FIELD(shouldTraceGround)>("shouldTraceGround");
    addField<DAS_BIND_MANAGED_FIELD(addToWorld)>("addToWorld");
    addField<DAS_BIND_MANAGED_FIELD(skipUpdateOnSleep)>("skipUpdateOnSleep");
    addField<DAS_BIND_MANAGED_FIELD(centerOfMass)>("centerOfMass");

    // addField<DAS_BIND_MANAGED_FIELD(collision)>("collision"); // Tab<CollisionObject>
    addField<DAS_BIND_MANAGED_FIELD(ccdSpheres)>("ccdSpheres");

    addField<DAS_BIND_MANAGED_FIELD(collisionNodeTm)>("collisionNodeTm");
    addField<DAS_BIND_MANAGED_FIELD(collisionNodes)>("collisionNodes");

    addField<DAS_BIND_MANAGED_FIELD(groundCollisionPoint)>("groundCollisionPoint");
    addField<DAS_BIND_MANAGED_FIELD(frtCollisionNormal)>("frtCollisionNormal");

    addField<DAS_BIND_MANAGED_FIELD(appliedCT)>("appliedCT");
    addField<DAS_BIND_MANAGED_FIELD(producedCT)>("producedCT");
    addField<DAS_BIND_MANAGED_FIELD(unapprovedCT)>("unapprovedCT");
    addField<DAS_BIND_MANAGED_FIELD(previousState)>("previousState");
    addField<DAS_BIND_MANAGED_FIELD(currentState)>("currentState");
    addProperty<DAS_BIND_MANAGED_PROP(getAuthorityApprovedState)>("authorityApprovedState", "getAuthorityApprovedState");
    addField<DAS_BIND_MANAGED_FIELD(authorityApprovedPartialState)>("authorityApprovedPartialState");
    addField<DAS_BIND_MANAGED_FIELD(visualLocation)>("visualLocation");
    addField<DAS_BIND_MANAGED_FIELD(timeStep)>("timeStep");
  }
};

namespace bind_dascript
{
class PhysObjModule final : public das::Module
{
public:
  PhysObjModule() : das::Module("PhysObj")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("GamePhys"));

    addAnnotation(das::make_smart<PhysObjStateAnnotation>(lib));
    addAnnotation(das::make_smart<PhysObjControlStateAnnotation>(lib));
    addAnnotation(das::make_smart<PhysObjAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(bind_dascript::phys_obj_rescheduleAuthorityApprovedSend)>(*this, lib,
      "phys_obj_rescheduleAuthorityApprovedSend", das::SideEffects::modifyArgument,
      "bind_dascript::phys_obj_rescheduleAuthorityApprovedSend");
    using method_addForce = DAS_CALL_MEMBER(PhysObj::addForce);
    das::addExtern<DAS_CALL_METHOD(method_addForce)>(*this, lib, "phys_obj_addForce", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysObj::addForce));
    using method_isCollisionValid = DAS_CALL_MEMBER(PhysObj::isCollisionValid);
    das::addExtern<DAS_CALL_METHOD(method_isCollisionValid)>(*this, lib, "isCollisionValid", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(PhysObj::isCollisionValid));
    using method_wakeUp = DAS_CALL_MEMBER(PhysObj::wakeUp);
    das::addExtern<DAS_CALL_METHOD(method_wakeUp)>(*this, lib, "wakeUp", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysObj::wakeUp));
    using method_updatePhys = DAS_CALL_MEMBER(PhysObj::updatePhys);
    das::addExtern<DAS_CALL_METHOD(method_updatePhys)>(*this, lib, "updatePhys", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysObj::updatePhys));
    using method_updatePhysInWorld = DAS_CALL_MEMBER(PhysObj::updatePhysInWorld);
    das::addExtern<DAS_CALL_METHOD(method_updatePhysInWorld)>(*this, lib, "updatePhysInWorld", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysObj::updatePhysInWorld));

    das::addExtern<DAS_BIND_FUN(bind_dascript::registerCustomPhysStateSyncer<PhysObj>)>(*this, lib, "registerCustomPhysStateSyncer",
      das::SideEffects::modifyArgument, "bind_dascript::registerCustomPhysStateSyncer<PhysObj>");
    das::addExtern<DAS_BIND_FUN(bind_dascript::unregisterCustomPhysStateSyncer<PhysObj>)>(*this, lib,
      "unregisterCustomPhysStateSyncer", das::SideEffects::modifyArgument, "bind_dascript::unregisterCustomPhysStateSyncer<PhysObj>");

    using method_initCustomControls = DAS_CALL_MEMBER(PhysObj::initCustomControls);
    das::addExtern<DAS_CALL_METHOD(method_initCustomControls)>(*this, lib, "initCustomControls", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysObj::initCustomControls));
    using method_getAppliedCustomControls = DAS_CALL_MEMBER(PhysObj::getAppliedCustomControls);
    das::addExtern<DAS_CALL_METHOD(method_getAppliedCustomControls)>(*this, lib, "getAppliedCustomControls",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysObj::getAppliedCustomControls));
    using method_getProducedCustomControls = DAS_CALL_MEMBER(PhysObj::getProducedCustomControls);
    das::addExtern<DAS_CALL_METHOD(method_getProducedCustomControls)>(*this, lib, "getProducedCustomControls",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(PhysObj::getProducedCustomControls));

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotPhysObj.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PhysObjModule, bind_dascript);
