// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>

#include <billboardDecals/billboardDecals.h>
#include <billboard_decals/render/billboardDecals.h>

namespace bind_dascript
{
class BillboardDecalsModule final : public das::Module
{
public:
  BillboardDecalsModule() : das::Module("BillboardDecals")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(add_billboard_decal)>(*this, lib, "add_billboard_decal", das::SideEffects::modifyExternal,
      "add_billboard_decal");
    das::addExtern<DAS_BIND_FUN(erase_billboard_decals)>(*this, lib, "erase_billboard_decals", das::SideEffects::modifyExternal,
      "erase_billboard_decals");
    das::addExtern<DAS_BIND_FUN(erase_all_billboard_decals)>(*this, lib, "erase_all_billboard_decals",
      das::SideEffects::modifyExternal, "erase_all_billboard_decals");
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <billboard_decals/render/billboardDecals.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
AUTO_REGISTER_MODULE_IN_NAMESPACE(BillboardDecalsModule, bind_dascript);

size_t BillboardDecalsModule_pull = 0;
