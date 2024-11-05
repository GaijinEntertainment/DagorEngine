// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/ui.h"
#include "dasModules/aotDagorMath.h"

namespace bind_dascript
{
class DngUIModule final : public das::Module
{
public:
  DngUIModule() : das::Module("DngUI")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(get_ui_view_tm)>(*this, lib, "get_ui_view_tm", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::get_ui_view_tm");
    das::addExtern<DAS_BIND_FUN(get_ui_view_itm)>(*this, lib, "get_ui_view_itm", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::get_ui_view_itm");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/ui.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngUIModule, bind_dascript);
