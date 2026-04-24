// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/rpcConnectionVerify.h>
#include <testHelpers/websocketLibraryInterceptor.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>


static constexpr char suiteTags[] = "[wsJsonRpcConnection][createAndInit]";
using namespace tests;


namespace
{

const websocketjsonrpc::WsJsonRpcConnectionConfig DEFAULT_WS_CONFIG = []() {
  websocketjsonrpc::WsJsonRpcConnectionConfig config;
  config.serverUrl = "wss://localhost:12345";
  return config;
}();

} // namespace


// ===============================================================================


TEST_CASE("Create WsJsonRpcConnection", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection(); // current implementation currently does not need this line

  const websocketjsonrpc::WsJsonRpcConnectionConfig config = DEFAULT_WS_CONFIG;
  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(config);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;

  CHECK(connection.getConfig().serverUrl == config.serverUrl);
  CHECK_NOTHROW(connection.getConnectionId());
  verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);

  SECTION("...and destroy")
  {
    // Nothing to do here
    match_ws_mock_scenario(wsMock, WsMockScenario::NOTHING_CALLED);
  }

  SECTION("...and initiateClose, destroy")
  {
    connection.initiateClose();
    match_ws_mock_scenario(wsMock, WsMockScenario::NOTHING_CALLED);
  }

  SECTION("...and poll, destroy")
  {
    connection.poll(); // connect is made here
    verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }

  SECTION("...and poll twice at the same tick, destroy")
  {
    connection.poll(); // connect is made here
    verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    CHECK_NOTHROW(connection.poll());
    verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }

  SECTION("...and poll, initiateClose, destroy")
  {
    connection.poll(); // connect is made here
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    connection.initiateClose();
    // close was not called because lack of poll() after initiateClose():
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }

  SECTION("...and initiateClose, poll, destroy")
  {
    connection.initiateClose();
    connection.poll();
    verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);
    match_ws_mock_scenario(wsMock, WsMockScenario::NOTHING_CALLED);
  }

  SECTION("...and pollx2, initiateClosex2, pollx2, destroy")
  {
    connection.poll(); // connect is made here
    verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    connection.poll();
    verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    connection.initiateClose();
    // close was not called because lack of poll() after initiateClose():
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    connection.initiateClose();
    connection.poll();
    connection.poll();
    verify_rpc_connection_state(connection, RpcConnectionState::CLOSING);
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_CLOSE);
  }

  {
    INFO("Destroy connection");
    connectionPtr.reset();
  }
}


TEST_CASE("Create WsJsonRpcConnection, call `poll` and close it gracefully (initiateClose + poll in a loop)", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;

  connection.poll();

  connection.initiateClose();

  {
    INFO("Wait until closing finishes");
    while (true)
    {
      connection.poll();
      if (connection.isClosed())
      {
        REQUIRE(ws.timer->getCurrentTimeMs() >= 3000);
        break;
      }
      verify_rpc_connection_state(connection, RpcConnectionState::CLOSING);
      REQUIRE(ws.timer->getCurrentTimeMs() < 3000);
      ws.timer->addTimeMs(100);
    }
  }

  verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_CLOSE);
  }
}


TEST_CASE("Create WsJsonRpcConnection and wait until connection attempt will fail", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldFailConnectingAtMs(1000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;

  {
    INFO("Wait until connection attempt failure");
    while (true)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
      connection.poll();
      if (connection.isClosed())
      {
        REQUIRE(ws.timer->getCurrentTimeMs() >= 1000);
        break;
      }
      REQUIRE(ws.timer->getCurrentTimeMs() < 1000);
      ws.timer->addTimeMs(100);
    }
  }

  verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}


TEST_CASE("Create WsJsonRpcConnection, wait until connected and following network error", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;

  {
    INFO("Wait until connected");
    while (true)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
      connection.poll();
      if (connection.isConnected())
      {
        REQUIRE(ws.timer->getCurrentTimeMs() >= 1000);
        break;
      }
      REQUIRE(ws.timer->getCurrentTimeMs() < 1000);
      ws.timer->addTimeMs(100);
    }
  }

  {
    INFO("Wait until connection is broken");
    while (true)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTED);
      connection.poll();
      if (connection.isClosed())
      {
        REQUIRE(ws.timer->getCurrentTimeMs() >= 3000);
        break;
      }
      REQUIRE(ws.timer->getCurrentTimeMs() < 3000);
      ws.timer->addTimeMs(100);
    }
  }

  verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}
