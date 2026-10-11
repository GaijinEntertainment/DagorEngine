// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testServer.h"

#include <osApiWrappers/dag_localConv.h>

#include <EASTL/algorithm.h>
#include <string.h>
#include <time.h>

namespace wstest
{

TestServer::TestServer()
{
  static uint16_t nextPort = uint16_t(20000 + time(nullptr) % 20000);
  for (int attempt = 0; attempt < 200 && listenSocket == OS_SOCKET_INVALID; ++attempt)
  {
    const uint16_t candidate = nextPort++;
    if (nextPort >= 60000)
      nextPort = 20000;

    os_socket_t s = os_socket_create(OSAF_IPV4, OST_TCP);
    if (s == OS_SOCKET_INVALID)
      break;
    const sockets::SocketAddr<OSAF_IPV4> addr("127.0.0.1", candidate);
    int addrLen = 0;
    const os_socket_addr *rawAddr = addr.getRawAddr(addrLen);
    if (os_socket_bind(s, rawAddr, addrLen) == 0 && os_socket_listen(s, 4) == 0 && os_socket_set_option(s, OSO_NONBLOCK, 1) == 0)
    {
      listenSocket = s;
      port = candidate;
    }
    else
      os_socket_close(s);
  }
}

TestServer::~TestServer()
{
  closeClient();
  if (listenSocket != OS_SOCKET_INVALID)
    os_socket_close(listenSocket);
}

eastl::string TestServer::getUrl(const char *scheme) const
{
  return {eastl::string::CtorSprintf(), "%s://127.0.0.1:%d/ws", scheme, (int)port};
}

eastl::string TestServer::findRequestHeader(const char *name) const
{
  const size_t nameLen = strlen(name);
  size_t lineStart = request.find("\r\n");
  while (lineStart != eastl::string::npos)
  {
    lineStart += 2;
    const size_t lineEnd = request.find("\r\n", lineStart);
    if (lineEnd == eastl::string::npos)
      break;
    if (lineEnd - lineStart > nameLen && request[lineStart + nameLen] == ':' &&
        dd_strnicmp(request.c_str() + lineStart, name, (int)nameLen) == 0)
    {
      size_t valueStart = lineStart + nameLen + 1;
      while (valueStart < lineEnd && request[valueStart] == ' ')
        ++valueStart;
      return request.substr(valueStart, lineEnd - valueStart);
    }
    lineStart = lineEnd;
  }
  return {};
}

void TestServer::poll()
{
  acceptClient();
  if (hasClient())
    readFromClient();
  if (hasClient())
    writeToClient();
}

Payload TestServer::encodeFrame(Opcode opcode, websocket::PayloadView payload, bool fin)
{
  Payload frame;
  frame.push_back(uint8_t((fin ? 0x80 : 0) | uint8_t(opcode)));
  const uint64_t size = payload.size();
  if (size <= 125)
    frame.push_back(uint8_t(size));
  else if (size <= 0xFFFF)
  {
    frame.push_back(126);
    frame.push_back(uint8_t(size >> 8));
    frame.push_back(uint8_t(size));
  }
  else
  {
    frame.push_back(127);
    for (int shift = 56; shift >= 0; shift -= 8)
      frame.push_back(uint8_t(size >> shift));
  }
  frame.insert(frame.end(), payload.begin(), payload.end());
  return frame;
}

void TestServer::sendFrame(Opcode opcode, websocket::PayloadView payload, bool fin)
{
  const Payload frame = encodeFrame(opcode, payload, fin);
  output.insert(output.end(), frame.begin(), frame.end());
}

void TestServer::sendText(eastl::string_view text, bool fin)
{
  sendFrame(fragmented ? Opcode::Continuation : Opcode::Text, websocket::make_payload_view(text.data(), text.size()), fin);
  fragmented = !fin;
}

void TestServer::sendClose(uint16_t status, eastl::string_view reason)
{
  Payload payload = {uint8_t(status >> 8), uint8_t(status)};
  payload.insert(payload.end(), reason.begin(), reason.end());
  sendFrame(Opcode::Close, payload);
  closeSent = true;
}

void TestServer::sendCloseWithoutStatus()
{
  sendFrame(Opcode::Close, {});
  closeSent = true;
}

void TestServer::dropClient() { closeClient(); }

void TestServer::sendTextThenDropClient(eastl::string_view text)
{
  sendText(text);
  closeAfterFlush = true;
}

void TestServer::acceptClient()
{
  if (hasClient() || !isListening())
    return;
  const os_socket_t s = os_socket_accept(listenSocket, nullptr, nullptr);
  if (s == OS_SOCKET_INVALID)
    return;
  os_socket_set_option(s, OSO_NONBLOCK, 1);
  os_socket_set_no_delay(s, true);
  client = s;
  ++acceptedClients;
  handshakeDone = false;
  closeSent = false;
  request.clear();
}

void TestServer::readFromClient()
{
  if (!readClient)
    return;

  char buffer[16 << 10];
  bool gone = false;
  for (;;)
  {
    const int received = os_socket_recvfrom(client, buffer, (int)sizeof(buffer));
    if (received > 0)
    {
      input.insert(input.end(), buffer, buffer + received);
      continue;
    }
    gone = received == 0 || os_socket_last_error() != OS_SOCKET_ERR_WOULDBLOCK;
    break;
  }

  if (!handshakeDone)
    processRequest();
  while (handshakeDone && hasClient() && processFrame()) {}

  if (gone)
    closeClient();
}

void TestServer::writeToClient()
{
  while (outputPos < output.size())
  {
    const int chunk = (int)eastl::min<size_t>(output.size() - outputPos, 1 << 20);
    const int sent = os_socket_send(client, (const char *)output.data() + outputPos, chunk);
    if (sent > 0)
    {
      outputPos += sent;
      continue;
    }
    if (sent < 0 && os_socket_last_error() == OS_SOCKET_ERR_WOULDBLOCK)
      return;
    closeClient();
    return;
  }
  output.clear();
  outputPos = 0;
  if (closeAfterFlush)
    closeClient();
}

void TestServer::processRequest()
{
  if (!request.empty())
    return;
  static const char headerEnd[] = "\r\n\r\n";
  const auto endPos = eastl::search(input.begin(), input.end(), headerEnd, headerEnd + 4);
  if (endPos == input.end())
    return;
  request.assign((const char *)input.data(), (const char *)endPos + 4);
  input.erase(input.begin(), endPos + 4);

  eastl::string response;
  switch (handshake)
  {
    case Handshake::NoReply: return;
    case Handshake::NotFound:
      response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      closeAfterFlush = true;
      break;
    case Handshake::Accept:
    case Handshake::ConnectionList:
    case Handshake::WrongAcceptKey:
    case Handshake::NoUpgradeHeader:
    case Handshake::WrongUpgradeToken:
    case Handshake::NoConnectionHeader:
    case Handshake::ConnectionWithoutUpgrade:
    {
      eastl::string key = findRequestHeader("Sec-WebSocket-Key");
      if (handshake == Handshake::WrongAcceptKey)
        key += "x";
      response = "HTTP/1.1 101 Switching Protocols\r\n";
      if (handshake == Handshake::WrongUpgradeToken)
        response += "Upgrade: h2c\r\n";
      else if (handshake != Handshake::NoUpgradeHeader)
        response += "Upgrade: websocket\r\n";
      if (handshake == Handshake::ConnectionList)
        response += "Connection: keep-alive,\tUpgrade\r\n";
      else if (handshake == Handshake::ConnectionWithoutUpgrade)
        response += "Connection: keep-alive\r\n";
      else if (handshake != Handshake::NoConnectionHeader)
        response += "Connection: Upgrade\r\n";
      response += "Sec-WebSocket-Accept: " + websocket::websocket_sec_answer(key) + "\r\n\r\n";
      handshakeDone = true;
      break;
    }
  }
  output.insert(output.end(), response.begin(), response.end());
  if (handshakeDone)
    output.insert(output.end(), afterHandshake.begin(), afterHandshake.end());
}

bool TestServer::processFrame()
{
  if (input.size() < 2)
    return false;
  const bool fin = (input[0] & 0x80) != 0;
  const Opcode opcode = Opcode(input[0] & 0x0F);
  const bool masked = (input[1] & 0x80) != 0;
  uint64_t size = input[1] & 0x7F;
  size_t pos = 2;
  if (size == 126)
  {
    if (input.size() < 4)
      return false;
    size = (uint64_t(input[2]) << 8) | input[3];
    pos = 4;
  }
  else if (size == 127)
  {
    if (input.size() < 10)
      return false;
    size = 0;
    for (int i = 0; i < 8; ++i)
      size = (size << 8) | input[2 + i];
    pos = 10;
  }
  const size_t maskPos = pos;
  if (masked)
    pos += 4;
  if (input.size() < pos + size)
    return false;

  const size_t payloadSize = (size_t)size;
  Payload payload(input.begin() + pos, input.begin() + pos + payloadSize);
  if (masked)
    for (size_t i = 0; i < payload.size(); ++i)
      payload[i] ^= input[maskPos + i % 4];
  else
    ++protocolErrors;
  input.erase(input.begin(), input.begin() + pos + payloadSize);

  switch (opcode)
  {
    case Opcode::Text:
    case Opcode::Binary:
      if (!fragments.empty())
        ++protocolErrors;
      fragmentsText = opcode == Opcode::Text;
      fragments = eastl::move(payload);
      if (fin)
        onMessage({eastl::exchange(fragments, Payload()), fragmentsText});
      break;
    case Opcode::Continuation:
      fragments.insert(fragments.end(), payload.begin(), payload.end());
      if (fin)
        onMessage({eastl::exchange(fragments, Payload()), fragmentsText});
      break;
    case Opcode::Ping: sendFrame(Opcode::Pong, payload); break;
    case Opcode::Pong: pongs.push_back(eastl::move(payload)); break;
    case Opcode::Close:
      closeStatuses.push_back(payload.size() >= 2 ? (payload[0] << 8) | payload[1] : noCloseStatus);
      if (closeSent)
        closeAfterFlush = true;
      else if (answerClose)
      {
        payload.resize(eastl::min<size_t>(payload.size(), 2));
        sendFrame(Opcode::Close, payload);
        closeSent = true;
        closeAfterFlush = true;
      }
      break;
    default: ++protocolErrors; break;
  }
  return true;
}

void TestServer::onMessage(websocket::ReceivedMessage &&message)
{
  if (echoMessages)
    sendFrame(message.isText ? Opcode::Text : Opcode::Binary, message.payload);
  messages.push_back(eastl::move(message));
}

void TestServer::closeClient()
{
  if (client != OS_SOCKET_INVALID)
    os_socket_close(client);
  client = OS_SOCKET_INVALID;
  input.clear();
  output.clear();
  outputPos = 0;
  fragments.clear();
  fragmented = false;
  closeAfterFlush = false;
}

} // namespace wstest
