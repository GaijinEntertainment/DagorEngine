// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>

struct mg_connection;
struct mg_context;

namespace bind_websocket
{
class WebSocketServer : public das::ptr_ref_count
{
public:
  struct Message
  {
    Message(const das::vector<uint8_t> &data, mg_connection *connection) : data(data), connection(connection) {}

    Message() = default;

    das::vector<uint8_t> data;
    mg_connection *connection = nullptr;
  };

public:
  WebSocketServer();
  WebSocketServer &operator=(WebSocketServer &&) = default;
  virtual ~WebSocketServer();

  void addConnection(mg_connection *conn);
  void removeConnection(mg_connection *conn);

  bool init(int port = 9000, const char *document_root = "");

  static void on_websocket_ready(mg_connection *conn);

  static int on_websocket_data(mg_connection *conn);

  bool is_open() const;

  bool is_connected() const;

  void tick();

  bool send_msg(char *data, int size);
  void ping();
  void pong(mg_connection *conn);

protected:
  virtual void onConnect() {}
  virtual void onDisconnect() {}
  virtual void onData(char *, int) {}
  virtual void onError(const char *, int) {}

protected:
  mg_context *mgCtx = nullptr;
  das::vector<mg_connection *> connections;
  das::vector<Message> messages;
  time_t nextPing = 0;
};
} // namespace bind_websocket
