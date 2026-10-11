// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <websocket_common.h>

#include <osApiWrappers/dag_sockets.h>

#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/vector.h>

namespace wstest
{

using websocket::Payload;

enum class Opcode : uint8_t
{
  Continuation = 0x0,
  Text = 0x1,
  Binary = 0x2,
  Close = 0x8,
  Ping = 0x9,
  Pong = 0xA
};

enum class Handshake
{
  Accept,
  ConnectionList,
  WrongAcceptKey,
  NoUpgradeHeader,
  WrongUpgradeToken,
  NoConnectionHeader,
  ConnectionWithoutUpgrade,
  NotFound,
  NoReply
};

class TestServer
{
public:
  TestServer();
  ~TestServer();
  TestServer(const TestServer &) = delete;
  TestServer &operator=(const TestServer &) = delete;

  bool isListening() const { return listenSocket != OS_SOCKET_INVALID; }
  uint16_t getPort() const { return port; }
  eastl::string getUrl(const char *scheme = "ws") const;
  bool hasClient() const { return client != OS_SOCKET_INVALID; }
  eastl::string findRequestHeader(const char *name) const;

  void poll();

  void sendFrame(Opcode opcode, websocket::PayloadView payload, bool fin = true);
  void sendText(eastl::string_view text, bool fin = true);
  void sendClose(uint16_t status, eastl::string_view reason);
  void sendCloseWithoutStatus();
  void dropClient();
  void sendTextThenDropClient(eastl::string_view text);

  static Payload encodeFrame(Opcode opcode, websocket::PayloadView payload, bool fin = true);

  static constexpr int noCloseStatus = -1;

  Handshake handshake = Handshake::Accept;
  bool echoMessages = true;
  bool answerClose = true;
  bool readClient = true;
  Payload afterHandshake;

  bool handshakeDone = false;
  int acceptedClients = 0;
  eastl::string request;
  eastl::vector<websocket::ReceivedMessage> messages;
  eastl::vector<Payload> pongs;
  eastl::vector<int> closeStatuses;
  int protocolErrors = 0;

private:
  void acceptClient();
  void readFromClient();
  void writeToClient();
  void processRequest();
  bool processFrame();
  void onMessage(websocket::ReceivedMessage &&message);
  void closeClient();

  os_socket_t listenSocket = OS_SOCKET_INVALID;
  os_socket_t client = OS_SOCKET_INVALID;
  uint16_t port = 0;
  Payload input;
  Payload output;
  size_t outputPos = 0;
  Payload fragments;
  bool fragmentsText = false;
  bool fragmented = false;
  bool closeSent = false;
  bool closeAfterFlush = false;
};

} // namespace wstest
