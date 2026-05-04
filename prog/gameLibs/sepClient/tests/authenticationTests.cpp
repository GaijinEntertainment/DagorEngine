// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/sepClient.h>

#include <sepTestHelpers/sepClientVerify.h>

#include <testHelpers/websocketLibraryInterceptor.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>


static constexpr char suiteTags[] = "[sepClient][authentication]";
using namespace tests;


namespace
{

const auto get_default_sep_config = []() {
  sepclient::SepClientConfig config;
  config.serverUrls.emplace_back("wss://localhost:12345");
  config.authorizationToken = "test_dummy_token";
  config.logLevel = sepclient::LogLevel::FULL_DUMP; // execute as much code as possible
  config.reconnectDelay.maxDelayMs = 0;             // do not make delays for now
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

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  int authFailureCount = 0;

  const auto processAuthenticationFailuresCallback = [&ws, &client, &authFailureCount](
                                                       sepclient::AuthenticationFailurePtr &&authentication_failure) {
    constexpr auto tokenGenerator = [](int failureCount) {
      return eastl::string(eastl::string::CtorSprintf(), "new_token_after_failed_auth_%d", failureCount);
    };

    const auto now = ws.timer->getCurrentTimeMs();
    CAPTURE(now);
    CAPTURE(authentication_failure->authorizationToken);

    ++authFailureCount;
    CAPTURE(authFailureCount);
    const eastl::string expectedPreviousToken = authFailureCount == 1 ? "test_dummy_token" : tokenGenerator(authFailureCount - 1);
    const eastl::string nextToken = tokenGenerator(authFailureCount);

    REQUIRE(authentication_failure->authorizationToken == expectedPreviousToken);
    REQUIRE(authentication_failure->serverUrl == "wss://localhost:12345");
    REQUIRE(authentication_failure->errorMessage == "SEP: Authentication error: invalid auth token");

    client.updateAuthorizationToken(nextToken);
  };

  client.initialize(processAuthenticationFailuresCallback, no_incoming_requests_expected_callback, nullptr);

  while (true)
  {
    client.poll();
    const auto now = ws.timer->getCurrentTimeMs();
    CAPTURE(now);

    const auto nowInRange = [now](int start, int end) { return now >= start && now < end; };

    verify_client_state(client, SepClientState::WORKING);

    if (now > 15000)
    {
      break;
    }

    ws.timer->addTimeMs(250);
  }

  {
    INFO("Destroy connection and make final checks");
    clientPtr.reset();

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
