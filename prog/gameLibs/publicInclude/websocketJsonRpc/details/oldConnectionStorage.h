//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/wsJsonRpcConnection.h>

#include <EASTL/deque.h>
#include <EASTL/string.h>


namespace websocketjsonrpc::details
{

// This class is not thread safe.
// But it allows reentrance from application callbacks (except calling destructor).
class OldConnectionStorage
{
public:
  explicit OldConnectionStorage(eastl::string_view log_prefix);

  void addDrainingConnection(const WsJsonRpcConnectionPtr &connection);
  void addClosingConnection(const WsJsonRpcConnectionPtr &connection);

  // Application callbacks may be called here.
  void poll();

  void initiateClose();

  bool empty() const { return drainingConnections.empty() && closingConnections.empty(); }

  void hardReset()
  {
    drainingConnections.clear();
    closingConnections.clear();
  }

private:
  void pollAllConnections();
  void processOutdatedConnections();
  void removeNullPointers();

private:
  const eastl::string logPrefix;

  // This connections need some time to wait for responses after graceful WebSocket reconnect
  eastl::deque<WsJsonRpcConnectionPtr> drainingConnections;

  // These connections were called initiateClose() and need some time for polling during graceful WebSocket shutdown
  eastl::deque<WsJsonRpcConnectionPtr> closingConnections;
};

} // namespace websocketjsonrpc::details
