// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include "dasModules/websocket/webSocketServer.h"

namespace bind_websocket
{

class WebSocketServerAdapter : public WebSocketServer
{
public:
  WebSocketServerAdapter(char *pClass, const das::StructInfo *info, das::Context *ctx) { update(pClass, info, ctx); }

  void update(char *pClass, const das::StructInfo *info, das::Context *ctx)
  {
    context = ctx;
    classPtr = pClass;
    pServer = (void **)adapt_field("_websocket", pClass, info);
    if (pServer)
      *pServer = this;
    fnOnConnect = adapt("onConnect", pClass, info);
    fnOnDisconnect = adapt("onDisconnect", pClass, info);
    fnOnData = adapt("onData", pClass, info);
    fnOnError = adapt("onError", pClass, info);
  }

  void onConnect() override
  {
    if (fnOnConnect)
    {
      return das::das_invoke_function<void>::invoke<void *>(context, nullptr, fnOnConnect, classPtr);
    }
  }

  void onDisconnect() override
  {
    if (fnOnDisconnect)
    {
      return das::das_invoke_function<void>::invoke<void *>(context, nullptr, fnOnDisconnect, classPtr);
    }
  }

  void onData(char *buf, int size) override
  {
    if (fnOnData)
    {
      return das::das_invoke_function<void>::invoke<void *, char *, int32_t>(context, nullptr, fnOnData, classPtr, buf, size);
    }
  }

  void onError(const char *msg, int code) override
  {
    if (fnOnError)
    {
      return das::das_invoke_function<void>::invoke<void *, const char *, int32_t>(context, nullptr, fnOnError, classPtr, msg, code);
    }
  }

  bool isValid() const { return pServer != nullptr; }

protected:
  void **pServer = nullptr;
  das::Func fnOnConnect;
  das::Func fnOnDisconnect;
  das::Func fnOnData;
  das::Func fnOnError;

protected:
  void *classPtr = nullptr;
  das::Context *context = nullptr;
};
} // namespace bind_websocket

MAKE_TYPE_FACTORY(WebSocketServer, bind_websocket::WebSocketServer)

namespace bind_dascript
{
inline bool make_websocket(const void *pClass, const das::StructInfo *info, das::Context *context)
{
  auto server = das::make_smart<bind_websocket::WebSocketServerAdapter>((char *)pClass, info, context);
  if (!server->isValid())
    return false;
  server.orphan();
  return true;
}

inline bool websocket_init(das::smart_ptr_raw<bind_websocket::WebSocketServer> server, int port, das::Context *context)
{
  if (!server)
    context->throw_error("null server");
  return server->init(port);
}

inline bool websocket_init_path(
  das::smart_ptr_raw<bind_websocket::WebSocketServer> server, int port, const char *serve_path, das::Context *context)
{
  if (!server)
    context->throw_error("null server");
  return server->init(port, serve_path);
}

inline bool websocket_is_open(das::smart_ptr_raw<bind_websocket::WebSocketServer> server, das::Context *context)
{
  if (!server)
    context->throw_error("null server");
  return server->is_open();
}

inline bool websocket_is_connected(das::smart_ptr_raw<bind_websocket::WebSocketServer> server, das::Context *context)
{
  if (!server)
    context->throw_error("null server");
  return server->is_connected();
}

inline bool websocket_send(
  das::smart_ptr_raw<bind_websocket::WebSocketServer> server, uint8_t *data, int32_t size, das::Context *context)
{
  if (!server)
    context->throw_error("null server");
  return server->send_msg((char *)data, size);
}

inline void websocket_tick(das::smart_ptr_raw<bind_websocket::WebSocketServer> server, das::Context *context)
{
  if (!server)
    context->throw_error("null server");
  server->tick();
}

inline void websocket_restore(
  das::smart_ptr_raw<bind_websocket::WebSocketServer> server, const void *pClass, const das::StructInfo *info, das::Context *context)
{
  if (!server)
    context->throw_error("null server");
  auto adapter = (bind_websocket::WebSocketServerAdapter *)server.get();
  adapter->update((char *)pClass, info, context);
}
} // namespace bind_dascript
