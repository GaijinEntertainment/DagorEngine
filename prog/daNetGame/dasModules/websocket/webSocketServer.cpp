// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/websocket/webSocketServer.h"
#include <util/dag_string.h>
#include <daScript/src/builtin/module_builtin_rtti.h>
#include <osApiWrappers/dag_critSec.h>
#include <mongoose/mongoose.h>

namespace bind_websocket
{
WinCritSec crit_sec;

WebSocketServer::WebSocketServer() {}

WebSocketServer::~WebSocketServer()
{
  if (mgCtx)
    mg_stop(mgCtx);
}

void WebSocketServer::addConnection(mg_connection *conn)
{
  for (auto &connection : connections)
    if (connection == conn)
      return;

  connections.push_back(conn);
}

void WebSocketServer::removeConnection(mg_connection *conn)
{
  for (auto connection = connections.begin(); connection != connections.end(); ++connection)
    if (*connection == conn)
    {
      connections.erase(connection);
      break;
    }
}

bool WebSocketServer::init(int port, const char *document_root)
{
  String portStr(0, "%d", port);
  const char *options[] = {"document_root", document_root, "listening_ports", portStr.c_str(), "num_threads", "4", nullptr};

  mg_callbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.websocket_ready = on_websocket_ready;
  callbacks.websocket_data = on_websocket_data;

  mgCtx = mg_start(&callbacks, this, options);
  return mgCtx != nullptr;
}

void WebSocketServer::on_websocket_ready(mg_connection *conn)
{
  WebSocketServer *self = (WebSocketServer *)mg_get_request_info(conn)->user_data;
  if (self)
  {
    self->addConnection(conn);
    self->onConnect();
  }
}

int WebSocketServer::on_websocket_data(mg_connection *conn)
{
  WebSocketServer *self = (WebSocketServer *)mg_get_request_info(conn)->user_data;
  if (!self)
    return 0;
  uint8_t body[1025];
  const int len = mg_read(conn, body, 1024);

  if (len < 6)
    return 0;

  const int length = body[1] & 127;
  const int maskIndex = length == 126 ? 4 : length == 127 ? 10 : 2;
  const int bodyStart = maskIndex + 4;
  for (int i = bodyStart, j = 0; i < len; ++i, ++j)
    body[i] ^= body[maskIndex + (j % 4)];

  if (len - bodyStart == 2 && body[bodyStart] == 0x03 && body[bodyStart + 1] == 0xe9)
  {
    self->removeConnection(conn);
    return 0;
  }
  if (len - bodyStart == 8 && strncmp((char *)(body + bodyStart), "__ping__", 8) == 0)
  {
    self->pong(conn);
    return 1;
  }
  body[len] = 0;
  {
    WinAutoLock lock(crit_sec);
    das::vector<uint8_t> data(body + bodyStart, body + len);
    self->messages.emplace_back(eastl::move(data), conn);
  }
  return 1;
}

bool WebSocketServer::is_open() const { return mgCtx != nullptr; }

bool WebSocketServer::is_connected() const { return is_open() && !connections.empty(); }

void WebSocketServer::tick()
{
  {
    WinAutoLock lock(crit_sec);
    for (auto &msg : messages)
    {
      onData((char *)msg.data.data(), msg.data.size());
    }
    messages.clear();
  }

  const time_t now = ::time(nullptr);
  if (now > nextPing)
  {
    ping();
    nextPing = now + 25;
  }
  mg_process_requests(mgCtx);
}

bool WebSocketServer::send_msg(char *data, int size)
{
  for (auto &connection : connections)
    mg_websocket_write(connection, WEBSOCKET_OPCODE_TEXT, data, size);
  return !connections.empty();
}

void WebSocketServer::ping()
{
  for (auto &connection : connections)
    mg_websocket_write(connection, WEBSOCKET_OPCODE_PING, "__ping__", 8);
}

void WebSocketServer::pong(mg_connection *conn) { mg_websocket_write(conn, WEBSOCKET_OPCODE_PONG, "__pong__", 8); }

} // namespace bind_websocket
