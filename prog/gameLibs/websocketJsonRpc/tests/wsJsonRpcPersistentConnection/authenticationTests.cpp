// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/rpcConnectionVerify.h>
#include <testHelpers/websocketLibraryInterceptor.h>

#include <catch2/catch_test_macros.hpp>


static constexpr char suiteTags[] = "[wsJsonRpcPersistentConnection][authentication]";
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


TEST_CASE("WsJsonRpcConnection: Authentication complex test", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock1 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 2000)->shouldFailAuthenticationAtMs(1500);
  const auto wsMock2 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(3000, 5000)->shouldFailAuthenticationAtMs(3000);
  const auto wsMock3 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(6000, 8000)->shouldFailAuthenticationAtMs(7000);
  const auto wsMock4 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(9000, 11000)->shouldSucceedAuthenticationAtMs(9000);
  const auto wsMock5 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(12000, 14000)->shouldSucceedAuthenticationAtMs(13000);
  const auto wsMockLast = ws.addFutureConnection()->shouldFailConnectingAtMs(999'999);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcPersistentConnection::create();
  websocketjsonrpc::WsJsonRpcPersistentConnection &connection = *connectionPtr;

  connection.initialize(default_ws_config_generator, no_incoming_requests_expected_callback, nullptr);

  while (true)
  {
    connection.poll();
    const auto now = ws.timer->getCurrentTimeMs();
    CAPTURE(now);

    const auto nowInRange = [now](int start, int end) { return now >= start && now < end; };

    if (nowInRange(0, 9000))
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTING);
    }
    else if (nowInRange(9000, 11000) || nowInRange(13000, 14000))
    {
      verify_rpc_connection_state(connection, RpcConnectionState::CONNECTED);
    }
    else if (nowInRange(11000, 13000))
    {
      verify_rpc_connection_state(connection, RpcConnectionState::RECONNECTING);
    }
    else if (now < 15000)
    {
      verify_rpc_connection_state(connection, RpcConnectionState::RECONNECTING);
    }
    else
    {
      break;
    }

    ws.timer->addTimeMs(250);
  }

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();

    INFO("wsMock1");
    match_ws_mock_scenario(wsMock1, WsMockScenario::CONNECT_THEN_CLOSE); // implicit close was called after auth failed
    INFO("wsMock2");
    match_ws_mock_scenario(wsMock2, WsMockScenario::CONNECT_THEN_CLOSE); // implicit close was called after auth failed
    INFO("wsMock3");
    match_ws_mock_scenario(wsMock3, WsMockScenario::CONNECT_THEN_CLOSE); // implicit close was called after auth failed
    INFO("wsMock4");
    match_ws_mock_scenario(wsMock4, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    INFO("wsMock5");
    match_ws_mock_scenario(wsMock5, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    INFO("wsMockLast");
    match_ws_mock_scenario(wsMockLast, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}
