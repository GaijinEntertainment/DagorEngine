// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/dasModules/priorityManagedShadervar.h"

namespace bind_dascript
{
class PriorityShadervarModule final : public das::Module
{
public:
  PriorityShadervarModule() : das::Module("PriorityShadervar")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(priority_shadervar_set_float)>(*this, lib, "set_float", das::SideEffects::modifyExternal,
      "bind_dascript::priority_shadervar_set_float");
    das::addExtern<DAS_BIND_FUN(priority_shadervar_set_int)>(*this, lib, "set_int", das::SideEffects::modifyExternal,
      "bind_dascript::priority_shadervar_set_int");
    das::addExtern<DAS_BIND_FUN(priority_shadervar_set_float4)>(*this, lib, "set_float4", das::SideEffects::modifyExternal,
      "bind_dascript::priority_shadervar_set_float4");
    das::addExtern<DAS_BIND_FUN(priority_shadervar_set_int4)>(*this, lib, "set_int4", das::SideEffects::modifyExternal,
      "bind_dascript::priority_shadervar_set_int4");
    das::addExtern<DAS_BIND_FUN(priority_shadervar_clear)>(*this, lib, "clear", das::SideEffects::modifyExternal,
      "bind_dascript::priority_shadervar_clear");

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"render/dasModules/priorityManagedShadervar.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PriorityShadervarModule, bind_dascript);
