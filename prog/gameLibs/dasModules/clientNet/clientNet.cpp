#include <dasModules/aotClientNet.h>
#include <daScript/daScript.h>
#include <daECS/net/dasEvents.h>
#include <dasModules/dasEvent.h>
#include <dasModules/dasModulesCommon.h>

namespace bind_dascript
{

class ClientNetModule final : public das::Module
{
public:
  ClientNetModule() : das::Module("ClientNet")
  {
    das::ModuleLibrary lib(this);

    addEnumeration(das::make_smart<EnumerationEchoResponseResult>());

    das::addExtern<DAS_BIND_FUN(::get_current_server_route_id)>(*this, lib, "get_current_server_route_id",
      das::SideEffects::accessExternal, "::get_current_server_route_id");
    das::addExtern<DAS_BIND_FUN(::get_server_route_host)>(*this, lib, "get_server_route_host", das::SideEffects::accessExternal,
      "::get_server_route_host");
    das::addExtern<DAS_BIND_FUN(::get_server_route_count)>(*this, lib, "get_server_route_count", das::SideEffects::accessExternal,
      "::get_server_route_count");
    das::addExtern<DAS_BIND_FUN(::switch_server_route)>(*this, lib, "switch_server_route", das::SideEffects::modifyExternal,
      "::switch_server_route");
    das::addExtern<DAS_BIND_FUN(::send_echo_msg)>(*this, lib, "send_echo_msg", das::SideEffects::modifyExternal, "::send_echo_msg");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotClientNet.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(ClientNetModule, bind_dascript);
