// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/incomingRequestHistory.h>
#include <testHelpers/incomingResponseHistory.h>
#include <testHelpers/websocketLibraryInterceptor.h>

#include <sepClient/sepClient.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>


static constexpr char suiteTags[] = "[sepClient][reentrancy]";
using namespace tests;


namespace
{


const auto get_default_sep_config = []() {
  sepclient::SepClientConfig config;
  config.serverUrls.emplace_back("wss://localhost:12345");
  config.authorizationToken = "test_dummy_token";
  config.logLevel = sepclient::LogLevel::FULL_DUMP; // execute as much code as possible
  config.reconnectDelay.maxDelayMs = 0;             // do not make delays for now
  config.requestTimeoutMs = 200;                    // quick timeout
  return config;
};


const auto no_incoming_requests_expected_callback = [](websocketjsonrpc::RpcRequestPtr &&incoming_request) {
  FAIL("Should not be executed here (no incoming JSON RPC requests are expected here)");
};


const auto no_authentication_failures_expected_callback = [](sepclient::AuthenticationFailurePtr &&authentication_failure) {
  FAIL("Should not be executed here (no authentication failures are expected in this test)");
};


} // namespace


static void check_notification_argument(const IncomingRequestHistory &incoming_requests, size_t notification_index,
  int expected_argument)
{
  INFO("check_notification_argument");
  CAPTURE(notification_index);
  CAPTURE(expected_argument);

  REQUIRE(incoming_requests.size() > notification_index);
  const websocketjsonrpc::RpcRequest &request = *incoming_requests.requests[notification_index];
  REQUIRE(request.isValid() == true);
  CHECK(request.isNotification() == true);
  CHECK(request.getSerializedMessageId() == "");
  CHECK(request.getMethod() == "testNotification01");

  REQUIRE(request.getParams() != nullptr);
  REQUIRE(request.getParams()->IsInt() == true);
  CHECK(request.getParams()->GetInt() == expected_argument);
}


TEST_CASE("SepClient reentrancy: call client methods from JSON RPC notification callback", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 999'999);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;

  IncomingRequestHistory incomingRequests;

  {
    INFO("Initialize");

    auto incomingRequestCallback = [&client, internalCallback = incomingRequests.createCallback()](
                                     websocketjsonrpc::RpcRequestPtr &&incoming_request) {
      internalCallback(eastl::move(incoming_request));

      REQUIRE(client.isCloseCompleted() == false);

      CHECK_NOTHROW(client.isCallable());

      CHECK_NOTHROW(client.rpcCall(client.createRpcRequest("svc1", "onNotificationReceived"),
        [&client](sepclient::RpcResponsePtr &&) { client.poll(); }));

      CHECK_NOTHROW(client.poll());
    };

    client.initialize(no_authentication_failures_expected_callback, incomingRequestCallback, nullptr);

    CHECK(client.isCallable() == true);
  }

  {
    INFO("Wait until connected");
    client.poll();
    ws.timer->addTimeMs(1000);
    client.poll();
  }

  {
    INFO("Receiving single JSON RPC notification");
    wsMock->emulateReceivedMessage(ws.relativeMs(0), R"( {"jsonrpc": "2.0", "method": "testNotification01", "params": 100} )");
    incomingRequests.clear();

    client.poll();

    CHECK(incomingRequests.size() == 1);
    check_notification_argument(incomingRequests, 0, 100);
  }

  {
    INFO("Receiving multiple JSON RPC notification");
    wsMock->emulateReceivedMessage(ws.relativeMs(0), R"( {"jsonrpc": "2.0", "method": "testNotification01", "params": 201} )");
    wsMock->emulateReceivedMessage(ws.relativeMs(1), R"( {"jsonrpc": "2.0", "method": "testNotification01", "params": 202} )");
    wsMock->emulateReceivedMessage(ws.relativeMs(2), R"( {"jsonrpc": "2.0", "method": "testNotification01", "params": 203} )");
    incomingRequests.clear();

    ws.timer->addTimeMs(100);
    client.poll();

    CHECK(incomingRequests.size() == 3);
    check_notification_argument(incomingRequests, 0, 201);
    check_notification_argument(incomingRequests, 1, 202);
    check_notification_argument(incomingRequests, 2, 203);
  }

  {
    INFO("Notification during stopping");
    wsMock->emulateReceivedMessage(ws.relativeMs(100), R"( {"jsonrpc": "2.0", "method": "testNotification01", "params": 300} )");
    incomingRequests.clear();

    client.initiateClose();
    CHECK(client.isCallable() == false);

    client.poll();
    ws.timer->addTimeMs(100);
    client.poll();

    CHECK(incomingRequests.size() == 1);
    check_notification_argument(incomingRequests, 0, 300);
  }

  {
    INFO("Destroy connection and make final checks");
    // no messages are sent or received any more:
    incomingRequests.clear();

    client.poll();
    ws.timer->addTimeMs(9999);
    client.poll();

    REQUIRE(incomingRequests.size() == 0);

    clientPtr.reset();
  }
}


static void check_response_value(const IncomingResponseHistory &incoming_responses, size_t response_index, int expected_value)
{
  INFO("check_response_value");
  CAPTURE(response_index);
  CAPTURE(expected_value);

  REQUIRE(incoming_responses.size() > response_index);
  const websocketjsonrpc::RpcResponse &response = *incoming_responses.responses[response_index];
  REQUIRE(response.isError() == false);
  CHECK(response.getResult().IsInt() == true);
  CHECK(response.getResult().GetInt() == expected_value);
}


TEST_CASE("SepClient reentrancy: call client methods from RPC response callback", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 999'999);

  auto sepClientConfig = get_default_sep_config();
  sepClientConfig.requestTimeoutMs = 5000;

  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  {
    INFO("Wait until connected");
    client.poll();
    ws.timer->addTimeMs(1000);
    client.poll();
  }

  {
    INFO("Receive single response during normal connection");

    wsMock->clearSendHistory();
    incomingResponses.clear();
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);

    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    wsMock->emulateReceivedMessage(ws.relativeMs(100), generate_success_response(sentRequest, "100"));

    ws.timer->addTimeMs(100);
    client.poll();

    REQUIRE(wsMock->sendCallCount == 1);
    REQUIRE(incomingResponses.size() == 1);
    check_response_value(incomingResponses, 0, 100);
  }

  {
    INFO("Receive multiple responses during normal connection");

    wsMock->clearSendHistory();
    incomingResponses.clear();
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);
    client.rpcCall(client.createRpcRequest("svc1", "method02"), onResponse);
    client.rpcCall(client.createRpcRequest("svc1", "method03"), onResponse);

    REQUIRE(wsMock->sentRequests.size() == 3);
    wsMock->emulateReceivedMessage(ws.relativeMs(1), generate_success_response(wsMock->sentRequests[2], "200"));
    wsMock->emulateReceivedMessage(ws.relativeMs(2), generate_success_response(wsMock->sentRequests[1], "201"));
    wsMock->emulateReceivedMessage(ws.relativeMs(3), generate_success_response(wsMock->sentRequests[0], "202"));

    ws.timer->addTimeMs(100);
    client.poll();

    REQUIRE(wsMock->sendCallCount == 3);
    REQUIRE(incomingResponses.size() == 3);
    check_response_value(incomingResponses, 0, 200);
    check_response_value(incomingResponses, 1, 201);
    check_response_value(incomingResponses, 2, 202);
  }

  {
    INFO("Receive single response during connection stop");

    wsMock->clearSendHistory();
    incomingResponses.clear();
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);

    client.initiateClose();
    CHECK(client.isCallable() == false);

    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    wsMock->emulateReceivedMessage(ws.relativeMs(100), generate_success_response(sentRequest, "300"));

    ws.timer->addTimeMs(100);
    client.poll();

    REQUIRE(wsMock->sendCallCount == 1);
    REQUIRE(incomingResponses.size() == 1);
    check_response_value(incomingResponses, 0, 300);
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();
  }
}
