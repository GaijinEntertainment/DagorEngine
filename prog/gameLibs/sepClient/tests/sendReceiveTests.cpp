// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/incomingRequestHistory.h>
#include <testHelpers/incomingResponseHistory.h>
#include <testHelpers/websocketLibraryInterceptor.h>

#include <sepTestHelpers/sepTimeout.h>

#include <sepClient/sepClient.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>


static constexpr char suiteTags[] = "[sepClient][sendReceive]";
using namespace tests;


namespace
{


const auto get_default_sep_config = []() {
  sepclient::SepClientConfig config;
  config.serverUrls.emplace_back("wss://localhost:12345");
  config.authorizationToken = "test_dummy_token";
  config.logLevel = sepclient::LogLevel::FULL_DUMP; // execute as much code as possible
  config.reconnectDelay.maxDelayMs = 0;             // do not make delays for now
  config.requestTimeoutMs = 1000;                   // timeout for RPC calls
  config.requestTimeoutWithoutNetworkMs = 7000;     // much bigger timeout for waiting network connection
  return config;
};


const auto no_incoming_requests_expected_callback = [](websocketjsonrpc::RpcRequestPtr &&incoming_request) {
  FAIL("Should not be executed here (no incoming JSON RPC requests are expected here)");
};


const auto no_authentication_failures_expected_callback = [](sepclient::AuthenticationFailurePtr &&authentication_failure) {
  FAIL("Should not be executed here (no authentication failures are expected in this test)");
};


} // namespace


TEST_CASE("SepClient: receive server-initiated JSON RPC messages (reverse direction)", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 999'999);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;

  IncomingRequestHistory incomingRequests;
  bool haveIncomingRequestCallback = true;

  REQUIRE(client.isCallable() == false);

  SECTION("Set incoming notification callback")
  {
    client.initialize(no_authentication_failures_expected_callback, incomingRequests.createCallback(), nullptr);
  }

  SECTION("Skip setting incoming notification callback")
  {
    haveIncomingRequestCallback = false;
    client.initialize(no_authentication_failures_expected_callback, nullptr, nullptr);
  }

  REQUIRE(client.isCallable() == true);

  {
    INFO("Wait until connected");
    client.poll();
    ws.timer->addTimeMs(1000);
    client.poll();
  }

  {
    INFO("Receiving JSON RPC notification");
    wsMock->emulateReceivedMessage(ws.relativeMs(0), R"( {"jsonrpc": "2.0", "method": "method01"} )");
    incomingRequests.clear();
    client.poll();
    if (haveIncomingRequestCallback)
    {
      REQUIRE(incomingRequests.size() == 1);
      const websocketjsonrpc::RpcRequest &request = incomingRequests.last();
      REQUIRE(request.isValid());
      CHECK(request.isNotification() == true);
      CHECK(request.getSerializedMessageId() == "");
      CHECK(request.getMethod() == "method01");
      CHECK(request.getParams() == nullptr);
    }
  }

  {
    INFO("Silently skipping incoming JSON RPC request");
    wsMock->emulateReceivedMessage(ws.relativeMs(0),
      R"( {"jsonrpc": "2.0", "method": "method02", "id": "0.01.05-myID", "params": [-10,20,30]} )");
    incomingRequests.clear();
    client.poll();
    if (haveIncomingRequestCallback)
    {
      REQUIRE(incomingRequests.size() == 0);
    }
  }

  {
    INFO("Receiving multiple JSON RPC notifications and requests between polls");
    // Notifications
    wsMock->emulateReceivedMessage(ws.relativeMs(1), R"( {"jsonrpc": "2.0", "method": "method01"} )");
    wsMock->emulateReceivedMessage(ws.relativeMs(2), R"( {"jsonrpc": "2.0", "method": "method01"} )");
    wsMock->emulateReceivedMessage(ws.relativeMs(3), R"( {"jsonrpc": "2.0", "method": "method_another"} )");
    // Requests
    wsMock->emulateReceivedMessage(ws.relativeMs(4), R"( {"jsonrpc": "2.0", "method": "req01", "id": 1} )");
    wsMock->emulateReceivedMessage(ws.relativeMs(5), R"( {"jsonrpc": "2.0", "method": "req01", "id": 2} )");
    wsMock->emulateReceivedMessage(ws.relativeMs(6), R"( {"jsonrpc": "2.0", "method": "req_another", "id": 3} )");

    ws.timer->addTimeMs(6);
    CHECK(ws.timer->getCurrentTimeMs() == 1006);

    incomingRequests.clear();
    client.poll();
    if (haveIncomingRequestCallback)
    {
      REQUIRE(incomingRequests.size() == 3);

      CHECK(incomingRequests.requests[0]->isNotification() == true);
      CHECK(incomingRequests.requests[0]->getMethod() == "method01");

      CHECK(incomingRequests.requests[1]->isNotification() == true);
      CHECK(incomingRequests.requests[1]->getMethod() == "method01");

      CHECK(incomingRequests.requests[2]->isNotification() == true);
      CHECK(incomingRequests.requests[2]->getMethod() == "method_another");
    }
  }

  {
    INFO("Destroy connection and make final checks");
    // no messages are sent or received any more:
    wsMock->clearSendHistory();
    incomingRequests.clear();

    client.poll();
    ws.timer->addTimeMs(9999);
    client.poll();

    REQUIRE(incomingRequests.size() == 0);
    CHECK(wsMock->sendCallCount == 0);

    clientPtr.reset();
  }
}


TEST_CASE("SepClient: execute RPC call without existing network connection", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 2000);
  const auto wsMock2 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(5000, 999'999);

  auto sepClientConfig = get_default_sep_config();
  sepClientConfig.requestTimeoutMs = 5000;

  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  REQUIRE(client.isCallable() == true);

  {
    INFO("Execute RPC before first poll");

    incomingResponses.clear();
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);

    CHECK(wsMock->sendCallCount == 0); // no actual send

    client.poll();
    CHECK(incomingResponses.size() == 0); // no error response
  }

  {
    INFO("Execute RPC before connection established");

    incomingResponses.clear();
    ws.timer->addTimeMs(100);
    client.rpcCall(client.createRpcRequest("svc2", "method02"), onResponse);

    CHECK(wsMock->sendCallCount == 0); // no actual send

    client.poll();
    CHECK(incomingResponses.size() == 0); // no error response
  }

  {
    INFO("Wait until connected");
    ws.timer->addTimeMs(1000);
    client.poll();

    REQUIRE(wsMock->sendCallCount == 2); // two delayed sends happened
    {
      const auto &sentRequest = wsMock->sentRequests[0];
      REQUIRE(sentRequest.isValid() == true);
      REQUIRE(sentRequest.isNotification() == false);
      REQUIRE(sentRequest.getMethod() == "svc1.method01");
    }
    {
      const auto &sentRequest = wsMock->sentRequests[1];
      REQUIRE(sentRequest.isValid() == true);
      REQUIRE(sentRequest.isNotification() == false);
      REQUIRE(sentRequest.getMethod() == "svc2.method02");
    }
    wsMock->clearSendHistory();
  }

  bool initiatedClose = true;

  SECTION("Initiate connection close and execute RPC call after initiateClose")
  {
    REQUIRE(client.isCallable() == true);
    client.initiateClose();
    REQUIRE(client.isCallable() == false);
    REQUIRE(client.isCloseCompleted() == false);

    incomingResponses.clear();
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);

    REQUIRE(wsMock->sendCallCount == 0); // no actual send

    client.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
  }

  SECTION("Skip closing connection manually")
  {
    initiatedClose = false;
    CHECK(true);
  }

  {
    INFO("Wait until underlaying connection is closed");
    ws.timer->addTimeMs(1000);
    client.poll();
  }

  {
    INFO("Execute RPC call after connection is closed");

    incomingResponses.clear();
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);

    CHECK(wsMock->sendCallCount == 0); // no actual send

    client.poll();
    if (initiatedClose)
    {
      REQUIRE(incomingResponses.size() == 1);
      const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
      CHECK(response.isError() == true);
      CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
      incomingResponses.clear();
    }
    else
    {
      CHECK(incomingResponses.size() == 0); // no error response
    }
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();
    CHECK(incomingResponses.size() == 0); // no error response during destructor
  }
}


TEST_CASE("SepClient: delayed request is expired in the queue", suiteTags)
{
  const int requestTimeoutMs = 2000;
  const int sendTimeoutMs = tests::get_send_expiration_duration_ms(requestTimeoutMs);

  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(requestTimeoutMs + 1000, 999'999);

  auto sepClientConfig = get_default_sep_config();
  sepClientConfig.requestTimeoutMs = requestTimeoutMs;

  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  REQUIRE(client.isCallable() == true);

  {
    INFO("First poll");
    ws.timer->addTimeMs(100);
    CHECK_NOTHROW(client.poll());
    ws.timer->addTimeMs(100);
  }

  {
    INFO("Execute RPC before connection established");
    incomingResponses.clear();
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);

    CHECK_NOTHROW(client.poll());
    CHECK(incomingResponses.size() == 0);
  }

  {
    INFO("Execute RPC before connection established");

    incomingResponses.clear();
    ws.timer->addTimeMs(sendTimeoutMs - 1);
    CHECK_NOTHROW(client.poll());

    CHECK(incomingResponses.size() == 0);

    ws.timer->addTimeMs(1);
    CHECK_NOTHROW(client.poll());

    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
    incomingResponses.clear();
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();
    CHECK(incomingResponses.size() == 0); // no error response during destructor
  }
}


TEST_CASE("SepClient: execute RPC call", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 999'999);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  {
    INFO("Wait until connected");
    client.poll();
    ws.timer->addTimeMs(1000);
    client.poll();
    REQUIRE(client.isCallable() == true);
  }

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  SECTION("Execute JSON RPC call with timeout (no reply at all)")
  {
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);

    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.isNotification() == false);
    CHECK(sentRequest.getMethod() == "svc1.method01");
    CHECK(sentRequest.getParams() != nullptr);

    ws.timer->addTimeMs(999);
    client.poll();
    CHECK(incomingResponses.size() == 0);

    ws.timer->addTimeMs(1);
    client.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT);
  }

  SECTION("Execute JSON RPC call with timeout (delayed reply, callback must not be called twice)")
  {
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);

    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.getMethod() == "svc1.method01");

    wsMock->emulateReceivedMessage(ws.relativeMs(1500), generate_success_response(sentRequest, "{}"));

    ws.timer->addTimeMs(1000);
    client.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT);

    {
      INFO("Do poll after delayed timed out response is received");
      incomingResponses.clear();
      ws.timer->addTimeMs(1000);
      client.poll();
      CHECK(incomingResponses.size() == 0);
    }
  }

  SECTION("Execute JSON RPC call with error reply")
  {
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);
    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.isNotification() == false);
    CHECK(sentRequest.getMethod() == "svc1.method01");
    CHECK(sentRequest.getParams() != nullptr);

    wsMock->emulateReceivedMessage(ws.relativeMs(999), generate_error_response(sentRequest, -37500, "myMessage"));

    ws.timer->addTimeMs(998);
    client.poll();
    CHECK(incomingResponses.size() == 0);

    ws.timer->addTimeMs(1);
    client.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
    CHECK(response.getErrorCode() == -37500);
    CHECK(response.getErrorMessage() == "myMessage");
  }

  SECTION("Execute JSON RPC call with success reply")
  {
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);
    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.isNotification() == false);
    CHECK(sentRequest.getMethod() == "svc1.method01");
    CHECK(sentRequest.getParams() != nullptr);

    wsMock->emulateReceivedMessage(ws.relativeMs(999), generate_success_response(sentRequest, "[5,4,3,2,1]"));

    ws.timer->addTimeMs(998);
    client.poll();
    CHECK(incomingResponses.size() == 0);

    ws.timer->addTimeMs(1);
    client.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == false);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
    REQUIRE(response.getResult().IsArray());
    CHECK(response.getResult().GetArray().Size() == 5);
  }

  SECTION("Execute JSON RPC call with duplicated success reply (receive all at single poll)")
  {
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);
    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.getMethod() == "svc1.method01");

    wsMock->emulateReceivedMessage(ws.relativeMs(100), generate_success_response(sentRequest, "{}"));
    wsMock->emulateReceivedMessage(ws.relativeMs(200), generate_success_response(sentRequest, "{}"));
    wsMock->emulateReceivedMessage(ws.relativeMs(300), generate_success_response(sentRequest, "{}"));

    ws.timer->addTimeMs(500);
    client.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == false);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
  }

  SECTION("Execute JSON RPC call with duplicated success reply (receive in differrent polls)")
  {
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);
    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.getMethod() == "svc1.method01");

    wsMock->emulateReceivedMessage(ws.relativeMs(100), generate_success_response(sentRequest, "{}"));
    wsMock->emulateReceivedMessage(ws.relativeMs(200), generate_success_response(sentRequest, "{}"));
    wsMock->emulateReceivedMessage(ws.relativeMs(300), generate_success_response(sentRequest, "{}"));

    ws.timer->addTimeMs(100);
    client.poll();

    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == false);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());

    {
      INFO("Receiving duplicated replies. Ignoring them");
      incomingResponses.clear();
      ws.timer->addTimeMs(500);
      client.poll();
      CHECK(incomingResponses.size() == 0);
    }
  }

  SECTION("Execute multiple JSON RPC calls between calls. Replies are received before first poll")
  {
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);
    client.rpcCall(client.createRpcRequest("svc2", "method02"), onResponse);
    client.rpcCall(client.createRpcRequest("svc3", "method03"), onResponse);
    REQUIRE(wsMock->sendCallCount == 3);
    const auto &req1 = wsMock->sentRequests[0];
    const auto &req2 = wsMock->sentRequests[1];
    const auto &req3 = wsMock->sentRequests[2];
    CHECK(req1.getMethod() == "svc1.method01");
    CHECK(req2.getMethod() == "svc2.method02");
    CHECK(req3.getMethod() == "svc3.method03");

    wsMock->emulateReceivedMessage(ws.relativeMs(2), generate_success_response(req1, "1001"));
    wsMock->emulateReceivedMessage(ws.relativeMs(3), generate_success_response(req2, "1002"));
    // this message will come first:
    wsMock->emulateReceivedMessage(ws.relativeMs(1), generate_success_response(req3, "1003"));
    ws.timer->addTimeMs(3);

    client.poll();

    REQUIRE(incomingResponses.size() == 3);

    REQUIRE(incomingResponses.responses[0]->isError() == false);
    REQUIRE(incomingResponses.responses[0]->getMessageId() == req3.getSerializedMessageId());
    REQUIRE(incomingResponses.responses[0]->getResult().IsInt());
    REQUIRE(incomingResponses.responses[0]->getResult().GetInt() == 1003);

    REQUIRE(incomingResponses.responses[1]->isError() == false);
    REQUIRE(incomingResponses.responses[1]->getMessageId() == req1.getSerializedMessageId());
    REQUIRE(incomingResponses.responses[1]->getResult().IsInt());
    REQUIRE(incomingResponses.responses[1]->getResult().GetInt() == 1001);

    REQUIRE(incomingResponses.responses[2]->isError() == false);
    REQUIRE(incomingResponses.responses[2]->getMessageId() == req2.getSerializedMessageId());
    REQUIRE(incomingResponses.responses[2]->getResult().IsInt());
    REQUIRE(incomingResponses.responses[2]->getResult().GetInt() == 1002);
  }

  {
    INFO("Destroy connection and make final checks");
    // no messages are sent or received any more:
    incomingResponses.clear();
    wsMock->clearSendHistory();

    client.poll();
    ws.timer->addTimeMs(9999);
    client.poll();

    CHECK(incomingResponses.size() == 0);
    CHECK(wsMock->sendCallCount == 0);

    clientPtr.reset();
  }
}


static void test_send_try_count(bool enable_request_retry_on_disconnect)
{
  WebsocketLibraryInterceptor ws;

  CAPTURE(enable_request_retry_on_disconnect);
  const int expectedTries = enable_request_retry_on_disconnect ? 2 : 1;
  CAPTURE(expectedTries);

  auto sepClientConfig = get_default_sep_config();
  sepClientConfig.requestTimeoutMs = 3000 + expectedTries * 3000;
  sepClientConfig.enableRequestRetryOnDisconnect = enable_request_retry_on_disconnect;

  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  REQUIRE(client.isCallable() == true);

  {
    INFO("Execute RPC call");
    client.rpcCall(client.createRpcRequest("svc1", "method01"), onResponse);
  }

  eastl::vector<tests::WebsocketMockPtr> wsMocks;
  wsMocks.reserve(expectedTries + 1);

  for (int tryNumber = 1; tryNumber <= expectedTries + 1; ++tryNumber)
  {
    const int connectedTimeMs = (tryNumber - 1) * 2000 + 1000;
    wsMocks.emplace_back(ws.addFutureConnection()->shouldSucceedConnectingAtMs(connectedTimeMs, connectedTimeMs + 1000));
  }

  {
    INFO("First poll to initiate connection");
    client.poll();
  }

  for (int mockIndex = 0; mockIndex < expectedTries; ++mockIndex)
  {
    CAPTURE(mockIndex);

    const auto &wsMock = wsMocks[mockIndex];

    {
      INFO("Wait until connected. Automatic send/retry should happened");
      ws.timer->addTimeMs(1000);
      CAPTURE(ws.timer->getCurrentTimeMs());
      CHECK(wsMock->sendCallCount == 0);
      client.poll();
      CHECK(incomingResponses.size() == 0);
      REQUIRE(wsMock->sendCallCount == 1);
      CHECK(wsMock->sentRequests[0].getMethod() == "svc1.method01");
    }

    {
      INFO("Wait until disconnect");
      ws.timer->addTimeMs(1000);
      CAPTURE(ws.timer->getCurrentTimeMs());
      client.poll();

      const bool lastAttempt = mockIndex == expectedTries - 1;
      if (!lastAttempt)
      {
        CHECK(incomingResponses.size() == 0); // no error response is generated, retry is expected
      }
      else
      {
        INFO("Last attempt fails after disconnect");
        REQUIRE(incomingResponses.size() == 1);
        const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
        CHECK(response.isError() == true);
        CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::LOST_WEBSOCKET_CONNECTION_AFTER_SENDING);
        incomingResponses.clear();
      }
    }
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();

    for (int mockIndex = 0; mockIndex <= expectedTries; ++mockIndex)
    {
      CAPTURE(mockIndex);
      const auto &wsMock = wsMocks[mockIndex];
      if (mockIndex == expectedTries)
      {
        CHECK(wsMock->sendCallCount == 0);
      }
      else
      {
        CHECK(wsMock->sendCallCount == 1);
      }
    }
    CHECK(incomingResponses.size() == 0);
  }
}


TEST_CASE("SepClient: execute RPC call with sudden disconnect before reply (test retry setting)", suiteTags)
{
  SECTION("Check SepClient setting: enableRequestRetryOnDisconnect = false") { test_send_try_count(false); }
  SECTION("Check SepClient setting: enableRequestRetryOnDisconnect = true") { test_send_try_count(true); }
}


TEST_CASE("SepClient: execute RPC call with initiated stop before reply", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 4500);

  auto sepClientConfig = get_default_sep_config();
  sepClientConfig.requestTimeoutMs = 3000;

  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  IncomingResponseHistory succesResponses;
  IncomingResponseHistory timeoutResponses;
  IncomingResponseHistory abortedResponses; // abort because of disconnect

  {
    INFO("Wait until connected");
    client.poll();
    ws.timer->addTimeMs(1000);
    client.poll();
    REQUIRE(client.isCallable() == true);
  }

  {
    INFO("Execute RPC calls");

    client.rpcCall(client.createRpcRequest("svc1", "method01"), succesResponses.createCallback());
    REQUIRE(wsMock->sendCallCount == 1);
    const auto &req1 = wsMock->sentRequests[0];
    wsMock->emulateReceivedMessage(ws.relativeMs(2900), generate_success_response(req1, "{}")); // success at 3900

    CHECK(ws.timer->getCurrentTimeMs() == 1000);
    client.rpcCall(client.createRpcRequest("svc2", "method02"), timeoutResponses.createCallback()); // timeout at 4000
    ws.timer->addTimeMs(1000);
    client.rpcCall(client.createRpcRequest("svc3", "method03"), abortedResponses.createCallback()); // timeout at 5000
  }

  {
    INFO("Initiate close");
    client.initiateClose();
    client.poll(); // apply initiateClose
  }


  {
    INFO("Read success response during connection closing phase");
    ws.timer->addTimeMs(1800);
    CHECK(ws.timer->getCurrentTimeMs() == 3800);

    client.poll();
    CHECK(succesResponses.size() == 0);

    ws.timer->addTimeMs(100);
    client.poll();
    CHECK(client.isCloseCompleted() == false);
    CHECK(succesResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = succesResponses.last();
    CHECK(response.isError() == false);
    CHECK(response.getMessageId() == wsMock->sentRequests[0].getSerializedMessageId());
  }

  {
    INFO("Read timeout response");
    ws.timer->addTimeMs(100);
    CHECK(ws.timer->getCurrentTimeMs() == 4000);

    CHECK(timeoutResponses.size() == 0);
    client.poll();
    CHECK(client.isCloseCompleted() == false);
    CHECK(timeoutResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = timeoutResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT);
  }

  {
    INFO("Read aborted RPC response");
    ws.timer->addTimeMs(500);
    CHECK(ws.timer->getCurrentTimeMs() == 4500);

    CHECK(abortedResponses.size() == 0);
    client.poll();
    CHECK(client.isCloseCompleted() == true);
    CHECK(abortedResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = abortedResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::LOST_WEBSOCKET_CONNECTION_AFTER_SENDING);
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();
  }
}
