// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "localization.h"

#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>

namespace bind_dascript
{
class LocalizationModule final : public das::Module
{
public:
  LocalizationModule() : das::Module("Localization")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_localized_text)>(*this, lib, "get_localized_text", das::SideEffects::accessExternal,
      "bind_dascript::get_localized_text");

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_localized_text_silent)>(*this, lib, "get_localized_text_silent",
      das::SideEffects::accessExternal, "bind_dascript::get_localized_text_silent");

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"localization/dasModules/localization.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(LocalizationModule, bind_dascript);
