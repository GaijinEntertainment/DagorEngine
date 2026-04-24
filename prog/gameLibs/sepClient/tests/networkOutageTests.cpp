// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/incomingRequestHistory.h>
#include <testHelpers/incomingResponseHistory.h>
#include <testHelpers/websocketLibraryInterceptor.h>

#include <sepTestHelpers/sepTimeout.h>

#include <sepClient/sepClient.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>


static constexpr char suiteTags[] = "[sepClient][networkOutage]";
using namespace tests;


namespace
{


const auto get_default_sep_config = []() {
  sepclient::SepClientConfig config;
  config.serverUrls.emplace_back("wss://localhost:12345");
  config.authorizationToken = "test_dummy_token";
  config.logLevel = sepclient::LogLevel::FULL_DUMP; // execute as much code as possible
  config.reconnectDelay.maxDelayMs = 0;             // do not make delays for now
  config.requestTimeoutMs = 20'000;                 // 20 sec timeout
  config.requestTimeoutWithoutNetworkMs = 5'000;    // 5 sec timeout without network
  config.maxSimultaneousRequests = 1;               // second request will be delayed
  return config;
};


const auto no_incoming_requests_expected_callback = [](websocketjsonrpc::RpcRequestPtr &&incoming_request) {
  FAIL("Should not be executed here (no incoming JSON RPC requests are expected here)");
};


const auto no_authentication_failures_expected_callback = [](sepclient::AuthenticationFailurePtr &&authentication_failure) {
  FAIL("Should not be executed here (no authentication failures are expected in this test)");
};


} // namespace


TEST_CASE("SepClient network outage: timeout before the first connection", suiteTags)
{
  WebsocketLibraryInterceptor ws;

  const auto wsMock = ws.addFutureConnection()->shouldFailConnectingAtMs(10'000);

  const auto sepClientConfig = get_default_sep_config();

  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  IncomingResponseHistory incomingDelayedResponses;
  const auto onDelayedResponse = incomingDelayedResponses.createCallback();

  {
    INFO("Initialize");
    client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);
    CHECK(client.isCallable() == true);

    CHECK_NOTHROW(client.poll());
  }

  {
    INFO("Execute pair of calls during long connection try: expect short 5 sec timeout");

    ws.timer->addTimeMs(1000); // do not send anything at zero time point
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);
    client.rpcCall(client.createRpcRequest("svc1", "method01.delayed"), onDelayedResponse);

    ws.timer->addTimeMs(4999);
    CHECK_NOTHROW(client.poll());

    CHECK(incomingResponses.size() == 0);
    CHECK(incomingDelayedResponses.size() == 0);

    ws.timer->addTimeMs(1);
    CHECK_NOTHROW(client.poll());

    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);

    REQUIRE(incomingDelayedResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &delayedResponse = incomingResponses.last();
    CHECK(delayedResponse.isError() == true);
    CHECK(delayedResponse.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();
  }
}


TEST_CASE("SepClient network outage: connection is established in the middle of request lifetime", suiteTags)
{
  WebsocketLibraryInterceptor ws;

  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(10'000, 30'000);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;

  const int sendTimeoutMs = tests::get_send_expiration_duration_ms(sepClientConfig.requestTimeoutMs);

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  IncomingResponseHistory incomingDelayedResponses;
  const auto onDelayedResponse = incomingDelayedResponses.createCallback();

  {
    INFO("Initialize");
    client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);
    CHECK(client.isCallable() == true);

    CHECK_NOTHROW(client.poll());

    ws.timer->addTimeMs(7000);
  }

  {
    INFO("Execute pair of calls just before connection established"
         ". Expect 20 sec timeout for the first one, 15 sec timeout for the second one");

    client.rpcCall(client.createRpcRequest("svc1", "method02"), onResponse);
    client.rpcCall(client.createRpcRequest("svc1", "method02.delayed"), onDelayedResponse);

    ws.timer->addTimeMs(3'000);
    CHECK_NOTHROW(client.poll()); // connected + sent one of requests at 10'000 ms

    const int timeoutForSentRequestMs = sepClientConfig.requestTimeoutMs;
    const int timeoutForDelayedRequestMs = sendTimeoutMs; // smaller!

    CHECK(incomingResponses.size() == 0);
    CHECK(incomingDelayedResponses.size() == 0);

    ws.timer->addTimeMs(timeoutForDelayedRequestMs - 3000 - 1);
    CHECK_NOTHROW(client.poll());

    CHECK(incomingResponses.size() == 0);
    CHECK(incomingDelayedResponses.size() == 0);

    ws.timer->addTimeMs(1);
    CHECK(ws.timer->getCurrentTimeMs() == 22'000);
    CHECK_NOTHROW(client.poll()); // got response for delayed request

    CHECK(incomingResponses.size() == 0);
    CHECK(incomingDelayedResponses.size() == 1);

    ws.timer->addTimeMs(timeoutForSentRequestMs - timeoutForDelayedRequestMs - 1);
    CHECK_NOTHROW(client.poll());

    CHECK(incomingResponses.size() == 0);
    CHECK(incomingDelayedResponses.size() == 1); // still only delayed response

    ws.timer->addTimeMs(1);
    CHECK(ws.timer->getCurrentTimeMs() == 27'000);
    CHECK_NOTHROW(client.poll()); // got response for the sent request

    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT);

    REQUIRE(incomingDelayedResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &delayedResponse = incomingResponses.last();
    CHECK(delayedResponse.isError() == true);
    CHECK(delayedResponse.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT);
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();
  }
}


TEST_CASE("SepClient network outage: connection is lost after the request was created", suiteTags)
{
  WebsocketLibraryInterceptor ws;

  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(10'000, 30'000);
  ws.addFutureConnection()->shouldFailConnectingAtMs(999'999);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  IncomingResponseHistory incomingDelayedResponses;
  const auto onDelayedResponse = incomingDelayedResponses.createCallback();

  {
    INFO("Initialize");
    client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);
    CHECK(client.isCallable() == true);

    CHECK_NOTHROW(client.poll());

    ws.timer->addTimeMs(10'000);
    CHECK_NOTHROW(client.poll()); // connection established

    ws.timer->addTimeMs(19'000);
  }

  {
    INFO("Execute pair of calls just before connection lost at 30 sec"
         ". Expect 5 sec timeout: 1 second before connection lost and 4 sec after sending");

    CHECK(ws.timer->getCurrentTimeMs() == 29'000);
    // Connection will be lost at 30'000 ms

    client.rpcCall(client.createRpcRequest("svc1", "method03"), onResponse);
    client.rpcCall(client.createRpcRequest("svc1", "method03.delayed"), onDelayedResponse);

    ws.timer->addTimeMs(1'000);
    CHECK_NOTHROW(client.poll());

    // Wait 4 sec after connection was lost (short timeout minus 1 sec)

    ws.timer->addTimeMs(3'999);
    CHECK_NOTHROW(client.poll());

    CHECK(incomingResponses.size() == 0);
    CHECK(incomingDelayedResponses.size() == 0);

    ws.timer->addTimeMs(1);
    CHECK_NOTHROW(client.poll());

    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);

    REQUIRE(incomingDelayedResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &delayedResponse = incomingResponses.last();
    CHECK(delayedResponse.isError() == true);
    CHECK(delayedResponse.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();
  }
}


TEST_CASE("SepClient network outage: request is created after connection is lost", suiteTags)
{
  WebsocketLibraryInterceptor ws;

  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(10'000, 30'000);
  ws.addFutureConnection()->shouldFailConnectingAtMs(999'999);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  IncomingResponseHistory incomingDelayedResponses;
  const auto onDelayedResponse = incomingDelayedResponses.createCallback();

  {
    INFO("Initialize");
    client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);
    CHECK(client.isCallable() == true);

    CHECK_NOTHROW(client.poll());

    ws.timer->addTimeMs(10'000);
    CHECK_NOTHROW(client.poll()); // connection established

    ws.timer->addTimeMs(20'000);
    CHECK_NOTHROW(client.poll()); // connection is lost

    ws.timer->addTimeMs(4'000);
  }

  {
    INFO("Execute pair of calls while trying to reconnect. Expect 5 sec timeout");

    CHECK(ws.timer->getCurrentTimeMs() == 34'000);

    client.rpcCall(client.createRpcRequest("svc1", "method04"), onResponse);
    client.rpcCall(client.createRpcRequest("svc1", "method04.delayed"), onDelayedResponse);

    ws.timer->addTimeMs(4'999);
    CHECK_NOTHROW(client.poll());

    CHECK(incomingResponses.size() == 0);
    CHECK(incomingDelayedResponses.size() == 0);

    ws.timer->addTimeMs(1);
    CHECK_NOTHROW(client.poll());

    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);

    REQUIRE(incomingDelayedResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &delayedResponse = incomingResponses.last();
    CHECK(delayedResponse.isError() == true);
    CHECK(delayedResponse.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();
  }
}
