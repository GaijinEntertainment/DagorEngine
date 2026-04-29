// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/rpcConnectionVerify.h>
#include <testHelpers/websocketLibraryInterceptor.h>

#include <catch2/catch_test_macros.hpp>


static constexpr char suiteTags[] = "[wsJsonRpcConnection][authentication]";
using namespace tests;


namespace
{

const websocketjsonrpc::WsJsonRpcConnectionConfig DEFAULT_WS_CONFIG = []() {
  websocketjsonrpc::WsJsonRpcConnectionConfig config;
  config.serverUrl = "wss://localhost:12345";
  return config;
}();


const auto no_incoming_requests_expected_callback = [](websocketjsonrpc::RpcRequestPtr &&incoming_request) {
  FAIL("Should not be executed here (no incoming JSON RPC requests are expected here)");
};

} // namespace


// ===============================================================================


TEST_CASE("WsJsonRpcConnection: Authentication succeeds", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;

  connection.setIncomingRequestCallback(no_incoming_requests_expected_callback);

  int authenticationFinishAtMs = 0;
  SECTION("Authentication needs 0 ms") { authenticationFinishAtMs = 1000; }
  SECTION("Authentication needs 1000 ms") { authenticationFinishAtMs = 2000; }
  REQUIRE(authenticationFinishAtMs >= 1000);
  wsMock->shouldSucceedAuthenticationAtMs(authenticationFinishAtMs);

  while (true)
  {
    connection.poll();
    const auto now = ws.timer->getCurrentTimeMs();
    CAPTURE(now);

    if (now < authenticationFinishAtMs)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    }
    else if (now < 3000)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTED);
    }
    else if (now < 4000)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);
    }
    else
    {
      break;
    }

    ws.timer->addTimeMs(100);
  }

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}


TEST_CASE("WsJsonRpcConnection: Authentication fails", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;

  connection.setIncomingRequestCallback(no_incoming_requests_expected_callback);

  int authenticationFinishAtMs = 0;
  SECTION("Authentication needs 0 ms") { authenticationFinishAtMs = 1000; }
  SECTION("Authentication needs 1000 ms") { authenticationFinishAtMs = 2000; }
  REQUIRE(authenticationFinishAtMs >= 1000);
  wsMock->shouldFailAuthenticationAtMs(authenticationFinishAtMs);

  while (true)
  {
    connection.poll();
    const auto now = ws.timer->getCurrentTimeMs();
    CAPTURE(now);

    if (now < authenticationFinishAtMs)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    }
    else if (now < 3000)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CLOSING);
    }
    else if (now < 4000)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);
    }
    else
    {
      break;
    }

    ws.timer->addTimeMs(100);
  }

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_CLOSE); // implicit close was called after auth failed
  }
}
