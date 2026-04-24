// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <websocketJsonRpc/wsJsonRpcPersistentConnection.h>

#include <catch2/catch_tostring.hpp>


namespace tests
{

enum class RpcConnectionState
{
  WAITING_FOR_INIT, // valid only for persistent connection
  CONNECTING,
  CONNECTED,
  RECONNECTING, // valid only for persistent connection
  CLOSING,
  CLOSED,
};

void verify_rpc_connection_state(const websocketjsonrpc::WsJsonRpcConnection &connection, RpcConnectionState expected_state);
void verify_rpc_connection_state(const websocketjsonrpc::WsJsonRpcPersistentConnection &connection, RpcConnectionState expected_state);

} // namespace tests


CATCH_REGISTER_ENUM(tests::RpcConnectionState, tests::RpcConnectionState::WAITING_FOR_INIT, tests::RpcConnectionState::CONNECTING,
  tests::RpcConnectionState::CONNECTED, tests::RpcConnectionState::RECONNECTING, tests::RpcConnectionState::CLOSING,
  tests::RpcConnectionState::CLOSED);
