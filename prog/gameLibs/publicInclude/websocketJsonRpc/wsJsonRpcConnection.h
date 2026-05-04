//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/details/commonTypes.h>
#include <websocketJsonRpc/details/responseCallbackCollection.h>
#include <websocketJsonRpc/rpcRequest.h>
#include <websocketJsonRpc/rpcResponse.h>

#include <websocket/websocket.h>

#include <EASTL/deque.h>
#include <EASTL/optional.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/variant.h>

#include <mutex>


namespace websocketjsonrpc
{


struct WsJsonRpcConnectionConfig final
{
  // Connection settings
  eastl::string serverUrl;
  eastl::string connectIpHint; // if non-empty: override IP address for serverUrl
  websocket::Headers additionalHttpHeaders;

  bool waitForAuthenticationResponse = true; // SEP specific authentication response with message id `null`
  bool verifyCert = true;
  bool verifyHost = true;
  int connectTimeoutMs = 5'000;

  // Settings for established connection
  unsigned maxReceivedMessageSize = 1'000'000; // 0 means unlimited message size; value can be ignored on some platforms

  bool enableZstdDecompressionSupport = true; // decompress responses using zstd

  // Requests from client to server:
  // It is useful to have the following two options enabled for light logging:
  // - logOutgoingRequestEvents
  // - dumpIncomingErrorResponses
  // In this case it would be possible to trace what exact action caused RPC error
  bool logOutgoingRequestEvents = false;
  bool logIncomingResponseEvents = false;
  bool dumpOutgoingRequests = false;
  bool dumpIncomingResponses = false;      // enables `dumpIncomingErrorResponses` implicitly
  bool dumpIncomingErrorResponses = false; // dump extended info about error messages

  // Requests from server to client:
  // It is useful to have the following two options enabled for light logging:
  // - logOutgoingRequestEvents
  // - dumpIncomingErrorResponses
  bool logIncomingRequestEvents = false;
  bool logOutgoingResponseEvents = false;
  bool dumpIncomingRequests = false;
  bool dumpOutgoingResponses = false;      // enables `dumpOutgoingErrorResponses` implicitly
  bool dumpOutgoingErrorResponses = false; // dump extended info about error messages

  size_t maxBytesToDump = 1024;
};


using WsJsonRpcConnectionConfigPtr = eastl::unique_ptr<WsJsonRpcConnectionConfig>;


// Thread safety is different for different methods:
//
// Thread-safe methods:
// - getConfig
// - getConnectionId
//
// Methods which are not thread safe against each other calls:
// - getActualUrlIp
// - constructor
// - setIncomingRequestCallback
// - poll
// - rpcCall
// - sendResponseResult
// - sendResponseError
// - initiateClose
// - isConnecting
// - isConnected
// - isClosed
// - getInProgressCallCount
//
// Application-provided callbacks are called from `poll()` method only.
// It is safe to call any class methods from the callback except class destructor.
//
class WsJsonRpcConnection final : public eastl::enable_shared_from_this<WsJsonRpcConnection>
{
private:
  class PrivateConstructorTag final
  {};

public:
  using IncomingRequestCallback = eastl::function<void(RpcRequestPtr &&incoming_request)>;

public:
  // `set_ca_bundle` needs to be called once per process to enable server certificate verification
  // Note: XBox currently ignores this value.
  static void set_ca_bundle(eastl::string_view ca_bundle_path);

  static eastl::shared_ptr<WsJsonRpcConnection> create(const WsJsonRpcConnectionConfig &config)
  {
    return eastl::make_shared<WsJsonRpcConnection>(config, PrivateConstructorTag{});
  }

public:
  WsJsonRpcConnection(const WsJsonRpcConnectionConfig &config, PrivateConstructorTag);
  ~WsJsonRpcConnection();

  const WsJsonRpcConnectionConfig &getConfig() const { return config; }
  ConnectionId getConnectionId() const { return connectionId; }

  const eastl::string &getActualUrlIp() const { return actualUrlIp; }

  // This method must be called ONLY before the first poll() call.
  void setIncomingRequestCallback(IncomingRequestCallback &&callback);

  // Poll starts connection on its first call.
  // This method should be called regularly until `isClosed()` becomes `true`.
  // Poll guarantees to process all delayed events from other methods by one single call.
  // This is the only method which calls application-provided callbacks.
  // The method is reentrant-safe (in the same thread).
  void poll();

  // If `isConnected()` returns `false`, the method will fail.
  // `message_id` must be unique across the lifetime of the connection.
  // `message_id` must be greater than 0.
  // The provided callback is guaranteed to be called exactly once.
  // No callbacks are called from this method.
  void rpcCall(RpcMessageId message_id, eastl::string_view method, const rapidjson::Value *optional_params,
    ResponseCallback &&callback, int request_timeout_ms);
  // Empty `params_as_json` means no value.
  void rpcCall(RpcMessageId message_id, eastl::string_view method, eastl::string_view params_as_json, ResponseCallback &&callback,
    int request_timeout_ms);

  // This method is useful when some requests are sent from the server (not from client).
  // If `isConnected()` returns `false`, does nothing (except logging)
  // No callbacks are called from this method.
  void sendResponseResult(eastl::string_view serialized_message_id, const rapidjson::Document &rpc_result);

  // This method is useful when some requests are sent from the server (not from client).
  // If `isConnected()` returns `false`, does nothing (except logging)
  // No callbacks are called from this method.
  void sendResponseError(eastl::string_view serialized_message_id, int error_code, eastl::string_view error_message,
    eastl::string_view error_context);

  // Existing RPC callbacks still can get valid responses after this call.
  // But not later than `isClosed()` becomes `true`.
  // No callbacks are called from this method.
  void initiateClose();

  // Public property lifetime diagram:
  //   past ---> future:
  // +---------------------------------------+----------+
  // |              Closed=0                 | Closed=1 |
  // +--------------+-------------+----------+          |
  // | Connecting=1 | Connected=1 |          |          |
  // +--------------+-------------+----------+----------+
  //        |                |
  //        |                +- abandon ->|-------->
  //        |                +- lost connection --->
  //        +- abandon by user ---------->|-------->
  //        +- when auth failed --------->|-------->
  //        +- failed to connect ------------------>

  // We should call `poll()` until it is callable or not pollable.
  bool isConnecting() const { return state == State::INITIAL || state == State::CONNECTING || state == State::AUTHENTICATING; }

  bool isConnected() const { return state == State::CONNECTED; }

  // Object can be gracefully deleted once `false` is returned.
  bool isClosed() const { return state == State::CLOSED; }

  // Returns number of callbacks waiting for RPC result message
  size_t getInProgressCallCount() const { return callbacks.getInProgressCount(); }

  // This method returns time elapsed for connection establishment in milliseconds.
  int64_t getConnectionEstablishTimeMs() const;

  // This method returns time elapsed after connection was established and before it was closed in milliseconds.
  int64_t getConnectionLifetimeMs() const;

  // Returns last error which happened on connection (e.g. after connection failure or disconnection)
  // May be empty on some platforms. Always valid if `isAuthenticationError() == true`
  const websocket::Error &getLastError() const { return lastError; }

  // This error is set before `state` becomes `CLOSED` if connection failed because of authentication error.
  // Last error usually contains SEP_AUTHENTICATION_ERROR (-29502) when authentication error occurred.
  bool isAuthenticationError() const { return authenticationFailed; }

private:
  using WebSocketClientPtr = eastl::unique_ptr<websocket::WebSocketClient>;

  enum class State
  {
    INITIAL,
    CONNECTING,
    AUTHENTICATING,
    CONNECTED, // i.e. connected and authenticated
    CLOSING,
    CLOSED,
  };

  struct ConnectionSucceededEvent final
  {
    eastl::string actualUrlIp;
  };

  struct ConnectionFailedEvent final
  {
    ConnectionFailedEvent(eastl::string_view actual_url_ip_, websocket::Error &&error_) :
      actualUrlIp(actual_url_ip_), error(eastl::move(error_))
    {}

    eastl::string actualUrlIp;
    websocket::Error error;
  };

  struct DisconnectEvent final
  {
    DisconnectEvent(websocket::CloseStatusInt status_, websocket::Error &&error_) : status(status_), error(eastl::move(error_)) {}

    websocket::CloseStatusInt status{};
    websocket::Error error;
  };

  struct CannotMakeRpcCallEvent final
  {
    CannotMakeRpcCallEvent(RpcMessageId message_id, eastl::string_view method_, ResponseCallback &&callback_) :
      messageId(message_id), method(method_), callback(eastl::move(callback_))
    {}

    RpcMessageId messageId{};
    eastl::string method;
    ResponseCallback callback;
  };

  // Request or notification from the server
  // (non-standard JSON RPC message direction)
  struct RequestFromServerEvent final
  {
    RequestFromServerEvent(rapidjson::Document &&full_json_rpc_request, size_t payload_size) :
      request(eastl::make_unique<RpcRequest>(eastl::move(full_json_rpc_request), payload_size))
    {}

    RpcRequestPtr request;
  };

  struct ResponseFromServerEvent final
  {
    ResponseFromServerEvent(const eastl::optional<RpcMessageId> &message_id, rapidjson::Document &&full_json_rpc_response,
      size_t payload_size) :
      messageId(message_id), response(eastl::make_unique<RpcResponse>(eastl::move(full_json_rpc_response), payload_size))
    {}

    eastl::optional<RpcMessageId> messageId;
    RpcResponsePtr response;
  };

  struct InitiateCloseEvent final
  {};

  using Event = eastl::variant<ConnectionSucceededEvent, eastl::unique_ptr<ConnectionFailedEvent>, eastl::unique_ptr<DisconnectEvent>,
    eastl::unique_ptr<CannotMakeRpcCallEvent>, RequestFromServerEvent, ResponseFromServerEvent, InitiateCloseEvent>;

private:
  void addEvent(Event &&event);
  bool extractEvent(Event &event);

  void onConnectCallback(websocket::ConnectStatus status, const eastl::string &actual_uri_ip, websocket::Error &&error);
  void onMessageCallback(eastl::string_view payload, bool is_text);
  void onCloseCallback(websocket::CloseStatusInt status, websocket::Error &&error);

  void notifyAllCallbacks(AdditionalErrorCodes error_code, eastl::string_view error_description, int reason_error_code);
  void onStateInitial();
  void processTimeouts();

  static const char *getStateName(State state_value);
  void switchState(State previous_state, State next_state);
  void verifyState(bool state_is_ok, const char *verify_context); // log and alert if state is not ok

  static eastl::optional<Event> parseRawMessage(eastl::string_view payload, bool is_text, const WsJsonRpcConnectionConfig &config,
    const eastl::string &log_prefix);

  static eastl::optional<Event> parseDecompressedMessage(eastl::string_view payload, size_t rawPayloadSize,
    const WsJsonRpcConnectionConfig &config, const eastl::string &log_prefix);

  static eastl::optional<Event> parseIncomingRequestMessage(eastl::string_view payload, rapidjson::Document &&document,
    const WsJsonRpcConnectionConfig &config, const eastl::string &log_prefix);

  void processAuthenticationResult(RpcResponsePtr &&response);

  void onEvent(ConnectionSucceededEvent &&event);
  void onEvent(eastl::unique_ptr<ConnectionFailedEvent> &&event);
  void onEvent(eastl::unique_ptr<DisconnectEvent> &&event);
  void onEvent(eastl::unique_ptr<CannotMakeRpcCallEvent> &&event);
  void onEvent(RequestFromServerEvent &&event);
  void onEvent(ResponseFromServerEvent &&event);
  void onEvent(InitiateCloseEvent &&event);

private:
  const WsJsonRpcConnectionConfig config;
  const ConnectionId connectionId;
  const eastl::string logPrefix;
  WebSocketClientPtr wsClient;

  State state = State::INITIAL;

  std::mutex protectIncomingEvents;
  eastl::deque<Event, EASTLAllocatorType, 64> incomingEvents;

  IncomingRequestCallback incomingRequestCallback;
  details::ResponseCallbackCollection callbacks;

  RpcResponsePtr delayedAuthenticationResponse;
  eastl::string actualUrlIp;

  TickCount createTimeTick = 0;
  TickCount onConnectTimeTick = 0;
  TickCount onCloseTimeTick = 0;
  websocket::Error lastError;
  bool authenticationFailed = false;
};


using WsJsonRpcConnectionPtr = eastl::shared_ptr<WsJsonRpcConnection>;


namespace details
{

using ConnectionIdType = std::underlying_type_t<ConnectionId>;

inline ConnectionIdType get_connection_id(const WsJsonRpcConnectionPtr &connection)
{
  const ConnectionIdType connectionId = connection ? eastl::to_underlying(connection->getConnectionId()) : 0;
  return connectionId;
}

} // namespace details


} // namespace websocketjsonrpc
