// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotCurrentCircuit.h>


namespace bind_dascript
{
class CurrentCircuitModule final : public das::Module
{
public:
  CurrentCircuitModule() : das::Module("CurrentCircuit")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_circuit_name)>(*this, lib, "get_circuit_name", das::SideEffects::accessExternal,
      "bind_dascript::get_circuit_name");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotCurrentCircuit.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(CurrentCircuitModule, bind_dascript);
