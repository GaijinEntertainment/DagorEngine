// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/dasModules/daSkies.h"
#include <main/weatherPreset.h>
#include <shaders/light_consts.hlsli>

struct DngSkiesAnnotation : das::ManagedStructureAnnotation<DngSkies, false>
{
  DngSkiesAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DngSkies", ml)
  {
    cppName = " ::DngSkies";
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getStrataClouds)>("strataClouds", "getStrataClouds");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getSunDir)>("sunDir", "getSunDir");
  }
};

struct StrataCloudsAnnotation : das::ManagedStructureAnnotation<DaSkies::StrataClouds, false>
{
  StrataCloudsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("StrataClouds", ml)
  {
    cppName = " ::DaSkies::StrataClouds";
    addField<DAS_BIND_MANAGED_FIELD(amount)>("amount");
    addField<DAS_BIND_MANAGED_FIELD(altitude)>("altitude");
  }
};

static char daSkies_das[] =
#include "daSkies.das.inl"
  ;

namespace bind_dascript
{
class DaSkiesModule final : public das::Module
{
public:
  DaSkiesModule() : das::Module("daSkies")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorDataBlock"));
    addBuiltinDependency(lib, require("DagorDataBlock"));
    addAnnotation(das::make_smart<StrataCloudsAnnotation>(lib));
    addAnnotation(das::make_smart<DngSkiesAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(get_daskies_time)>(*this, lib, "get_daskies_time", das::SideEffects::accessExternal,
      "get_daskies_time");
    das::addExtern<DAS_BIND_FUN(set_daskies_time)>(*this, lib, "set_daskies_time", das::SideEffects::modifyExternal,
      "set_daskies_time");
    das::addConstant(*this, "NIGHT_SUN_COS", NIGHT_SUN_COS);
    das::addExtern<DAS_BIND_FUN(move_cumulus_clouds)>(*this, lib, "move_cumulus_clouds", das::SideEffects::modifyExternal,
      "move_cumulus_clouds");
    das::addExtern<DAS_BIND_FUN(move_strata_clouds)>(*this, lib, "move_strata_clouds", das::SideEffects::modifyExternal,
      "move_strata_clouds");
    das::addExtern<DAS_BIND_FUN(get_daskies)>(*this, lib, "get_daskies", das::SideEffects::accessExternal, "get_daskies");
    das::addExtern<DAS_BIND_FUN(load_daSkies)>(*this, lib, "load_daSkies", das::SideEffects::modifyArgument,
      "bind_dascript::load_daSkies");
    das::addExtern<DAS_BIND_FUN(daskies_getCloudsStartAltSettings)>(*this, lib, "daskies_getCloudsStartAltSettings",
      das::SideEffects::none, "bind_dascript::daskies_getCloudsStartAltSettings");
    das::addExtern<DAS_BIND_FUN(daskies_getCloudsTopAltSettings)>(*this, lib, "daskies_getCloudsTopAltSettings",
      das::SideEffects::none, "bind_dascript::daskies_getCloudsTopAltSettings");
    das::addExtern<DAS_BIND_FUN(daskies_getStarsJulianDay)>(*this, lib, "daskies_getStarsJulianDay", das::SideEffects::none,
      "bind_dascript::daskies_getStarsJulianDay");
    das::addExtern<DAS_BIND_FUN(daskies_setStarsJulianDay)>(*this, lib, "daskies_setStarsJulianDay", das::SideEffects::modifyArgument,
      "bind_dascript::daskies_setStarsJulianDay");
    das::addExtern<DAS_BIND_FUN(daskies_currentGroundSunSkyColor)>(*this, lib, "daskies_currentGroundSunSkyColor",
      das::SideEffects::modifyArgument, "bind_dascript::daskies_currentGroundSunSkyColor");
    das::addExtern<DAS_BIND_FUN(select_weather_preset_delayed)>(*this, lib, "select_weather_preset_delayed",
      das::SideEffects::modifyExternal, "select_weather_preset_delayed");
    das::addExtern<DAS_BIND_FUN(das_get_current_weather_preset_name)>(*this, lib, "get_current_weather_preset_name",
      das::SideEffects::accessExternal, "::bind_dascript::das_get_current_weather_preset_name");
    compileBuiltinModule("daSkies.das", (unsigned char *)daSkies_das, sizeof(daSkies_das));
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"render/dasModules/daSkies.h\"\n";
    tw << "#include \"main/weatherPreset.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DaSkiesModule, bind_dascript);
