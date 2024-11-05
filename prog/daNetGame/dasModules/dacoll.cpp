// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <phys/collRes.h>

namespace bind_dascript
{
class DngDacollModule final : public das::Module
{
public:
  DngDacollModule() : das::Module("DngDacoll")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("Dacoll"));

    das::addExtern<DAS_BIND_FUN(get_collres_body_tm)>(*this, lib, "get_collres_body_tm", das::SideEffects::accessExternal,
      "get_collres_body_tm");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <ecs/scripts/dasEcsEntity.h>\n";
    tw << "#include <phys/collRes.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngDacollModule, bind_dascript);
