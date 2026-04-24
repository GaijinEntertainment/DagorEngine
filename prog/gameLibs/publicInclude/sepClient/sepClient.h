//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sepClient/details/connectionConfigGenerator.h>
#include <sepClient/details/delayedRpcCallCollection.h>
#include <sepClient/reconnectDelayConfig.h>
#include <sepClient/rpcRequest.h>
#include <sepClient/sepClientStats.h>

#include <websocketJsonRpc/wsJsonRpcPersistentConnection.h>

#include <mutex>
#include <variant>


namespace sepclient
{


struct SepClientConfig final
{
  bool verifyCert = true; // used only if `set_ca_bundle()` was called; value can be ignored on some platforms
  bool verifyHost = true; // used only if `set_ca_bundle()` was called; value can be ignored on some platforms

  LogLevel logLevel = LogLevel::DEFAULT;

  int connectionTryTimeoutMs = 15'000;        // network timeout for connection try; value can be ignored on some platforms
  int requestTimeoutMs = 60'000;              // time to wait for a response to an RPC call before error is reported
  int requestTimeoutWithoutNetworkMs = 7'000; // time to wait to for network connection before sending RPC request
                                              // (<0 disables the setting)

  bool enableRequestRetryOnDisconnect = true;
  bool enableZstdDecompressionSupport = true;

  int maxSimultaneousRequests = 4; // new requests will be delayed if this limit is reached

  eastl::string authorizationToken;        // Gaijin SSO Token. Currently JWT token; no connection is made while it is empty
  eastl::vector<eastl::string> serverUrls; // WebSocket server URLs (ws:/..., wss:/...

  ReconnectDelayConfig reconnectDelay;

  eastl::string defaultProjectId; // for logging and for UserAgent
};


using NotificationFromServer = websocketjsonrpc::RpcRequest;
using NotificationFromServerPtr = eastl::unique_ptr<NotificationFromServer>;

using IncomingNotificationCallback = eastl::function<void(NotificationFromServerPtr &&notification)>;


struct AuthenticationFailure final
{
  AuthenticationFailure(eastl::string &&authorization_token, eastl::string_view server_url, eastl::string_view error_message) :
    authorizationToken(eastl::move(authorization_token)), serverUrl(server_url), errorMessage(error_message)
  {}

  eastl::string authorizationToken;
  eastl::string serverUrl;
  eastl::string errorMessage;
};

using AuthenticationFailurePtr = eastl::unique_ptr<AuthenticationFailure>;

using AuthenticationFailureCallback = eastl::function<void(AuthenticationFailurePtr &&authentication_failure)>;


// The class is thread safe:
//    All methods can be called from different threads. But it is much better to call them from the same thread.
// The class is reentrant-safe:
//    It is safe to call class members from the notification_callback and from RPC response callback.
// Callbacks are called from the `poll()` method only.
class SepClient final : public eastl::enable_shared_from_this<SepClient>
{
private:
  class PrivateConstructorTag final
  {};

public:
  // `set_ca_bundle` needs to be called once per process to enable server certificate verification
  // Note: XBox currently ignores this value.
  static void set_ca_bundle(eastl::string_view ca_bundle_path);

  static eastl::shared_ptr<SepClient> create(const SepClientConfig &config)
  {
    return eastl::make_shared<SepClient>(config, PrivateConstructorTag{});
  }

public:
  SepClient(const SepClientConfig &config, PrivateConstructorTag);
  ~SepClient();

  // nullptr is allowed for notification_callback
  void initialize(AuthenticationFailureCallback &&authentication_failure_callback,
    IncomingNotificationCallback &&notification_callback, const SepClientStatsCallbackPtr &stats_callback);

  // Initiate connection if token was empty before.
  // Initiate graceful reconnection if connection was already established.
  bool updateAuthorizationToken(eastl::string_view authorization_token);

  // returns unique message id (positive signed 64-bit integer value)
  static RpcMessageId generateMessageId();

  // returns a random positive signed 64-bit integer value
  static TransactionId generateTransactionId();

  // Create a new RPC request object for using in `rpcCall()`.
  // Provided transaction id and JSON RPC message id
  // should be used and registered in the game before starting the RPC call.
  // This will avoid data races in case response callback is called too quickly.
  RpcRequest createRpcRequest(eastl::string_view service, eastl::string_view action,
    eastl::optional<sepclient::RpcMessageId> message_id = eastl::nullopt,
    eastl::optional<TransactionId> transaction_id = eastl::nullopt);

  void rpcCall(const RpcRequest &request, ResponseCallback &&response_callback);

  void poll();

  void initiateClose();

  bool isCallable() const;

  bool isCloseCompleted() const;

private:
  enum class State
  {
    INITIAL,
    INITIALIZED,
    WORKING, // I do not think we should distinguish between established and reconnecting underlaying states
    CLOSING,
    CLOSED,
  };

  struct IncomingRequestData final
  {
    websocketjsonrpc::RpcRequestPtr request;
  };

  struct IncomingResponseData final
  {
    details::RpcContextPtr context;
    websocketjsonrpc::RpcResponsePtr response;
  };

  using Connection = websocketjsonrpc::WsJsonRpcPersistentConnection;
  using IncomingData = eastl::variant<IncomingResponseData, IncomingRequestData, AuthenticationFailurePtr>;

private:
  static const char *getStateName(State state_value);
  void switchState(State previous_state, State next_state);

  void onLowerLevelResponse(details::RpcContextPtr &&context, websocketjsonrpc::RpcResponsePtr &&response);
  void executeRpcCall(details::RpcContextPtr &&context);
  void delayRpcCall(details::RpcContextPtr &&context, eastl::string_view delay_reason);
  void abandonRpcCall(details::RpcContextPtr &&context, websocketjsonrpc::AdditionalErrorCodes error_code,
    eastl::string_view error_description);
  void processIncomingData(IncomingRequestData &&data) const;
  void processIncomingData(IncomingResponseData &&data);
  void processIncomingData(AuthenticationFailurePtr &&data) const;

  void tryExecuteAllDelayedCalls();
  void processExpiredDelayedCalls();
  void abandonAllDelayedCalls();

  websocketjsonrpc::WsJsonRpcConnectionConfigPtr generateConfig(
    const websocketjsonrpc::WsJsonRpcConnectionPtr &optional_failed_connection, int connect_retry_number);

  bool isDebugLog() const { return logLevel == LogLevel::FULL_DUMP; }
  bool isInfoLog() const { return logLevel != LogLevel::NONE; }

private:
  const eastl::string logPrefix;
  const LogLevel logLevel = LogLevel::DEFAULT;
  const details::RpcContextSettings defaultContextSettings;
  const int maxSimultaneousRequests = -1;

  mutable std::mutex mutex; // protects all members below

  AuthenticationFailureCallback authenticationFailureCallback;
  IncomingNotificationCallback notificationCallback;
  SepClientStatsCallbackPtr statsCallback;

  const websocketjsonrpc::WsJsonRpcPersistentConnectionPtr connection;

  details::ConnectionConfigGenerator connectionConfigGenerator;
  websocketjsonrpc::WsJsonRpcConnectionPtr previousOptionalFailedConnection;

  State state = State::INITIAL;

  int numberOfExecutingRequests = 0;
  eastl::deque<IncomingData, EASTLAllocatorType, 64> incomingDataQueue; // filled and flushed during poll()

  details::DelayedRpcCallCollection delayedRpcCalls;
};


using SepClientPtr = eastl::shared_ptr<SepClient>;


} // namespace sepclient
