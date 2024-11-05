// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotGpuReadbackQuery.h>
#include "heightmapQueryManager.h"

namespace bind_dascript
{

struct HeightmapQueryResultAnnotation : das::ManagedStructureAnnotation<HeightmapQueryResult, false>
{
  HeightmapQueryResultAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HeightmapQueryResult", ml)
  {
    cppName = " ::HeightmapQueryResult";
    addField<DAS_BIND_MANAGED_FIELD(heightNoOffset)>("heightNoOffset");
    addField<DAS_BIND_MANAGED_FIELD(heightWithOffset)>("heightWithOffset");
    addField<DAS_BIND_MANAGED_FIELD(heightWithOffsetDeform)>("heightWithOffsetDeform");
  }
};


class HeightmapQueryManagerModule final : public das::Module
{
public:
  HeightmapQueryManagerModule() : das::Module("HeightmapQueryManager")
  {
    das::ModuleLibrary lib;
    lib.addModule(this);
    lib.addModule(require("gpuReadbackQuery"));

    addAnnotation(das::make_smart<HeightmapQueryResultAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(heightmap_query_start)>(*this, lib, "heightmap_query_start", das::SideEffects::modifyExternal,
      "bind_dascript::heightmap_query_start");

    das::addExtern<DAS_BIND_FUN(heightmap_query_value)>(*this, lib, "heightmap_query_value",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::heightmap_query_value");

    das::addCtorAndUsing<HeightmapQueryResult>(*this, lib, "HeightmapQueryResult", "::HeightmapQueryResult");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotGpuReadbackQuery.h>\n";
    tw << "#include \"render/dasModules/heightmapQueryManager.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(HeightmapQueryManagerModule, bind_dascript);