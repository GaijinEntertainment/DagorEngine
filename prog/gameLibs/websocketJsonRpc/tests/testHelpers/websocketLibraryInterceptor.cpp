// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "websocketLibraryInterceptor.h"

#include <catch2/catch_test_macros.hpp>


namespace tests
{


void match_ws_mock_scenario(const tests::WebsocketMockPtr &wsConnection, WsMockScenario scenario)
{
  CAPTURE(scenario);

  using MinMax = eastl::pair<int, int>;
  MinMax expectedConnectCount = {};
  MinMax expectedCloseCount = {};
  MinMax expectedPollCount = {};

  switch (scenario)
  {
    case WsMockScenario::NOTHING_CALLED:
      expectedConnectCount = {0, 0};
      expectedCloseCount = {0, 0};
      expectedPollCount = {0, 0};
      break;

    case WsMockScenario::CONNECT_THEN_NO_CLOSE:
      expectedConnectCount = {1, 1};
      expectedCloseCount = {0, 0};
      expectedPollCount = {1, 100};
      break;

    case WsMockScenario::CONNECT_THEN_CLOSE:
      expectedConnectCount = {1, 1};
      expectedCloseCount = {1, 1};
      expectedPollCount = {1, 100};
      break;

    case WsMockScenario::IF_CONNECT_THEN_CLOSE:
      expectedConnectCount = {0, 1};
      expectedCloseCount = {0, 1};
      expectedPollCount = {0, 100};
      break;

    case WsMockScenario::IF_CONNECT_THEN_NO_CLOSE:
      expectedConnectCount = {0, 1};
      expectedCloseCount = {0, 0};
      expectedPollCount = {0, 100};
      break;

    default: FAIL("Unknown WsMockScenario value");
  }

  REQUIRE(wsConnection->connectCallCount >= expectedConnectCount.first);
  REQUIRE(wsConnection->connectCallCount <= expectedConnectCount.second);

  REQUIRE(wsConnection->closeCallCount >= expectedCloseCount.first);
  REQUIRE(wsConnection->closeCallCount <= expectedCloseCount.second);

  REQUIRE(wsConnection->pollCallCount >= expectedPollCount.first);
  REQUIRE(wsConnection->pollCallCount <= expectedPollCount.second);
}


} // namespace tests
