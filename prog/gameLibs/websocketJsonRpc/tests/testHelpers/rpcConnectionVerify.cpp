// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rpcConnectionVerify.h"

#include <catch2/catch_test_macros.hpp>


namespace tests
{


void verify_rpc_connection_state(const websocketjsonrpc::WsJsonRpcConnection &connection, RpcConnectionState expected_state)
{
  CAPTURE(expected_state);
  REQUIRE(expected_state != RpcConnectionState::WAITING_FOR_INIT);
  REQUIRE(expected_state != RpcConnectionState::RECONNECTING);
  REQUIRE(connection.getInProgressCallCount() == 0);
  REQUIRE(connection.isConnecting() == (expected_state == RpcConnectionState::CONNECTING));
  REQUIRE(connection.isConnected() == (expected_state == RpcConnectionState::CONNECTED));
  REQUIRE(connection.isClosed() == (expected_state == RpcConnectionState::CLOSED));
}


void verify_rpc_connection_state(const websocketjsonrpc::WsJsonRpcPersistentConnection &connection, RpcConnectionState expected_state)
{
  CAPTURE(expected_state);
  REQUIRE(connection.isCallable() == (expected_state == RpcConnectionState::CONNECTED));
  REQUIRE(connection.isCloseCompleted() == (expected_state == RpcConnectionState::CLOSED));
}


} // namespace tests
