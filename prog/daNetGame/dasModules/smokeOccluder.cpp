// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>

#include <smokeOccluder/smokeOccluder.h>

namespace bind_dascript
{
class SmokeOccluderModule final : public das::Module
{
public:
  SmokeOccluderModule() : das::Module("SmokeOccluder")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(rayhit_smoke_occluders)>(*this, lib, "rayhit_smoke_occluders", das::SideEffects::accessExternal,
      "::rayhit_smoke_occluders");
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <smokeOccluder/smokeOccluder.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(SmokeOccluderModule, bind_dascript);