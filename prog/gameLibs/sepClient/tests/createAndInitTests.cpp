// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/sepClient.h>

#include <sepTestHelpers/sepClientVerify.h>

#include <testHelpers/websocketLibraryInterceptor.h>

#include <debug/dag_log.h>

#include <catch2/catch_test_macros.hpp>


static constexpr char suiteTags[] = "[sepClient][createAndInit]";
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


const auto no_authentication_failures_expected_callback = [](sepclient::AuthenticationFailurePtr &&authentication_failure) {
  FAIL("Should not be executed here (no authentication failures are expected in this test)");
};


} // namespace


// ===============================================================================


TEST_CASE("Create SepClient and exit", suiteTags)
{
  WebsocketLibraryInterceptor ws;

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  verify_client_state(client, SepClientState::WORKING);

  {
    INFO("Destroy client");
    clientPtr.reset();
  }
}


TEST_CASE("Create SepClient, call `initiateClose` and exit", suiteTags)
{
  WebsocketLibraryInterceptor ws;

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  verify_client_state(client, SepClientState::WORKING);

  client.initiateClose();

  {
    INFO("Destroy client");
    clientPtr.reset();
  }
}


TEST_CASE("Create SepClient, call `poll` and exit", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection();

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  verify_client_state(client, SepClientState::WORKING);

  client.poll(); // connect is created here and `connect` is called

  verify_client_state(client, SepClientState::WORKING);

  {
    INFO("Destroy client and make final checks");
    clientPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}


TEST_CASE("Create SepClient and close it gracefully (initiateClose + poll in a loop)", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);
  const auto testStartTimeMs = ws.timer->getCurrentTimeMs();

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  client.initiateClose();

  {
    INFO("Wait until closing finishes");
    while (true)
    {
      client.poll();
      if (client.isCloseCompleted())
      {
        CAPTURE(ws.timer->getCurrentTimeMs());
        // Either it was immediately closed or it was connected and then closed after 3 seconds
        REQUIRE((ws.timer->getCurrentTimeMs() >= 3000 || ws.timer->getCurrentTimeMs() == testStartTimeMs));
        break;
      }
      verify_client_state(client, SepClientState::CLOSING);
      REQUIRE(ws.timer->getCurrentTimeMs() < 3000);
      ws.timer->addTimeMs(100);
    }
  }

  verify_client_state(client, SepClientState::CLOSED);

  {
    INFO("Destroy client and make final checks");
    clientPtr.reset();
    // Possible cases:
    // - Mock client could be not created
    // - Mock client could be created and destroyed without any calls
    // - Mock client could be created, initialized, called connect, called close
    match_ws_mock_scenario(wsMock, WsMockScenario::IF_CONNECT_THEN_CLOSE);
  }
}


TEST_CASE("Create SepClient, call `poll` and close it gracefully (initiateClose + poll in a loop)", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  // Emulate case when game called poll() multiple times,
  // started client
  // and then initiated close before client was established
  client.poll();
  client.poll();
  client.poll();

  client.initiateClose();

  {
    INFO("Wait until closing finishes");
    while (true)
    {
      client.poll();
      if (client.isCloseCompleted())
      {
        REQUIRE(ws.timer->getCurrentTimeMs() >= 3000);
        break;
      }
      verify_client_state(client, SepClientState::CLOSING);
      REQUIRE(ws.timer->getCurrentTimeMs() < 3000);
      ws.timer->addTimeMs(100);
    }
  }

  verify_client_state(client, SepClientState::CLOSED);

  {
    INFO("Destroy client and make final checks");
    clientPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_CLOSE);
  }
}


TEST_CASE("Create SepClient and wait until client attempt will fail", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldFailConnectingAtMs(1000);
  const auto wsMock2 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1500, 3500);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  {
    INFO("Wait until internal client attempt failure and second connection succeeds");
    while (true)
    {
      client.poll();
      verify_client_state(client, SepClientState::WORKING);

      const auto now = ws.timer->getCurrentTimeMs();
      CAPTURE(now);
      CHECK((now < 100 || now % 100 == 0)); // ensure TimerMock gets into exact time points

      if (now < 1000)
      {
        match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
        match_ws_mock_scenario(wsMock2, WsMockScenario::NOTHING_CALLED);
      }
      else if (now >= 1000 && now < 1500)
      {
        match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
        match_ws_mock_scenario(wsMock2, WsMockScenario::CONNECT_THEN_NO_CLOSE);
      }
      else if (now >= 1500 && now < 3500)
      {
        match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
        match_ws_mock_scenario(wsMock2, WsMockScenario::CONNECT_THEN_NO_CLOSE);
      }

      if (now >= 1800)
      {
        break;
      }
      ws.timer->addTimeMs(100);
    }
  }

  verify_client_state(client, SepClientState::WORKING);

  {
    INFO("Destroy client and make final checks");
    clientPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    match_ws_mock_scenario(wsMock2, WsMockScenario::CONNECT_THEN_NO_CLOSE);
  }
}


TEST_CASE("Create SepClient, wait until connected and following network error", suiteTags)
{
  WebsocketLibraryInterceptor ws;
  const auto wsMock = ws.addFutureConnection()->shouldSucceedConnectingAtMs(1000, 3000);
  const auto wsMock2 = ws.addFutureConnection()->shouldSucceedConnectingAtMs(3500, 4500);

  const auto sepClientConfig = get_default_sep_config();
  auto clientPtr = sepclient::SepClient::create(sepClientConfig);
  sepclient::SepClient &client = *clientPtr;
  client.initialize(no_authentication_failures_expected_callback, no_incoming_requests_expected_callback, nullptr);

  {
    INFO("Wait until connected and later conection is broken");
    while (true)
    {
      client.poll();
      verify_client_state(client, SepClientState::WORKING);

      const auto now = ws.timer->getCurrentTimeMs();
      CAPTURE(now);
      CHECK((now < 100 || now % 100 == 0)); // ensure TimerMock gets into exact time points

      if (now < 1000)
      {
        match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
        match_ws_mock_scenario(wsMock2, WsMockScenario::NOTHING_CALLED);
      }
      else if (now >= 1000 && now < 3000)
      {
        match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
        match_ws_mock_scenario(wsMock2, WsMockScenario::NOTHING_CALLED);
      }
      else if (now >= 3000 && now < 3500)
      {
        match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
        match_ws_mock_scenario(wsMock2, WsMockScenario::CONNECT_THEN_NO_CLOSE);
      }

      if (now >= 3300)
      {
        break;
      }
      ws.timer->addTimeMs(100);
    }
  }

  verify_client_state(client, SepClientState::WORKING);

  {
    INFO("Destroy client and make final checks");
    clientPtr.reset();
    match_ws_mock_scenario(wsMock, WsMockScenario::CONNECT_THEN_NO_CLOSE);
    match_ws_mock_scenario(wsMock2, WsMockScenario::IF_CONNECT_THEN_NO_CLOSE);
  }
}
