// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testServer.h"

#include <websocket_io.h>

#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <perfMon/dag_cpuFreq.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>

#include <EASTL/functional.h>
#include <EASTL/initializer_list.h>

#include <string>

namespace Catch
{
template <>
struct StringMaker<websocket::Payload>
{
  static std::string convert(const websocket::Payload &value)
  {
    std::string text = "{" + std::to_string(value.size()) + " bytes:";
    for (size_t i = 0; i < value.size() && i < 16; ++i)
      text += " " + std::to_string(value[i]);
    return text + (value.size() > 16 ? " ...}" : "}");
  }
};
} // namespace Catch

using namespace websocket;
using wstest::Handshake;
using wstest::Opcode;
using wstest::TestServer;

namespace
{

enum class Impl
{
  CurlWs,
  Compat,
  Default
};

struct TestClient
{
  explicit TestClient(Impl impl, ClientConfig &&config = {})
  {
    config.onMessage = [this](WebSocketClient *, PayloadView payload, bool is_text) {
      messages.push_back({Payload(payload.begin(), payload.end()), is_text});
      if (messageHook)
        messageHook();
    };
    config.onClose = [this](WebSocketClient *, CloseStatusInt status, Error &&error) {
      ++closeCalls;
      closeStatus = status;
      closeError = eastl::move(error);
      if (closeHook)
        closeHook();
    };
    switch (impl)
    {
      case Impl::CurlWs: client = create_curl_ws_client(eastl::move(config)); break;
      case Impl::Compat: client = create_compat_client(eastl::move(config)); break;
      case Impl::Default: client = create_client(eastl::move(config)); break;
    }
    REQUIRE(bool(client));
  }
  TestClient(const TestClient &) = delete;
  TestClient &operator=(const TestClient &) = delete;

  void connect(const eastl::string &url, const eastl::string &ip_hint = {}, const Headers &headers = {})
  {
    client->connect(
      url, ip_hint,
      [this](WebSocketClient *, ConnectStatus status, const eastl::string &ip, Error &&error) {
        ++connectCalls;
        connectStatus = status;
        connectIp = ip;
        connectError = eastl::move(error);
      },
      headers);
  }

  void send(const Payload &payload, bool is_text = false) { client->send(payload, is_text); }
  void send(eastl::string_view text) { client->send(make_payload_view(text.data(), text.size()), true); }

  eastl::unique_ptr<WebSocketClient> client;

  int connectCalls = 0;
  ConnectStatus connectStatus = ConnectStatus::ConnectIsActive;
  eastl::string connectIp;
  Error connectError;

  int closeCalls = 0;
  CloseStatusInt closeStatus = 0;
  Error closeError;

  eastl::vector<ReceivedMessage> messages;
  eastl::function<void()> messageHook;
  eastl::function<void()> closeHook;
};

template <typename Pred>
bool run_until_all(std::initializer_list<TestClient *> clients, std::initializer_list<TestServer *> servers, Pred &&done,
  int timeout_ms = 5000)
{
  const int startMs = get_time_msec();
  while (!done())
  {
    if (get_time_msec() - startMs > timeout_ms)
      return false;
    for (TestClient *client : clients)
      client->client->poll();
    for (TestServer *server : servers)
      server->poll();
    sleep_msec(1);
  }
  return true;
}

template <typename Pred>
bool run_until(TestClient &client, TestServer &server, Pred &&done, int timeout_ms = 5000)
{
  return run_until_all({&client}, {&server}, eastl::forward<Pred>(done), timeout_ms);
}

void run_for(TestClient &client, TestServer &server, int duration_ms)
{
  run_until(client, server, [] { return false; }, duration_ms);
}

void connect_and_wait(TestClient &client, TestServer &server, const Headers &headers = {})
{
  REQUIRE(server.isListening());
  client.connect(server.getUrl(), {}, headers);
  REQUIRE(run_until(client, server, [&] { return client.connectCalls > 0; }));
  INFO("connect error " << client.connectError.code << ": " << client.connectError.message.c_str());
  REQUIRE(client.connectStatus == ConnectStatus::Ok);
  REQUIRE(run_until(client, server, [&] { return server.handshakeDone && server.hasClient(); }));
}

Payload make_payload(size_t size, uint8_t seed)
{
  Payload payload(size);
  for (size_t i = 0; i < size; ++i)
    payload[i] = uint8_t(i * 31 + seed);
  return payload;
}

Payload to_payload(eastl::string_view text) { return Payload(text.begin(), text.end()); }

bool contains(const eastl::string &text, const char *part) { return text.find(part) != eastl::string::npos; }

struct ForceWakeSocket
{
  ForceWakeSocket() { force_wake_socket_for_tests(true); }
  ~ForceWakeSocket() { force_wake_socket_for_tests(false); }
};

struct DropWakeups
{
  DropWakeups() { drop_wakeups_for_tests(true); }
  ~DropWakeups() { drop_wakeups_for_tests(false); }
};

template <typename F>
void for_each_backend(F &&test)
{
  SECTION("curl ws")
  {
    if (curl_has_websocket())
      test(Impl::CurlWs);
    else
      WARN("libcurl " << curl_version_info(CURLVERSION_NOW)->version << " has no WebSocket support");
  }
  SECTION("compat") { test(Impl::Compat); }
}

// the wake socket path is the one a libcurl without curl_multi_wakeup takes, as on Sony
template <typename F>
void for_each_impl(F &&test)
{
  SECTION("curl wakeup") { for_each_backend(test); }
  SECTION("wake socket")
  {
    ForceWakeSocket force;
    for_each_backend(test);
  }
}

} // namespace

#if WEBSOCKET_TESTS_BUNDLED_CURL
TEST_CASE("the bundled libcurl has WebSocket support", "[websocket]") { CHECK(curl_has_websocket()); }

TEST_CASE("with the bundled libcurl the I/O thread wakes through curl_multi_wakeup", "[websocket]")
{
  TestClient client(Impl::Default);
  CHECK_FALSE(get_io_stats().ioWakeSocket);
}
#endif

TEST_CASE("Sec-WebSocket-Accept matches the RFC 6455 sample", "[websocket]")
{
  CHECK(websocket_sec_answer("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

TEST_CASE("handshake sends the upgrade request headers", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server, {{"X-Test", "value"}});

    CHECK(client.connectIp == "127.0.0.1");
    CHECK(dd_stricmp(server.findRequestHeader("Upgrade").c_str(), "websocket") == 0);
    eastl::string connection = server.findRequestHeader("Connection");
    connection.make_lower();
    CHECK(contains(connection, "upgrade"));
    CHECK(server.findRequestHeader("Sec-WebSocket-Version") == "13");
    CHECK(server.findRequestHeader("Sec-WebSocket-Key").size() == 24);
    CHECK(server.findRequestHeader("X-Test") == "value");
  });
}

TEST_CASE("an http URL is accepted as ws", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    client.connect(server.getUrl("http"));
    REQUIRE(run_until(client, server, [&] { return client.connectCalls > 0; }));
    CHECK(client.connectStatus == ConnectStatus::Ok);
  });
}

TEST_CASE("create_client() picks the client that matches libcurl", "[websocket]")
{
  TestServer server;
  TestClient client(Impl::Default);
  connect_and_wait(client, server);

  server.sendFrame(Opcode::Ping, to_payload("abc"));
  REQUIRE(run_until(client, server, [&] { return !server.pongs.empty(); }));

  const bool wsClientEchoesPing = curl_has_websocket();
  CHECK(server.pongs[0] == (wsClientEchoesPing ? to_payload("abc") : Payload()));
}

TEST_CASE("a Connection header with several tokens is accepted", "[websocket]")
{
  if (!curl_has_websocket())
  {
    WARN("libcurl has no WebSocket support");
    return;
  }

  TestServer server;
  server.handshake = Handshake::ConnectionList;
  TestClient client(Impl::CurlWs);
  connect_and_wait(client, server);
}

TEST_CASE("text and binary messages keep their type and order", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    client.send("hello");
    client.send(Payload{0, 1, 2, 255});
    client.send("");
    REQUIRE(run_until(client, server, [&] { return client.messages.size() == 3; }));

    CHECK(client.messages[0].isText);
    CHECK(client.messages[0].payload == to_payload("hello"));
    CHECK_FALSE(client.messages[1].isText);
    CHECK(client.messages[1].payload == Payload{0, 1, 2, 255});
    CHECK(client.messages[2].isText);
    CHECK(client.messages[2].payload.empty());
    CHECK(server.protocolErrors == 0);
  });
}

TEST_CASE("messages of every length encoding round trip", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    const size_t sizes[] = {1, 125, 126, 127, 65535, 65536, 65537, 1 << 20};
    for (size_t size : sizes)
    {
      INFO("payload size " << size);
      const Payload payload = make_payload(size, uint8_t(size));
      client.messages.clear();
      client.send(payload);
      REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
      CHECK(server.messages.back().payload == payload);
      CHECK(client.messages[0].payload == payload);
    }
    CHECK(server.protocolErrors == 0);
  });
}

TEST_CASE("fragmented server message with a ping between the fragments", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    server.sendText("Hel", false);
    server.sendFrame(Opcode::Ping, to_payload("abc"));
    server.sendText("lo ", false);
    server.sendText("world");
    REQUIRE(run_until(client, server, [&] { return !client.messages.empty() && !server.pongs.empty(); }));

    REQUIRE(client.messages.size() == 1);
    CHECK(client.messages[0].isText);
    CHECK(client.messages[0].payload == to_payload("Hello world"));
    if (impl == Impl::CurlWs)
      CHECK(server.pongs[0] == to_payload("abc"));
  });
}

TEST_CASE("a frame sent together with the 101 response is received", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    server.afterHandshake = TestServer::encodeFrame(Opcode::Text, to_payload("greeting"));
    TestClient client(impl);
    connect_and_wait(client, server);

    REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
    CHECK(client.messages[0].payload == to_payload("greeting"));
  });
}

TEST_CASE("message over maxMessageSize closes with 1009", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    ClientConfig config;
    config.maxMessageSize = 1000;
    TestClient client(impl, eastl::move(config));
    connect_and_wait(client, server);

    server.sendFrame(Opcode::Binary, make_payload(1000, 1));
    REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));

    SECTION("single frame") { server.sendFrame(Opcode::Binary, make_payload(1001, 2)); }
    SECTION("fragments")
    {
      server.sendFrame(Opcode::Binary, make_payload(600, 3), false);
      server.sendFrame(Opcode::Continuation, make_payload(600, 4));
    }
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }));

    CHECK(client.messages.size() == 1);
    CHECK(client.closeStatus == (CloseStatusInt)CloseStatus::TooBigData);
    CHECK(client.closeError.code == (int)CloseStatus::TooBigData);
    REQUIRE(run_until(client, server, [&] { return !server.closeStatuses.empty(); }));
    CHECK(server.closeStatuses[0] == (int)CloseStatus::TooBigData);
  });
}

TEST_CASE("close() finishes the close handshake", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    client.send("last");
    client.client->close(CloseStatus::Normal);
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }));

    CHECK(client.closeStatus == (CloseStatusInt)CloseStatus::Normal);
    if (impl == Impl::CurlWs)
      CHECK(contains(client.closeError.message, "closed by the application"));
    REQUIRE(server.closeStatuses.size() == 1);
    CHECK(server.closeStatuses[0] == (int)CloseStatus::Normal);
    REQUIRE(!server.messages.empty());
    CHECK(server.messages.back().payload == to_payload("last"));
  });
}

TEST_CASE("server close passes its status and reason", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    SECTION("with status")
    {
      server.sendClose(4001, "bye");
      REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }));
      CHECK(client.closeStatus == 4001);
      CHECK(contains(client.closeError.message, "bye"));
      if (impl == Impl::CurlWs)
      {
        REQUIRE(run_until(client, server, [&] { return !server.closeStatuses.empty(); }));
        CHECK(server.closeStatuses[0] == 4001);
      }
    }
    SECTION("without status")
    {
      server.sendCloseWithoutStatus();
      REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }));
      CHECK(client.closeStatus == (CloseStatusInt)CloseStatus::NoStatusRcvd);
      if (impl == Impl::CurlWs)
      {
        REQUIRE(run_until(client, server, [&] { return !server.closeStatuses.empty(); }));
        CHECK(server.closeStatuses[0] == TestServer::noCloseStatus);
      }
    }
  });
}

TEST_CASE("the close reply waits behind a frame that curl has begun", "[websocket]")
{
  if (!curl_has_websocket())
  {
    WARN("libcurl has no WebSocket support");
    return;
  }

  TestServer server;
  server.echoMessages = false;
  ClientConfig config;
  bool peerReadsAgain = false;
  int expectedWaitMs = 1000;
  SECTION("peer reads again") { peerReadsAgain = true; }
  SECTION("default wait") {}
  SECTION("closeTimeoutMs")
  {
    config.closeTimeoutMs = 300;
    expectedWaitMs = 300;
  }
  TestClient client(Impl::CurlWs, eastl::move(config));
  connect_and_wait(client, server);

  server.readClient = false;
  const int messageCount = 64;
  for (int i = 0; i < messageCount; ++i)
    client.send(make_payload(512 << 10, uint8_t(i)));
  run_for(client, server, 200);
  const int closeMs = get_time_msec();
  server.sendClose(4001, "bye");

  if (!peerReadsAgain)
  {
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }, expectedWaitMs * 3));
    CHECK(get_time_msec() - closeMs >= expectedWaitMs);
    CHECK(client.closeStatus == 4001);
    return;
  }

  run_for(client, server, 300);
  CHECK(client.closeCalls == 0);
  server.readClient = true;
  REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0 && !server.closeStatuses.empty(); }, 30000));

  CHECK(client.closeStatus == 4001);
  CHECK(server.closeStatuses[0] == 4001);
  CHECK(server.messages.size() < messageCount);
  CHECK(server.protocolErrors == 0);
}

TEST_CASE("a frame flood arrives intact and in order", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    const int messageCount = 1000;
    const int pingEvery = 10;
    for (int i = 0; i < messageCount; ++i)
    {
      const eastl::string text(eastl::string::CtorSprintf(), "message %d", i);
      server.sendText(text);
      if (i % pingEvery == 0)
        server.sendFrame(Opcode::Ping, to_payload("abc"));
    }
    REQUIRE(run_until(client, server, [&] { return client.messages.size() == messageCount; }));

    for (int i = 0; i < messageCount; ++i)
    {
      INFO("message " << i);
      CHECK(client.messages[i].payload == to_payload(eastl::string(eastl::string::CtorSprintf(), "message %d", i)));
    }
    // constraint: the compat client sends one PONG for all pings parsed in one transfer step
    if (impl == Impl::CurlWs)
      REQUIRE(run_until(client, server, [&] { return server.pongs.size() == messageCount / pingEvery; }));
    CHECK(client.closeCalls == 0);
  });
}

TEST_CASE("closeTimeoutMs ends the wait for a silent peer", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    server.answerClose = false;
    ClientConfig config;
    config.closeTimeoutMs = 200;
    TestClient client(impl, eastl::move(config));
    connect_and_wait(client, server);

    const int closeMs = get_time_msec();
    client.client->close(CloseStatus::Normal);
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }, 600));

    CHECK(get_time_msec() - closeMs >= 200);
    CHECK(client.closeStatus == (CloseStatusInt)CloseStatus::AbnormalClosure);
    CHECK(contains(client.closeError.message, "no close frame from the peer"));
  });
}

TEST_CASE("close() during the handshake fails the connect", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    server.handshake = Handshake::NoReply;
    TestClient client(impl);
    client.connect(server.getUrl());
    REQUIRE(run_until(client, server, [&] { return !server.request.empty(); }));

    client.client->close(CloseStatus::GoingAway);
    CHECK(client.connectCalls == 0);
    REQUIRE(run_until(client, server, [&] { return client.connectCalls > 0; }));
    run_for(client, server, 50);

    CHECK(client.connectCalls == 1);
    CHECK(client.connectStatus == ConnectStatus::HttpConnectFailed);
    CHECK(client.connectError.code == (int)CloseStatus::GoingAway);
    CHECK(client.closeCalls == 0);
  });
}

TEST_CASE("HTTP error response fails the connect", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    server.handshake = Handshake::NotFound;
    TestClient client(impl);
    client.connect(server.getUrl());
    REQUIRE(run_until(client, server, [&] { return client.connectCalls > 0; }));

    CHECK(client.connectStatus == ConnectStatus::HttpConnectFailed);
    if (impl == Impl::CurlWs)
      CHECK(client.connectError.code == 404);
  });
}

TEST_CASE("invalid upgrade response fails the connect", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    SECTION("wrong Sec-WebSocket-Accept") { server.handshake = Handshake::WrongAcceptKey; }
    SECTION("no Upgrade header") { server.handshake = Handshake::NoUpgradeHeader; }
    SECTION("Upgrade without websocket") { server.handshake = Handshake::WrongUpgradeToken; }
    SECTION("no Connection header") { server.handshake = Handshake::NoConnectionHeader; }
    SECTION("Connection without upgrade") { server.handshake = Handshake::ConnectionWithoutUpgrade; }
    TestClient client(impl);
    client.connect(server.getUrl());
    REQUIRE(run_until(client, server, [&] { return client.connectCalls > 0; }));

    CHECK(client.connectStatus == ConnectStatus::HttpConnectFailed);
    CHECK(client.connectError.code != 0);
    CHECK(client.closeCalls == 0);
  });
}

TEST_CASE("refused TCP connect fails the connect", "[websocket]")
{
  for_each_impl([](Impl impl) {
    eastl::string url;
    {
      TestServer closedServer;
      url = closedServer.getUrl();
    }
    TestServer server;
    TestClient client(impl);
    client.connect(url);
    REQUIRE(run_until(client, server, [&] { return client.connectCalls > 0; }, 15000));

    CHECK(client.connectStatus == ConnectStatus::HttpConnectFailed);
    CHECK(client.connectError.code == CURLE_COULDNT_CONNECT);
  });
}

TEST_CASE("dropped TCP connection reports the close", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    server.dropClient();
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }));

    CHECK(client.closeStatus == 0);
    CHECK(client.closeError.code != 0);
    if (impl == Impl::CurlWs)
      CHECK(client.closeError.code == CURLE_GOT_NOTHING);
  });
}

TEST_CASE("a peer close of an idle connection is reported at once", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    server.dropClient();
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }, 1000));

    INFO("close error " << client.closeError.code << ": " << client.closeError.message.c_str());
    CHECK(client.closeError.code != 0);
    CHECK_FALSE(contains(client.closeError.message, "instead of WebSocket upgrade"));
    if (impl == Impl::Compat)
      CHECK(contains(client.closeError.message, "without a close frame"));
  });
}

TEST_CASE("a message sent right before the peer close is not lost", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    server.echoMessages = false;
    TestClient client(impl);
    connect_and_wait(client, server);

    server.sendTextThenDropClient("last words");
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }, 1000));

    REQUIRE(client.messages.size() == 1);
    CHECK(client.messages[0].payload == to_payload("last words"));
  });
}

TEST_CASE("large sends stay intact while the peer does not read", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    server.echoMessages = false;
    TestClient client(impl);
    connect_and_wait(client, server);

    server.readClient = false;
    const int messageCount = 32;
    for (int i = 0; i < messageCount; ++i)
      client.send(make_payload(512 << 10, uint8_t(i)));
    run_for(client, server, 200);

    server.readClient = true;
    REQUIRE(run_until(client, server, [&] { return server.messages.size() == messageCount; }, 30000));
    for (int i = 0; i < messageCount; ++i)
    {
      INFO("message " << i);
      CHECK(server.messages[i].payload == make_payload(512 << 10, uint8_t(i)));
    }
    CHECK(server.protocolErrors == 0);
    CHECK(client.closeCalls == 0);
  });
}

TEST_CASE("connect while connected reports ConnectIsActive", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    client.connect(server.getUrl());
    CHECK(client.connectCalls == 1);
    REQUIRE(run_until(client, server, [&] { return client.connectCalls == 2; }));

    CHECK(client.connectStatus == ConnectStatus::ConnectIsActive);
    client.send("still open");
    REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
  });
}

TEST_CASE("connect IP hint replaces the DNS lookup", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    const eastl::string url{eastl::string::CtorSprintf(), "ws://websocket-test.invalid:%d/ws", (int)server.getPort()};
    client.connect(url, "127.0.0.1");
    REQUIRE(run_until(client, server, [&] { return client.connectCalls > 0; }));

    CHECK(client.connectStatus == ConnectStatus::Ok);
    CHECK(client.connectIp == "127.0.0.1");
  });
}

TEST_CASE("a connect IP hint stays with its own client", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient keeper(impl);
    TestClient hinted(impl);
    const eastl::string url{eastl::string::CtorSprintf(), "ws://websocket-test.invalid:%d/ws", (int)server.getPort()};
    hinted.connect(url, "127.0.0.1");
    REQUIRE(run_until(hinted, server, [&] { return hinted.connectCalls > 0; }));
    REQUIRE(hinted.connectStatus == ConnectStatus::Ok);
    hinted.client.reset();
    REQUIRE(run_until(keeper, server, [&] { return !server.hasClient(); }));

    ClientConfig config;
    config.connectTimeoutMs = 2000;
    TestClient plain(impl, eastl::move(config));
    plain.connect(url);
    REQUIRE(run_until(plain, server, [&] { return plain.connectCalls > 0; }));
    CHECK(plain.connectStatus == ConnectStatus::HttpConnectFailed);
  });
}

TEST_CASE("the client connects again after a close", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);
    client.client->close(CloseStatus::Normal);
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }));
    REQUIRE(run_until(client, server, [&] { return !server.hasClient(); }));

    client.connectCalls = 0;
    connect_and_wait(client, server);
    client.send("again");
    REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
    CHECK(client.messages[0].payload == to_payload("again"));
    CHECK(server.acceptedClients == 2);
  });
}

TEST_CASE("the peer's close drops data queued before close()", "[websocket]")
{
  if (!curl_has_websocket())
  {
    WARN("libcurl has no WebSocket support");
    return;
  }

  TestServer server;
  server.echoMessages = false;
  ClientConfig config;
  config.closeTimeoutMs = 10000;
  TestClient client(Impl::CurlWs, eastl::move(config));
  connect_and_wait(client, server);

  server.readClient = false;
  const int messageCount = 64;
  for (int i = 0; i < messageCount; ++i)
    client.send(make_payload(512 << 10, uint8_t(i)));
  run_for(client, server, 200);
  client.client->close(CloseStatus::Normal);
  server.sendClose(4001, "bye");
  run_for(client, server, 300);

  server.readClient = true;
  REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0 && !server.closeStatuses.empty(); }, 30000));

  CHECK(client.closeStatus == (CloseStatusInt)CloseStatus::Normal);
  CHECK(server.closeStatuses[0] == (int)CloseStatus::Normal);
  CHECK(server.messages.size() < messageCount);
  CHECK(server.protocolErrors == 0);
}

TEST_CASE("two clients share the worker thread", "[websocket]")
{
  for_each_impl([](Impl impl) {
    struct Delivery
    {
      Payload payload;
      int64_t threadId = 0;
    };
    WinCritSec mutex;
    eastl::vector<Delivery> deliveries[2];
    auto makeConfig = [&](int index) {
      ClientConfig config;
      config.onMessageAsync = [&, index](WebSocketClient *, PayloadView payload, bool) {
        WinAutoLock lock(mutex);
        deliveries[index].push_back({Payload(payload.begin(), payload.end()), get_current_thread_id()});
      };
      return config;
    };

    TestServer server1;
    TestServer server2;
    TestClient client1(impl, makeConfig(0));
    TestClient client2(impl, makeConfig(1));
    connect_and_wait(client1, server1);
    connect_and_wait(client2, server2);

    const IoStats stats = get_io_stats();
    CHECK(stats.ioThreadRunning);
    CHECK(stats.ioClients == 2);

    client1.send("one");
    client2.send("two");
    REQUIRE(run_until_all({&client1, &client2}, {&server1, &server2}, [&] {
      WinAutoLock lock(mutex);
      return deliveries[0].size() == 1 && deliveries[1].size() == 1;
    }));
    WinAutoLock lock(mutex);
    CHECK(deliveries[0][0].payload == to_payload("one"));
    CHECK(deliveries[1][0].payload == to_payload("two"));
    CHECK(deliveries[0][0].threadId == deliveries[1][0].threadId);
    CHECK(deliveries[0][0].threadId != get_current_thread_id());
  });
}

TEST_CASE("the handshake completes without poll()", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    REQUIRE(server.isListening());
    client.connect(server.getUrl());
    REQUIRE(run_until_all({}, {&server}, [&] { return server.handshakeDone && server.hasClient(); }));
    // a PONG shows that the client read the 101 response and answers frames without poll()
    server.sendFrame(Opcode::Ping, to_payload("abc"));
    REQUIRE(run_until_all({}, {&server}, [&] { return !server.pongs.empty(); }));
    CHECK(client.connectCalls == 0);

    REQUIRE(run_until(client, server, [&] { return client.connectCalls > 0; }));
    CHECK(client.connectStatus == ConnectStatus::Ok);
  });
}

TEST_CASE("without curl_multi_wakeup the I/O thread wakes through its own socket", "[websocket]")
{
  ForceWakeSocket force;
  TestServer server;
  TestClient client(Impl::Default);
  CHECK(get_io_stats().ioWakeSocket);
  connect_and_wait(client, server);
  client.send("woken by the wake socket");
  REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
  CHECK(client.messages[0].payload == to_payload("woken by the wake socket"));
}

TEST_CASE("a command wakes an idle I/O thread at once", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);
    client.send("warm up");
    REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
    run_for(client, server, 200);

    // far below the 1 s bound that ends even a wait no wakeup ended
    const int startMs = get_time_msec();
    client.send("at once");
    REQUIRE(run_until(client, server, [&] { return client.messages.size() == 2; }));
    CHECK(get_time_msec() - startMs < 300);
  });
}

TEST_CASE("lost wakeups delay the commands but do not stall them", "[websocket]")
{
  for_each_impl([](Impl impl) {
    DropWakeups drop;
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);
    const int startMs = get_time_msec();
    client.send("late but delivered");
    REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
    CHECK(get_time_msec() - startMs < 2000);
    CHECK(client.messages[0].payload == to_payload("late but delivered"));
  });
}

TEST_CASE("shutdown() reports a client its owner keeps", "[websocket]")
{
  {
    TestClient client(Impl::Default);
    CHECK(websocket::shutdown(100) == 1);
  }
  CHECK(websocket::shutdown() == 0);
  CHECK_FALSE(get_io_stats().ioThreadRunning);
}

TEST_CASE("shutdown() closes the clients and their owners release them on close", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    ClientConfig config;
    config.closeTimeoutMs = 50;
    TestClient client(impl, eastl::move(config));
    connect_and_wait(client, server);
    client.closeHook = [&] { client.client.reset(); };

    CHECK(websocket::shutdown() == 0);
    CHECK(client.client.get() == nullptr);
    CHECK(client.closeCalls == 1);
    REQUIRE(run_until_all({}, {&server}, [&] { return !server.closeStatuses.empty(); }));
    CHECK(server.closeStatuses[0] == (int)CloseStatus::GoingAway);
  });
}

TEST_CASE("an owner can release another client from its shutdown close", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server1;
    TestServer server2;
    TestClient older(impl);
    ClientConfig config;
    config.closeTimeoutMs = 50;
    TestClient newer(impl, eastl::move(config));
    connect_and_wait(older, server1);
    connect_and_wait(newer, server2);
    // newer is polled first and its close releases both, while older waits for a peer close that never comes
    newer.closeHook = [&] {
      older.client.reset();
      newer.client.reset();
    };

    CHECK(websocket::shutdown() == 0);
    CHECK(older.client.get() == nullptr);
    CHECK(newer.client.get() == nullptr);
  });
}

TEST_CASE("shutdown() on another thread closes but does not poll this thread's clients", "[websocket]")
{
  TestServer server;
  ClientConfig config;
  config.closeTimeoutMs = 50;
  TestClient client(Impl::Default, eastl::move(config));
  connect_and_wait(client, server);
  client.closeHook = [&] { client.client.reset(); };

  class ShutdownThread final : public DaThread
  {
  public:
    ShutdownThread() : DaThread("wsTestShutdown") {}
    void execute() override { interlocked_release_store(alive, (int)websocket::shutdown(200)); }
    volatile int alive = -1;
  } other;
  REQUIRE(other.start());
  other.terminate(true);
  CHECK(interlocked_acquire_load(other.alive) == 1);
  CHECK(client.closeCalls == 0);

  REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }));
  CHECK(client.client.get() == nullptr);
}

TEST_CASE("the I/O threads run only while a client exists", "[websocket]")
{
  for_each_impl([](Impl impl) {
    REQUIRE_FALSE(get_io_stats().ioThreadRunning);
    CHECK(get_io_stats().ioClients == 0);
    {
      TestServer server;
      TestClient client(impl);
      CHECK(get_io_stats().ioThreadRunning);
      CHECK(get_io_stats().ioClients == 1);
      connect_and_wait(client, server);
    }
    CHECK_FALSE(get_io_stats().ioThreadRunning);
    CHECK(get_io_stats().ioClients == 0);

    TestServer server;
    TestClient client(impl);
    CHECK(get_io_stats().ioThreadRunning);
    connect_and_wait(client, server);
    client.send("again");
    REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
  });
}

TEST_CASE("onMessageAsync runs on the worker thread in order and before the close", "[websocket]")
{
  for_each_impl([](Impl impl) {
    struct AsyncInbox
    {
      WinCritSec mutex;
      eastl::vector<Payload> messages;
      eastl::vector<int64_t> threadIds;
      volatile int inside = 0;
      volatile int overlaps = 0;
    } inbox;

    TestServer server;
    ClientConfig config;
    config.onMessageAsync = [&inbox](WebSocketClient *, PayloadView payload, bool) {
      if (interlocked_increment(inbox.inside) != 1)
        interlocked_increment(inbox.overlaps);
      bool first = false;
      {
        WinAutoLock lock(inbox.mutex);
        first = inbox.messages.empty();
      }
      if (first)
        sleep_msec(50);
      {
        WinAutoLock lock(inbox.mutex);
        inbox.messages.emplace_back(payload.begin(), payload.end());
        inbox.threadIds.push_back(get_current_thread_id());
      }
      interlocked_decrement(inbox.inside);
    };
    TestClient client(impl, eastl::move(config));
    size_t messagesAtClose = 0;
    client.closeHook = [&] {
      WinAutoLock lock(inbox.mutex);
      messagesAtClose = inbox.messages.size();
    };
    connect_and_wait(client, server);

    const int messageCount = 200;
    for (int i = 0; i < messageCount; ++i)
      server.sendText(eastl::string(eastl::string::CtorSprintf(), "message %d", i));
    server.sendClose(4001, "bye");
    REQUIRE(run_until(client, server, [&] { return client.closeCalls > 0; }));

    CHECK(client.closeStatus == 4001);
    CHECK(messagesAtClose == messageCount);
    CHECK(client.messages.empty());
    CHECK(interlocked_acquire_load(inbox.overlaps) == 0);
    WinAutoLock lock(inbox.mutex);
    REQUIRE(inbox.messages.size() == messageCount);
    const int64_t testThreadId = get_current_thread_id();
    for (int i = 0; i < messageCount; ++i)
    {
      INFO("message " << i);
      CHECK(inbox.messages[i] == to_payload(eastl::string(eastl::string::CtorSprintf(), "message %d", i)));
      CHECK(inbox.threadIds[i] != testThreadId);
    }
  });
}

TEST_CASE("onMessageAsync waits until poll() ran the connect callback", "[websocket]")
{
  for_each_impl([](Impl impl) {
    WinCritSec mutex;
    eastl::vector<Payload> messages;
    TestServer server;
    server.afterHandshake = TestServer::encodeFrame(Opcode::Text, to_payload("greeting"));
    ClientConfig config;
    config.onMessageAsync = [&](WebSocketClient *, PayloadView payload, bool) {
      WinAutoLock lock(mutex);
      messages.emplace_back(payload.begin(), payload.end());
    };
    TestClient client(impl, eastl::move(config));
    REQUIRE(server.isListening());
    client.connect(server.getUrl());

    const int startMs = get_time_msec();
    while (get_time_msec() - startMs < 200)
    {
      server.poll();
      sleep_msec(1);
    }
    REQUIRE(server.handshakeDone);
    {
      WinAutoLock lock(mutex);
      CHECK(messages.empty());
    }

    REQUIRE(run_until(client, server, [&] {
      WinAutoLock lock(mutex);
      return !messages.empty();
    }));
    CHECK(client.connectCalls == 1);
    CHECK(client.connectStatus == ConnectStatus::Ok);
    WinAutoLock lock(mutex);
    CHECK(messages[0] == to_payload("greeting"));
  });
}

TEST_CASE("an idle open connection does not wake the I/O thread", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);
    client.send("warm up");
    REQUIRE(run_until(client, server, [&] { return !client.messages.empty(); }));
    run_for(client, server, 200);

    const unsigned wakeupsBefore = get_io_stats().ioWakeups;
    run_for(client, server, 300);
    const unsigned wakeups = get_io_stats().ioWakeups - wakeupsBefore;
    CHECK(wakeups == 0);
    CHECK(client.closeCalls == 0);
  });
}

TEST_CASE("send() and close() work from another thread", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server;
    TestClient client(impl);
    connect_and_wait(client, server);

    class SenderThread final : public DaThread
    {
    public:
      explicit SenderThread(WebSocketClient *ws_client) : DaThread("wsTestSender"), wsClient(ws_client) {}
      void execute() override
      {
        static const char text[] = "from a thread";
        wsClient->send(make_payload_view(text, sizeof(text) - 1), true);
        wsClient->close(CloseStatus::Normal);
        interlocked_release_store(done, 1);
      }

      WebSocketClient *wsClient;
      volatile int done = 0;
    } sender(client.client.get());
    REQUIRE(sender.start());
    REQUIRE(run_until(client, server, [&] { return interlocked_acquire_load(sender.done) && client.closeCalls > 0; }));
    sender.terminate(true);

    CHECK(client.closeStatus == (CloseStatusInt)CloseStatus::Normal);
    REQUIRE(!server.messages.empty());
    CHECK(server.messages.back().payload == to_payload("from a thread"));
    REQUIRE(server.closeStatuses.size() == 1);
    CHECK(server.closeStatuses[0] == (int)CloseStatus::Normal);
  });
}

TEST_CASE("a poll() callback can destroy its own client", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server1;
    TestServer server2;
    TestClient client1(impl);
    TestClient client2(impl);
    connect_and_wait(client1, server1);
    connect_and_wait(client2, server2);
    client1.messageHook = [&] { client1.client.reset(); };

    server1.sendText("first");
    server1.sendText("second");
    server1.sendClose(1000, "bye");
    // the connection drops only after the I/O thread read all three frames, so the next poll() gets them in one batch;
    // the sleep covers the hand-over to the worker thread
    REQUIRE(run_until_all({}, {&server1}, [&] { return !server1.hasClient(); }));
    sleep_msec(50);
    client1.client->poll();

    CHECK(client1.client.get() == nullptr);
    CHECK(client1.messages.size() == 1);
    CHECK(client1.closeCalls == 0);

    client2.send("still here");
    REQUIRE(run_until(client2, server2, [&] { return !client2.messages.empty(); }));
    CHECK(client2.messages[0].payload == to_payload("still here"));
  });
}

TEST_CASE("destroying a connected client leaves the other client working", "[websocket]")
{
  for_each_impl([](Impl impl) {
    TestServer server1;
    TestServer server2;
    TestClient client1(impl);
    TestClient client2(impl);
    connect_and_wait(client1, server1);
    connect_and_wait(client2, server2);

    client1.client.reset();
    REQUIRE(run_until(client2, server1, [&] { return !server1.hasClient(); }));
    CHECK(server1.closeStatuses.empty());

    client2.send("still here");
    REQUIRE(run_until(client2, server2, [&] { return !client2.messages.empty(); }));
    CHECK(client2.messages[0].payload == to_payload("still here"));
    CHECK(client2.closeCalls == 0);
  });
}
