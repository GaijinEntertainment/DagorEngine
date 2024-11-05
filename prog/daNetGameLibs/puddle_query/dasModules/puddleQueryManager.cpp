// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotGpuReadbackQuery.h>
#include "puddleQueryManager.h"

namespace bind_dascript
{
class PuddleQueryManagerModule final : public das::Module
{
public:
  PuddleQueryManagerModule() : das::Module("puddleQueryManager")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("gpuReadbackQuery"));

    das::addExtern<DAS_BIND_FUN(puddle_query_start)>(*this, lib, "puddle_query_start", das::SideEffects::modifyExternal,
      "bind_dascript::puddle_query_start");

    das::addExtern<DAS_BIND_FUN(puddle_query_value)>(*this, lib, "puddle_query_value", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::puddle_query_value");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotGpuReadbackQuery.h>\n";
    tw << "#include <puddle_query/dasModules/puddleQueryManager.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(PuddleQueryManagerModule, bind_dascript);
