// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <websocket/websocket.h>
#include <websocketJsonRpc/details/abstractionLayer.h>
#include <websocketJsonRpc/details/commonTypes.h>
#include <websocketJsonRpc/rpcRequest.h>
#include <websocketJsonRpc/rpcResponse.h>

#include <catch2/catch_tostring.hpp>

#include <EASTL/deque.h>
#include <EASTL/priority_queue.h>


namespace tests
{

using TickCount = websocketjsonrpc::TickCount;


struct WebsocketMessage
{
  TickCount messageTime = 0; // received time or expected send time (depends on direction)
  eastl::string messageData;
  bool isText = true;
};


// Make priority_queue to store elements with smaller messageTime on the top:
struct LessWebsocketMessagePriority final
{
  bool operator()(const WebsocketMessage &a, const WebsocketMessage &b) const noexcept { return a.messageTime > b.messageTime; }
};


// ===========================================================


class WebsocketMock final : public websocket::WebSocketClient, public eastl::enable_shared_from_this<WebsocketMock>
{
protected:
  using ConnectCb = websocket::ConnectCb;
  using Headers = websocket::Headers;
  using PayloadView = websocket::PayloadView;
  using CloseStatus = websocket::CloseStatus;

  using WebsocketMockPtr = eastl::shared_ptr<WebsocketMock>;

private:
  void connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
    const Headers &additional_headers) override;

  void send(PayloadView message, bool is_text) override;

  void close(CloseStatus) override;

  void poll() override;

public:
  void initialize(websocket::ClientConfig &&config_);

public:
  WebsocketMockPtr shouldFailConnectingAtMs(int fail_ms);
  WebsocketMockPtr shouldSucceedConnectingAtMs(int connect_ms, int break_connection_ms);

  WebsocketMockPtr shouldFailAuthenticationAtMs(int timestamp_ms);
  WebsocketMockPtr shouldSucceedAuthenticationAtMs(int timestamp_ms);

  WebsocketMockPtr emulateReceivedMessage(int message_time_ms, eastl::string_view messageData, bool isText = true);

  const websocketjsonrpc::RpcRequest &lastSentRequest() const;
  const websocketjsonrpc::RpcResponse &lastSentResponse() const;
  void clearSendHistory();

public:
  int getNotReceivedMessageCount() const { return receiveQueue.size(); }

public:
  enum State
  {
    INITIAL,
    INITIALIZED,
    CONNECT_CALLED, // `connect` should not be called after `close` is called
    CONNECTED,      // connection succeeded
    CLOSE_CALLED,
    CLOSED,
  };

public:
  websocket::ClientConfig config;

  // stats
  int connectCallCount = 0;
  int sendCallCount = 0;
  int onMessageCallbackCallCount = 0;
  int closeCallCount = 0;
  int pollCallCount = 0;

  State state = State::INITIAL; // the state is changed by `WebSocketClient` interface methods

  // saved data
  eastl::string connectUri;
  eastl::string connectIpHint;
  websocket::Headers connectHeaders;
  eastl::deque<websocketjsonrpc::RpcRequest> sentRequests;
  eastl::deque<websocketjsonrpc::RpcResponse> sentResponses;

private:
  TickCount succeedConnectingTick = 1'000;
  bool authenticationWillFail = false;
  TickCount finishAuthenticationTick = 0; // 0 means `at succeedConnectingTick`
  TickCount failConnectingTick = 0;
  TickCount breakConnectionTick = 60'000;

  TickCount lastConnectCallTick = 0;

  ConnectCb connectCallback;
  eastl::priority_queue<WebsocketMessage, eastl::vector<WebsocketMessage>, LessWebsocketMessagePriority> receiveQueue;
};


// ===========================================================


using WebsocketMockPtr = eastl::shared_ptr<WebsocketMock>;


// ===========================================================


class WebsocketMockFactory final : public websocketjsonrpc::abstraction::WebsocketClientFactory
{
public:
  using WebSocketClientPtr = websocketjsonrpc::abstraction::WebSocketClientPtr;

  WebSocketClientPtr createClient(websocket::ClientConfig &&config) override;

  void addFutureClient(const WebsocketMockPtr &client) { futureClients.emplace_back(client); }

public:
  int createClientCallCount = 0;

  eastl::deque<WebsocketMockPtr> futureClients;
};


// ===========================================================


eastl::string generate_success_response(const websocketjsonrpc::RpcRequest &request, const eastl::string &json_result);

eastl::string generate_error_response(const websocketjsonrpc::RpcRequest &request, int error_code, const eastl::string &error_message);


} // namespace tests


CATCH_REGISTER_ENUM(tests::WebsocketMock::State, tests::WebsocketMock::State::INITIAL, tests::WebsocketMock::State::INITIALIZED,
  tests::WebsocketMock::State::CONNECT_CALLED, tests::WebsocketMock::State::CONNECTED, tests::WebsocketMock::State::CLOSE_CALLED,
  tests::WebsocketMock::State::CLOSED);
