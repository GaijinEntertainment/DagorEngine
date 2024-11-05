// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/netPropsRegistry.h"

namespace bind_dascript
{
class NetPropsRegistryModule final : public das::Module
{
public:
  NetPropsRegistryModule() : das::Module("NetPropsRegistry")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(register_net_props)>(*this, lib, "register_net_props", das::SideEffects::modifyExternal,
      "::bind_dascript::register_net_props");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/netPropsRegistry.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(NetPropsRegistryModule, bind_dascript);