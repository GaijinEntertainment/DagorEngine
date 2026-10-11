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

    das::addExtern<DAS_BIND_FUN(is_hosted_internal_server_active)>(*this, lib, "is_hosted_internal_server_active",
      das::SideEffects::accessExternal, "bind_dascript::is_hosted_internal_server_active");
    das::addExtern<DAS_BIND_FUN(get_hosted_internal_server_uid)>(*this, lib, "get_hosted_internal_server_uid",
      das::SideEffects::accessExternal, "bind_dascript::get_hosted_internal_server_uid");
    das::addExtern<DAS_BIND_FUN(allocate_hosted_server_uid)>(*this, lib, "allocate_hosted_server_uid",
      das::SideEffects::modifyExternal, "bind_dascript::allocate_hosted_server_uid");
    das::addExtern<DAS_BIND_FUN(set_hosted_server_start_uid)>(*this, lib, "set_hosted_server_start_uid",
      das::SideEffects::modifyExternal, "bind_dascript::set_hosted_server_start_uid");
    das::addExtern<DAS_BIND_FUN(is_main_thread_network)>(*this, lib, "is_main_thread_network", das::SideEffects::accessExternal,
      "bind_dascript::is_main_thread_network");
    das::addExtern<DAS_BIND_FUN(request_start_hosted_server)>(*this, lib, "request_start_hosted_server",
      das::SideEffects::modifyExternal, "bind_dascript::request_start_hosted_server");
    das::addExtern<DAS_BIND_FUN(request_stop_hosted_server)>(*this, lib, "request_stop_hosted_server",
      das::SideEffects::modifyExternal, "bind_dascript::request_stop_hosted_server");
    das::addExtern<DAS_BIND_FUN(for_each_work_status)>(*this, lib, "for_each_work_status", das::SideEffects::invokeAndAccessExternal,
      "bind_dascript::for_each_work_status")
      ->args({"block", "context", "at"});

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
