// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include "dasModules/websocket/webSocket.h"
#include <daScript/src/builtin/module_builtin_rtti.h>


struct WebSocketServerAnnotation : das::ManagedStructureAnnotation<bind_websocket::WebSocketServer>
{
  WebSocketServerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("WebSocketServer", ml) {}
};

static char webSocket_net_das[] =
#include "webSocket.das.inl"
  ;

namespace bind_dascript
{
class WebSocketModule final : public das::Module
{
public:
  WebSocketModule() : das::Module("webSocket")
  {
    das::ModuleLibrary lib(this);
    lib.addBuiltInModule();
    addBuiltinDependency(lib, Module::require("rtti"));
    // sever
    addAnnotation(das::make_smart<WebSocketServerAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(make_websocket)>(*this, lib, "make_websocket", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::make_websocket");
    das::addExtern<DAS_BIND_FUN(websocket_init)>(*this, lib, "websocket_init", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::websocket_init");
    das::addExtern<DAS_BIND_FUN(websocket_init_path)>(*this, lib, "websocket_init_path", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::websocket_init_path");
    das::addExtern<DAS_BIND_FUN(websocket_is_open)>(*this, lib, "websocket_is_open", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::websocket_is_open");
    das::addExtern<DAS_BIND_FUN(websocket_is_connected)>(*this, lib, "websocket_is_connected",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::websocket_is_connected");
    das::addExtern<DAS_BIND_FUN(websocket_tick)>(*this, lib, "websocket_tick", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::websocket_tick");
    das::addExtern<DAS_BIND_FUN(websocket_send)>(*this, lib, "websocket_send", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::websocket_send");
    das::addExtern<DAS_BIND_FUN(websocket_restore)>(*this, lib, "websocket_restore", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::websocket_restore");

    compileBuiltinModule("webSocket.das", (unsigned char *)webSocket_net_das, sizeof(webSocket_net_das));
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/capsuleApproximation.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(WebSocketModule, bind_dascript);
