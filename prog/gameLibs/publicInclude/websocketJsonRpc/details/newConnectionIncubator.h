//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/details/reentranceProtection.h>
#include <websocketJsonRpc/wsJsonRpcConnection.h>


namespace websocketjsonrpc::details
{

// This class is not thread safe
class NewConnectionIncubator final
{
public:
  using ConnectionFactory =
    eastl::function<WsJsonRpcConnectionPtr(const WsJsonRpcConnectionPtr &optional_failed_connection, int connect_retry_number)>;

  using EventCallback = eastl::function<void(const WsJsonRpcConnectionPtr &failed_connection)>;

public:
  // All provided callbacks MUST not call object methods directly or indirectly.
  NewConnectionIncubator(ConnectionFactory &&connection_factory, const WsJsonRpcConnectionPtr &optional_failed_connection,
    eastl::string_view log_prefix, EventCallback &&on_initiate_connection, EventCallback &&on_connection_failure);

  // Returns non-null connection only once.
  // The `poll` must not be called after it returned non-null connection or after `cancel` was called.
  // Application callbacks may be called here.
  [[nodiscard]] WsJsonRpcConnectionPtr poll();

  // Extracts existing connection which can be nullptr or non-connected connection.
  WsJsonRpcConnectionPtr cancel();

private:
  void trySetupNewConnection();
  void internalPoll();

  WsJsonRpcConnectionPtr finishAndExtract();

private:
  const eastl::string logPrefix;
  const ConnectionFactory connectionFactory;
  const EventCallback onInitiateConnection;
  const EventCallback onConnectionFailure;

  details::ReentranceProtection reentranceProtection;
  bool finishedWork = false;

  bool previousFailedConnectionIsNew = true;
  WsJsonRpcConnectionPtr previousFailedConnection;
  WsJsonRpcConnectionPtr nextConnection;

  int connectRetryNumber = 0; // 0 means first try in a connect try sequence
};


using NewConnectionIncubatorPtr = eastl::shared_ptr<NewConnectionIncubator>;


} // namespace websocketjsonrpc::details
