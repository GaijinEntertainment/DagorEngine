// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/dngHostedServer.h"


namespace bind_dascript
{

class DngHostedServerModule final : public das::Module
{
public:
  DngHostedServerModule() : das::Module("DngHostedServer")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));

    das::addExtern<DAS_BIND_FUN(signal_hosted_server_ready)>(*this, lib, "signal_hosted_server_ready",
      das::SideEffects::modifyExternal, "bind_dascript::signal_hosted_server_ready");
    das::addExtern<DAS_BIND_FUN(disable_auto_hosted_server_ready)>(*this, lib, "disable_auto_hosted_server_ready",
      das::SideEffects::modifyExternal, "bind_dascript::disable_auto_hosted_server_ready");
    das::addExtern<DAS_BIND_FUN(is_hosted_internal_server_active)>(*this, lib, "is_hosted_internal_server_active",
      das::SideEffects::accessExternal, "bind_dascript::is_hosted_internal_server_active");
    das::addExtern<DAS_BIND_FUN(is_hosted_server_manual_ready_enabled)>(*this, lib, "is_hosted_server_manual_ready_enabled",
      das::SideEffects::accessExternal, "bind_dascript::is_hosted_server_manual_ready_enabled");
    das::addExtern<DAS_BIND_FUN(request_start_hosted_server)>(*this, lib, "request_start_hosted_server",
      das::SideEffects::modifyExternal, "bind_dascript::request_start_hosted_server");
    das::addExtern<DAS_BIND_FUN(request_stop_hosted_server)>(*this, lib, "request_stop_hosted_server",
      das::SideEffects::modifyExternal, "bind_dascript::request_stop_hosted_server");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/dngHostedServer.h\"\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DngHostedServerModule, bind_dascript);
