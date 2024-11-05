// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <daECS/net/dasEvents.h>
#include <dasModules/dasEvent.h>
#include <dasModules/dasModulesCommon.h>
#include "auth.h"
#include <ecs/core/entityManager.h>

namespace bind_dascript
{
class AuthModule final : public das::Module
{
public:
  AuthModule() : das::Module("auth")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_country_code)>(*this, lib, "get_country_code", das::SideEffects::accessExternal,
      "bind_dascript::get_country_code");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/auth.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(AuthModule, bind_dascript);
