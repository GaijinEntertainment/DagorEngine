// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotHumanPhys.h>
#include <dasModules/aotGamePhys.h>
#include <gamePhys/phys/walker/humanPhys.h>
#include <ecs/phys/netPhysResync.h>

DAS_BASE_BIND_ENUM_98(HUMoveState, HUMoveState, EMS_STAND, EMS_WALK, EMS_RUN, EMS_SPRINT, EMS_ROTATE_LEFT, EMS_ROTATE_RIGHT);
DAS_BASE_BIND_ENUM_98(HUStandState, HUStandState, ESS_STAND, ESS_CROUCH, ESS_CRAWL, ESS_DOWNED, ESS_SWIM, ESS_SWIM_UNDERWATER,
  ESS_CLIMB, ESS_CLIMB_THROUGH, ESS_CLIMB_OVER, ESS_CLIMB_LADDER, ESS_EXTERNALLY_CONTROLLED, ESS_NUM);
DAS_BASE_BIND_ENUM(HumanPhysState::StateFlag, StateFlag, ST_JUMP, ST_CROUCH, ST_CRAWL, ST_ON_GROUND, ST_SPRINT, ST_WALK, ST_SWIM,
  ST_DOWNED);
DAS_BASE_BIND_ENUM(HumanControlState::DodgeState, DodgeState, No, Left, Right, Back);
DAS_BASE_BIND_ENUM(RidingState, RidingState, E_NOT_RIDING, E_RIDING_STAND, E_RIDING_WALK, E_RIDING_SPRINT);

DAS_ANNOTATE_VECTOR(HumanWeaponParamsArray, HumanWeaponParamsArray)

struct HumanControlStateAnnotation : das::ManagedStructureAnnotation<HumanControlState, false>
{
  HumanControlStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanControlState", ml)
  {
    cppName = " ::HumanControlState";
    addField<DAS_BIND_MANAGED_FIELD(producedAtTick)>("producedAtTick");
    addField<DAS_BIND_MANAGED_FIELD(packedState)>("packedState", "packedState");
    addField<DAS_BIND_MANAGED_FIELD(walkPacked)>("walkPacked", "walkPacked");
    addField<DAS_BIND_MANAGED_FIELD(wishLookDirPacked.val)>("wishLookDirPacked", "wishLookDirPacked.val");
    addField<DAS_BIND_MANAGED_FIELD(wishShootDirPacked.val)>("wishShootDirPacked", "wishShootDirPacked.val");
    addField<DAS_BIND_MANAGED_FIELD(lastEnqueuedHctShoot)>("lastEnqueuedHctShoot", "lastEnqueuedHctShoot");
    addField<DAS_BIND_MANAGED_FIELD(haveUnenqueuedHctShoot)>("haveUnenqueuedHctShoot", "haveUnenqueuedHctShoot");
    addProperty<DAS_BIND_MANAGED_PROP(getLeanPosition)>("leanPosition", "getLeanPosition");
    addProperty<DAS_BIND_MANAGED_PROP(getDodgeState)>("dodgeState", "getDodgeState");
    addProperty<DAS_BIND_MANAGED_PROP(isDeviceStateSet)>("isDeviceStateSet");
    addProperty<DAS_BIND_MANAGED_PROP(isQuickReloadStateSet)>("isQuickReloadStateSet");
    addProperty<DAS_BIND_MANAGED_PROP(isAltAttackState)>("isAltAttackState");
    addProperty<DAS_BIND_MANAGED_PROP(getChosenWeapon)>("chosenWeapon", "getChosenWeapon");
    addProperty<DAS_BIND_MANAGED_PROP(getWishShootDir)>("wishShootDir", "getWishShootDir");
    addProperty<DAS_BIND_MANAGED_PROP(getWalkDir)>("walkDir", "getWalkDir");
    addProperty<DAS_BIND_MANAGED_PROP(getWishLookDir)>("wishLookDir", "getWishLookDir");
    addProperty<DAS_BIND_MANAGED_PROP(getWalkSpeed)>("walkSpeed", "getWalkSpeed");
    addProperty<DAS_BIND_MANAGED_PROP(isMoving)>("isMoving");
    addProperty<DAS_BIND_MANAGED_PROP(isControlsForged)>("isControlsForged");
  }
};

struct HumanWeaponParamsAnnotation : das::ManagedStructureAnnotation<HumanWeaponParams, false>
{
  HumanWeaponParamsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanWeaponParams", ml)
  {
    cppName = " ::HumanWeaponParams";

    addField<DAS_BIND_MANAGED_FIELD(walkMoveMagnitude)>("walkMoveMagnitude");
    addField<DAS_BIND_MANAGED_FIELD(holsterTime)>("holsterTime");
    addField<DAS_BIND_MANAGED_FIELD(equipTime)>("equipTime");

    addField<DAS_BIND_MANAGED_FIELD(breathShakeMagnitude)>("breathShakeMagnitude");
    addField<DAS_BIND_MANAGED_FIELD(crouchBreathShakeMagnitude)>("crouchBreathShakeMagnitude");
    addField<DAS_BIND_MANAGED_FIELD(crawlBreathShakeMagnitude)>("crawlBreathShakeMagnitude");

    addField<DAS_BIND_MANAGED_FIELD(predictTime)>("predictTime");
    addField<DAS_BIND_MANAGED_FIELD(targetSpdVisc)>("targetSpdVisc");
    addField<DAS_BIND_MANAGED_FIELD(gunSpdVisc)>("gunSpdVisc");
    addField<DAS_BIND_MANAGED_FIELD(gunSpdDeltaMult)>("gunSpdDeltaMult");
    addField<DAS_BIND_MANAGED_FIELD(maxGunSpd)>("maxGunSpd");
    addField<DAS_BIND_MANAGED_FIELD(moveToSpd)>("moveToSpd");
    addField<DAS_BIND_MANAGED_FIELD(vertOffsetRestoreVisc)>("vertOffsetRestoreVisc");
    addField<DAS_BIND_MANAGED_FIELD(vertOffsetMoveDownVisc)>("vertOffsetMoveDownVisc");
    addField<DAS_BIND_MANAGED_FIELD(equipSpeedMult)>("equipSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(holsterSwapSpeedMult)>("holsterSwapSpeedMult");

    addField<DAS_BIND_MANAGED_FIELD(offsAimNode)>("offsAimNode");
    addField<DAS_BIND_MANAGED_FIELD(offsCheckLeftNode)>("offsCheckLeftNode");
    addField<DAS_BIND_MANAGED_FIELD(offsCheckRightNode)>("offsCheckRightNode");
    addField<DAS_BIND_MANAGED_FIELD(gunLen)>("gunLen");
    addField<DAS_BIND_MANAGED_FIELD(exists)>("exists");
  }
};

struct HumanWeaponEquipStateAnnotation : das::ManagedStructureAnnotation<HumanWeaponEquipState, false>
{
  HumanWeaponEquipStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanWeaponEquipState", ml)
  {
    cppName = " ::HumanWeaponEquipState";
    addField<DAS_BIND_MANAGED_FIELD(progress)>("progress");
    addField<DAS_BIND_MANAGED_FIELD(curState)>("curState");
    addField<DAS_BIND_MANAGED_FIELD(curSlot)>("curSlot");
    addField<DAS_BIND_MANAGED_FIELD(nextSlot)>("nextSlot");
    addProperty<DAS_BIND_MANAGED_PROP(getEffectiveCurSlot)>("effectiveCurSlot", "getEffectiveCurSlot");
  }
};

struct HumanPhysStateAnnotation : das::ManagedStructureAnnotation<HumanPhysState, false>
{
  HumanPhysStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanPhysState", ml)
  {
    cppName = " ::HumanPhysState";
    // NOTE: please add new field according to fields order
    // begin HumanSerializableState
    addField<DAS_BIND_MANAGED_FIELD(atTick)>("atTick");
    addField<DAS_BIND_MANAGED_FIELD(lastAppliedControlsForTick)>("lastAppliedControlsForTick");
    addField<DAS_BIND_MANAGED_FIELD(canBeCheckedForSync)>("canBeCheckedForSync");
    addField<DAS_BIND_MANAGED_FIELD(location)>("location");
    addField<DAS_BIND_MANAGED_FIELD(velocity)>("velocity");
    addField<DAS_BIND_MANAGED_FIELD(standingVelocity)>("standingVelocity");
    addField<DAS_BIND_MANAGED_FIELD(walkDir)>("walkDir");
    addField<DAS_BIND_MANAGED_FIELD(bodyOrientDir)>("bodyOrientDir");
    addField<DAS_BIND_MANAGED_FIELD(walkNormal)>("walkNormal");
    addField<DAS_BIND_MANAGED_FIELD(vertDirection)>("vertDirection");
    addField<DAS_BIND_MANAGED_FIELD(posOffset)>("posOffset");
    addField<DAS_BIND_MANAGED_FIELD(gravDirection)>("gravDirection");
    addField<DAS_BIND_MANAGED_FIELD(gravMult)>("gravMult");
    addField<DAS_BIND_MANAGED_FIELD(gunDir)>("gunDir");
    addField<DAS_BIND_MANAGED_FIELD(breathOffset)>("breathOffset");
    addField<DAS_BIND_MANAGED_FIELD(handsShakeOffset)>("handsShakeOffset");
    addField<DAS_BIND_MANAGED_FIELD(headDir)>("headDir");
    addField<DAS_BIND_MANAGED_FIELD(gunAngles)>("gunAngles");
    addField<DAS_BIND_MANAGED_FIELD(targetGunSpd)>("targetGunSpd");
    addField<DAS_BIND_MANAGED_FIELD(gunSpd)>("gunSpd");
    addField<DAS_BIND_MANAGED_FIELD(prevAngles)>("prevAngles");
    addField<DAS_BIND_MANAGED_FIELD(gunAimOffset)>("gunAimOffset");
    addField<DAS_BIND_MANAGED_FIELD(height)>("height");
    addField<DAS_BIND_MANAGED_FIELD(heightCurVel)>("heightCurVel");
    addField<DAS_BIND_MANAGED_FIELD(stamina)>("stamina");
    addField<DAS_BIND_MANAGED_FIELD(aimPosition)>("aimPosition");
    addField<DAS_BIND_MANAGED_FIELD(leanPosition)>("leanPosition");
    addField<DAS_BIND_MANAGED_FIELD(aimForZoomProgress)>("aimForZoomProgress");
    addField<DAS_BIND_MANAGED_FIELD(breathShortness)>("breathShortness");
    addField<DAS_BIND_MANAGED_FIELD(breathShakeMult)>("breathShakeMult");
    addField<DAS_BIND_MANAGED_FIELD(breathTimer)>("breathTimer");
    addField<DAS_BIND_MANAGED_FIELD(breathTime)>("breathTime");
    addField<DAS_BIND_MANAGED_FIELD(handsShakeTime)>("handsShakeTime");
    addField<DAS_BIND_MANAGED_FIELD(handsShakeMagnitude)>("handsShakeMagnitude");
    addField<DAS_BIND_MANAGED_FIELD(handsShakeSpeedMult)>("handsShakeSpeedMult");
    // NOTE: please add new field according to fields order

    addField<DAS_BIND_MANAGED_FIELD(zoomPosition)>("zoomPosition");
    addField<DAS_BIND_MANAGED_FIELD(kineticEnergy)>("kineticEnergy");
    addField<DAS_BIND_MANAGED_FIELD(knockBackTimer)>("knockBackTimer");
    addField<DAS_BIND_MANAGED_FIELD(spdSummaryDiff)>("spdSummaryDiff");

    addField<DAS_BIND_MANAGED_FIELD(deltaVelIgnoreAmount)>("deltaVelIgnoreAmount");
    addField<DAS_BIND_MANAGED_FIELD(staminaDrainMult)>("staminaDrainMult");
    addField<DAS_BIND_MANAGED_FIELD(moveSpeedMult)>("moveSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(isWishToMove)>("isWishToMove");
    addField<DAS_BIND_MANAGED_FIELD(swimmingSpeedMult)>("swimmingSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(jumpSpeedMult)>("jumpSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(climbingSpeedMult)>("climbingSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(staminaSprintDrainMult)>("staminaSprintDrainMult");
    addField<DAS_BIND_MANAGED_FIELD(staminaClimbDrainMult)>("staminaClimbDrainMult");
    addField<DAS_BIND_MANAGED_FIELD(accelerationMult)>("accelerationMult");
    addField<DAS_BIND_MANAGED_FIELD(maxStaminaMult)>("maxStaminaMult");
    addField<DAS_BIND_MANAGED_FIELD(restoreStaminaMult)>("restoreStaminaMult");
    addField<DAS_BIND_MANAGED_FIELD(staminaBoostMult)>("staminaBoostMult");
    addField<DAS_BIND_MANAGED_FIELD(sprintSpeedMult)>("sprintSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(sprintLerpSpeedMult)>("sprintLerpSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(breathAmplitudeMult)>("breathAmplitudeMult");
    addField<DAS_BIND_MANAGED_FIELD(fasterChangeWeaponMult)>("fasterChangeWeaponMult");
    addField<DAS_BIND_MANAGED_FIELD(crawlCrouchSpeedMult)>("crawlCrouchSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(fasterChangePoseMult)>("fasterChangePoseMult");
    addField<DAS_BIND_MANAGED_FIELD(weaponTurningSpeedMult)>("weaponTurningSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(aimingAfterFireMult)>("aimingAfterFireMult");
    addField<DAS_BIND_MANAGED_FIELD(aimSpeedMult)>("aimSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(speedInAimStateMult)>("speedInAimStateMult");
    // NOTE: please add new field according to fields order
    // TODO: overrideWallClimb : 1
    addField<DAS_BIND_MANAGED_FIELD(ladderTm)>("ladderTm");
    addField<DAS_BIND_MANAGED_FIELD(guidedByLadder)>("guidedByLadder");
    addField<DAS_BIND_MANAGED_FIELD(climbFromPos)>("climbFromPos");
    addField<DAS_BIND_MANAGED_FIELD(climbToPos)>("climbToPos");
    addField<DAS_BIND_MANAGED_FIELD(climbContactPos)>("climbContactPos");
    addField<DAS_BIND_MANAGED_FIELD(climbDir)>("climbDir");
    addField<DAS_BIND_MANAGED_FIELD(climbNorm)>("climbNorm");
    addField<DAS_BIND_MANAGED_FIELD(climbDeltaHt)>("climbDeltaHt");
    addField<DAS_BIND_MANAGED_FIELD(climbStartAtTime)>("climbStartAtTime");

    addField<DAS_BIND_MANAGED_FIELD(gunBackoffAmount)>("gunBackoffAmount");
    addField<DAS_BIND_MANAGED_FIELD(gunTraceTimer)>("gunTraceTimer");
    addField<DAS_BIND_MANAGED_FIELD(fallbackToSphereCastTimer)>("fallbackToSphereCastTimer");
    // TODO: stoppedSprint : 1
    addField<DAS_BIND_MANAGED_FIELD(lastDashTime)>("lastDashTime");
    addField<DAS_BIND_MANAGED_FIELD(moveState)>("moveState");
    addField<DAS_BIND_MANAGED_FIELD(isInAirHistory)>("isInAirHistory");
    addField<DAS_BIND_MANAGED_FIELD(jumpStartTime)>("jumpStartTime");
    addField<DAS_BIND_MANAGED_FIELD(afterJumpDampingEndTime)>("afterJumpDampingEndTime");
    addField<DAS_BIND_MANAGED_FIELD(afterJumpDampingHeight)>("afterJumpDampingHeight");
    addField<DAS_BIND_MANAGED_FIELD(frictionMult)>("frictionMult");
    addField<DAS_BIND_MANAGED_FIELD(states)>("states");
    addField<DAS_BIND_MANAGED_FIELD(collisionLinksStateFrom)>("collisionLinksStateFrom");
    addField<DAS_BIND_MANAGED_FIELD(collisionLinksStateTo)>("collisionLinksStateTo");
    addField<DAS_BIND_MANAGED_FIELD(collisionLinkProgress)>("collisionLinkProgress");
    addField<DAS_BIND_MANAGED_FIELD(weapEquipState)>("weapEquipState");
    // NOTE: please add new field according to fields order
    // end HumanSerializableState

    // begin HumanPhysState
    // NOTE: please add new field according to fields order
    addField<DAS_BIND_MANAGED_FIELD(acceleration)>("acceleration");
    addField<DAS_BIND_MANAGED_FIELD(isControllable)>("isControllable");
    addField<DAS_BIND_MANAGED_FIELD(extHeight)>("extHeight");
    addField<DAS_BIND_MANAGED_FIELD(torsoCollisionTmPos)>("torsoCollisionTmPos");

    addField<DAS_BIND_MANAGED_FIELD(walkMatId)>("walkMatId");
    addField<DAS_BIND_MANAGED_FIELD(torsoContactMatId)>("torsoContactMatId");
    addField<DAS_BIND_MANAGED_FIELD(torsoContactRendinstPool)>("torsoContactRendinstPool");

    addProperty<DAS_BIND_MANAGED_PROP(isCrouch)>("isCrouch");
    addProperty<DAS_BIND_MANAGED_PROP(isCrawl)>("isCrawl");
    addProperty<DAS_BIND_MANAGED_PROP(isAiming)>("isAiming");
    addProperty<DAS_BIND_MANAGED_PROP(isZooming)>("isZooming");
    addProperty<DAS_BIND_MANAGED_PROP(getStandState)>("standState", "getStandState");
    addProperty<DAS_BIND_MANAGED_PROP(isAttachedToLadder)>("isAttachedToLadder");
    addProperty<DAS_BIND_MANAGED_PROP(isDetachedFromLadder)>("isDetachedFromLadder");
    addProperty<DAS_BIND_MANAGED_PROP(isExternalControlled)>("isExternalControlled");
    // NOTE: please add new field according to fields order
    // end HumanPhysState
  }
};

struct PrecomputedWeaponPositionsAnnotation : das::ManagedStructureAnnotation<PrecomputedWeaponPositions, false>
{
  PrecomputedWeaponPositionsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PrecomputedWeaponPositions", ml)
  {
    cppName = " ::PrecomputedWeaponPositions";

    addField<DAS_BIND_MANAGED_FIELD(isLoaded)>("isLoaded");
  }
};

struct HumanPhysAnnotation : das::ManagedStructureAnnotation<HumanPhys, false>
{
  HumanPhysAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanPhys", ml)
  {
    cppName = " ::HumanPhys";
    // PhysicsBase begin
    // NOTE: please add new field according to fields order
    addField<DAS_BIND_MANAGED_FIELD(timeStep)>("timeStep");
    addField<DAS_BIND_MANAGED_FIELD(producedCT)>("producedCT");
    addField<DAS_BIND_MANAGED_FIELD(appliedCT)>("appliedCT");
    addField<DAS_BIND_MANAGED_FIELD(unapprovedCT)>("unapprovedCT");
    addField<DAS_BIND_MANAGED_FIELD(previousState)>("previousState");
    addField<DAS_BIND_MANAGED_FIELD(currentState)>("currentState");
    addProperty<DAS_BIND_MANAGED_PROP(getAuthorityApprovedState)>("authorityApprovedState", "getAuthorityApprovedState");
    addField<DAS_BIND_MANAGED_FIELD(authorityApprovedPartialState)>("authorityApprovedPartialState");
    addField<DAS_BIND_MANAGED_FIELD(visualLocation)>("visualLocation");
    // NOTE: please add new field according to fields order
    // PhysicsBase end

    // HumanPhys begin
    // NOTE: please add new field according to fields order
    addField<DAS_BIND_MANAGED_FIELD(speedCollisionHardness)>("speedCollisionHardness");
    addField<DAS_BIND_MANAGED_FIELD(crouchHeight)>("crouchHeight");
    addField<DAS_BIND_MANAGED_FIELD(standingHeight)>("standingHeight");
    addField<DAS_BIND_MANAGED_FIELD(walkRad)>("walkRad");
    addField<DAS_BIND_MANAGED_FIELD(collRad)>("collRad");
    addField<DAS_BIND_MANAGED_FIELD(ccdRad)>("ccdRad");
    addField<DAS_BIND_MANAGED_FIELD(maxStamina)>("maxStamina");

    addField<DAS_BIND_MANAGED_FIELD(leanDegrees)>("leanDegrees");

    addField<DAS_BIND_MANAGED_FIELD(scale)>("scale");

    addField<DAS_BIND_MANAGED_FIELD(defAimSpeed)>("defAimSpeed");
    addField<DAS_BIND_MANAGED_FIELD(aimSpeed)>("aimSpeed");
    addField<DAS_BIND_MANAGED_FIELD(zoomSpeed)>("zoomSpeed");
    addField<DAS_BIND_MANAGED_FIELD(mass)>("mass");
    // NOTE: please add new field according to fields order
    addField<DAS_BIND_MANAGED_FIELD(swimPosOffset)>("swimPosOffset");

    addField<DAS_BIND_MANAGED_FIELD(swimmingLevelBias)>("swimmingLevelBias");
    addField<DAS_BIND_MANAGED_FIELD(waterSwimmingLevel)>("waterSwimmingLevel");

    addField<DAS_BIND_MANAGED_FIELD(canCrawl)>("canCrawl");
    addField<DAS_BIND_MANAGED_FIELD(canCrouch)>("canCrouch");
    addField<DAS_BIND_MANAGED_FIELD(canClimb)>("canClimb");
    addField<DAS_BIND_MANAGED_FIELD(canSprint)>("canSprint");
    addField<DAS_BIND_MANAGED_FIELD(canJump)>("canJump");
    addField<DAS_BIND_MANAGED_FIELD(canMove)>("canMove");
    addField<DAS_BIND_MANAGED_FIELD(canSwitchWeapon)>("canSwitchWeapon");
    addField<DAS_BIND_MANAGED_FIELD(hasGuns)>("hasGuns");
    addField<DAS_BIND_MANAGED_FIELD(hasExternalHeight)>("hasExternalHeight");
    addField<DAS_BIND_MANAGED_FIELD(allowWeaponSwitchOnSprint)>("allowWeaponSwitchOnSprint");

    addField<DAS_BIND_MANAGED_FIELD(gunOffset)>("gunOffset");

    addField<DAS_BIND_MANAGED_FIELD(rayMatId)>("rayMatId");
    addField<DAS_BIND_MANAGED_FIELD(airStaminaRestoreMult)>("airStaminaRestoreMult");

    addField<DAS_BIND_MANAGED_FIELD(weaponParams)>("weaponParams");
    addField<DAS_BIND_MANAGED_FIELD(precompWeaponPos)>("precompWeaponPos");

    addProperty<DAS_BIND_MANAGED_PROP(canStartSprint)>("canStartSprint");
    addProperty<DAS_BIND_MANAGED_PROP(getJumpStaminaDrain)>("getJumpStaminaDrain");
    addProperty<DAS_BIND_MANAGED_PROP(getClimbStaminaDrain)>("getClimbStaminaDrain");
    addProperty<DAS_BIND_MANAGED_PROP(calcCcdPos)>("ccdPos", "calcCcdPos");
    addProperty<DAS_BIND_MANAGED_PROP(getMaxObstacleHeight)>("maxObstacleHeight", "getMaxObstacleHeight");
    addProperty<DAS_BIND_MANAGED_PROP(getTorsoCollision)>("torsoCollision", "getTorsoCollision");
    addProperty<DAS_BIND_MANAGED_PROP(getClimberCollision)>("climberCollision", "getClimberCollision");
    addProperty<DAS_BIND_MANAGED_PROP(getWallJumpCollision)>("wallJumpCollision", "getWallJumpCollision");

    // NOTE: please add new field according to fields order
    // HumanPhys end
  }
};

namespace bind_dascript
{
class HumanPhysModule final : public das::Module
{
public:
  HumanPhysModule() : das::Module("HumanPhys")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("GamePhys"), true);
    addBuiltinDependency(lib, require("Dacoll"));
    addEnumeration(das::make_smart<EnumerationHUStandState>());
    addEnumeration(das::make_smart<EnumerationHUWeaponSlots>());
    addEnumeration(das::make_smart<EnumerationHUMoveState>());
    addEnumeration(das::make_smart<EnumerationHumanPhysControlType>());
    addEnumeration(das::make_smart<EnumerationHUWeaponEquipState>());
    addEnumeration(das::make_smart<EnumerationHumanControlThrowSlot>());
    addEnumeration(das::make_smart<EnumerationStateFlag>());
    addEnumeration(das::make_smart<EnumerationPrecomputedPresetMode>());
    addEnumeration(das::make_smart<EnumerationHumanStatePos>());
    addEnumeration(das::make_smart<EnumerationHumanStateMove>());
    addEnumeration(das::make_smart<EnumerationHumanStateUpperBody>());
    addEnumeration(das::make_smart<EnumerationStateJump>());
    addEnumeration(das::make_smart<EnumerationRidingState>());
    addEnumeration(das::make_smart<EnumerationHumanAnimStateFlags>());
    addEnumeration(das::make_smart<EnumerationDodgeState>());

    addAnnotation(das::make_smart<HumanControlStateAnnotation>(lib));
    addAnnotation(das::make_smart<HumanWeaponParamsAnnotation>(lib));
    addAnnotation(das::make_smart<HumanWeaponParamsArrayAnnotation>(lib));
    addAnnotation(das::make_smart<HumanWeaponEquipStateAnnotation>(lib));
    addAnnotation(das::make_smart<HumanPhysStateAnnotation>(lib));
    addAnnotation(das::make_smart<PrecomputedWeaponPositionsAnnotation>(lib));
    addAnnotation(das::make_smart<HumanPhysAnnotation>(lib));

    das::addEnumFlagOps<HumanPhysState::StateFlag>(*this, lib, "HumanPhysState::StateFlag");

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_human_weapon_slot_name)>(*this, lib, "get_human_weapon_slot_name",
      das::SideEffects::accessExternal, "bind_dascript::get_human_weapon_slot_name");
    das::addExtern<DAS_BIND_FUN(bind_dascript::cast_int_to_weapon_slots)>(*this, lib, "HUWeaponSlots", das::SideEffects::none,
      "bind_dascript::cast_int_to_weapon_slots");
    das::addExtern<DAS_BIND_FUN(bind_dascript::cast_str_to_weapon_slots)>(*this, lib, "HUWeaponSlots", das::SideEffects::none,
      "bind_dascript::cast_str_to_weapon_slots");

    // HumanPhys
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_is_hold_breath)>(*this, lib, "is_hold_breath", das::SideEffects::none,
      "bind_dascript::human_phys_state_is_hold_breath");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_is_hold_breath)>(*this, lib, "set_is_hold_breath",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_is_hold_breath");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_block_sprint)>(*this, lib, "human_phys_state_set_block_sprint",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_block_sprint");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_reset_stamina)>(*this, lib, "human_phys_state_reset_stamina",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_reset_stamina");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_wakeUp)>(*this, lib, "human_phys_wakeUp", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_wakeUp");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_rescheduleAuthorityApprovedSend)>(*this, lib,
      "human_phys_rescheduleAuthorityApprovedSend", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_rescheduleAuthorityApprovedSend");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_restore_stamina)>(*this, lib, "human_phys_restore_stamina",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_restore_stamina");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_calcGunPos)>(*this, lib, "human_phys_calcGunPos", das::SideEffects::none,
      "bind_dascript::human_phys_calcGunPos");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_calcGunTm)>(*this, lib, "human_phys_calcGunTm",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_calcGunTm");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_can_aim)>(*this, lib, "human_phys_state_set_can_aim",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_can_aim");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_can_zoom)>(*this, lib, "human_phys_state_set_can_zoom",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_can_zoom");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_can_aim)>(*this, lib, "human_phys_state_can_aim",
      das::SideEffects::none, "bind_dascript::human_phys_state_can_aim");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_can_zoom)>(*this, lib, "human_phys_state_can_zoom",
      das::SideEffects::none, "bind_dascript::human_phys_state_can_zoom");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_stoppedSprint)>(*this, lib, "human_phys_state_set_stoppedSprint",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_stoppedSprint");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_stoppedSprint)>(*this, lib, "human_phys_state_stoppedSprint",
      das::SideEffects::none, "bind_dascript::human_phys_state_stoppedSprint");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_attachedToExternalGun)>(*this, lib,
      "human_phys_state_set_attachedToExternalGun", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_state_set_attachedToExternalGun");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_attachedToExternalGun)>(*this, lib,
      "human_phys_state_attachedToExternalGun", das::SideEffects::none, "bind_dascript::human_phys_state_attachedToExternalGun");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_attachedToLadder)>(*this, lib,
      "human_phys_state_set_attachedToLadder", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_state_set_attachedToLadder");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_attachedToLadder)>(*this, lib, "human_phys_state_attachedToLadder",
      das::SideEffects::none, "bind_dascript::human_phys_state_attachedToLadder");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_detachedFromLadder)>(*this, lib,
      "human_phys_state_set_detachedFromLadder", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_state_set_detachedFromLadder");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_forceWeaponUp)>(*this, lib, "human_phys_state_set_forceWeaponUp",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_forceWeaponUp");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_forceWeaponUp)>(*this, lib, "human_phys_state_forceWeaponUp",
      das::SideEffects::none, "bind_dascript::human_phys_state_forceWeaponUp");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_useSecondaryCcd)>(*this, lib,
      "human_phys_state_set_useSecondaryCcd", das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_useSecondaryCcd");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_useSecondaryCcd)>(*this, lib, "human_phys_state_useSecondaryCcd",
      das::SideEffects::none, "bind_dascript::human_phys_state_useSecondaryCcd");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_overrideWallClimb)>(*this, lib,
      "human_phys_state_get_overrideWallClimb", das::SideEffects::none, "bind_dascript::human_phys_state_get_overrideWallClimb");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_overrideWallClimb)>(*this, lib,
      "human_phys_state_set_overrideWallClimb", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_state_set_overrideWallClimb");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_is_downed)>(*this, lib, "human_phys_state_get_is_downed",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_is_downed");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_is_downed)>(*this, lib, "human_phys_state_set_is_downed",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_is_downed");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_canShoot)>(*this, lib, "human_phys_state_get_canShoot",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_canShoot");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_canShoot)>(*this, lib, "human_phys_state_set_canShoot",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_canShoot");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_reduceToWalk)>(*this, lib, "human_phys_state_get_reduceToWalk",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_reduceToWalk");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_reduceToWalk)>(*this, lib, "human_phys_state_set_reduceToWalk",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_reduceToWalk");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_isLadderQuickMovingUp)>(*this, lib,
      "human_phys_state_get_isLadderQuickMovingUp", das::SideEffects::none,
      "bind_dascript::human_phys_state_get_isLadderQuickMovingUp");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_isLadderQuickMovingUp)>(*this, lib,
      "human_phys_state_set_isLadderQuickMovingUp", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_state_set_isLadderQuickMovingUp");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_isLadderQuickMovingDown)>(*this, lib,
      "human_phys_state_get_isLadderQuickMovingDown", das::SideEffects::none,
      "bind_dascript::human_phys_state_get_isLadderQuickMovingDown");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_isLadderQuickMovingDown)>(*this, lib,
      "human_phys_state_set_isLadderQuickMovingDown", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_state_set_isLadderQuickMovingDown");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_forceWeaponDown)>(*this, lib,
      "human_phys_state_get_forceWeaponDown", das::SideEffects::none, "bind_dascript::human_phys_state_get_forceWeaponDown");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_forceWeaponDown)>(*this, lib,
      "human_phys_state_set_forceWeaponDown", das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_forceWeaponDown");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_isClimbing)>(*this, lib, "human_phys_state_get_isClimbing",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_isClimbing");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_isClimbingOverObstacle)>(*this, lib,
      "human_phys_state_get_isClimbingOverObstacle", das::SideEffects::none,
      "bind_dascript::human_phys_state_get_isClimbingOverObstacle");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_isFastClimbing)>(*this, lib, "human_phys_state_get_isFastClimbing",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_isFastClimbing");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_is_swimming)>(*this, lib, "human_phys_state_get_is_swimming",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_is_swimming");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_is_underwater)>(*this, lib, "human_phys_state_get_is_underwater",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_is_underwater");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_isAttached)>(*this, lib, "human_phys_state_set_isAttached",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_isAttached");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_isAttached)>(*this, lib, "human_phys_state_get_isAttached",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_isAttached");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_disableCollision)>(*this, lib,
      "human_phys_state_set_disableCollision", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_state_set_disableCollision");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_disableCollision)>(*this, lib,
      "human_phys_state_get_disableCollision", das::SideEffects::none, "bind_dascript::human_phys_state_get_disableCollision");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_setCollisionMatId)>(*this, lib, "human_phys_setCollisionMatId",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_setCollisionMatId");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_isMount)>(*this, lib, "human_phys_state_get_isMount",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_isMount");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_isMount)>(*this, lib, "human_phys_state_set_isMount",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_isMount");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_isVisible)>(*this, lib, "human_phys_state_get_isVisible",
      das::SideEffects::none, "bind_dascript::human_phys_state_get_isVisible");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_isVisible)>(*this, lib, "human_phys_state_set_isVisible",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_state_set_isVisible");
    das::addExtern<DAS_BIND_FUN(human_phys_getTraceHandle)>(*this, lib, "human_phys_getTraceHandle", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_getTraceHandle");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_set_isExternalControlled)>(*this, lib,
      "human_phys_state_set_isExternalControlled", das::SideEffects::modifyArgument,
      "bind_dascript::human_phys_state_set_isExternalControlled");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_state_get_torso_contacts)>(*this, lib, "human_phys_state_get_torso_contacts",
      das::SideEffects::accessExternal, "bind_dascript::human_phys_state_get_torso_contacts");


    using method_setWeaponLen = DAS_CALL_MEMBER(HumanPhys::setWeaponLen);
    das::addExtern<DAS_CALL_METHOD(method_setWeaponLen)>(*this, lib, "human_phys_setWeaponLen", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(HumanPhys::setWeaponLen));
    using method_getWalkSpeed = DAS_CALL_MEMBER(HumanPhys::getWalkSpeed);
    das::addExtern<DAS_CALL_METHOD(method_getWalkSpeed)>(*this, lib, "human_phys_getWalkSpeed", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(HumanPhys::getWalkSpeed));
    using method_setWalkSpeed = DAS_CALL_MEMBER(HumanPhys::setWalkSpeed);
    das::addExtern<DAS_CALL_METHOD(method_setWalkSpeed)>(*this, lib, "human_phys_setWalkSpeed", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(HumanPhys::setWalkSpeed));
    using method_tryClimbing = DAS_CALL_MEMBER(HumanPhys::tryClimbing);
    das::addExtern<DAS_CALL_METHOD(method_tryClimbing)>(*this, lib, "human_phys_tryClimbing", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(HumanPhys::tryClimbing));
    using method_resetClimbing = DAS_CALL_MEMBER(HumanPhys::resetClimbing);
    das::addExtern<DAS_CALL_METHOD(method_resetClimbing)>(*this, lib, "human_phys_resetClimbing", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(HumanPhys::resetClimbing));
    using method_calcCollCenter = DAS_CALL_MEMBER(HumanPhys::calcCollCenter);
    das::addExtern<DAS_CALL_METHOD(method_calcCollCenter)>(*this, lib, "human_phys_calcCollCenter", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(HumanPhys::calcCollCenter));
    using method_setQuickReloadState = DAS_CALL_MEMBER(HumanControlState::setQuickReloadState);
    das::addExtern<DAS_CALL_METHOD(method_setQuickReloadState)>(*this, lib, "setQuickReloadState", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(HumanControlState::setQuickReloadState));

    // HumanControlState
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_walk_speed)>(*this, lib, "human_control_state_set_walk_speed",
      das::SideEffects::modifyArgument, "bind_dascript::human_control_state_set_walk_speed");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_control_bit)>(*this, lib, "human_control_state_set_control_bit",
      das::SideEffects::modifyArgument, "bind_dascript::human_control_state_set_control_bit");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_throw_state)>(*this, lib, "human_control_state_set_throw_state",
      das::SideEffects::modifyArgument, "bind_dascript::human_control_state_set_throw_state");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_lean_position)>(*this, lib,
      "human_control_state_set_lean_position", das::SideEffects::modifyArgument,
      "bind_dascript::human_control_state_set_lean_position");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_dodge_state)>(*this, lib, "human_control_state_set_dodge_state",
      das::SideEffects::modifyArgument, "bind_dascript::human_control_state_set_dodge_state");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_unpack_shootPos)>(*this, lib, "human_control_state_unpack_shootPos",
      das::SideEffects::none, "bind_dascript::human_control_state_unpack_shootPos");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_get_shootPos_packed)>(*this, lib,
      "human_control_state_get_shootPos_packed", das::SideEffects::none, "bind_dascript::human_control_state_get_shootPos_packed");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_walk_dir)>(*this, lib, "human_control_state_set_walk_dir",
      das::SideEffects::modifyArgument, "bind_dascript::human_control_state_set_walk_dir");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_world_walk_dir)>(*this, lib,
      "human_control_state_set_world_walk_dir", das::SideEffects::modifyArgument,
      "bind_dascript::human_control_state_set_world_walk_dir");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_wish_look_dir)>(*this, lib,
      "human_control_state_set_wish_look_dir", das::SideEffects::modifyArgument,
      "bind_dascript::human_control_state_set_wish_look_dir");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_wish_shoot_dir)>(*this, lib,
      "human_control_state_set_wish_shoot_dir", das::SideEffects::modifyArgument,
      "bind_dascript::human_control_state_set_wish_shoot_dir");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_neutral_ctrl)>(*this, lib,
      "human_control_state_set_neutral_ctrl", das::SideEffects::modifyArgument, "bind_dascript::human_control_state_set_neutral_ctrl");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_clear_unaproved_ctrl)>(*this, lib,
      "human_control_state_clear_unaproved_ctrl", das::SideEffects::modifyArgument,
      "bind_dascript::human_control_state_clear_unaproved_ctrl");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_reset)>(*this, lib, "human_control_state_reset",
      das::SideEffects::modifyArgument, "bind_dascript::human_control_state_reset");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_is_control_bit_set)>(*this, lib, "is_control_bit_set",
      das::SideEffects::none, "bind_dascript::human_control_state_is_control_bit_set");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_is_throw_state_set)>(*this, lib,
      "human_control_state_is_throw_state_set", das::SideEffects::none, "bind_dascript::human_control_state_is_throw_state_set");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_chosen_weapon)>(*this, lib, "set_chosen_weapon",
      das::SideEffects::modifyArgument, "bind_dascript::human_control_state_set_chosen_weapon");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_control_state_set_device_state)>(*this, lib,
      "human_control_state_set_device_state", das::SideEffects::modifyArgument, "bind_dascript::human_control_state_set_device_state");
    using method_setAltAttackState = DAS_CALL_MEMBER(HumanControlState::setAltAttackState);
    das::addExtern<DAS_CALL_METHOD(method_setAltAttackState)>(*this, lib, "human_control_state_setAltAttackState",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(HumanControlState::setAltAttackState));

    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_processCcdOffset)>(*this, lib, "human_phys_processCcdOffset",
      das::SideEffects::accessExternal, "bind_dascript::human_phys_processCcdOffset");
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_phys_getCollisionLinkData)>(*this, lib, "human_phys_getCollisionLinkData",
      das::SideEffects::modifyArgument, "bind_dascript::human_phys_getCollisionLinkData");
    using method_isAiming = DAS_CALL_MEMBER(HumanPhys::isAiming);
    das::addExtern<DAS_CALL_METHOD(method_isAiming)>(*this, lib, "human_phys_isAiming", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(HumanPhys::isAiming));

    das::addExtern<DAS_BIND_FUN(bind_dascript::registerCustomPhysStateSyncer<HumanPhys>)>(*this, lib, "registerCustomPhysStateSyncer",
      das::SideEffects::modifyArgument, "bind_dascript::registerCustomPhysStateSyncer<HumanPhys>");
    das::addExtern<DAS_BIND_FUN(bind_dascript::unregisterCustomPhysStateSyncer<HumanPhys>)>(*this, lib,
      "unregisterCustomPhysStateSyncer", das::SideEffects::modifyArgument,
      "bind_dascript::unregisterCustomPhysStateSyncer<HumanPhys>");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotHumanPhys.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(HumanPhysModule, bind_dascript);
