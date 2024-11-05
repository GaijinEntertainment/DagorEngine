// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasModulesCommon.h>
#include <render/clipmapDecals.h>

namespace bind_dascript
{
class ClipmapDecalsModule final : public das::Module
{
public:
  ClipmapDecalsModule() : das::Module("ClipmapDecals")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(clipmap_decals_mgr::getDecalTypeIdByName)>(*this, lib, "get_clipmap_decal_id_by_name",
      das::SideEffects::accessExternal, "clipmap_decals_mgr::getDecalTypeIdByName");
    das::addExtern<DAS_BIND_FUN(clipmap_decals_mgr::createDecal)>(*this, lib, "create_clipmap_decal", das::SideEffects::accessExternal,
      "clipmap_decals_mgr::createDecal");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <render/clipmapDecals.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(ClipmapDecalsModule, bind_dascript);
