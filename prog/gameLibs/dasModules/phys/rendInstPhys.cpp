// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotRendInstPhys.h>

namespace bind_dascript
{
class RendInstPhysModule final : public das::Module
{
public:
  RendInstPhysModule() : das::Module("RendInstPhys")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("RendInst"));

    das::addExtern<DAS_BIND_FUN(dacoll::check_ri_collision_filtered)>(*this, lib, "check_ri_collision_filtered",
      das::SideEffects::accessExternal, "dacoll::check_ri_collision_filtered");

    das::addExtern<DAS_BIND_FUN(move_ri_extra_tm)>(*this, lib, "move_ri_extra_tm", das::SideEffects::modifyExternal,
      "move_ri_extra_tm");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotRendInstPhys.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(RendInstPhysModule, bind_dascript);
