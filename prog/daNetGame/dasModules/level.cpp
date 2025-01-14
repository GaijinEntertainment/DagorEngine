// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <levelSplines/splineRegions.h>
#include <levelSplines/splineRoads.h>
#include "main/levelRoads.h"
#include "dasModules/level.h"

extern const char *get_region_name_by_pos(const Point2 &);

struct SplineRegionAnnotation : das::ManagedStructureAnnotation<splineregions::SplineRegion, false>
{
  SplineRegionAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SplineRegion", ml)
  {
    cppName = " ::splineregions::SplineRegion";
    addField<DAS_BIND_MANAGED_FIELD(border)>("border");
    addField<DAS_BIND_MANAGED_FIELD(originalBorder)>("originalBorder");
    addField<DAS_BIND_MANAGED_FIELD(area)>("area");
    addField<DAS_BIND_MANAGED_FIELD(boundingRadius)>("boundingRadius");
    addField<DAS_BIND_MANAGED_FIELD(center)>("center");
    addField<DAS_BIND_MANAGED_FIELD(triangulationIndexes)>("triangulationIndexes");
    addField<DAS_BIND_MANAGED_FIELD(isVisible)>("isVisible");
    addProperty<DAS_BIND_MANAGED_PROP(getAnyBorderPoint)>("anyBorderPoint", "getAnyBorderPoint");
    addProperty<DAS_BIND_MANAGED_PROP(getRandomBorderPoint)>("randomBorderPoint", "getRandomBorderPoint");
    addProperty<DAS_BIND_MANAGED_PROP(getRandomPoint)>("randomPoint", "getRandomPoint");
    addProperty<DAS_BIND_MANAGED_PROP(getNameStr)>("name", "getNameStr");
  }
};

namespace bind_dascript
{
class DngLevelModule final : public das::Module
{
public:
  DngLevelModule() : das::Module("level")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DagorMath"));
    addAnnotation(das::make_smart<SplineRegionAnnotation>(lib));
    das::typeFactory<LevelRegions>::make(lib);
    das::addExtern<DAS_BIND_FUN(get_region_name_by_pos)>(*this, lib, "get_region_name_by_pos", das::SideEffects::accessExternal,
      "::get_region_name_by_pos");
    das::addExtern<DAS_BIND_FUN(das_spline_region_checkPoint)>(*this, lib, "spline_region_checkPoint",
      das::SideEffects::accessExternal, "::bind_dascript::das_spline_region_checkPoint");
    das::addExtern<DAS_BIND_FUN(calculate_server_sun_dir)>(*this, lib, "calculate_server_sun_dir", das::SideEffects::accessExternal,
      "calculate_server_sun_dir");
    das::addExtern<DAS_BIND_FUN(is_level_loading)>(*this, lib, "is_level_loading", das::SideEffects::accessExternal,
      "is_level_loading");
    das::addExtern<DAS_BIND_FUN(is_level_loaded)>(*this, lib, "is_level_loaded", das::SideEffects::accessExternal,
      "::is_level_loaded");
    das::addExtern<DAS_BIND_FUN(is_level_loaded_not_empty)>(*this, lib, "is_level_loaded_not_empty", das::SideEffects::accessExternal,
      "::is_level_loaded_not_empty");
    das::addExtern<DAS_BIND_FUN(is_level_unloading)>(*this, lib, "is_level_unloading", das::SideEffects::accessExternal,
      "is_level_unloading");
    das::addExtern<DAS_BIND_FUN(get_current_level_eid)>(*this, lib, "get_current_level_eid", das::SideEffects::accessExternal,
      "get_current_level_eid");
    das::addExtern<DAS_BIND_FUN(select_weather_preset)>(*this, lib, "select_weather_preset", das::SideEffects::modifyExternal,
      "select_weather_preset");
    das::addExtern<DAS_BIND_FUN(das_get_points_on_road_splines)>(*this, lib, "get_points_on_road_splines",
      das::SideEffects::accessExternal, "::bind_dascript::das_get_points_on_road_splines");
    das::addExtern<DAS_BIND_FUN(das_add_region_to_list)>(*this, lib, "add_region_to_list", das::SideEffects::modifyArgument,
      "::bind_dascript::das_add_region_to_list");
    das::addExtern<DAS_BIND_FUN(das_remove_region_from_list)>(*this, lib, "remove_region_from_list", das::SideEffects::modifyArgument,
      "::bind_dascript::das_remove_region_from_list");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <ecs/game/zones/levelRegions.h>\n";
    tw << "#include \"main/levelRoads.h\"\n";
    tw << "#include \"dasModules/level.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngLevelModule, bind_dascript);
