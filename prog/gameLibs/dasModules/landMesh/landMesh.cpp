// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotLandMesh.h>
#include <dasModules/aotGpuReadbackQuery.h>


namespace bind_dascript
{
struct BiomeQueryResultAnnotation final : das::ManagedStructureAnnotation<BiomeQueryResult, false>
{
  BiomeQueryResultAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BiomeQueryResult", ml)
  {
    cppName = " ::BiomeQueryResult";
    addField<DAS_BIND_MANAGED_FIELD(mostFrequentBiomeGroupIndex)>("mostFrequentBiomeGroupIndex");
    addField<DAS_BIND_MANAGED_FIELD(mostFrequentBiomeGroupWeight)>("mostFrequentBiomeGroupWeight");
    addField<DAS_BIND_MANAGED_FIELD(secondMostFrequentBiomeGroupIndex)>("secondMostFrequentBiomeGroupIndex");
    addField<DAS_BIND_MANAGED_FIELD(secondMostFrequentBiomeGroupWeight)>("secondMostFrequentBiomeGroupWeight");
  }
  bool isLocal() const override { return true; } // force isLocal, because ctor is non trivial
};

class LandMeshModule final : public das::Module
{
public:
  LandMeshModule() : das::Module("landMesh")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("gpuReadbackQuery"));

    addAnnotation(das::make_smart<BiomeQueryResultAnnotation>(lib));

    das::addCtor<BiomeQueryResult>(*this, lib, "BiomeQueryResult", "::BiomeQueryResult");

    das::addExtern<DAS_BIND_FUN(biome_query_start)>(*this, lib, "biome_query_start", das::SideEffects::modifyExternal,
      "bind_dascript::biome_query_start");

    das::addExtern<GpuReadbackResultState (*)(int, BiomeQueryResult &), &biome_query::get_query_result>(*this, lib,
      "biome_query_result", das::SideEffects::modifyArgumentAndExternal, "::biome_query::get_query_result");

    das::addExtern<int (*)(const char *), &biome_query::get_biome_group_id>(*this, lib, "get_biome_group_id",
      das::SideEffects::accessExternal, "::biome_query::get_biome_group_id");

    das::addExtern<const char *(*)(int), &biome_query::get_biome_group_name>(*this, lib, "get_biome_group_name",
      das::SideEffects::accessExternal, "::biome_query::get_biome_group_name");

    das::addExtern<int (*)(), &biome_query::get_num_biome_groups>(*this, lib, "get_num_biome_groups", das::SideEffects::accessExternal,
      "::biome_query::get_num_biome_groups");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotGpuReadbackQuery.h>\n";
    tw << "#include <dasModules/aotLandMesh.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(LandMeshModule, bind_dascript);
