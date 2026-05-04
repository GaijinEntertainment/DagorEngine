// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/rpcConnectionVerify.h>
#include <testHelpers/websocketLibraryInterceptor.h>


#include <catch2/catch_test_macros.hpp>


static constexpr char suiteTags[] = "[wsJsonRpcPersistentConnection][configGenerator]";
using namespace tests;


namespace
{

const auto create_config = []() {
  auto config = eastl::make_unique<websocketjsonrpc::WsJsonRpcConnectionConfig>();
  config->serverUrl = "wss://localhost:12345";
  return config;
};


const auto create_config_generator = [](websocketjsonrpc::WsJsonRpcConnectionConfigPtr &next_config,
                                       int &config_generator_call_count) {
  const auto generator = [&next_config, &config_generator_call_count](
                           const websocketjsonrpc::WsJsonRpcConnectionPtr &optional_failed_connection, int connect_retry_number) {
    config_generator_call_count++;
    return eastl::move(next_config);
  };
  return generator;
};


const auto no_incoming_requests_expected_callback = [](websocketjsonrpc::RpcRequestPtr &&incoming_request) {
  FAIL("Should not be executed here (no incoming JSON RPC requests are expected here)");
};

} // namespace


// ===============================================================================


TEST_CASE("Using WsJsonRpcPersistentConnection without config ready", suiteTags)
{
  WebsocketLibraryInterceptor ws;

  auto connectionPtr = websocketjsonrpc::WsJsonRpcPersistentConnection::create();
  websocketjsonrpc::WsJsonRpcPersistentConnection &connection = *connectionPtr;

  int configGeneratorCallCount = 0;
  int expectedConfigGeneratorCallCount = 0;
  websocketjsonrpc::WsJsonRpcConnectionConfigPtr nextConfig;
  connection.initialize(create_config_generator(nextConfig, configGeneratorCallCount), no_incoming_requests_expected_callback,
    nullptr);

  {
    INFO("Make first poll() without config");
    ws.timer->addTimeMs(100);
    CHECK_NOTHROW(connection.poll());
    CHECK(configGeneratorCallCount == ++expectedConfigGeneratorCallCount);
  }

  {
    INFO("Make second poll() without config");
    ws.timer->addTimeMs(100);
    CHECK_NOTHROW(connection.poll());
    CHECK(configGeneratorCallCount == ++expectedConfigGeneratorCallCount);
  }

  {
    INFO("Make third poll() after a huge delay (1 hour)");
    ws.timer->addTimeMs(3'600'000);
    CHECK_NOTHROW(connection.poll());
    CHECK(configGeneratorCallCount == ++expectedConfigGeneratorCallCount);
  }

  const int startTimeMs = ws.timer->getCurrentTimeMs();

  nextConfig = create_config();

  {
    INFO("Initiate connection");
    const auto wsMock = ws.addFutureConnection()->shouldFailConnectingAtMs(startTimeMs + 1000);

    CHECK(nextConfig.get() != nullptr);
    CHECK(wsMock->connectCallCount == 0);
    CHECK_NOTHROW(connection.poll());
    CHECK(nextConfig.get() == nullptr);
    CHECK(wsMock->connectCallCount == 1);
    CHECK(configGeneratorCallCount == ++expectedConfigGeneratorCallCount);
  }

  {
    INFO("Poll during connecting");
    ws.timer->addTimeMs(500);
    CHECK_NOTHROW(connection.poll());
  }

  {
    INFO("Poll after connection failed. Try initiate new one");
    ws.timer->addTimeMs(500);
    CHECK_NOTHROW(connection.poll());
    CHECK(configGeneratorCallCount == ++expectedConfigGeneratorCallCount);
  }

  {
    INFO("Poll again without new config ready");
    ws.timer->addTimeMs(100);
    CHECK_NOTHROW(connection.poll());
    CHECK(configGeneratorCallCount == ++expectedConfigGeneratorCallCount);
  }

  nextConfig = create_config();

  {
    INFO("Initiate connection #2");
    const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(startTimeMs + 2000, startTimeMs + 3000);

    CHECK(nextConfig.get() != nullptr);
    CHECK(wsMock->connectCallCount == 0);
    CHECK_NOTHROW(connection.poll());
    CHECK(nextConfig.get() == nullptr);
    CHECK(wsMock->connectCallCount == 1);
    CHECK(configGeneratorCallCount == ++expectedConfigGeneratorCallCount);
  }

  {
    INFO("Destroy connection");
    connectionPtr.reset();
  }
}
