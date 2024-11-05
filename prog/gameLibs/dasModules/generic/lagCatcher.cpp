// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>

#include <lagCatcher/lagCatcher.h>

namespace bind_dascript
{
class LagCatcherModule final : public das::Module
{
public:
  LagCatcherModule() : das::Module("lagcatcher")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(lagcatcher::stop)>(*this, lib, "lagcatcher_stop", das::SideEffects::modifyExternal,
      "lagcatcher::stop");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"lagCatcher/lagCatcher.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(LagCatcherModule, bind_dascript);
