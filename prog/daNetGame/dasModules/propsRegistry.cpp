// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/propsRegistry.h"

namespace bind_dascript
{
class PropsRegistryModule final : public das::Module
{
public:
  PropsRegistryModule() : das::Module("PropsRegistry")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(register_props)>(*this, lib, "register_props", das::SideEffects::modifyExternal,
      "::bind_dascript::register_props");
    das::addExtern<DAS_BIND_FUN(get_props_id)>(*this, lib, "get_props_id", das::SideEffects::accessExternal,
      "::bind_dascript::get_props_id");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/propsRegistry.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PropsRegistryModule, bind_dascript);