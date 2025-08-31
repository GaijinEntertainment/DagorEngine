// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasModulesCommon.h>
#include <render/renderLibsAllowed.h>

namespace bind_dascript
{
class RenderLibsAllowedModule final : public das::Module
{
public:
  RenderLibsAllowedModule() : das::Module("RenderLibsAllowed")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(is_render_lib_allowed)>(*this, lib, "is_render_lib_allowed", das::SideEffects::accessExternal,
      "is_render_lib_allowed");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <render/renderLibsAllowed.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(RenderLibsAllowedModule, bind_dascript);
