// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/sepClient.h>

#include "sepClientVersion.h"

#include <websocketJsonRpc/details/abstractionLayer.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>

#include <EASTL/atomic.h>


namespace sepclient
{

namespace abstraction = websocketjsonrpc::abstraction;


eastl::atomic<RpcMessageId> next_rpc_message_id = 0;


static void initialize_rpc_message_id()
{
  if (next_rpc_message_id == 0)
  {
    // 1e9 start positions with step 10K (easy to debug, not too long numbers)
    static_assert(sizeof(RpcMessageId) >= 8);
    constexpr int64_t startPositionCount = 1'000'000'000LL;
    const RpcMessageId initialValue = (startPositionCount + abstraction::generate_random_uint64() % startPositionCount) * 10'000LL + 1;

    G_ASSERT(initialValue > 0); // do not use zero value just in case
    // leave enough values not to get overflow and negative numbers in case of many requests:
    G_ASSERT(initialValue <= std::numeric_limits<sepclient::RpcMessageId>::max() / 2);

    next_rpc_message_id = initialValue;
  }
}


static details::RpcContextSettings generate_rpc_context_settings(const SepClientConfig &sep_config)
{
  details::RpcContextSettings contextSettings;

  contextSettings.requestTimeoutMs = eastl::max(sep_config.requestTimeoutMs, 0);

  // It is a bad idea to make more than 2 tries (1 retry)
  // because it can lead to traffic multiplication in case of fast consecutive disconnects:
  contextSettings.maxExecutionTries = sep_config.enableRequestRetryOnDisconnect ? 2 : 1;

  contextSettings.extraVerboseLogging = (sep_config.logLevel == LogLevel::FULL_DUMP);

  return contextSettings;
}


static TickCount get_request_timeout_without_network_ticks(const SepClientConfig &config_)
{
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests

  const TickCount requestSendTimeoutTicks =
    details::RpcContext::get_send_expiration_duration(config_.requestTimeoutMs * ticksPerSecond / 1000);

  return config_.requestTimeoutWithoutNetworkMs >= 0
           ? eastl::min(config_.requestTimeoutWithoutNetworkMs * ticksPerSecond / 1000, requestSendTimeoutTicks)
           : requestSendTimeoutTicks;
}


void SepClient::set_ca_bundle(eastl::string_view ca_bundle_path)
{
  websocketjsonrpc::WsJsonRpcPersistentConnection::set_ca_bundle(ca_bundle_path);
}


SepClient::SepClient(const SepClientConfig &config_, PrivateConstructorTag) :
  logPrefix("[sep][sep_client] "),
  logLevel(config_.logLevel),
  connection(websocketjsonrpc::WsJsonRpcPersistentConnection::create()),
  defaultContextSettings(generate_rpc_context_settings(config_)),
  maxSimultaneousRequests(eastl::max(config_.maxSimultaneousRequests, 1)),
  connectionConfigGenerator(config_),
  delayedRpcCalls(get_request_timeout_without_network_ticks(config_))
{
  G_ASSERT(connection);

  if (isDebugLog())
  {
    logdbg("%sSepClient::SepClient() called", logPrefix.c_str());
  }

  logdbg("%sSepClient version: %s [%s], release date %s", logPrefix.c_str(), sepclient::SEP_VERSION,
    sepclient::SEP_VERSION_DESCRIPTION, sepclient::SEP_VERSION_RELEASE_DATE);
  logdbg("%sSepClient: HTTP UserAgent: %s", logPrefix.c_str(), connectionConfigGenerator.getUserAgent().c_str());
}


SepClient::~SepClient()
{
  if (isDebugLog())
  {
    logdbg("%sSepClient::~SepClient() called", logPrefix.c_str());
  }

  const std::scoped_lock lock(mutex);
  if (state != State::INITIAL && state != State::INITIALIZED && state != State::CLOSED)
  {
    logwarn("%sWARN! SepClient object was not closed properly! Network connection may be hard closed. (state: %s)", logPrefix.c_str(),
      getStateName(state));
  }
}


const char *SepClient::getStateName(State state_value)
{
  switch (state_value)
  {
    case State::INITIAL: return "INITIAL";
    case State::INITIALIZED: return "INITIALIZED";
    case State::WORKING: return "WORKING";
    case State::CLOSING: return "CLOSING";
    case State::CLOSED: return "CLOSED";
    default: return "UNKNOWN_STATE";
  }
}


void SepClient::switchState(State previous_state, State next_state)
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


void SepClient::initialize(AuthenticationFailureCallback &&authentication_failure_callback,
  IncomingNotificationCallback &&notification_callback, const SepClientStatsCallbackPtr &stats_callback)
{
  if (isDebugLog())
  {
    logdbg("%sinitialize() called", logPrefix.c_str());
  }

  initialize_rpc_message_id();

  const std::scoped_lock lock(mutex);

  G_ASSERT_RETURN(authentication_failure_callback, );
  authenticationFailureCallback = eastl::move(authentication_failure_callback);

  const auto weakSelf = weak_from_this();

  auto configGenerator = [weakSelf](const websocketjsonrpc::WsJsonRpcConnectionPtr &optional_failed_connection,
                           int connect_retry_number) -> websocketjsonrpc::WsJsonRpcConnectionConfigPtr {
    const auto sharedSelf = weakSelf.lock();
    if (!sharedSelf)
    {
      logerr("[sep][sep_client][sep_error] ERROR! SepClient object was found destroyed before calling configGenerator!");
      return nullptr;
    }

    return sharedSelf->generateConfig(optional_failed_connection, connect_retry_number);
  };

  notificationCallback = eastl::move(notification_callback);

  statsCallback = stats_callback;

  websocketjsonrpc::WsJsonRpcPersistentConnection::IncomingRequestCallback lowLevelNotificationCallback;

  if (notificationCallback)
  {
    lowLevelNotificationCallback = [weakSelf](websocketjsonrpc::RpcRequestPtr &&incoming_request) {
      G_ASSERT_RETURN(incoming_request, );

      const auto sharedSelf = weakSelf.lock();
      if (!sharedSelf)
      {
        logerr("[sep][sep_client][sep_error] ERROR! SepClient object was found destroyed before calling notificationCallbackWrapper!");
        return;
      }

      if (!incoming_request->isNotification())
      {
        if (sharedSelf->isInfoLog())
        {
          logwarn("%sWARN! Incoming JSON RPC request is not a notification! (JSON RPC method: %s)", sharedSelf->logPrefix.c_str(),
            incoming_request->getMethod().c_str());
        }
        return;
      }

      // Delay response processing so that we can unlock the mutex before calling the callback
      sharedSelf->incomingDataQueue.emplace_back(IncomingRequestData{eastl::move(incoming_request)});
    };
  }

  connection->initialize(eastl::move(configGenerator), eastl::move(lowLevelNotificationCallback), statsCallback);

  switchState(State::INITIAL, State::INITIALIZED);
}


websocketjsonrpc::WsJsonRpcConnectionConfigPtr SepClient::generateConfig(
  const websocketjsonrpc::WsJsonRpcConnectionPtr &optional_failed_connection, int connect_retry_number)
{
  const bool failedConnectionIsChanged = optional_failed_connection != previousOptionalFailedConnection;
  if (failedConnectionIsChanged)
  {
    previousOptionalFailedConnection = optional_failed_connection;
  }

  // The same failed connection may be provided numerous numbers of times until next connection is generated.
  if (optional_failed_connection && failedConnectionIsChanged)
  {
    const bool isAuthenticationError = optional_failed_connection->isAuthenticationError();
    if (isAuthenticationError)
    {
      const websocket::Error &lastError = optional_failed_connection->getLastError();
      const auto &failedConfig = optional_failed_connection->getConfig();

      logwarn("%sWARN! Got authentication error while connecting to the SEP server: %s; %s", logPrefix.c_str(),
        failedConfig.serverUrl.c_str(), lastError.message.c_str());

      eastl::string failedAuthToken = details::ConnectionConfigGenerator::extractAuthorizationToken(failedConfig);

      // Reset current authorization token not to be used again;
      const bool tokenWasReset = connectionConfigGenerator.resetAuthorizationTokenIfTheSame(failedAuthToken);
      if (!tokenWasReset)
      {
        logdbg("%sSkipped resetting authorization token (it was previously changed)", logPrefix.c_str());
      }
      else
      {
        incomingDataQueue.emplace_back(
          eastl::make_unique<AuthenticationFailure>(eastl::move(failedAuthToken), failedConfig.serverUrl, lastError.message));
      }
    }
  }

  websocketjsonrpc::WsJsonRpcConnectionConfigPtr cfg =
    connectionConfigGenerator.generate(optional_failed_connection, connect_retry_number);
  if (!cfg)
  {
    // most likely we have no SEP servers or authorization token (SSO JWT) is empty
    return nullptr;
  }

  if (isDebugLog())
  {
    logdbg("%sgenerateConfig() created new SEP connection config", logPrefix.c_str());
  }

  return cfg;
}


bool SepClient::updateAuthorizationToken(eastl::string_view new_authorization_token)
{
  if (isDebugLog())
  {
    logdbg("%supdateAuthorizationToken() called", logPrefix.c_str());
  }

  const std::scoped_lock lock(mutex);
  const bool tokenWasUpdated = connectionConfigGenerator.updateAuthorizationToken(new_authorization_token);

  if (!tokenWasUpdated)
  {
    // Applied authorization token is the same as previous. Nothing to do
    return false;
  }

  if (new_authorization_token.empty())
  {
    logwarn("%sWARN! New authorization token is empty!"
            " Connection to SEP server will be stopped until missing token is provided to the SepClient",
      logPrefix.c_str());
  }
  else
  {
    logdbg("%sGot new authorization token for the SepClient", logPrefix.c_str());
  }

  connection->initiateGracefulReconnect();
  return true;
}


RpcMessageId SepClient::generateMessageId() { return next_rpc_message_id++; }


TransactionId SepClient::generateTransactionId()
{
  const uint64_t randomValue = abstraction::generate_random_uint64();

  static_assert(std::numeric_limits<TransactionId>::max() == std::numeric_limits<uint64_t>::max() / 2);
  const TransactionId transactionId = randomValue / 2;
  return transactionId ? transactionId : 1; // ensure that we never return zero
}


RpcRequest SepClient::createRpcRequest(eastl::string_view service, eastl::string_view action,
  eastl::optional<sepclient::RpcMessageId> message_id, eastl::optional<TransactionId> optional_transaction_id)
{
  const RpcMessageId messageId = message_id.has_value() ? *message_id : generateMessageId();
  G_ASSERT(messageId > 0);

  const TransactionId transactionId = optional_transaction_id.has_value() ? *optional_transaction_id : generateTransactionId();
  G_ASSERT(transactionId > 0);

  RpcRequest request(messageId, transactionId, service, action);
  return request;
}


void SepClient::rpcCall(const RpcRequest &request, ResponseCallback &&response_callback)
{
  if (isDebugLog())
  {
    logdbg("%srpcCall() called", logPrefix.c_str());
  }

  G_ASSERT_RETURN(response_callback, );

  details::RpcContextPtr context =
    eastl::make_unique<details::RpcContext>(request, eastl::move(response_callback), defaultContextSettings);

  const std::scoped_lock lock(mutex);
  executeRpcCall(eastl::move(context));
}


void SepClient::onLowerLevelResponse(details::RpcContextPtr &&context, websocketjsonrpc::RpcResponsePtr &&response)
{
  G_ASSERT_RETURN(response, );
  G_ASSERT_RETURN(context, );

  using namespace websocketjsonrpc;

  const auto errorCodeInt = response->isError() ? response->getErrorCode() : 0;
  const auto errorCode = static_cast<websocketjsonrpc::AdditionalErrorCodes>(errorCodeInt);

  if (isDebugLog())
  {
    logdbg("%sonLowerLevelResponse() called: errorCode: %d; context: %s", logPrefix.c_str(), static_cast<int>(errorCodeInt),
      context->formatState().c_str());
  }

  if (errorCode == AdditionalErrorCodes::RPC_CALL_TIMEOUT)
  {
    // this error is generated by WsJsonRpcConnection locally when it could not get response in time
    logwarn("%s[sep_error] WARNING! Timeout while waiting for JSON RPC response from WebSocket; context: %s", logPrefix.c_str(),
      context->formatState().c_str());
  }
  else if (errorCode == AdditionalErrorCodes::LOST_WEBSOCKET_CONNECTION_AFTER_SENDING)
  {
    logdbg("%sConnection was lost after request was sent to server; context: %s", logPrefix.c_str(), context->formatState().c_str());

    if (context->getRemainingTimeBeforeSendMs() > 0)
    {
      details::report_stats_event(statsCallback, details::StatsEventType::REQUEST_RETRY_CAUSED_BY_NETWORK, context.get(),
        errorCodeInt);
      delayRpcCall(eastl::move(context), "lost WebSocket connection");
      return;
    }
  }

  if (response->isError())
  {
    if (errorCode == AdditionalErrorCodes::RPC_CALL_TIMEOUT)
    {
      details::report_stats_event(statsCallback, details::StatsEventType::REQUEST_FAIL_DUE_TO_TIMEOUT, context.get(), errorCodeInt);
    }
    else if (errorCode == AdditionalErrorCodes::LOST_WEBSOCKET_CONNECTION_AFTER_SENDING)
    {
      const int reasonErrorCode = response->getInternalReasonErrorCode();
      details::report_stats_event(statsCallback, details::StatsEventType::REQUEST_FAIL_DUE_TO_NETWORK, context.get(), reasonErrorCode);
    }
    else
    {
      details::report_stats_event(statsCallback, details::StatsEventType::REQUEST_ERROR_RPC_RESPONSE, context.get(), errorCodeInt);
    }
  }
  else
  {
    details::report_stats_event(statsCallback, details::StatsEventType::REQUEST_SUCCESS, context.get(), details::NO_ERROR);
  }

  // Delay response processing so that we can unlock the mutex before calling the callback
  incomingDataQueue.emplace_back(IncomingResponseData{eastl::move(context), eastl::move(response)});
}


void SepClient::executeRpcCall(details::RpcContextPtr &&context)
{
  G_ASSERT_RETURN(context, );

  if (state == State::INITIAL)
  {
    abandonRpcCall(eastl::move(context), websocketjsonrpc::AdditionalErrorCodes::CLIENT_LIBRARY_IS_NOT_INITIALIZED,
      "Cannot execute JSON RPC call: need to initialize SepClient first!");
    return;
  }

  if (!connection->isCallable())
  {
    delayRpcCall(eastl::move(context), "missing WebSocket connection");
    return;
  }
  if (numberOfExecutingRequests >= maxSimultaneousRequests)
  {
    delayRpcCall(eastl::move(context), "too much JSON RPC requests in progress");
    return;
  }

  const auto weakSelf = weak_from_this();

  G_ASSERT(numberOfExecutingRequests >= 0);
  numberOfExecutingRequests++;
  G_ASSERT(numberOfExecutingRequests <= maxSimultaneousRequests);

  context->onBeforeCall();

  if (isInfoLog())
  {
    logdbg("%sExecuting RPC call; numberOfExecutingRequests: %d / %d; context: %s", logPrefix.c_str(), numberOfExecutingRequests,
      maxSimultaneousRequests, context->formatState().c_str());
  }

  const RpcMessageId messageId = context->getMessageId();
  const eastl::string_view method = context->getRpcMethod();
  const eastl::string_view rpcParamsInJsonFormat = context->getRpcParamsInJsonFormat();
  const int remainingTimeMs = context->getRemainingReceiveTimeMs();

  auto lowerLevelResponseCallback = [weakSelf, context = eastl::move(context)](websocketjsonrpc::RpcResponsePtr &&response) mutable {
    const auto sharedSelf = weakSelf.lock();
    if (!sharedSelf)
    {
      logerr("[sep][sep_client][sep_error] ERROR! SepClient object was found destroyed before calling ResponseCallback!");
      return;
    }

    G_ASSERT(sharedSelf->numberOfExecutingRequests <= sharedSelf->maxSimultaneousRequests);
    sharedSelf->numberOfExecutingRequests--;
    G_ASSERT(sharedSelf->numberOfExecutingRequests >= 0);

    sharedSelf->onLowerLevelResponse(eastl::move(context), eastl::move(response));
  };

  G_ASSERT(connection->isCallable());
  connection->rpcCall(messageId, method, rpcParamsInJsonFormat, eastl::move(lowerLevelResponseCallback), remainingTimeMs);
}


void SepClient::delayRpcCall(details::RpcContextPtr &&context, eastl::string_view delay_reason)
{
  G_ASSERT_RETURN(context, );

  const int delayMs = context->getRemainingTimeBeforeSendMs();

  if (delayMs <= 0)
  {
    abandonRpcCall(eastl::move(context), websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT,
      "JSON RPC call skipped: not enough time or tries remaining for its execution");
    return;
  }

  if (state == State::CLOSING || state == State::CLOSED)
  {
    abandonRpcCall(eastl::move(context), websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY,
      "Cannot execute JSON RPC call: SEP client is closing or closed");
    return;
  }

  if (isInfoLog())
  {
    logdbg("%sDelay RPC call to queue: %.*s; numberOfExecutingRequests: %d / %d; max delay: %d ms; context: %s", logPrefix.c_str(),
      (int)delay_reason.size(), delay_reason.data(), numberOfExecutingRequests, maxSimultaneousRequests, delayMs,
      context->formatState().c_str());
  }

  delayedRpcCalls.addRpc(eastl::move(context));
}


void SepClient::abandonRpcCall(details::RpcContextPtr &&context, websocketjsonrpc::AdditionalErrorCodes error_code,
  eastl::string_view error_description)
{
  G_ASSERT_RETURN(context, );

  logwarn("%s[sep_error] WARNING! %.*s; context: %s", logPrefix.c_str(), (int)error_description.size(), error_description.data(),
    context->formatState().c_str());

  const eastl::string responseErrorDescription(eastl::string::CtorSprintf{}, "SepClient: %.*s", (int)error_description.size(),
    error_description.data());

  RpcResponsePtr errorResponse =
    eastl::make_unique<websocketjsonrpc::RpcResponse>(context->getMessageId(), error_code, responseErrorDescription);

  // Delay response processing so that we can unlock the mutex before calling the callback
  incomingDataQueue.emplace_back(IncomingResponseData{eastl::move(context), eastl::move(errorResponse)});
}


void SepClient::processIncomingData(IncomingRequestData &&data) const
{
  auto request = eastl::move(data.request);
  G_ASSERT_RETURN(request, );

  G_ASSERT_RETURN(notificationCallback, );
  // user-provided callback call is using stack data only:
  notificationCallback(eastl::move(request));
}


void SepClient::processIncomingData(IncomingResponseData &&data)
{
  details::RpcContextPtr context = eastl::move(data.context);
  websocketjsonrpc::RpcResponsePtr response = eastl::move(data.response);

  G_ASSERT_RETURN(context, );
  G_ASSERT_RETURN(response, );

  // user-provided callback call is using stack data only:
  context->executeCallback(eastl::move(response));
}


void SepClient::processIncomingData(AuthenticationFailurePtr &&original_data) const
{
  AuthenticationFailurePtr data = eastl::move(original_data);
  G_ASSERT_RETURN(data, );
  G_ASSERT_RETURN(authenticationFailureCallback, );

  // user-provided callback call is using stack data only:
  authenticationFailureCallback(eastl::move(data));
}


void SepClient::poll()
{
  if (isDebugLog())
  {
    // logdbg("%spoll() called", logPrefix.c_str());
  }

  std::unique_lock lock(mutex);
  if (state == State::INITIAL)
  {
    G_ASSERT(delayedRpcCalls.empty());
    return;
  }
  else if (state == State::INITIALIZED)
  {
    switchState(State::INITIALIZED, State::WORKING);

    if (!connectionConfigGenerator.haveEnoughConnectionParams())
    {
      logwarn("%sWARN! Missing authorization token or empty SEP server list!"
              " Connection to SEP server will be paused until missing data is provided to the SepClient",
        logPrefix.c_str());
    }
  }

  connection->poll();

  if (state == State::WORKING)
  {
    tryExecuteAllDelayedCalls();
  }
  else
  {
    processExpiredDelayedCalls();
  }

  if (state == State::CLOSING || state == State::CLOSED)
  {
    if (!delayedRpcCalls.empty())
    {
      logdbg("%sAbandoning all delayed RPC calls due to closing connection state", logPrefix.c_str());
      abandonAllDelayedCalls();
    }
  }

  while (!incomingDataQueue.empty())
  {
    auto data = eastl::move(incomingDataQueue.front());
    incomingDataQueue.pop_front();

    lock.unlock();
    eastl::visit([this](auto &&value) { processIncomingData(eastl::move(value)); }, eastl::move(data));
    lock.lock();
  }

  if (connection->isCloseCompleted())
  {
    if (state != State::CLOSED)
    {
      switchState(State::CLOSING, State::CLOSED);
      previousOptionalFailedConnection.reset(); // clean memory and resources a bit earlier
    }
  }
}


void SepClient::tryExecuteAllDelayedCalls()
{
  processExpiredDelayedCalls();

  while (connection->isCallable() && numberOfExecutingRequests < maxSimultaneousRequests)
  {
    details::RpcContextPtr context = delayedRpcCalls.extractAnyCall();
    if (!context)
    {
      break;
    }

    if (isInfoLog())
    {
      logdbg("%sResuming delayed RPC call; context: %s", logPrefix.c_str(), context->formatState().c_str());
    }

    executeRpcCall(eastl::move(context));
  }
}


void SepClient::processExpiredDelayedCalls()
{
  const bool haveNetworkConnection = connection->isCallable();
  if (haveNetworkConnection != delayedRpcCalls.getHaveNetworkConnection())
  {
    logdbg("%sNetwork connection state is changed: haveNetworkConnection = %s", logPrefix.c_str(),
      haveNetworkConnection ? "true" : "false");

    delayedRpcCalls.setHaveNetworkConnection(haveNetworkConnection);
  }

  while (details::RpcContextPtr context = delayedRpcCalls.extractExpiredCall(abstraction::get_current_tick_count()))
  {
    if (delayedRpcCalls.getHaveNetworkConnection())
    {
      abandonRpcCall(eastl::move(context), websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT, "Delayed JSON RPC call expired");
    }
    else
    {
      abandonRpcCall(eastl::move(context), websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY,
        "Delayed JSON RPC call expired without network");
    }
  }
}


void SepClient::abandonAllDelayedCalls()
{
  while (details::RpcContextPtr context = delayedRpcCalls.extractAnyCall())
  {
    abandonRpcCall(eastl::move(context), websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY,
      "Delayed JSON RPC call was abandoned (clear delayed call queue)");
  }
}


void SepClient::initiateClose()
{
  if (isDebugLog())
  {
    logdbg("%sinitiateClose() called", logPrefix.c_str());
  }

  const std::scoped_lock lock(mutex);
  connection->initiateClose();

  if (state != State::CLOSING && state != State::CLOSED)
  {
    switchState(state, State::CLOSING);
  }
}


bool SepClient::isCallable() const
{
  const std::scoped_lock lock(mutex);
  return state == State::INITIALIZED || state == State::WORKING;
}


bool SepClient::isCloseCompleted() const
{
  const std::scoped_lock lock(mutex);
  return state == State::CLOSED;
}


} // namespace sepclient
