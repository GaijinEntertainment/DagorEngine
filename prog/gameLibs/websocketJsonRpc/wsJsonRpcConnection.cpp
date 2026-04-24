// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/wsJsonRpcConnection.h>

#include "decompressBinaryMessage.h"
#include "jsonRpcProtocol.h"

#include <websocketJsonRpc/details/abstractionLayer.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>

#include <rapidjson/error/en.h>
#include <rapidjson/writer.h>

#include <EASTL/atomic.h>


namespace websocketjsonrpc
{
using Buffer = rapidjson::StringBuffer;
using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

static eastl::atomic<details::ConnectionIdType> next_connection_id = 1;
static_assert(std::is_unsigned_v<details::ConnectionIdType>); // there is no UB in case of overflow on increment


static const eastl::string NO_MESSAGE_ID = "<no_message_id>";


static constexpr const char *to_string(rapidjson::Type type)
{
  switch (type)
  {
    case rapidjson::kNullType: return "kNull";
    case rapidjson::kFalseType: return "kFalse";
    case rapidjson::kTrueType: return "kTrue";
    case rapidjson::kObjectType: return "kObject";
    case rapidjson::kArrayType: return "kArray";
    case rapidjson::kStringType: return "kString";
    case rapidjson::kNumberType: return "kNumber";
  }
  return "kUnknown";
}


static eastl::string truncate_payload(eastl::string_view payload, size_t maxBytesToDump)
{
  const char *const data = reinterpret_cast<const char *>(payload.data());
  if (payload.size() <= maxBytesToDump)
  {
    return {data, payload.size()};
  }

  constexpr eastl::string_view middle = " ... ";
  const size_t dumpedBoarderSize = eastl::min(payload.size() / 2, maxBytesToDump / 2);

  eastl::string result;
  result.reserve(dumpedBoarderSize * 2 + middle.size());
  result.append(data, dumpedBoarderSize);
  result.append(middle.data(), middle.size());
  result.append(data + payload.size() - dumpedBoarderSize, dumpedBoarderSize);
  return result;
}


void WsJsonRpcConnection::set_ca_bundle(eastl::string_view ca_bundle_path)
{
  const eastl::string caBundlePath{ca_bundle_path};
  websocket::set_ca_bundle(caBundlePath.c_str());
}


WsJsonRpcConnection::WsJsonRpcConnection(const WsJsonRpcConnectionConfig &config_, PrivateConstructorTag) :
  config(config_),
  connectionId(static_cast<ConnectionId>(next_connection_id.fetch_add(1, eastl::memory_order_relaxed))),
  logPrefix(eastl::string::CtorSprintf{}, "[sep][websocket_jsonrpc#%u] ", connectionId),
  createTimeTick(abstraction::get_current_tick_count())
{
  G_ASSERT(config.connectTimeoutMs >= 0);
  G_ASSERT(!wsClient);
}


WsJsonRpcConnection::~WsJsonRpcConnection()
{
  if (state != State::INITIAL && state != State::CLOSED)
  {
    logwarn("%sWARN! WebSocket connection object was not closed properly! (state: %s)", logPrefix.c_str(), getStateName(state));
  }
}


void WsJsonRpcConnection::setIncomingRequestCallback(IncomingRequestCallback &&callback)
{
  verifyState(state == State::INITIAL, "setIncomingRequestCallback");

  incomingRequestCallback = eastl::move(callback);
}


void WsJsonRpcConnection::addEvent(Event &&event)
{
  std::lock_guard<std::mutex> lock(protectIncomingEvents);
  incomingEvents.emplace_back(eastl::move(event));
}


bool WsJsonRpcConnection::extractEvent(Event &event)
{
  std::lock_guard<std::mutex> lock(protectIncomingEvents);
  if (incomingEvents.empty())
  {
    return false;
  }
  event = eastl::move(incomingEvents.front());
  incomingEvents.pop_front();
  return true;
}


void WsJsonRpcConnection::onConnectCallback(websocket::ConnectStatus status, const eastl::string &actual_uri_ip,
  websocket::Error &&error)
{
  switch (status)
  {
    case websocket::ConnectStatus::Ok: addEvent(ConnectionSucceededEvent{actual_uri_ip}); break;

    case websocket::ConnectStatus::HttpConnectFailed:
      addEvent(eastl::make_unique<ConnectionFailedEvent>(actual_uri_ip, eastl::move(error)));
      break;

    case websocket::ConnectStatus::ConnectIsActive: // pass
    default:
    {
      verifyState(false, "onConnectCallback");
      break;
    }
  }
}


void WsJsonRpcConnection::onCloseCallback(websocket::CloseStatusInt status, websocket::Error &&error)
{
  // Note: there is a known time lag here in Xbox:
  // WsJsonRpcConnection will continue receiving new RPC requests from the game
  // until nearest poll() call while connection is already broken.
  // Those requests will receive error responses during this nearest poll() call.

  addEvent(eastl::make_unique<DisconnectEvent>(status, eastl::move(error)));
}


void WsJsonRpcConnection::onMessageCallback(eastl::string_view payload, bool is_text)
{
  eastl::optional<Event> event = parseRawMessage(payload, is_text, config, logPrefix);
  if (event.has_value())
  {
    addEvent(eastl::move(*event));
  }
}


const char *WsJsonRpcConnection::getStateName(State state_value)
{
  switch (state_value)
  {
    case State::INITIAL: return "INITIAL";
    case State::CONNECTING: return "CONNECTING";
    case State::AUTHENTICATING: return "AUTHENTICATING";
    case State::CONNECTED: return "CONNECTED";
    case State::CLOSING: return "CLOSING";
    case State::CLOSED: return "CLOSED";
  }
  return "UNKNOWN_STATE";
}


void WsJsonRpcConnection::switchState(State previous_state, State next_state)
{
  const State actualState = state;

  if (static_cast<int>(previous_state) >= static_cast<int>(next_state))
  {
    state = State::CLOSED;

    G_ASSERT_LOG(false,
      "%s[sep_error] ERROR! Wanted to switch state from %s to %s, but this switch direction is forbidden!"
      " Current state: %s; Switched to CLOSED instead.",
      logPrefix.c_str(), getStateName(previous_state), getStateName(next_state), getStateName(actualState));

    return;
  }

  if (actualState == previous_state)
  {
    state = next_state;

    logdbg("%sSwitched state from %s to %s", logPrefix.c_str(), getStateName(previous_state), getStateName(next_state));
  }
  else
  {
    state = State::CLOSED;

    G_ASSERT_LOG(false,
      "%s[sep_error] ERROR! Wanted to switch state from %s to %s, but %s state was found; Switched to CLOSED instead.",
      logPrefix.c_str(), getStateName(previous_state), getStateName(next_state), getStateName(actualState));

    return;
  }
}


void WsJsonRpcConnection::verifyState(bool state_is_ok, const char *verify_context)
{
  if (DAGOR_LIKELY(state_is_ok))
    return;

  verify_context = verify_context ? verify_context : "Unknown verify context";
  G_ASSERT_LOG(false, "%s[sep_error] ERROR! %s: Unexpected WsJsonRpcConnection state: %s", logPrefix.c_str(), verify_context,
    getStateName(state));
}


void WsJsonRpcConnection::initiateClose()
{
  if (state == State::INITIAL)
  {
    logdbg("%sClosed WebSocket connection from initial state", logPrefix.c_str());
    switchState(State::INITIAL, State::CLOSED);
    return;
  }

  if (state != State::CLOSING && state != State::CLOSED)
  {
    logdbg("%sInitiate closing WebSocket connection", logPrefix.c_str());
    switchState(state, State::CLOSING);

    addEvent(InitiateCloseEvent{});
  }
}


void WsJsonRpcConnection::notifyAllCallbacks(AdditionalErrorCodes error_code, eastl::string_view error_description,
  int reason_error_code)
{
  verifyState(state == State::CLOSING, "notifyAllCallbacks");

  RpcMessageId messageId{};

  while (ResponseCallback callback = callbacks.extractAnyCallback(messageId))
  {
    auto response = eastl::make_unique<RpcResponse>(messageId, error_code, error_description, reason_error_code);

    G_ASSERT_CONTINUE(callback);
    callback(eastl::move(response));
  }

  G_ASSERT(callbacks.isEmpty());
}


void WsJsonRpcConnection::onStateInitial()
{
  logdbg("%sConnecting to WebSocket server %s (connect IP hint: %s)", logPrefix.c_str(), config.serverUrl.c_str(),
    (config.connectIpHint.empty() ? "not specified" : config.connectIpHint.c_str()));
  switchState(State::INITIAL, State::CONNECTING);

  const auto weakSelf = weak_from_this();

  auto connectCb = [weakSelf](websocket::WebSocketClient *, websocket::ConnectStatus status, const eastl::string &actual_uri_ip,
                     websocket::Error &&error) {
    WsJsonRpcConnectionPtr sharedSelf = weakSelf.lock();
    if (!sharedSelf)
    {
      logerr("[sep][websocket_jsonrpc][sep_error] ERROR! WsJsonRpcConnection object was found destroyed before calling onConnect!");
      return;
    }
    sharedSelf->onConnectCallback(status, actual_uri_ip, eastl::move(error));
  };

  websocket::ClientConfig clientConfig;
  clientConfig.verifyCert = config.verifyCert;
  clientConfig.verifyHost = config.verifyHost;
  clientConfig.connectTimeoutMs = config.connectTimeoutMs;
  clientConfig.maxMessageSize = config.maxReceivedMessageSize;

  clientConfig.onMessage = [weakSelf](websocket::WebSocketClient *, websocket::PayloadView payload, bool is_text) {
    WsJsonRpcConnectionPtr sharedSelf = weakSelf.lock();
    if (!sharedSelf)
    {
      logerr("[sep][websocket_jsonrpc][sep_error] ERROR! WsJsonRpcConnection object was found destroyed before calling onMessage!");
      return;
    }
    const eastl::string_view payloadView{reinterpret_cast<const char *>(payload.data()), payload.size()};
    sharedSelf->onMessageCallback(payloadView, is_text);
  };

  clientConfig.onClose = [weakSelf](websocket::WebSocketClient *, websocket::CloseStatusInt status, websocket::Error &&error) {
    WsJsonRpcConnectionPtr sharedSelf = weakSelf.lock();
    if (!sharedSelf)
    {
      logerr("[sep][websocket_jsonrpc][sep_error] ERROR! WsJsonRpcConnection object was found destroyed before calling onClose!");
      return;
    }
    sharedSelf->onCloseCallback(status, eastl::move(error));
  };

  G_ASSERT(!wsClient);
  wsClient = abstraction::create_websocket_client(eastl::move(clientConfig));

  G_ASSERT_RETURN(wsClient, );
  wsClient->connect(config.serverUrl, "", eastl::move(connectCb), config.additionalHttpHeaders);
}


void WsJsonRpcConnection::processTimeouts()
{
  RpcMessageId messageId{};

  while (ResponseCallback callback = callbacks.extractExpiredCallback(messageId))
  {
    auto response = eastl::make_unique<RpcResponse>(messageId, AdditionalErrorCodes::RPC_CALL_TIMEOUT, "JSON RPC: request timeout");

    G_ASSERT_CONTINUE(callback);
    callback(eastl::move(response));
  }
}


static void construct_json_rpc_request(Buffer &buffer, const RpcMessageId messageId, eastl::string_view method,
  eastl::string_view params_as_json)
{
  JsonWriter writer(buffer);
  {
    writer.StartObject();

    writer.Key(protocol::VERSION);
    writer.String(protocol::VERSION_VALUE);

    writer.Key(protocol::MESSAGE_ID);
    const int64_t messageIdAsInt64 = messageId;
    static_assert(sizeof(messageIdAsInt64) == sizeof(RpcMessageId));
    static_assert(std::is_signed_v<decltype(messageIdAsInt64)> == std::is_signed_v<RpcMessageId>);
    writer.Int64(messageIdAsInt64);

    writer.Key(protocol::METHOD);
    G_VERIFY(writer.String(method.data(), method.size()));

    if (!params_as_json.empty())
    {
      writer.Key(protocol::PARAMS);
      G_VERIFY(writer.RawValue(params_as_json.data(), params_as_json.size(), rapidjson::Type::kObjectType));
    }

    writer.EndObject();
  }

  G_ASSERT(writer.IsComplete());
}


void WsJsonRpcConnection::rpcCall(RpcMessageId message_id, eastl::string_view method, const rapidjson::Value *optional_params,
  ResponseCallback &&callback, int request_timeout_ms)
{
  Buffer buffer;
  eastl::string_view paramsAsJson;

  if (optional_params)
  {
    JsonWriter writer(buffer);
    G_VERIFY(optional_params->Accept(writer));
    G_ASSERT(writer.IsComplete());

    paramsAsJson = eastl::string_view{buffer.GetString(), buffer.GetSize()};
  }

  rpcCall(message_id, method, paramsAsJson, eastl::move(callback), request_timeout_ms);
}


void WsJsonRpcConnection::rpcCall(RpcMessageId message_id, eastl::string_view method, eastl::string_view params_as_json,
  ResponseCallback &&callback, int request_timeout_ms)
{
  G_ASSERT_RETURN(callback, );

  if (!isConnected())
  {
    addEvent(eastl::make_unique<CannotMakeRpcCallEvent>(message_id, method, eastl::move(callback)));
    return;
  }

  const bool callbackAddedOk = callbacks.addCallback(message_id, eastl::move(callback), request_timeout_ms);
  if (!callbackAddedOk)
  {
    logerr("%s[sep_error] ERROR! Dropped RPC call with duplicated RPC message id %lld: Method: %.*s; params size: %zu bytes",
      logPrefix.c_str(), (long long)message_id, (int)method.size(), method.data(), params_as_json.size());
    return;
  }

  Buffer buffer;
  construct_json_rpc_request(buffer, message_id, method, params_as_json);

  const eastl::string_view bufferView{buffer.GetString(), buffer.GetSize()};
  const websocket::PayloadView wsBufferView{reinterpret_cast<const uint8_t *>(bufferView.data()), bufferView.size()};

  if (config.logOutgoingRequestEvents)
  {
    logdbg("%sSending request with RPC message id %lld: Method: %.*s; Size: %zu bytes", logPrefix.c_str(), (long long)message_id,
      (int)method.size(), method.data(), bufferView.size());
  }

  constexpr bool isText = true;

  if (config.dumpOutgoingRequests)
  {
    logdbg("%sDump: Sending request with RPC message id %lld: Method: %.*s; Size: %zu bytes; isText: %d; Raw data = [%s]",
      logPrefix.c_str(), (long long)message_id, (int)method.size(), method.data(), bufferView.size(), isText,
      truncate_payload(bufferView, config.maxBytesToDump).c_str());
  }

  G_ASSERT_RETURN(wsClient, );
  wsClient->send(wsBufferView, isText);
}


eastl::optional<WsJsonRpcConnection::Event> WsJsonRpcConnection::parseRawMessage(eastl::string_view payload, bool is_text,
  const WsJsonRpcConnectionConfig &config, const eastl::string &logPrefix)
{
  const auto onFailedProcessing = [&payload, is_text, &config, &logPrefix]() {
    if (config.dumpIncomingErrorResponses || config.dumpIncomingResponses)
    {
      logdbg("%sDump: Failed processing received WebSocket message: Size: %zu bytes; isText: %d; Raw data = [%s]", logPrefix.c_str(),
        payload.size(), is_text, truncate_payload(payload, config.maxBytesToDump).c_str());
    }
  };

  details::MemoryChunk optionalStorageForDecompressedPayload;
  eastl::string_view decompressedPayloadView = payload;

  if (!is_text)
  {
    if (config.dumpIncomingResponses)
    {
      logdbg("%sReceived WebSocket binary (compressed) message: Size: %zu bytes", logPrefix.c_str(), payload.size());
    }

    eastl::string errorDetails;
    optionalStorageForDecompressedPayload = details::decompress_binary_message(payload, config, errorDetails);

    if (!errorDetails.empty())
    {
      logwarn("%sWARN! Received binary message from the server and failed decompression; Size: %zu bytes; error: %s",
        logPrefix.c_str(), payload.size(), errorDetails.c_str());
      onFailedProcessing();
      return eastl::nullopt;
    }

    decompressedPayloadView =
      eastl::string_view{optionalStorageForDecompressedPayload.data.get(), optionalStorageForDecompressedPayload.size};
  }

  auto event = parseDecompressedMessage(decompressedPayloadView, payload.size(), config, logPrefix);
  if (!event.has_value())
  {
    // Logging is already done in parseDecompressedMessage
    onFailedProcessing();
    return eastl::nullopt;
  }

  return event;
}


eastl::optional<WsJsonRpcConnection::Event> WsJsonRpcConnection::parseDecompressedMessage(eastl::string_view payload,
  size_t rawPayloadSize, const WsJsonRpcConnectionConfig &config, const eastl::string &logPrefix)
{
  rapidjson::Document doc;
  doc.Parse(payload.data(), payload.size());
  if (doc.HasParseError())
  {
    const char *const errorDetails = rapidjson::GetParseError_En(doc.GetParseError());
    logerr("%s[sep_error] ERROR! Failed parsing JSON message from WebSocket server! "
           "Size: %zu bytes (raw size: %zu); Error offset: %zu; Error details: %s",
      logPrefix.c_str(), payload.size(), rawPayloadSize, doc.GetErrorOffset(), errorDetails);
    return eastl::nullopt;
  }

  if (!doc.IsObject())
  {
    logerr("%s[sep_error] ERROR! Incoming message: invalid root element type", logPrefix.c_str());
    return eastl::nullopt;
  }

  // Distinguish between request/notification and response messages
  const auto iterMethod = doc.FindMember(protocol::METHOD);
  if (iterMethod != doc.MemberEnd())
  {
    return parseIncomingRequestMessage(payload, eastl::move(doc), config, logPrefix);
  }

  const auto iterMessageId = doc.FindMember(protocol::MESSAGE_ID);
  if (iterMessageId == doc.MemberEnd())
  {
    logerr("%s[sep_error] ERROR! Incoming message: missing message id for response message", logPrefix.c_str());
    return eastl::nullopt;
  }

  eastl::optional<RpcMessageId> messageId;

  if (iterMessageId->value.IsInt64())
  {
    const auto messageIdRawValue = iterMessageId->value.GetInt64();
    static_assert(sizeof(messageIdRawValue) == sizeof(RpcMessageId));
    static_assert(std::is_signed_v<decltype(messageIdRawValue)> == std::is_signed_v<RpcMessageId>);
    messageId = messageIdRawValue;
  }
  else if (iterMessageId->value.IsNull())
  {
    messageId = eastl::nullopt;
  }
  else
  {
    logerr("%s[sep_error] ERROR! Incoming message: unexpected message id type (got type %s instead of Int64 or null)",
      logPrefix.c_str(), to_string(iterMessageId->value.GetType()));
    return eastl::nullopt;
  }

  ResponseFromServerEvent event{messageId, eastl::move(doc), payload.size()};
  const RpcResponse &response = *event.response;

  if (response.isError())
  {
    if (config.dumpIncomingErrorResponses || config.dumpIncomingResponses)
    {
      logwarn("%sWARN! Dump: Received JSON RPC error: message id: %lld; "
              "Code: %d; Desc: %s; Size: %zu bytes (raw size: %zu); Raw data = [%s]",
        logPrefix.c_str(), (long long)messageId.value_or(-1), response.getErrorCode(), response.getErrorMessage(), payload.size(),
        rawPayloadSize, truncate_payload(payload, config.maxBytesToDump).c_str());
    }
  }
  else if (config.dumpIncomingResponses)
  {
    logdbg("%sDump: Received JSON RPC response: message id: %lld; Size: %zu bytes (raw size: %zu); Raw data = [%s]", logPrefix.c_str(),
      (long long)messageId.value_or(-1), payload.size(), rawPayloadSize, truncate_payload(payload, config.maxBytesToDump).c_str());
  }

  if (config.logIncomingResponseEvents)
  {
    logdbg("%sGot response for RPC message id %lld; Size: %zu bytes (raw size: %zu)", logPrefix.c_str(),
      (long long)messageId.value_or(-1), payload.size(), rawPayloadSize);
  }

  return Event{eastl::move(event)};
}


static const eastl::string &get_readable_message_id(const RpcRequest &request)
{
  if (request.isNotification())
  {
    return NO_MESSAGE_ID;
  }
  return request.getSerializedMessageId();
}


eastl::optional<WsJsonRpcConnection::Event> WsJsonRpcConnection::parseIncomingRequestMessage(eastl::string_view payload,
  rapidjson::Document &&document, const WsJsonRpcConnectionConfig &config, const eastl::string &logPrefix)
{
  RequestFromServerEvent event{eastl::move(document), payload.size()};
  const RpcRequest &incomingRequest = *event.request;

  if (config.dumpIncomingRequests)
  {
    logdbg("%sDump: Received JSON RPC request/notification: "
           "message id: %s; method: %s; Size: %zu bytes; Raw data = [%s]",
      logPrefix.c_str(), get_readable_message_id(incomingRequest).c_str(), incomingRequest.getMethod().c_str(), payload.size(),
      truncate_payload(payload, config.maxBytesToDump).c_str());
  }
  if (config.logIncomingRequestEvents)
  {
    logdbg("%sReceived JSON RPC request/notification: message id: %s; method: %s; Size: %zu bytes", logPrefix.c_str(),
      get_readable_message_id(incomingRequest).c_str(), incomingRequest.getMethod().c_str(), payload.size());
  }

  if (!incomingRequest.isValid())
  {
    logwarn("%sWARN! Received JSON RPC request/notification was found invalid (see details above in the log)", logPrefix.c_str());
    return eastl::nullopt;
  }

  return Event{eastl::move(event)};
}


void WsJsonRpcConnection::sendResponseResult(eastl::string_view serialized_message_id, const rapidjson::Document &rpc_result)
{
  const eastl::string_view formattedMessageId = serialized_message_id.empty() ? "<NO_MESSAGE_ID>" : serialized_message_id;
  if (!isConnected() || serialized_message_id.empty())
  {
    logwarn("%sWARN! Incorrect client state or message id: skip sending response result for RPC message id %.*s", logPrefix.c_str(),
      (int)formattedMessageId.size(), formattedMessageId.data());
    return;
  }

  Buffer buffer;
  JsonWriter writer(buffer);
  {
    writer.StartObject();

    writer.Key(protocol::VERSION);
    writer.String(protocol::VERSION_VALUE);

    writer.Key(protocol::MESSAGE_ID);
    writer.RawValue(serialized_message_id.data(), serialized_message_id.size(), rapidjson::Type::kStringType);

    writer.Key(protocol::RESULT);
    G_VERIFY(rpc_result.Accept(writer));

    writer.EndObject();
  }

  G_ASSERT(writer.IsComplete());

  const eastl::string_view bufferView{buffer.GetString(), buffer.GetSize()};
  const websocket::PayloadView wsBufferView{reinterpret_cast<const uint8_t *>(bufferView.data()), bufferView.size()};

  if (config.logOutgoingResponseEvents)
  {
    logdbg("%sSending response result with RPC message id %.*s: Size: %zu bytes", logPrefix.c_str(), (int)formattedMessageId.size(),
      formattedMessageId.data(), bufferView.size());
  }

  constexpr bool isText = true;

  if (config.dumpOutgoingResponses)
  {
    logdbg("%sDump: Sending response result with RPC message id %.*s: Size: %zu bytes; isText: %d; Raw data = [%s]", logPrefix.c_str(),
      (int)formattedMessageId.size(), formattedMessageId.data(), bufferView.size(), isText,
      truncate_payload(bufferView, config.maxBytesToDump).c_str());
  }

  G_ASSERT_RETURN(wsClient, );
  wsClient->send(wsBufferView, isText);
}


void WsJsonRpcConnection::sendResponseError(eastl::string_view serialized_message_id, int error_code, eastl::string_view error_message,
  eastl::string_view error_context)
{
  const eastl::string_view formattedMessageId = serialized_message_id.empty() ? "<NO_MESSAGE_ID>" : serialized_message_id;
  if (!isConnected() || serialized_message_id.empty())
  {
    logwarn("%sWARN! Incorrect client state or message id: skip sending response error for RPC message id %.*s: "
            "Error code: %d; Error message: %.*s; Context: %.*s",
      logPrefix.c_str(), (int)formattedMessageId.size(), formattedMessageId.data(), error_code, (int)error_message.size(),
      error_message.data(), (int)error_context.size(), error_context.data());
    return;
  }

  Buffer buffer;
  JsonWriter writer(buffer);
  {
    writer.StartObject();

    writer.Key(protocol::VERSION);
    writer.String(protocol::VERSION_VALUE);

    writer.Key(protocol::MESSAGE_ID);
    writer.RawValue(serialized_message_id.data(), serialized_message_id.size(), rapidjson::Type::kStringType);

    writer.Key(protocol::ERROR);
    {
      writer.StartObject();

      writer.Key(protocol::ERROR_CODE);
      writer.Int(error_code);

      writer.Key(protocol::ERROR_MESSAGE);
      writer.String(error_message.data(), error_message.size());

      writer.Key(protocol::ERROR_DATA);
      {
        writer.StartObject();

        writer.Key(protocol::ERROR_DATA_CONTEXT);
        writer.String(error_context.data(), error_context.size());

        writer.EndObject();
      }

      writer.EndObject();
    }

    writer.EndObject();
  }

  G_ASSERT(writer.IsComplete());

  const eastl::string_view bufferView{buffer.GetString(), buffer.GetSize()};
  const websocket::PayloadView wsBufferView{reinterpret_cast<const uint8_t *>(bufferView.data()), bufferView.size()};

  if (config.logOutgoingResponseEvents)
  {
    logdbg("%sSending response error with RPC message id %.*s: "
           "Error code: %d; Error message: %.*s; Context: %.*s; Size: %zu bytes",
      logPrefix.c_str(), (int)formattedMessageId.size(), formattedMessageId.data(), error_code, (int)error_message.size(),
      error_message.data(), (int)error_context.size(), error_context.data(), bufferView.size());
  }

  constexpr bool isText = true;

  if (config.dumpOutgoingResponses || config.dumpOutgoingErrorResponses)
  {
    logdbg("%sDump: Sending response error with RPC message id %.*s: "
           "Error code: %d; Error message: %.*s; Context: %.*s; Size: %zu bytes; isText: %d; Raw data = [%s]",
      logPrefix.c_str(), (int)formattedMessageId.size(), formattedMessageId.data(), error_code, (int)error_message.size(),
      error_message.data(), (int)error_context.size(), error_context.data(), bufferView.size(), isText,
      truncate_payload(bufferView, config.maxBytesToDump).c_str());
  }

  G_ASSERT_RETURN(wsClient, );
  wsClient->send(wsBufferView, isText);
}


void WsJsonRpcConnection::onEvent(ConnectionSucceededEvent &&event)
{
  logdbg("%sWebSocket connection to %s was established! (actual IP: %s)", logPrefix.c_str(), config.serverUrl.c_str(),
    (event.actualUrlIp.empty() ? "unknown" : event.actualUrlIp.c_str()));
  verifyState(state == State::CONNECTING || state == State::CLOSING, "onEvent(ConnectionSucceededEvent)");

  onConnectTimeTick = abstraction::get_current_tick_count();
  actualUrlIp = eastl::move(event.actualUrlIp);

  if (state == State::CLOSING)
  {
    // This is a rare case.
    // initiateClose() was called during CONNECTING state.
    // And the next poll() was called only after the connection was established.
    logdbg("%sGot ConnectionSucceeded event, but we are already in CLOSING state."
           " So keep the current state and expect Disconnect event soon.",
      logPrefix.c_str());
    return;
  }

  if (config.waitForAuthenticationResponse)
  {
    logdbg("%sStarting authentication process", logPrefix.c_str());
    switchState(State::CONNECTING, State::AUTHENTICATING);
    if (delayedAuthenticationResponse)
    {
      processAuthenticationResult(eastl::move(delayedAuthenticationResponse));
      delayedAuthenticationResponse.reset();
    }
    return;
  }

  switchState(State::CONNECTING, State::CONNECTED);
}


void WsJsonRpcConnection::onEvent(eastl::unique_ptr<ConnectionFailedEvent> &&event)
{
  G_ASSERT_RETURN(event, );
  logwarn("%sWARN! Failed to connect to WebSocket server %s! (actual IP: %s); %s", logPrefix.c_str(), config.serverUrl.c_str(),
    (event->actualUrlIp.empty() ? "unknown" : event->actualUrlIp.c_str()), event->error.message.c_str());
  verifyState(state == State::CONNECTING || state == State::CLOSING, "onEvent(ConnectionFailedEvent)");

  const int reasonErrorCode = event->error.code;

  onConnectTimeTick = abstraction::get_current_tick_count();
  actualUrlIp = eastl::move(event->actualUrlIp);
  lastError = eastl::move(event->error);

  if (state != State::CLOSING)
  {
    switchState(state, State::CLOSING);
  }

  notifyAllCallbacks(AdditionalErrorCodes::LOST_WEBSOCKET_CONNECTION_AFTER_SENDING, "Failed to connect to WebSocket server",
    reasonErrorCode);
  switchState(State::CLOSING, State::CLOSED);
}


void WsJsonRpcConnection::onEvent(eastl::unique_ptr<DisconnectEvent> &&event)
{
  G_ASSERT_RETURN(event, );
  if (state == State::CLOSING)
  {
    logdbg("%sWebSocket connection was closed by our side (close status: %d); %d", logPrefix.c_str(), static_cast<int>(event->status),
      event->error.message.c_str());
  }
  else
  {
    logwarn("%sWARN! WebSocket connection was found unexpectedly closed/lost (close status: %d); %s", logPrefix.c_str(),
      static_cast<int>(event->status), event->error.message.c_str());
  }

  const int reasonErrorCode = event->error.code;

  onCloseTimeTick = abstraction::get_current_tick_count();
  if (lastError.code == 0) // keep previous authentication error if any
  {
    lastError = eastl::move(event->error);
  }

  verifyState(state == State::AUTHENTICATING || state == State::CONNECTED || state == State::CLOSING, "onEvent(DisconnectEvent)");

  if (state != State::CLOSING)
  {
    switchState(state, State::CLOSING);
  }

  notifyAllCallbacks(AdditionalErrorCodes::LOST_WEBSOCKET_CONNECTION_AFTER_SENDING,
    "WebSocket connection with the server was closed/lost", reasonErrorCode);
  switchState(State::CLOSING, State::CLOSED);
}


void WsJsonRpcConnection::onEvent(eastl::unique_ptr<CannotMakeRpcCallEvent> &&event)
{
  G_ASSERT_RETURN(event, );
  const eastl::string errorMessage(eastl::string::CtorSprintf{},
    "Cannot make JSON RPC call in this WsJsonRpcConnection state (method: %s)", event->method.c_str());

  auto response =
    eastl::make_unique<RpcResponse>(event->messageId, AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY, errorMessage.c_str());

  G_ASSERT_RETURN(event->callback, );
  event->callback(eastl::move(response));
}


void WsJsonRpcConnection::onEvent(RequestFromServerEvent &&event)
{
  G_ASSERT_RETURN(event.request, );

  if (incomingRequestCallback)
  {
    incomingRequestCallback(eastl::move(event.request));
    return;
  }

  logdbg("%sIncoming request/notification messages from server are not supported", logPrefix.c_str());
  if (config.dumpIncomingRequests)
  {
    const bool isText = true; // non-text requests are dropped earlier before generating RequestFromServerEvent
    logdbg("%sDump: Failed processing received WebSocket request from server: Size: %zu bytes; isText: %d", logPrefix.c_str(),
      event.request->getPayloadSize(), isText);
  }
}


void WsJsonRpcConnection::processAuthenticationResult(RpcResponsePtr &&response)
{
  G_ASSERT(config.waitForAuthenticationResponse);
  G_ASSERT(state == State::AUTHENTICATING);

  authenticationFailed = response->isError();
  if (authenticationFailed)
  {
    lastError.code = response->getErrorCode();
    lastError.message = response->getErrorMessage();

    logwarn("%s[sep_error] WARNING! Authentication process failed! Error code: %d; Error message: %s", logPrefix.c_str(),
      response->getErrorCode(), lastError.message.c_str());
    initiateClose();
  }
  else
  {
    logdbg("%sAuthentication process finished successfully!", logPrefix.c_str());
    switchState(State::AUTHENTICATING, State::CONNECTED);
  }
}


void WsJsonRpcConnection::onEvent(ResponseFromServerEvent &&event)
{
  G_ASSERT_RETURN(event.response, );

  if (!event.messageId.has_value())
  {
    if (config.waitForAuthenticationResponse && state == State::CONNECTING)
    {
      logdbg("%sGot early authentication response. Save it for later use; Size: %zu bytes", logPrefix.c_str(),
        event.response->getPayloadSize());
      delayedAuthenticationResponse = eastl::move(event.response);
      return;
    }
    else if (config.waitForAuthenticationResponse && state == State::AUTHENTICATING)
    {
      processAuthenticationResult(eastl::move(event.response));
      return;
    }

    logdbg("%sIncoming message: 'null' message id was found; Size: %zu bytes", logPrefix.c_str(), event.response->getPayloadSize());
    return;
  }

  if (ResponseCallback callback = callbacks.extractCallback(*event.messageId); callback)
  {
    callback(eastl::move(event.response));
  }
  else
  {
    logdbg("%sIncoming message: no callback was found for RPC message id %lld; Size: %zu bytes", logPrefix.c_str(),
      (long long)event.messageId.value_or(-1), event.response->getPayloadSize());
  }
}


void WsJsonRpcConnection::onEvent(InitiateCloseEvent &&)
{
  if (state == State::CLOSING)
  {
    logdbg("%sClose WebSocket connection due to previous initiate close call", logPrefix.c_str());

    G_ASSERT_RETURN(wsClient, );
    // Potentially application callbacks can be called here (maybe in Xbox, but I'm not sure):
    wsClient->close(websocket::CloseStatus::Normal);
  }
  else if (state == State::CLOSED)
  {
    logdbg("%sSkip closing WebSocket connection: it was already closed", logPrefix.c_str());
  }
  else
  {
    verifyState(false, "onEvent(InitiateCloseEvent)");
  }
}


void WsJsonRpcConnection::poll()
{
  if (state == State::INITIAL)
  {
    onStateInitial(); // state is always switched to CONNECTING here
  }

  if (state != State::CLOSED)
  {
    G_ASSERT_RETURN(wsClient, );
    wsClient->poll(); // here low-level callbacks can be called, all events are added to the queue

    processTimeouts(); // application callbacks may be called here
  }
  else
  {
    G_ASSERT(callbacks.isEmpty()); // i.e. we already called notifyAllCallbacks or no RPC calls were made
  }

  Event event;
  while (extractEvent(event))
  {
    // application callbacks may be called here:
    eastl::visit([this](auto &&event) { onEvent(eastl::move(event)); }, eastl::move(event));
  }
}


int64_t WsJsonRpcConnection::getConnectionEstablishTimeMs() const
{
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
  const TickCount elapsedTicks = onConnectTimeTick - createTimeTick;
  return elapsedTicks <= 0 ? 0 : elapsedTicks * 1000 / ticksPerSecond;
}


int64_t WsJsonRpcConnection::getConnectionLifetimeMs() const
{
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
  const TickCount elapsedTicks = onCloseTimeTick - onConnectTimeTick;
  return elapsedTicks <= 0 ? 0 : elapsedTicks * 1000 / ticksPerSecond;
}


} // namespace websocketjsonrpc
