// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/wsJsonRpcPersistentConnection.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>


namespace websocketjsonrpc
{

void WsJsonRpcPersistentConnection::set_ca_bundle(eastl::string_view ca_bundle_path)
{
  WsJsonRpcConnection::set_ca_bundle(ca_bundle_path);
}


WsJsonRpcPersistentConnection::WsJsonRpcPersistentConnection(PrivateConstructorTag) :
  logPrefix("[sep][websocket_jsonrpc_persistent] "), oldConnectionStorage(logPrefix)
{}


WsJsonRpcPersistentConnection::~WsJsonRpcPersistentConnection()
{
#ifdef __cpp_exceptions
  try
#endif
  {
    verifyObjectInvariant();

    if (state != State::CLOSED)
    {
      logwarn("%sWARN! WebSocket persistent connection object was not closed properly! (state: %s)", logPrefix.c_str(),
        getStateName(state));
    }
  }
#ifdef __cpp_exceptions
  catch (const std::exception &e)
  {
    logerr("%s[sep_error] ERROR! ~WsJsonRpcPersistentConnection() exception: %s\n", logPrefix.c_str(), e.what());
  }
#endif
}


const char *WsJsonRpcPersistentConnection::getStateName(State state_value)
{
  switch (state_value)
  {
    case State::INITIAL: return "INITIAL";
    case State::INITIALIZED: return "INITIALIZED";
    case State::CONNECTING: return "CONNECTING";
    case State::WORKING: return "WORKING";
    case State::RECONNECTING: return "RECONNECTING";
    case State::CLOSING: return "CLOSING";
    case State::CLOSED: return "CLOSED";
  }
  return "UNKNOWN_STATE";
}


void WsJsonRpcPersistentConnection::switchState(State previous_state, State next_state)
{
  const State actualState = state;

  const bool specialAllowedSwitchCase = (previous_state == State::RECONNECTING && next_state == State::WORKING);
  if (!specialAllowedSwitchCase)
  {
    if (static_cast<int>(previous_state) >= static_cast<int>(next_state))
    {
      G_ASSERT_LOG(false,
        "%s[sep_error] ERROR! Wanted to switch state from %s to %s, but this switch direction is forbidden!"
        " Current state: %s; Switched to CLOSED instead.",
        logPrefix.c_str(), getStateName(previous_state), getStateName(next_state), getStateName(actualState));

      hardReset();
      return;
    }
  }

  if (actualState == previous_state)
  {
    state = next_state;

    logdbg("%sSwitched state from %s to %s", logPrefix.c_str(), getStateName(previous_state), getStateName(next_state));
  }
  else
  {
    G_ASSERT_LOG(false,
      "%s[sep_error] ERROR! Wanted to switch state from %s to %s, but %s state was found; Switched to CLOSED instead.",
      logPrefix.c_str(), getStateName(previous_state), getStateName(next_state), getStateName(actualState));

    hardReset();
    return;
  }
}


void WsJsonRpcPersistentConnection::softReset()
{
  G_ASSERT(state == State::CLOSED);
  // clean user-provided callbacks (and maybe some lambda captures):
  connectionConfigGenerator = nullptr;
  incomingRequestCallback = nullptr;
}


void WsJsonRpcPersistentConnection::hardReset()
{
  state = State::CLOSED;
  softReset();

  // make object consistent with the CLOSED state
  currentConnection.reset();
  newConnectionIncubator = nullptr;
  oldConnectionStorage.hardReset();
}


void WsJsonRpcPersistentConnection::verifyObjectInvariant() const
{
  switch (state)
  {
    case State::INITIAL:
    case State::CLOSED:
    {
      G_ASSERT(!connectionConfigGenerator); //-V1037 // Two or more case-branches perform the same actions
      G_ASSERT(!newConnectionIncubator);
      G_ASSERT(!currentConnection);
      G_ASSERT(oldConnectionStorage.empty());
      break;
    }

    case State::INITIALIZED:
    {
      G_ASSERT(connectionConfigGenerator);
      G_ASSERT(!newConnectionIncubator);
      G_ASSERT(!currentConnection);
      G_ASSERT(oldConnectionStorage.empty());
      break;
    }

    case State::CONNECTING:
    case State::RECONNECTING:
    {
      G_ASSERT(connectionConfigGenerator);
      // States of newConnectionIncubator, currentConnection, and oldConnectionStorage may vary.
      break;
    }

    case State::WORKING:
    {
      G_ASSERT(connectionConfigGenerator); //-V1037 // Two or more case-branches perform the same actions
      G_ASSERT(!newConnectionIncubator);
      G_ASSERT(currentConnection);
      break;
    }

    case State::CLOSING:
    {
      G_ASSERT(connectionConfigGenerator);
      G_ASSERT(!newConnectionIncubator);
      G_ASSERT(!currentConnection);
      break;
    }

    default:
    {
      logerr("%s[sep_error] ERROR! verifyObjectInvariant: unexpected connection state: %s", logPrefix.c_str(), getStateName(state));
      G_ASSERT(false);
      break;
    }
  }
}


void WsJsonRpcPersistentConnection::initialize(ConnectionConfigGeneratorCallback &&connection_config_generator,
  IncomingRequestCallback &&incoming_request_callback, const WsJsonRpcStatsCallbackPtr &stats_callback)
{
  const auto reentranceGuard = reentranceProtection.capture();
  G_ASSERT_RETURN(reentranceGuard.isCaptured(), );

  verifyObjectInvariant();

  G_ASSERT_RETURN(connection_config_generator, );

  connectionConfigGenerator = eastl::move(connection_config_generator);
  incomingRequestCallback = eastl::move(incoming_request_callback);
  statsCallback = stats_callback;

  switchState(State::INITIAL, State::INITIALIZED);
}


WsJsonRpcConnectionPtr WsJsonRpcPersistentConnection::createNewRpcConnection(const WsJsonRpcConnectionPtr &optional_failed_connection,
  int connect_retry_number)
{
  G_ASSERT_RETURN(connectionConfigGenerator, nullptr);

  // application callbacks may be called here:
  const WsJsonRpcConnectionConfigPtr newConnectionConfig = connectionConfigGenerator(optional_failed_connection, connect_retry_number);
  if (!newConnectionConfig)
  {
    return nullptr;
  }

  WsJsonRpcConnectionPtr newConnection = WsJsonRpcConnection::create(*newConnectionConfig);

  if (incomingRequestCallback)
  {
    const auto weakSelf = weak_from_this();

    auto onIncomingRequestCallback = [weakSelf](RpcRequestPtr &&incoming_request) {
      WsJsonRpcPersistentConnectionPtr sharedSelf = weakSelf.lock();
      if (!sharedSelf)
      {
        logerr("[sep][websocket_jsonrpc_persistent][sep_error] ERROR! "
               "Connection object was found destroyed before calling incomingRequestCallback!");
        return;
      }

      sharedSelf->verifyObjectInvariant();

      G_ASSERT_RETURN(sharedSelf->incomingRequestCallback, );
      sharedSelf->incomingRequestCallback(eastl::move(incoming_request)); // application callbacks may be called here
    };

    newConnection->setIncomingRequestCallback(eastl::move(onIncomingRequestCallback));
  }
  return newConnection;
}


// application callbacks may be called here
void WsJsonRpcPersistentConnection::setupNewConnectionIncubator(const WsJsonRpcConnectionPtr &optional_failed_connection)
{
  const auto weakSelf = weak_from_this();

  auto connectionFactory = [weakSelf](const WsJsonRpcConnectionPtr &optional_failed_connection,
                             int connect_retry_number) -> WsJsonRpcConnectionPtr {
    WsJsonRpcPersistentConnectionPtr sharedSelf = weakSelf.lock();
    if (!sharedSelf)
    {
      logerr("[sep][websocket_jsonrpc_persistent][sep_error] ERROR! "
             "Connection object was found destroyed before calling connectionFactory!");
      return nullptr;
    }

    // application callbacks may be called here:
    WsJsonRpcConnectionPtr newConnection = sharedSelf->createNewRpcConnection(optional_failed_connection, connect_retry_number);
    return newConnection;
  };

  auto onInitiateConnection = [this](const WsJsonRpcConnectionPtr &next_connection) {
    G_ASSERT_RETURN(next_connection, );
    details::report_stats_event(statsCallback, details::StatsEventType::CONNECT_INITIATE, *next_connection);
  };

  auto onConnectionFailure = [this](const WsJsonRpcConnectionPtr &failed_connection) {
    G_ASSERT_RETURN(failed_connection, );

    // Replace error only if we have not current connection available
    // I.e. ignore error if we are doing graceful reconnect.
    if (!currentConnection)
    {
      lastError = failed_connection->getLastError();
    }

    details::report_stats_event(statsCallback, details::StatsEventType::CONNECT_FAILURE, *failed_connection);
  };

  G_ASSERT(!newConnectionIncubator);
  newConnectionIncubator = eastl::make_shared<details::NewConnectionIncubator>(eastl::move(connectionFactory),
    optional_failed_connection, logPrefix, eastl::move(onInitiateConnection), eastl::move(onConnectionFailure));

  // Force start connecting as soon as possible:
  pollNewConnectionIncubator(); // application callbacks may be called here
}


void WsJsonRpcPersistentConnection::cancelNewConnection()
{
  if (newConnectionIncubator)
  {
    WsJsonRpcConnectionPtr newConnection = newConnectionIncubator->cancel();
    if (newConnection)
    {
      oldConnectionStorage.addClosingConnection(newConnection);
    }
    newConnectionIncubator.reset();
  }
}


void WsJsonRpcPersistentConnection::pollNewConnectionIncubator() // application callbacks may be called here
{
  if (!newConnectionIncubator)
  {
    return;
  }

  details::NewConnectionIncubatorPtr newConnectionIncubatorRefCopy = newConnectionIncubator;
  WsJsonRpcConnectionPtr newConnection = newConnectionIncubatorRefCopy->poll(); // application callbacks may be called here

  if (!newConnection)
  {
    return;
  }

  logdbg("%sConnection #%u was found established (mark as current)!", logPrefix.c_str(), details::get_connection_id(newConnection));

  lastError = websocket::Error::no_error(); // clear last error on successful connection
  details::report_stats_event(statsCallback, details::StatsEventType::CONNECT_SUCCESS, *newConnection);

  newConnectionIncubatorRefCopy.reset();
  newConnectionIncubator.reset();
  G_ASSERT(newConnection->isConnected());

  if (currentConnection)
  {
    oldConnectionStorage.addDrainingConnection(currentConnection);
  }
  currentConnection = newConnection;

  if (state == State::RECONNECTING)
  {
    switchState(State::RECONNECTING, State::WORKING);
  }
  else
  {
    switchState(State::CONNECTING, State::WORKING);
  }
}


void WsJsonRpcPersistentConnection::pollAllPollableConnections() // application callbacks may be called here
{
  if (currentConnection && !currentConnection->isClosed())
  {
    WsJsonRpcConnectionPtr connectionRefCopy = currentConnection;
    connectionRefCopy->poll(); // application callbacks may be called here
  }

  pollNewConnectionIncubator(); // application callbacks may be called here

  oldConnectionStorage.poll(); // application callbacks may be called here
}


void WsJsonRpcPersistentConnection::processCurrentConnectionState() // application callbacks may be called here
{
  WsJsonRpcConnectionPtr failedCurrentConnection;

  if (currentConnection)
  {
    if (currentConnection->isClosed())
    {
      lastError = currentConnection->getLastError();

      logwarn("%sWARN! Connection #%u (current) was found disconnected!; %s", logPrefix.c_str(),
        details::get_connection_id(currentConnection), lastError.message.c_str());

      details::report_stats_event(statsCallback, details::StatsEventType::DISCONNECT, *currentConnection);

      failedCurrentConnection = eastl::move(currentConnection);

      if (state == State::WORKING)
      {
        switchState(State::WORKING, State::RECONNECTING);
      }
    }
    else
    {
      // currentConnection gets CLOSED state. It cannot get to CLOSING state without moving to `closingConnections`.
      G_ASSERT(currentConnection->isConnected());
    }
  }

  if ((state == State::CONNECTING || state == State::RECONNECTING) && !newConnectionIncubator)
  {
    setupNewConnectionIncubator(failedCurrentConnection); // application callbacks may be called here
  }
}


void WsJsonRpcPersistentConnection::poll()
{
  const auto reentranceGuard = reentranceProtection.capture();
  G_ASSERT_RETURN(reentranceGuard.isCaptured(), );

  verifyObjectInvariant();

  if (state == State::INITIAL)
  {
    return;
  }
  else if (state == State::INITIALIZED)
  {
    switchState(State::INITIALIZED, State::CONNECTING);
  }
  else if (state == State::CLOSED)
  {
    return;
  }

  pollAllPollableConnections(); // application callbacks may be called here

  verifyObjectInvariant();

  // Clean up collections from non-pollable connections:
  processCurrentConnectionState(); // application callbacks may be called here

  if (state == State::CLOSING)
  {
    verifyObjectInvariant();

    if (oldConnectionStorage.empty())
    {
      switchState(State::CLOSING, State::CLOSED);
      softReset();
      verifyObjectInvariant();
    }
  }
}


void WsJsonRpcPersistentConnection::rpcCall(RpcMessageId message_id, eastl::string_view method,
  const rapidjson::Value *optional_params, ResponseCallback &&callback, int request_timeout_ms)
{
  G_ASSERT_RETURN(callback, );
  verifyObjectInvariant();

  if (currentConnection && state != State::CLOSED)
  {
    currentConnection->rpcCall(message_id, method, optional_params, eastl::move(callback), request_timeout_ms);
    return;
  }

  auto response = eastl::make_unique<RpcResponse>(message_id, AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY,
    "Cannot make JSON RPC call in this state of persistent client");
  callback(eastl::move(response));
}


void WsJsonRpcPersistentConnection::rpcCall(RpcMessageId message_id, eastl::string_view method, eastl::string_view params_as_json,
  ResponseCallback &&callback, int request_timeout_ms)
{
  G_ASSERT_RETURN(callback, );
  verifyObjectInvariant();

  if (currentConnection && state != State::CLOSED)
  {
    currentConnection->rpcCall(message_id, method, params_as_json, eastl::move(callback), request_timeout_ms);
    return;
  }

  auto response = eastl::make_unique<RpcResponse>(message_id, AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY,
    "Cannot make JSON RPC call in this state of persistent client");
  callback(eastl::move(response));
}


void WsJsonRpcPersistentConnection::initiateClose()
{
  const auto reentranceGuard = reentranceProtection.capture();
  G_ASSERT_RETURN(reentranceGuard.isCaptured(), );

  verifyObjectInvariant();

  if (state != State::CLOSING && state != State::CLOSED)
  {
    logdbg("%sClosing WebSocket persistent connection (current state: %s)", logPrefix.c_str(), getStateName(state));

    if (state == State::INITIAL)
    {
      switchState(State::INITIAL, State::CLOSED);
      verifyObjectInvariant();
      return;
    }

    switchState(state, State::CLOSING);

    if (currentConnection)
    {
      oldConnectionStorage.addClosingConnection(currentConnection);
      currentConnection.reset();
    }

    cancelNewConnection();

    oldConnectionStorage.initiateClose();
  }
}


void WsJsonRpcPersistentConnection::initiateGracefulReconnect()
{
  const auto reentranceGuard = reentranceProtection.capture();
  G_ASSERT_RETURN(reentranceGuard.isCaptured(), );

  verifyObjectInvariant();

  switch (state)
  {
    case State::INITIAL:
      // Do nothing. Application should call initialize() soon. And then poll().
      break;

    case State::INITIALIZED:
      // Do nothing. Application should call poll() soon to initiate the very first connection.
      break;

    case State::CONNECTING:
    case State::WORKING:
    case State::RECONNECTING:
    {
      logdbg("%sInitiate graceful reconnect for WebSocket persistent connection (current connection: #%u; current state: %s)",
        logPrefix.c_str(), details::get_connection_id(currentConnection), getStateName(state));

      if (state == State::WORKING)
      {
        switchState(State::WORKING, State::RECONNECTING);
      }

      cancelNewConnection();

      break;
    }

    case State::CLOSING:
    case State::CLOSED:
    default:
    {
      logwarn("%sWARN! Ignoring graceful reconnect call because current WebSocket persistent connection state is %s",
        logPrefix.c_str(), getStateName(state));
    }
  }
}


websocket::Error WsJsonRpcPersistentConnection::copyLastError() const
{
  if (currentConnection && currentConnection->isClosed())
  {
    // It seems we had not chance to detect connection failure yet.
    // This branch is covering case with processing
    // generated JSON RPC response with error code LOST_WEBSOCKET_CONNECTION_AFTER_SENDING.
    return currentConnection->getLastError();
  }

  return lastError;
}


} // namespace websocketjsonrpc
