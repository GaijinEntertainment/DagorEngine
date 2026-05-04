//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/details/commonTypes.h>
#include <websocketJsonRpc/details/newConnectionIncubator.h>
#include <websocketJsonRpc/details/oldConnectionStorage.h>
#include <websocketJsonRpc/details/reentranceProtection.h>
#include <websocketJsonRpc/wsJsonRpcConnection.h>
#include <websocketJsonRpc/wsJsonRpcStats.h>


namespace websocketjsonrpc
{

class RpcRequest;


// This class is not thread safe
//
// Application-provided callbacks are called from `poll()` and `rpcCall` methods only.
// All provided callbacks MUST not call object methods directly or indirectly.
//
// Object may be safely deleted before first `poll()` call.
// Graceful deletion after first `poll()` call needs `isCloseCompleted() == true`
// otherwise connection would be closed abruptly and errors may be logged.
//
class WsJsonRpcPersistentConnection : public eastl::enable_shared_from_this<WsJsonRpcPersistentConnection>
{
private:
  class PrivateConstructorTag final
  {};

public:
  using IncomingRequestCallback = eastl::function<void(RpcRequestPtr &&incoming_request)>;
  using ConnectionConfigGeneratorCallback =
    eastl::function<WsJsonRpcConnectionConfigPtr(const WsJsonRpcConnectionPtr &optional_failed_connection, int connect_retry_number)>;

public:
  // `set_ca_bundle` needs to be called once per process to enable server certificate verification
  // Note: XBox currently ignores this value.
  static void set_ca_bundle(eastl::string_view ca_bundle_path);

  static eastl::shared_ptr<WsJsonRpcPersistentConnection> create()
  {
    return eastl::make_shared<WsJsonRpcPersistentConnection>(PrivateConstructorTag{});
  }

public:
  explicit WsJsonRpcPersistentConnection(PrivateConstructorTag);
  ~WsJsonRpcPersistentConnection();

  // This method must be called ONLY before the first poll() call.
  // The provided callbacks MUST not call object methods directly or indirectly.
  void initialize(ConnectionConfigGeneratorCallback &&connection_config_generator, IncomingRequestCallback &&incoming_request_callback,
    const WsJsonRpcStatsCallbackPtr &stats_callback);

  // This method should be called regularly until `isCloseCompleted()` becomes `true`.
  // This method is NOT reentrant.
  // Application callbacks may be called here.
  void poll();

  // If `isCallable()` returns `false`, the method will fail.
  // Application callbacks may be called here.
  void rpcCall(RpcMessageId message_id, eastl::string_view method, const rapidjson::Value *optional_params,
    ResponseCallback &&callback, int request_timeout_ms);
  void rpcCall(RpcMessageId message_id, eastl::string_view method, eastl::string_view params_as_json, ResponseCallback &&callback,
    int request_timeout_ms);

  // Existing RPC callbacks still can get valid responses after this call.
  // But not later than `isCloseCompleted()` becomes `true`.
  // Application callbacks are NEVER called here.
  void initiateClose();

  // Public property lifetime diagram:
  //   past ---> future:
  // +-----------------------------------------------------------------+--------------------+
  // |                      CloseCompleted=0                           |  CloseCompleted=1  |
  // +-------------+--------------+---------------+--------------+-----+                    |
  // |  /connect/  |  Callable=1  |  /reconnect/  |  Callable=1  |     |                    |
  // +-------------+--------------+---------------+--------------+-----+--------------------+

  bool isCallable() const { return currentConnection != nullptr; }

  // Object can be gracefully deleted once `false` is returned.
  bool isCloseCompleted() const { return state == State::CLOSED; }

  // Returns number of callbacks waiting for RPC result message in current connection.
  size_t getInProgressCallCount() const { return currentConnection ? currentConnection->getInProgressCallCount() : 0; }

  // Returns last error which happened on connection (e.g. after connection failure or disconnection)
  // May be empty on some platforms.
  websocket::Error copyLastError() const;

public:
  // Specific API for Persistent connection:

  // initiateGracefulReconnect() is useful once application updated list of servers or credentials.
  // Call this method once you configured connectionConfigGenerator to return updated config.
  // Application callbacks are NEVER called here.
  void initiateGracefulReconnect();

private:
  enum class State
  {
    INITIAL,
    INITIALIZED,
    CONNECTING,
    WORKING,
    RECONNECTING,
    CLOSING,
    CLOSED,
  };

  // We can call this method at any time to verify object state is consistent (debug purposes)
  void verifyObjectInvariant() const;

  WsJsonRpcConnectionPtr createNewRpcConnection(const WsJsonRpcConnectionPtr &optional_failed_connection, int connect_retry_number);
  // application callbacks may be called here:
  void setupNewConnectionIncubator(const WsJsonRpcConnectionPtr &optional_failed_connection);

  void cancelNewConnection();

  // Returns a pointer to a newly connected connection (nullptr in most of the calls)
  void pollNewConnectionIncubator();    // application callbacks may be called here
  void pollAllPollableConnections();    // application callbacks may be called here
  void processCurrentConnectionState(); // application callbacks may be called here

  static const char *getStateName(State state_value);
  void switchState(State previous_state, State next_state);

  void softReset();
  void hardReset();

private:
  const eastl::string logPrefix;

  ConnectionConfigGeneratorCallback connectionConfigGenerator;
  IncomingRequestCallback incomingRequestCallback;
  WsJsonRpcStatsCallbackPtr statsCallback;

  details::ReentranceProtection reentranceProtection;

  State state = State::INITIAL;

  WsJsonRpcConnectionPtr currentConnection;

  details::NewConnectionIncubatorPtr newConnectionIncubator;
  details::OldConnectionStorage oldConnectionStorage;
  websocket::Error lastError;
};


using WsJsonRpcPersistentConnectionPtr = eastl::shared_ptr<WsJsonRpcPersistentConnection>;


} // namespace websocketjsonrpc
