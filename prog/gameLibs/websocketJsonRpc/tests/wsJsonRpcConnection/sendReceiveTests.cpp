// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <testHelpers/incomingRequestHistory.h>
#include <testHelpers/incomingResponseHistory.h>
#include <testHelpers/rpcConnectionVerify.h>
#include <testHelpers/websocketLibraryInterceptor.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>


static constexpr char suiteTags[] = "[wsJsonRpcConnection][sendReceive]";
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


TEST_CASE("WsJsonRpcConnection: Receive unexpected data and discard it safely", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 999'999);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;

  SECTION("Set incoming request callback") { connection.setIncomingRequestCallback(no_incoming_requests_expected_callback); }

  SECTION("Skip setting incoming request callback") { connection.setIncomingRequestCallback(nullptr); }

  {
    INFO("Wait until connected");
    connection.poll();
    ws.timer->addTimeMs(1000);
    connection.poll();
    REQUIRE(connection.isConnected());
  }

  REQUIRE(wsMock->onMessageCallbackCallCount == 0);
  int receivedMessageCount = 0;

  for (int isTextInt = 0; isTextInt <= 1; ++isTextInt)
  {
    const bool isText = !!isTextInt;
    CAPTURE(isText);
    {
      INFO("Receiving empty string body");
      wsMock->emulateReceivedMessage(ws.relativeMs(0), "", isText);
      connection.poll();
      REQUIRE(wsMock->onMessageCallbackCallCount == ++receivedMessageCount);
    }
    {
      INFO("Receiving random text");
      wsMock->emulateReceivedMessage(ws.relativeMs(0), "Hello, World!", isText);
      connection.poll();
      REQUIRE(wsMock->onMessageCallbackCallCount == ++receivedMessageCount);
    }
    {
      INFO("Receiving empty dict body");
      wsMock->emulateReceivedMessage(ws.relativeMs(0), "{}", isText);
      connection.poll();
      REQUIRE(wsMock->onMessageCallbackCallCount == ++receivedMessageCount);
    }
    {
      INFO("Receiving empty list body");
      wsMock->emulateReceivedMessage(ws.relativeMs(0), "[]", isText);
      connection.poll();
      REQUIRE(wsMock->onMessageCallbackCallCount == ++receivedMessageCount);
    }
    {
      INFO("Receiving invalid incoming request JSON RPC message");
      wsMock->emulateReceivedMessage(ws.relativeMs(0), R"( {"jsonrpc": "2.0", "method": null } )", isText);
      connection.poll();
      REQUIRE(wsMock->onMessageCallbackCallCount == ++receivedMessageCount);
    }
    if (!isText)
    {
      INFO("Receiving BINARY (unsupported) incoming request JSON RPC message (notification)");
      wsMock->emulateReceivedMessage(ws.relativeMs(0), R"( {"jsonrpc": "2.0", "method": "method01" } )", isText);
      connection.poll();
      REQUIRE(wsMock->onMessageCallbackCallCount == ++receivedMessageCount);
    }
  }

  {
    INFO("Destroy connection and make final checks");
    // no messages are received any more:
    connection.poll();
    ws.timer->addTimeMs(9999);
    connection.poll();

    CHECK(wsMock->sendCallCount == 0);
    CHECK(wsMock->onMessageCallbackCallCount == receivedMessageCount);

    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}


TEST_CASE("WsJsonRpcConnection: receive server-initiated JSON RPC messages (reverse direction)", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 999'999);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;

  IncomingRequestHistory incomingRequests;
  bool haveIncomingRequestCallback = true;

  SECTION("Set incoming request callback") { connection.setIncomingRequestCallback(incomingRequests.createCallback()); }

  SECTION("Skip setting incoming request callback") { haveIncomingRequestCallback = false; }

  {
    INFO("Wait until connected");
    connection.poll();
    ws.timer->addTimeMs(1000);
    connection.poll();
    REQUIRE(connection.isConnected());
  }

  {
    INFO("Receiving JSON RPC notification");
    wsMock->emulateReceivedMessage(ws.relativeMs(0), R"( {"jsonrpc": "2.0", "method": "method01"} )");
    incomingRequests.clear();
    connection.poll();
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
    INFO("Receiving JSON RPC request");
    wsMock->emulateReceivedMessage(ws.relativeMs(0),
      R"( {"jsonrpc": "2.0", "method": "method02", "id": "0.01.05-myID", "params": [-10,20,30]} )");
    incomingRequests.clear();
    connection.poll();
    if (haveIncomingRequestCallback)
    {
      REQUIRE(incomingRequests.size() == 1);
      const websocketjsonrpc::RpcRequest &request = incomingRequests.last();
      REQUIRE(request.isValid());
      CHECK(request.isNotification() == false);
      CHECK(request.getSerializedMessageId() == "\"0.01.05-myID\"");
      CHECK(request.getMethod() == "method02");
      REQUIRE(request.getParams() != nullptr);
      REQUIRE(request.getParams()->IsArray());
      REQUIRE(request.getParams()->GetArray().Size() == 3);
      CHECK(request.getParams()->GetArray()[0].GetInt() == -10);
      CHECK(request.getParams()->GetArray()[1].GetInt() == 20);
      CHECK(request.getParams()->GetArray()[2].GetInt() == 30);
    }
  }

  {
    INFO("Receiving JSON RPC request and sending error response");
    wsMock->emulateReceivedMessage(ws.relativeMs(0), R"( {"jsonrpc": "2.0", "method": "method.03", "id": 222} )");
    incomingRequests.clear();
    connection.poll();
    if (haveIncomingRequestCallback)
    {
      REQUIRE(incomingRequests.size() == 1);
      const websocketjsonrpc::RpcRequest &request = incomingRequests.last();
      REQUIRE(request.isValid());
      CHECK(request.isNotification() == false);
      CHECK(request.getSerializedMessageId() == "222");
      CHECK(request.getMethod() == "method.03");
      REQUIRE(request.getParams() == nullptr);

      wsMock->clearSendHistory();
      connection.sendResponseError("222", -32600, "Testing failed reverse request", "Testing error context");
      REQUIRE(wsMock->sendCallCount == 1);
      const websocketjsonrpc::RpcResponse &response = wsMock->lastSentResponse();
      REQUIRE(response.isError() == true);
      REQUIRE(response.getMessageId() == "222");
      REQUIRE(response.getErrorCode() == -32600);
    }
  }

  {
    INFO("Receiving JSON RPC request and sending success response");
    wsMock->emulateReceivedMessage(ws.relativeMs(0), R"( {"jsonrpc": "2.0", "method": "method.03", "id": 333} )");
    incomingRequests.clear();
    connection.poll();
    if (haveIncomingRequestCallback)
    {
      REQUIRE(incomingRequests.size() == 1);
      const websocketjsonrpc::RpcRequest &request = incomingRequests.last();
      REQUIRE(request.isValid());
      CHECK(request.isNotification() == false);
      CHECK(request.getSerializedMessageId() == "333");
      CHECK(request.getMethod() == "method.03");
      REQUIRE(request.getParams() == nullptr);

      rapidjson::Document responseResult(rapidjson::kArrayType);
      wsMock->clearSendHistory();
      connection.sendResponseResult("333", responseResult);
      REQUIRE(wsMock->sendCallCount == 1);
      const websocketjsonrpc::RpcResponse &response = wsMock->lastSentResponse();
      REQUIRE(response.isError() == false);
      REQUIRE(response.getMessageId() == "333");
      REQUIRE(response.getResult().IsArray());
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
    connection.poll();
    if (haveIncomingRequestCallback)
    {
      REQUIRE(incomingRequests.size() == 6);

      CHECK(incomingRequests.requests[0]->isNotification() == true);
      CHECK(incomingRequests.requests[0]->getMethod() == "method01");

      CHECK(incomingRequests.requests[1]->isNotification() == true);
      CHECK(incomingRequests.requests[1]->getMethod() == "method01");

      CHECK(incomingRequests.requests[2]->isNotification() == true);
      CHECK(incomingRequests.requests[2]->getMethod() == "method_another");

      CHECK(incomingRequests.requests[3]->isNotification() == false);
      CHECK(incomingRequests.requests[3]->getSerializedMessageId() == "1");
      CHECK(incomingRequests.requests[3]->getMethod() == "req01");

      CHECK(incomingRequests.requests[4]->isNotification() == false);
      CHECK(incomingRequests.requests[4]->getSerializedMessageId() == "2");
      CHECK(incomingRequests.requests[4]->getMethod() == "req01");

      CHECK(incomingRequests.requests[5]->isNotification() == false);
      CHECK(incomingRequests.requests[5]->getSerializedMessageId() == "3");
      CHECK(incomingRequests.requests[5]->getMethod() == "req_another");
    }
  }

  {
    INFO("Destroy connection and make final checks");
    // no messages are sent or received any more:
    wsMock->clearSendHistory();
    incomingRequests.clear();

    connection.poll();
    ws.timer->addTimeMs(9999);
    connection.poll();

    REQUIRE(incomingRequests.size() == 0);
    CHECK(wsMock->sendCallCount == 0);

    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}


TEST_CASE("WsJsonRpcConnection: execute RPC call in incorrect moment", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 2000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;
  connection.setIncomingRequestCallback(no_incoming_requests_expected_callback);

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  {
    INFO("Execute RPC before first poll");

    incomingResponses.clear();
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);

    CHECK(wsMock->sendCallCount == 0);    // no actual send
    CHECK(incomingResponses.size() == 0); // no callback called

    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
  }

  {
    INFO("Execute RPC before connection established");

    incomingResponses.clear();
    ws.timer->addTimeMs(100);
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);

    CHECK(wsMock->sendCallCount == 0);    // no actual send
    CHECK(incomingResponses.size() == 0); // no callback called

    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
  }

  {
    INFO("Wait until connected");
    ws.timer->addTimeMs(1000);
    connection.poll();
    REQUIRE(connection.isConnected());
  }

  SECTION("Initiate connection close and execute RPC call after initiateClose")
  {
    REQUIRE(connection.isConnected() == true);
    connection.initiateClose();
    REQUIRE(connection.isConnected() == false);
    REQUIRE(connection.isClosed() == false);

    incomingResponses.clear();
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);

    CHECK(wsMock->sendCallCount == 0);    // no actual send
    CHECK(incomingResponses.size() == 0); // no callback called

    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
  }

  SECTION("Skip closing connection manually") { CHECK(true); }

  {
    INFO("Wait until connection is closed");
    ws.timer->addTimeMs(1000);
    REQUIRE(connection.isClosed() == false);
    connection.poll();
    REQUIRE(connection.isConnected() == false);
    REQUIRE(connection.isClosed() == true);
  }

  {
    INFO("Execute RPC call after connection is closed");

    incomingResponses.clear();
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);

    CHECK(wsMock->sendCallCount == 0);    // no actual send
    CHECK(incomingResponses.size() == 0); // no callback called

    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::WEBSOCKET_CONNECTION_IS_NOT_READY);
  }

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
  }
}


TEST_CASE("WsJsonRpcConnection: execute RPC call", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 999'999);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;
  connection.setIncomingRequestCallback(no_incoming_requests_expected_callback);

  {
    INFO("Wait until connected");
    connection.poll();
    ws.timer->addTimeMs(1000);
    connection.poll();
    REQUIRE(connection.isConnected());
  }

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  SECTION("Execute JSON RPC call with timeout (no reply at all)")
  {
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);

    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.isNotification() == false);
    CHECK(sentRequest.getMethod() == "method01");
    CHECK(sentRequest.getParams() == nullptr); // missing "params" in request

    ws.timer->addTimeMs(999);
    connection.poll();
    CHECK(incomingResponses.size() == 0);

    ws.timer->addTimeMs(1);
    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT);
  }

  SECTION("Execute JSON RPC call with timeout (delayed reply, callback must not be called twice)")
  {
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);

    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.getMethod() == "method01");

    wsMock->emulateReceivedMessage(ws.relativeMs(1500), generate_success_response(sentRequest, "{}"));

    ws.timer->addTimeMs(1000);
    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT);

    {
      INFO("Do poll after delayed timed out response is received");
      incomingResponses.clear();
      ws.timer->addTimeMs(1000);
      connection.poll();
      CHECK(incomingResponses.size() == 0);
    }
  }

  SECTION("Execute JSON RPC call with error reply")
  {
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);
    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.isNotification() == false);
    CHECK(sentRequest.getMethod() == "method01");
    CHECK(sentRequest.getParams() == nullptr);

    wsMock->emulateReceivedMessage(ws.relativeMs(999), generate_error_response(sentRequest, -37500, "myMessage"));

    ws.timer->addTimeMs(998);
    connection.poll();
    CHECK(incomingResponses.size() == 0);

    ws.timer->addTimeMs(1);
    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
    CHECK(response.getErrorCode() == -37500);
    CHECK(response.getErrorMessage() == "myMessage");
  }

  SECTION("Execute JSON RPC call with success reply")
  {
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);
    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.isNotification() == false);
    CHECK(sentRequest.getMethod() == "method01");
    CHECK(sentRequest.getParams() == nullptr);

    wsMock->emulateReceivedMessage(ws.relativeMs(999), generate_success_response(sentRequest, "[5,4,3,2,1]"));

    ws.timer->addTimeMs(998);
    connection.poll();
    CHECK(incomingResponses.size() == 0);

    ws.timer->addTimeMs(1);
    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == false);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
    REQUIRE(response.getResult().IsArray());
    CHECK(response.getResult().GetArray().Size() == 5);
  }

  SECTION("Execute JSON RPC call with duplicated success reply (receive all at single poll)")
  {
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);
    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.getMethod() == "method01");

    wsMock->emulateReceivedMessage(ws.relativeMs(100), generate_success_response(sentRequest, "{}"));
    wsMock->emulateReceivedMessage(ws.relativeMs(200), generate_success_response(sentRequest, "{}"));
    wsMock->emulateReceivedMessage(ws.relativeMs(300), generate_success_response(sentRequest, "{}"));

    ws.timer->addTimeMs(500);
    connection.poll();
    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == false);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());
  }

  SECTION("Execute JSON RPC call with duplicated success reply (receive in differrent polls)")
  {
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);
    REQUIRE(wsMock->sendCallCount == 1);
    const websocketjsonrpc::RpcRequest &sentRequest = wsMock->lastSentRequest();
    CHECK(sentRequest.getMethod() == "method01");

    wsMock->emulateReceivedMessage(ws.relativeMs(100), generate_success_response(sentRequest, "{}"));
    wsMock->emulateReceivedMessage(ws.relativeMs(200), generate_success_response(sentRequest, "{}"));
    wsMock->emulateReceivedMessage(ws.relativeMs(300), generate_success_response(sentRequest, "{}"));

    ws.timer->addTimeMs(100);
    connection.poll();

    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == false);
    CHECK(response.getMessageId() == sentRequest.getSerializedMessageId());

    {
      INFO("Receiving duplicated replies. Ignoring them");
      incomingResponses.clear();
      ws.timer->addTimeMs(500);
      connection.poll();
      CHECK(incomingResponses.size() == 0);
    }
  }

  SECTION("Execute multiple JSON RPC calls between calls. Replies are received before first poll")
  {
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 1000);
    connection.rpcCall(next_rpc_message_id++, "method02", "", onResponse, 1000);
    connection.rpcCall(next_rpc_message_id++, "method03", "", onResponse, 1000);
    REQUIRE(wsMock->sendCallCount == 3);
    const auto &req1 = wsMock->sentRequests[0];
    const auto &req2 = wsMock->sentRequests[1];
    const auto &req3 = wsMock->sentRequests[2];
    CHECK(req1.getMethod() == "method01");
    CHECK(req2.getMethod() == "method02");
    CHECK(req3.getMethod() == "method03");

    wsMock->emulateReceivedMessage(ws.relativeMs(2), generate_success_response(req1, "1001"));
    wsMock->emulateReceivedMessage(ws.relativeMs(3), generate_success_response(req2, "1002"));
    // this message will come first:
    wsMock->emulateReceivedMessage(ws.relativeMs(1), generate_success_response(req3, "1003"));
    ws.timer->addTimeMs(3);

    connection.poll();

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

    connection.poll();
    ws.timer->addTimeMs(9999);
    connection.poll();

    CHECK(incomingResponses.size() == 0);
    CHECK(wsMock->sendCallCount == 0);

    connectionPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}


TEST_CASE("WsJsonRpcConnection: execute RPC call with sudden disconnect before reply", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 2000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;
  connection.setIncomingRequestCallback(no_incoming_requests_expected_callback);

  IncomingResponseHistory incomingResponses;
  const auto onResponse = incomingResponses.createCallback();

  {
    INFO("Wait until connected");
    connection.poll();
    ws.timer->addTimeMs(1000);
    connection.poll();
    REQUIRE(connection.isConnected());
  }

  {
    INFO("Execute RPC with disconnect before reply");
    connection.rpcCall(next_rpc_message_id++, "method01", "", onResponse, 3000);
    connection.poll();
  }

  {
    INFO("Wait until disconnect");
    ws.timer->addTimeMs(1000);
    connection.poll();
    REQUIRE(connection.isClosed());

    REQUIRE(incomingResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = incomingResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::LOST_WEBSOCKET_CONNECTION_AFTER_SENDING);
  }

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
  }
}


TEST_CASE("WsJsonRpcConnection: execute RPC call with initiated stop before reply", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 2000);

  auto connectionPtr = websocketjsonrpc::WsJsonRpcConnection::create(DEFAULT_WS_CONFIG);
  websocketjsonrpc::WsJsonRpcConnection &connection = *connectionPtr;
  connection.setIncomingRequestCallback(no_incoming_requests_expected_callback);

  IncomingResponseHistory succesResponses;
  IncomingResponseHistory timeoutResponses;
  IncomingResponseHistory abortedResponses; // abort because of disconnect

  {
    INFO("Wait until connected");
    connection.poll();
    ws.timer->addTimeMs(1000);
    connection.poll();
    REQUIRE(connection.isConnected());
  }

  {
    INFO("Execute RPC calls");

    connection.rpcCall(next_rpc_message_id++, "method01", "", succesResponses.createCallback(), 3000);
    REQUIRE(wsMock->sendCallCount == 1);
    const auto &req1 = wsMock->sentRequests[0];
    wsMock->emulateReceivedMessage(ws.relativeMs(950), generate_success_response(req1, "{}"));

    connection.rpcCall(next_rpc_message_id++, "method02", "", timeoutResponses.createCallback(), 750);
    connection.rpcCall(next_rpc_message_id++, "method03", "", abortedResponses.createCallback(), 3000);
  }

  {
    INFO("Initiate close");
    ws.timer->addTimeMs(500);
    connection.initiateClose();
    connection.poll(); // apply initiateClose
  }

  {
    INFO("Read timeout response");
    ws.timer->addTimeMs(250);

    CHECK(timeoutResponses.size() == 0);
    connection.poll();
    CHECK(timeoutResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = timeoutResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::RPC_CALL_TIMEOUT);
  }

  {
    INFO("Read success response during connection closing phase");
    ws.timer->addTimeMs(200);

    CHECK(succesResponses.size() == 0);
    connection.poll();
    CHECK(succesResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = succesResponses.last();
    CHECK(response.isError() == false);
    CHECK(response.getMessageId() == wsMock->sentRequests[0].getSerializedMessageId());
  }

  {
    INFO("Read aborted RPC response");
    ws.timer->addTimeMs(50);

    CHECK(abortedResponses.size() == 0);
    connection.poll();
    CHECK(abortedResponses.size() == 1);
    const websocketjsonrpc::RpcResponse &response = abortedResponses.last();
    CHECK(response.isError() == true);
    CHECK(response.getErrorCode() == (int)websocketjsonrpc::AdditionalErrorCodes::LOST_WEBSOCKET_CONNECTION_AFTER_SENDING);
  }

  {
    INFO("Destroy connection and make final checks");
    connectionPtr.reset();
  }
}
