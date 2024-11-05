// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "staticFlares.h"


namespace bind_dascript
{
class StaticFlaresModule final : public das::Module
{
public:
  StaticFlaresModule() : das::Module("StaticFlares")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(add_static_flare)>(*this, lib, "add_static_flare", das::SideEffects::modifyExternal,
      "add_static_flare");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <static_flares/dasModules/staticFlares.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
AUTO_REGISTER_MODULE_IN_NAMESPACE(StaticFlaresModule, bind_dascript);
size_t StaticFlaresModule_pull = 0;