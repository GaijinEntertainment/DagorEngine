// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <testHelpers/timerMock.h>
#include <testHelpers/websocketMock.h>

#include <catch2/catch_tostring.hpp>


namespace tests
{


inline websocketjsonrpc::RpcMessageId next_rpc_message_id = 1;


class WebsocketLibraryInterceptor final
{
public:
  WebsocketLibraryInterceptor() :
    connectionFactory(eastl::make_shared<tests::WebsocketMockFactory>()), timer(eastl::make_shared<tests::TimerMock>())
  {
    websocketjsonrpc::abstraction::set_custom_websocket_client_factory(connectionFactory);
    websocketjsonrpc::abstraction::set_custom_timer_implementation(timer);
  }

  ~WebsocketLibraryInterceptor()
  {
    websocketjsonrpc::abstraction::set_custom_websocket_client_factory(nullptr);
    websocketjsonrpc::abstraction::set_custom_timer_implementation(nullptr);
  }

  tests::WebsocketMockPtr addFutureConnection()
  {
    auto wsConnPtr = eastl::make_shared<tests::WebsocketMock>();
    connectionFactory->addFutureClient(wsConnPtr);
    return wsConnPtr;
  }

  TickCount relativeMs(int diff_ms) const { return timer->getRelativeMs(diff_ms); }

public:
  const eastl::shared_ptr<tests::WebsocketMockFactory> connectionFactory;
  const eastl::shared_ptr<tests::TimerMock> timer;
};


enum class WsMockScenario
{
  NOTHING_CALLED,
  CONNECT_THEN_NO_CLOSE,
  CONNECT_THEN_CLOSE,
  IF_CONNECT_THEN_CLOSE,
  IF_CONNECT_THEN_NO_CLOSE,
};


void match_ws_mock_scenario(const tests::WebsocketMockPtr &wsConnection, WsMockScenario scenario);

} // namespace tests


CATCH_REGISTER_ENUM(tests::WsMockScenario, tests::WsMockScenario::NOTHING_CALLED, tests::WsMockScenario::CONNECT_THEN_NO_CLOSE,
  tests::WsMockScenario::CONNECT_THEN_CLOSE, tests::WsMockScenario::IF_CONNECT_THEN_CLOSE,
  tests::WsMockScenario::IF_CONNECT_THEN_NO_CLOSE);
