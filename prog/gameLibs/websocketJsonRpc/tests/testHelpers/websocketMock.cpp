// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "websocketMock.h"

#include <websocketJsonRpc/details/abstractionLayer.h>

#include <catch2/catch_test_macros.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>


using WebSocketClient = websocket::WebSocketClient;


using namespace tests;


// ===========================================================


void WebsocketMock::connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
  const Headers &additional_headers)
{
  INFO("WebsocketMockFactory::connect");
  connectCallCount++;
  lastConnectCallTick = websocketjsonrpc::abstraction::get_current_tick_count();

  REQUIRE(state == INITIALIZED);
  state = CONNECT_CALLED;

  connectIpHint = connect_ip_hint;
  connectUri = uri;
  connectHeaders = additional_headers;
  connectCallback = eastl::move(connect_cb);
  REQUIRE(!!connectCallback);
}


void WebsocketMock::send(PayloadView message, bool is_text)
{
  INFO("WebsocketMockFactory::send");
  sendCallCount++;

  CAPTURE(state);
  REQUIRE((state == CONNECT_CALLED || state == CONNECTED));
  REQUIRE(is_text == true);

  const eastl::string_view messageData{reinterpret_cast<const char *>(message.data()), message.size()};

  rapidjson::Document jsonDoc;
  jsonDoc.Parse(messageData.data(), messageData.size());
  REQUIRE(jsonDoc.HasParseError() == false);

  const bool isRequest = jsonDoc.IsObject() && jsonDoc.GetObject().HasMember("method");
  if (isRequest)
  {
    sentRequests.emplace_back(eastl::move(jsonDoc), messageData.size());
    REQUIRE(sentRequests.back().isValid());
  }
  else
  {
    sentResponses.emplace_back(eastl::move(jsonDoc), messageData.size());
  }
}


void WebsocketMock::close(CloseStatus)
{
  INFO("WebsocketMockFactory::close");
  closeCallCount++;

  if (state != CLOSED)
  {
    state = CLOSE_CALLED;
  }
}


void WebsocketMock::poll()
{
  INFO("WebsocketMockFactory::poll");
  pollCallCount++;

  TickCount now = 0;

  now = websocketjsonrpc::abstraction::get_current_tick_count();
  if (succeedConnectingTick != 0 && succeedConnectingTick <= now)
  {
    CAPTURE(now);
    CAPTURE(state);
    REQUIRE((state == CONNECT_CALLED || state == CLOSE_CALLED));

    // do not connect immediately without a `connect` call in advance. It may be a bug in the test.
    REQUIRE(now > lastConnectCallTick);

    if (finishAuthenticationTick == 0)
    {
      finishAuthenticationTick = succeedConnectingTick;
    }
    succeedConnectingTick = 0;
    state = CONNECTED;
    REQUIRE(!!connectCallback);
    connectCallback(this, websocket::ConnectStatus::Ok, connectIpHint, websocket::Error::no_error());
  }

  now = websocketjsonrpc::abstraction::get_current_tick_count();
  if (finishAuthenticationTick != 0 && finishAuthenticationTick <= now)
  {
    CAPTURE(now);
    CAPTURE(state);
    REQUIRE(state == CONNECTED);

    finishAuthenticationTick = 0;

    eastl::string message;

    if (authenticationWillFail)
    {
      message =
        R"({"jsonrpc":"2.0","id":null,"error":{"code":-29502,"message":"SEP: Authentication error: invalid auth token","data":{"contextName":"CatchUnitTest"}}})";
    }
    else
    {
      message = R"({"jsonrpc":"2.0","id":null,"result":{"status":"ok","details":"SEP: Authentication passed"}})";
    }

    config.onMessage(this, websocket::PayloadView{reinterpret_cast<const uint8_t *>(message.data()), message.size()}, true);
  }

  now = websocketjsonrpc::abstraction::get_current_tick_count();
  if (failConnectingTick != 0 && failConnectingTick <= now)
  {
    CAPTURE(now);
    CAPTURE(state);
    REQUIRE((state == CONNECT_CALLED || state == CLOSE_CALLED));
    failConnectingTick = 0;
    state = CONNECTED;
    REQUIRE(!!connectCallback);
    websocket::Error error;
    error.code = -1;
    error.message = "Simulated failure to connect";
    connectCallback(this, websocket::ConnectStatus::HttpConnectFailed, connectIpHint, eastl::move(error));
  }

  now = websocketjsonrpc::abstraction::get_current_tick_count();
  while (!receiveQueue.empty() && receiveQueue.top().messageTime <= now)
  {
    WebsocketMessage message;
    receiveQueue.pop(message);

    CAPTURE(now);
    CAPTURE(state);
    REQUIRE((state == CONNECTED || state == CLOSE_CALLED));

    REQUIRE(!!config.onMessage);
    onMessageCallbackCallCount++;
    config.onMessage(this,
      websocket::PayloadView{reinterpret_cast<const uint8_t *>(message.messageData.data()), message.messageData.size()},
      message.isText);
  }

  now = websocketjsonrpc::abstraction::get_current_tick_count();
  if (breakConnectionTick != 0 && breakConnectionTick <= now)
  {
    CAPTURE(now);
    CAPTURE(state);
    REQUIRE((state == CONNECTED || state == CLOSE_CALLED));
    breakConnectionTick = 0;
    state = CLOSED;
    REQUIRE(!!config.onClose);
    websocket::Error error;
    error.code = -2;
    error.message = "Simulated internet connection failure";
    config.onClose(this, (int)websocket::CloseStatus::Normal, eastl::move(error));
  }
}

// ===========================================================


void WebsocketMock::initialize(websocket::ClientConfig &&config_)
{
  INFO("WebsocketMockFactory::initialize");
  REQUIRE(state == INITIAL);
  state = INITIALIZED;
  config = eastl::move(config_);
}


WebsocketMockPtr WebsocketMock::shouldFailConnectingAtMs(int fail_ms)
{
  REQUIRE(finishAuthenticationTick == 0);
  const TickCount ticksPerSecond = websocketjsonrpc::abstraction::get_ticks_per_second();

  succeedConnectingTick = 0;
  failConnectingTick = fail_ms * ticksPerSecond / 1000;
  breakConnectionTick = 0;
  return shared_from_this();
}


WebsocketMockPtr WebsocketMock::shouldSucceedConnectingAtMs(int connect_ms, int break_connection_ms)
{
  REQUIRE(connect_ms <= break_connection_ms);

  const TickCount ticksPerSecond = websocketjsonrpc::abstraction::get_ticks_per_second();

  succeedConnectingTick = connect_ms * ticksPerSecond / 1000;
  failConnectingTick = 0;
  breakConnectionTick = break_connection_ms * ticksPerSecond / 1000;

  if (finishAuthenticationTick != 0)
  {
    REQUIRE(succeedConnectingTick <= finishAuthenticationTick);
  }
  return shared_from_this();
}


WebsocketMockPtr WebsocketMock::shouldFailAuthenticationAtMs(int timestamp_ms)
{
  REQUIRE(succeedConnectingTick > 0);
  const TickCount ticksPerSecond = websocketjsonrpc::abstraction::get_ticks_per_second();

  authenticationWillFail = true;
  finishAuthenticationTick = timestamp_ms * ticksPerSecond / 1000;

  REQUIRE(succeedConnectingTick <= finishAuthenticationTick);
  return shared_from_this();
}


WebsocketMockPtr WebsocketMock::shouldSucceedAuthenticationAtMs(int timestamp_ms)
{
  REQUIRE(succeedConnectingTick > 0);
  const TickCount ticksPerSecond = websocketjsonrpc::abstraction::get_ticks_per_second();

  authenticationWillFail = false;
  finishAuthenticationTick = timestamp_ms * ticksPerSecond / 1000;

  REQUIRE(succeedConnectingTick <= finishAuthenticationTick);
  return shared_from_this();
}


WebsocketMockPtr WebsocketMock::emulateReceivedMessage(int message_time_ms, eastl::string_view messageData, bool isText)
{
  const TickCount ticksPerSecond = websocketjsonrpc::abstraction::get_ticks_per_second();

  receiveQueue.emplace(WebsocketMessage{message_time_ms * ticksPerSecond / 1000, eastl::string{messageData}, isText});
  return shared_from_this();
}


const websocketjsonrpc::RpcRequest &WebsocketMock::lastSentRequest() const
{
  REQUIRE(sentRequests.size() > 0);
  return sentRequests.back();
}


const websocketjsonrpc::RpcResponse &WebsocketMock::lastSentResponse() const
{
  REQUIRE(sentResponses.size() > 0);
  return sentResponses.back();
}


void WebsocketMock::clearSendHistory()
{
  sentRequests.clear();
  sentResponses.clear();
  sendCallCount = 0;
}

// ===========================================================


template <typename Client>
struct WebSocketClientProxy final : public WebSocketClient
{
public:
  using ConnectCb = websocket::ConnectCb;
  using Headers = websocket::Headers;
  using PayloadView = websocket::PayloadView;
  using CloseStatus = websocket::CloseStatus;

public:
  explicit WebSocketClientProxy(const eastl::shared_ptr<Client> &impl_) : impl(impl_), implOriginalType(impl_) {}

  void connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
    const Headers &additional_headers) override
  {
    impl->connect(uri, connect_ip_hint, eastl::move(connect_cb), additional_headers);
  }

  void send(PayloadView message, bool is_text) override { impl->send(message, is_text); }

  void close(CloseStatus status) override { impl->close(status); }

  void poll() override { impl->poll(); }

private:
  eastl::shared_ptr<websocket::WebSocketClient> impl;
  eastl::shared_ptr<Client> implOriginalType; // for debugging
};


// ===========================================================


WebsocketMockFactory::WebSocketClientPtr WebsocketMockFactory::createClient(websocket::ClientConfig &&config)
{
  INFO("WebsocketMockFactory::createClient");
  createClientCallCount++;

  if (futureClients.empty())
  {
    FAIL("unexpected test case: no more WS clients are ready for the test");
    return nullptr;
  }

  const WebsocketMockPtr client = futureClients.front();
  futureClients.pop_front();

  client->initialize(eastl::move(config));

  return eastl::make_unique<WebSocketClientProxy<WebsocketMock>>(client);
}


// ===========================================================


eastl::string tests::generate_success_response(const websocketjsonrpc::RpcRequest &request, const eastl::string &json_result)
{
  REQUIRE(json_result.empty() == false); // it is not allowed to skip result in JSON RPC 2.0
  REQUIRE(request.isNotification() == false);
  const eastl::string responseData{eastl::string::CtorSprintf{}, R"({"jsonrpc": "2.0", "id": %s, "result": %s})",
    request.getSerializedMessageId().c_str(), json_result.c_str()};
  return responseData;
}


eastl::string tests::generate_error_response(const websocketjsonrpc::RpcRequest &request, int error_code,
  const eastl::string &error_message)
{
  REQUIRE(request.isNotification() == false);
  const eastl::string &messageId = request.getSerializedMessageId();

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  {
    writer.StartObject();

    writer.Key("jsonrpc");
    writer.String("2.0");

    writer.Key("id");
    writer.RawValue(messageId.data(), messageId.size(), rapidjson::Type::kObjectType);

    writer.Key("error");
    {
      writer.StartObject();

      writer.Key("code");
      writer.Int(error_code);

      writer.Key("message");
      writer.String(error_message.c_str());

      writer.EndObject();
    }

    writer.EndObject();
  }

  REQUIRE(writer.IsComplete());

  return {buffer.GetString(), buffer.GetSize()};
}


// ===========================================================
