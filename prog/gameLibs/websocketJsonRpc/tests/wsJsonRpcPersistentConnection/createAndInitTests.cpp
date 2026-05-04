// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/rpcConnectionVerify.h>
#include <testHelpers/websocketLibraryInterceptor.h>


#include <catch2/catch_test_macros.hpp>


static constexpr char suiteTags[] = "[wsJsonRpcPersistentConnection][createAndInit]";
using namespace tests;


namespace
{

const auto default_ws_config_generator = [](const websocketjsonrpc::WsJsonRpcConnectionPtr &optional_failed_connection,
                                           int connect_retry_number) {
  auto config = eastl::make_unique<websocketjsonrpc::WsJsonRpcConnectionConfig>();
  config->serverUrl = "wss://localhost:12345";
  return config;
};


const auto no_incoming_requests_expected_callback = [](websocketjsonrpc::RpcRequestPtr &&incoming_request) {
  FAIL("Should not be executed here (no incoming JSON RPC requests are expected here)");
};

} // namespace


// ===============================================================================


TEST_CASE("Create WsJsonRpcPersistentConnection with NO initialize", suiteTags)
{
  WebsocketLibraryInterceptor ws;

  auto connectionPtr = websocketjsonrpc::WsJsonRpcPersistentConnection::create();
  websocketjsonrpc::WsJsonRpcPersistentConnection &connection = *connectionPtr;

  verify_rpc_connection_state(connection, RpcConnectionState::WAITING_FOR_INIT);

  SECTION("...and destroy")
  {
    // Nothing to do here
  }

  SECTION("...and initiateClose, destroy")
  {
    connection.initiateClose();
    verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);
  }

  SECTION("...and poll, destroy")
  {
    connection.poll(); // no connect since no initialize
    verify_rpc_connection_state(connection, RpcConnectionState::WAITING_FOR_INIT);
  }

  SECTION("...and poll, initiateClose, destroy")
  {
    connection.poll(); // no connect since no initialize
    verify_rpc_connection_state(connection, RpcConnectionState::WAITING_FOR_INIT);
    connection.initiateClose();
  }

  SECTION("...and initiateClose, poll, destroy")
  {
    connection.initiateClose();
    connection.poll();
    verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);
  }

  SECTION("...and pollx2, initiateClosex2, pollx2, destroy")
  {
    connection.poll(); // no connect since no initialize
    connection.poll();
    verify_rpc_connection_state(connection, RpcConnectionState::WAITING_FOR_INIT);
    connection.initiateClose();
    connection.initiateClose();
    connection.poll();
    connection.poll();
    verify_rpc_connection_state(connection, RpcConnectionState::CLOSED);
  }

  {
    INFO("Destroy connection");
    connectionPtr.reset();
  }
}


TEST_CASE("Create WsJsonRpcPersistentConnection with initialize", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection();

  auto connectionPtr = websocketjsonrpc::WsJsonRpcPersistentConnection::create();
  websocketjsonrpc::WsJsonRpcPersistentConnection &connection = *connectionPtr;
  connection.initialize(default_ws_config_generator, no_incoming_requests_expected_callback, nullptr);

  verify_rpc_connection_state(connection, RpcConnectionState::WAITING_FOR_INIT);

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


TEST_CASE("Create WsJsonRpcPersistentConnection and close it gracefully (initiateClose + poll in a loop)", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);
  const auto testStartTime = ws.timer->getCurrentTimeMs();

  auto connectionPtr = websocketjsonrpc::WsJsonRpcPersistentConnection::create();
  websocketjsonrpc::WsJsonRpcPersistentConnection &connection = *connectionPtr;
  connection.initialize(default_ws_config_generator, no_incoming_requests_expected_callback, nullptr);

  connection.initiateClose();

  {
    INFO("Wait until closing finishes");
    while (true)
    {
      connection.poll();
      if (connection.isCloseCompleted())
      {
        CAPTURE(ws.timer->getCurrentTimeMs());
        // Either it was immediately closed or it was connected and then closed after 3 seconds
        REQUIRE((ws.timer->getCurrentTimeMs() >= 3000 || ws.timer->getCurrentTimeMs() == testStartTime));
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
    // Possible cases:
    // - Mock connection could be not created
    // - Mock connection could be created and destroyed without any calls
    // - Mock connection could be created, initialized, called connect, called close
    match_ws_mock_scenario(wsMock, WsMockScenario::IF_CONNECT_THEN_CLOSE);
  }
}


TEST_CASE("Create WsJsonRpcPersistentConnection, call `poll` and close it gracefully (initiateClose + poll in a loop)", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcPersistentConnection::create();
  websocketjsonrpc::WsJsonRpcPersistentConnection &connection = *connectionPtr;
  connection.initialize(default_ws_config_generator, no_incoming_requests_expected_callback, nullptr);

  // Emulate case when game called poll() multiple times,
  // started connection
  // and then initiated close before connection was established
  connection.poll();
  connection.poll();
  connection.poll();

  connection.initiateClose();

  {
    INFO("Wait until closing finishes");
    while (true)
    {
      connection.poll();
      if (connection.isCloseCompleted())
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


TEST_CASE("Create WsJsonRpcPersistentConnection and wait until connection attempt will fail", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldFailConnectingAtMs(1000);
  const auto wsMock2 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1500, 3500);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcPersistentConnection::create();
  websocketjsonrpc::WsJsonRpcPersistentConnection &connection = *connectionPtr;
  connection.initialize(default_ws_config_generator, no_incoming_requests_expected_callback, nullptr);

  {
    INFO("Wait until internal connection attempt failure and second connection succeeds");
    while (true)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
      connection.poll();
      if (connection.isCallable())
      {
        REQUIRE(ws.timer->getCurrentTimeMs() >= 1500);
        break;
      }
      REQUIRE(ws.timer->getCurrentTimeMs() < 1500);
      ws.timer->addTimeMs(100);
    }
  }

  verify_rpc_connection_state(connection, RpcConnectionState::CONNECTED);

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    match_ws_mock_scenario(wsMock2, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}


TEST_CASE("Create WsJsonRpcPersistentConnection, wait until connected and following network error", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);
  const auto wsMock2 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(3500, 4500);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcPersistentConnection::create();
  websocketjsonrpc::WsJsonRpcPersistentConnection &connection = *connectionPtr;
  connection.initialize(default_ws_config_generator, no_incoming_requests_expected_callback, nullptr);

  {
    INFO("Wait until connected");
    while (true)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
      connection.poll();
      if (connection.isCallable())
      {
        REQUIRE(ws.timer->getCurrentTimeMs() >= 1000);
        break;
      }
      REQUIRE(ws.timer->getCurrentTimeMs() < 1000);
      ws.timer->addTimeMs(100);
    }
  }

  {
    INFO("Wait until internal connection is broken");
    while (true)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTED);
      if (ws.timer->getCurrentTimeMs() >= 3000)
      {
        connection.poll();
      }
      connection.poll();
      if (!connection.isCallable())
      {
        REQUIRE(ws.timer->getCurrentTimeMs() >= 3000);
        break;
      }
      REQUIRE(ws.timer->getCurrentTimeMs() < 3000);
      ws.timer->addTimeMs(100);
    }
  }

  verify_rpc_connection_state(connection, RpcConnectionState::RECONNECTING);

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    match_ws_mock_scenario(wsMock2, WsMockScenario::IF_CONNECT_THEN_NO_CLOSE);
  }
}
