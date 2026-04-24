// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/time.h>
#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>

namespace bind_dascript
{

class AppTimeModule final : public das::Module
{
public:
  AppTimeModule() : das::Module("AppTime")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(::get_sync_time)>(*this, lib, "get_sync_time", das::SideEffects::accessExternal, "::get_sync_time");
    das::addExtern<DAS_BIND_FUN(::get_sync_time_d)>(*this, lib, "get_sync_time_d", das::SideEffects::accessExternal,
      "::get_sync_time_d");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <daECS/net/time.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(AppTimeModule, bind_dascript);
