// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotGamePhys.h>
#include <gamePhys/common/fixed_dt.h>
#include <ecs/phys/netPhysResync.h>

struct OrientAnnotation : das::ManagedStructureAnnotation<gamephys::Orient, false>
{
  OrientAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Orient", ml)
  {
    cppName = " ::gamephys::Orient";
    addProperty<DAS_BIND_MANAGED_PROP(getAzimuth)>("azimuth", "getAzimuth");
    addProperty<DAS_BIND_MANAGED_PROP(getPitch)>("pitch", "getPitch");
    addProperty<DAS_BIND_MANAGED_PROP(getTangage)>("tangage", "getTangage");
    addProperty<DAS_BIND_MANAGED_PROP(getRoll)>("roll", "getRoll");
    addProperty<DAS_BIND_MANAGED_PROP(getYaw)>("yaw", "getYaw");
    addProperty<DAS_BIND_MANAGED_PROP(getQuat)>("quat", "getQuat");
  }
};

struct LocalOrientAnnotation : das::ManagedStructureAnnotation<gamephys::SimpleLoc::LocalOrient, false>
{
  LocalOrientAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("LocalOrient", ml)
  {
    cppName = " ::gamephys::SimpleLoc::LocalOrient";
    addField<DAS_BIND_MANAGED_FIELD(quat)>("quat");
  }
};

struct SimpleLocAnnotation : das::ManagedStructureAnnotation<gamephys::SimpleLoc, false>
{
  SimpleLocAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SimpleLoc", ml)
  {
    cppName = " ::gamephys::SimpleLoc";
    addField<DAS_BIND_MANAGED_FIELD(P)>("P");
    addField<DAS_BIND_MANAGED_FIELD(O)>("O");
  }
};

struct LocAnnotation : das::ManagedStructureAnnotation<gamephys::Loc, false>
{
  LocAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Loc", ml)
  {
    cppName = " ::gamephys::Loc";
    addField<DAS_BIND_MANAGED_FIELD(P)>("P");
    addField<DAS_BIND_MANAGED_FIELD(O)>("O");
    addProperty<DAS_BIND_MANAGED_PROP(getFwd)>("fwd", "getFwd");
  }
};

struct CommonPhysPartialStateAnnotation : das::ManagedStructureAnnotation<CommonPhysPartialState, false>
{
  CommonPhysPartialStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CommonPhysPartialState", ml)
  {
    cppName = " ::CommonPhysPartialState";
    addField<DAS_BIND_MANAGED_FIELD(location)>("location");
    addField<DAS_BIND_MANAGED_FIELD(velocity)>("velocity");
    addField<DAS_BIND_MANAGED_FIELD(atTick)>("atTick");
    addField<DAS_BIND_MANAGED_FIELD(lastAppliedControlsForTick)>("lastAppliedControlsForTick");
    addField<DAS_BIND_MANAGED_FIELD(isProcessed)>("isProcessed");
  }
};

struct GamePhysMassAnnotation : das::ManagedStructureAnnotation<gamephys::Mass, false>
{
  GamePhysMassAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Mass", ml)
  {
    cppName = " ::gamephys::Mass";
    addField<DAS_BIND_MANAGED_FIELD(massEmpty)>("massEmpty");
    addField<DAS_BIND_MANAGED_FIELD(mass)>("mass");
    addField<DAS_BIND_MANAGED_FIELD(maxWeight)>("maxWeight");
    addField<DAS_BIND_MANAGED_FIELD(payloadMass)>("payloadMass");
    addField<DAS_BIND_MANAGED_FIELD(payloadMomentOfInertia)>("payloadMomentOfInertia");
    addField<DAS_BIND_MANAGED_FIELD(numFuelSystems)>("numFuelSystems");
    addField<DAS_BIND_MANAGED_FIELD(separateFuelTanks)>("separateFuelTanks");
    addField<DAS_BIND_MANAGED_FIELD(numTanks)>("numTanks");
    addField<DAS_BIND_MANAGED_FIELD(crewMass)>("crewMass");
    addField<DAS_BIND_MANAGED_FIELD(oilMass)>("oilMass");
    addField<DAS_BIND_MANAGED_FIELD(nitro)>("nitro");
    addField<DAS_BIND_MANAGED_FIELD(maxNitro)>("maxNitro");
    addField<DAS_BIND_MANAGED_FIELD(centerOfGravity)>("centerOfGravity");
    addField<DAS_BIND_MANAGED_FIELD(initialCenterOfGravity)>("initialCenterOfGravity");
    addField<DAS_BIND_MANAGED_FIELD(centerOfGravityClampY)>("centerOfGravityClampY");
    addField<DAS_BIND_MANAGED_FIELD(momentOfInertiaNorm)>("momentOfInertiaNorm");
    addField<DAS_BIND_MANAGED_FIELD(momentOfInertiaNormMult)>("momentOfInertiaNormMult");
    addField<DAS_BIND_MANAGED_FIELD(momentOfInertia)>("momentOfInertia");
    addField<DAS_BIND_MANAGED_FIELD(advancedMass)>("advancedMass");
    addField<DAS_BIND_MANAGED_FIELD(doesPayloadAffectCOG)>("doesPayloadAffectCOG");
  }
};

struct VolumetricDamageDataAnnotation : das::ManagedStructureAnnotation<gamephys::VolumetricDamageData, false>
{
  VolumetricDamageDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("VolumetricDamageData", ml)
  {
    cppName = " ::gamephys::VolumetricDamageData";
    addField<DAS_BIND_MANAGED_FIELD(pos)>("pos");
    addField<DAS_BIND_MANAGED_FIELD(damage)>("damage");
    addField<DAS_BIND_MANAGED_FIELD(radius)>("radius");
    addField<DAS_BIND_MANAGED_FIELD(timer)>("timer");
    addField<DAS_BIND_MANAGED_FIELD(offenderId)>("offenderId");
    addField<DAS_BIND_MANAGED_FIELD(reason)>("reason");
  }
};

struct ECSCustomPhysStateSyncerDataAnnotation : das::ManagedStructureAnnotation<ECSCustomPhysStateSyncer, false>
{
  ECSCustomPhysStateSyncerDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ECSCustomPhysStateSyncer", ml)
  {
    cppName = " ::ECSCustomPhysStateSyncer";
  }
};

namespace bind_dascript
{
class GamePhysModule final : public das::Module
{
public:
  GamePhysModule() : das::Module("GamePhys")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));

    addEnumeration(das::make_smart<EnumerationDamageReason>());

    addAnnotation(das::make_smart<LocalOrientAnnotation>(lib));
    addAnnotation(das::make_smart<SimpleLocAnnotation>(lib));
    addAnnotation(das::make_smart<OrientAnnotation>(lib));
    addAnnotation(das::make_smart<LocAnnotation>(lib));
    addAnnotation(das::make_smart<CommonPhysPartialStateAnnotation>(lib));
    addAnnotation(das::make_smart<GamePhysMassAnnotation>(lib));
    addAnnotation(das::make_smart<VolumetricDamageDataAnnotation>(lib));

    addAnnotation(das::make_smart<ECSCustomPhysStateSyncerDataAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(gamephys::atmosphere::g)>(*this, lib, "gravity", das::SideEffects::accessExternal,
      "gamephys::atmosphere::g");
    das::addExtern<DAS_BIND_FUN(gamephys::atmosphere::density)>(*this, lib, "atmosphere_density", das::SideEffects::accessExternal,
      "gamephys::atmosphere::density");
    das::addExtern<DAS_BIND_FUN(gamephys::atmosphere::water_density)>(*this, lib, "water_density", das::SideEffects::accessExternal,
      "gamephys::atmosphere::water_density");
    das::addExtern<DAS_BIND_FUN(gamephys::atmosphere::get_wind)>(*this, lib, "get_wind", das::SideEffects::accessExternal,
      "gamephys::atmosphere::get_wind");
    das::addExtern<DAS_BIND_FUN(gamephys::atmosphere::temperature)>(*this, lib, "atmosphere_temperature",
      das::SideEffects::accessExternal, "gamephys::atmosphere::temperature")
      ->arg("height");
    das::addExtern<DAS_BIND_FUN(gamephys::atmosphere::sonicSpeed)>(*this, lib, "atmosphere_sonicSpeed",
      das::SideEffects::accessExternal, "gamephys::atmosphere::sonicSpeed");
    das::addExtern<DAS_BIND_FUN(orient_setYP0)>(*this, lib, "orient_setYP0", das::SideEffects::modifyArgument,
      "bind_dascript::orient_setYP0");
    das::addExtern<DAS_BIND_FUN(orient_transformInv)>(*this, lib, "orient_transformInv", das::SideEffects::modifyArgument,
      "bind_dascript::orient_transformInv");
    das::addExtern<DAS_BIND_FUN(location_toTM)>(*this, lib, "location_toTM", das::SideEffects::modifyArgument,
      "bind_dascript::location_toTM");
    das::addExtern<DAS_BIND_FUN(location_makeTM), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "location_makeTM",
      das::SideEffects::none, "bind_dascript::location_makeTM");
    das::addExtern<DAS_BIND_FUN(make_empty_SimpleLoc), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "SimpleLoc",
      das::SideEffects::none, "bind_dascript::make_empty_SimpleLoc");

    using method_gamephysSimpleLocFromTM = DAS_CALL_MEMBER(gamephys::SimpleLoc::fromTM);
    das::addExtern<DAS_CALL_METHOD(method_gamephysSimpleLocFromTM)>(*this, lib, "fromTM", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(gamephys::SimpleLoc::fromTM));

    using method_gamephysOrientSetQuat = DAS_CALL_MEMBER(gamephys::Orient::setQuat);
    das::addExtern<DAS_CALL_METHOD(method_gamephysOrientSetQuat)>(*this, lib, "orient_setQuat", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(gamephys::Orient::setQuat));

    using method_gamephysMassSetFuel = DAS_CALL_MEMBER(gamephys::Mass::setFuel);
    das::addExtern<DAS_CALL_METHOD(method_gamephysMassSetFuel)>(*this, lib, "setFuel", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(gamephys::Mass::setFuel));

    using method_gamephysMassHasFuel = DAS_CALL_MEMBER(gamephys::Mass::hasFuel);
    das::addExtern<DAS_CALL_METHOD(method_gamephysMassHasFuel)>(*this, lib, "hasFuel", das::SideEffects::accessExternal,
      DAS_CALL_MEMBER_CPP(gamephys::Mass::hasFuel));

    using method_gamephysMassGetFuelMassCurrent =
      das::das_call_member<float (gamephys::Mass::*)(int) const, &gamephys::Mass::getFuelMassCurrent>;
    das::addExtern<DAS_CALL_METHOD(method_gamephysMassGetFuelMassCurrent)>(*this, lib, "getFuelMassCurrent",
      das::SideEffects::accessExternal,
      "das::das_call_member< float(gamephys::Mass::*)(int) const, &gamephys::Mass::getFuelMassCurrent >::invoke");

    using method_gamephysMassGetFuelAllMassCurrent =
      das::das_call_member<float (gamephys::Mass::*)() const, &gamephys::Mass::getFuelMassCurrent>;
    das::addExtern<DAS_CALL_METHOD(method_gamephysMassGetFuelAllMassCurrent)>(*this, lib, "getFuelMassCurrent",
      das::SideEffects::accessExternal,
      "das::das_call_member< float(gamephys::Mass::*)() const, &gamephys::Mass::getFuelMassCurrent >::invoke");

    using method_gamephysMassGetFuelMassMax =
      das::das_call_member<float (gamephys::Mass::*)(int) const, &gamephys::Mass::getFuelMassMax>;
    das::addExtern<DAS_CALL_METHOD(method_gamephysMassGetFuelMassMax)>(*this, lib, "getFuelMassMax", das::SideEffects::accessExternal,
      "das::das_call_member< float(gamephys::Mass::*)(int) const, &gamephys::Mass::getFuelMassMax >::invoke");

    using method_gamephysMassGetFuelAllMassMax =
      das::das_call_member<float (gamephys::Mass::*)() const, &gamephys::Mass::getFuelMassMax>;
    das::addExtern<DAS_CALL_METHOD(method_gamephysMassGetFuelAllMassMax)>(*this, lib, "getFuelMassMax",
      das::SideEffects::accessExternal,
      "das::das_call_member< float(gamephys::Mass::*)() const, &gamephys::Mass::getFuelMassMax >::invoke");


    {
      using method_init = DAS_CALL_MEMBER(ECSCustomPhysStateSyncer::init);
      das::addExtern<DAS_CALL_METHOD(method_init)>(*this, lib, "init", das::SideEffects::modifyArgument,
        DAS_CALL_MEMBER_CPP(ECSCustomPhysStateSyncer::init))
        ->arg_init(/*resv*/ 1, das::make_smart<das::ExprConstInt>(0));

      using method_registerSyncComponentFloat = das::das_call_member<void (ECSCustomPhysStateSyncer::*)(const char *, float &),
        &ECSCustomPhysStateSyncer::registerSyncComponent>;
      das::addExtern<DAS_CALL_METHOD(method_registerSyncComponentFloat)>(*this, lib, "registerSyncComponent",
        das::SideEffects::modifyArgument,
        "das::das_call_member< void (ECSCustomPhysStateSyncer::*)(const char*, float&), "
        "&ECSCustomPhysStateSyncer::registerSyncComponent >::invoke");

      using method_registerSyncComponentInt = das::das_call_member<void (ECSCustomPhysStateSyncer::*)(const char *, int &),
        &ECSCustomPhysStateSyncer::registerSyncComponent>;
      das::addExtern<DAS_CALL_METHOD(method_registerSyncComponentInt)>(*this, lib, "registerSyncComponent",
        das::SideEffects::modifyArgument,
        "das::das_call_member< void (ECSCustomPhysStateSyncer::*)(const char*, int&), "
        "&ECSCustomPhysStateSyncer::registerSyncComponent >::invoke");

      using method_registerSyncComponentBool = das::das_call_member<void (ECSCustomPhysStateSyncer::*)(const char *, bool &),
        &ECSCustomPhysStateSyncer::registerSyncComponent>;
      das::addExtern<DAS_CALL_METHOD(method_registerSyncComponentBool)>(*this, lib, "registerSyncComponent",
        das::SideEffects::modifyArgument,
        "das::das_call_member< void (ECSCustomPhysStateSyncer::*)(const char*, bool&), "
        "&ECSCustomPhysStateSyncer::registerSyncComponent >::invoke");
    }

    das::addConstant(*this, "MAIN_FUEL_SYSTEM", (int)gamephys::FuelTank::MAIN_SYSTEM);
    das::addConstant(*this, "MAX_FUEL_SYSTEMS", (int)gamephys::FuelTank::MAX_SYSTEMS);
    das::addConstant(*this, "MAX_FUEL_TANKS", (int)gamephys::FuelTank::MAX_TANKS);
    das::addConstant(*this, "PHYSICS_UPDATE_FIXED_DT", (float)gamephys::PHYSICS_UPDATE_FIXED_DT);

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotGamePhys.h>\n";
    tw << "#include <ecs/phys/netPhysResync.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(GamePhysModule, bind_dascript);
