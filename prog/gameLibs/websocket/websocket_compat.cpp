// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "websocket_io.h"

#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_sockets.h>
#include <util/dag_strUtil.h>

#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <dagCrypto/rand.h>

#include <string.h>

#ifdef _MSC_VER
#include <stdlib.h>
#include <Winsock2.h> // hton*, ntoh*
#define byte_swap_64 _byteswap_uint64

#else // GCC, clang
#include <arpa/inet.h>
#define byte_swap_64 __builtin_bswap64
// TODO: add dag_endian.h and move it there
#endif

#ifndef htonll
// assume client is always little-endian
#define ntohll(x) byte_swap_64(x)
#define htonll(x) byte_swap_64(x)
#endif


namespace websocket
{
namespace
{

enum WSOpcodes
{
  CONTINUATION = 0,
  TEXT = 0x1,
  BINARY = 0x2,
  CLOSE = 0x8,
  PING = 0x9,
  PONG = 0xA
};

#pragma pack(push, 1)
struct WSHeader
{
  unsigned char opcode : 4;
  unsigned char reserved : 3;
  unsigned char fin : 1;

  unsigned char payloadLen : 7;
  unsigned char mask : 1;
};

struct WSHeaderPL16 : WSHeader
{
  uint16_t payloadLen16;
};

struct WSHeaderPL64 : WSHeader
{
  uint64_t payloadLen64;
};
#pragma pack(pop)

struct RingBuffer
{
  void write(PayloadView data)
  {
    if (data.empty() && data.data() == nullptr)
      return; // avoid UB in memcpy, memmove

    size_t freeSize = buffer.size() - dataSize;
    if (freeSize < data_size(data))
      growBuf(data_size(data));

    if (dataSize == 0)
    {
      dataSize = data_size(data);
      dataStart = 0;
      dataEnd = dataSize;
      memcpy(&buffer[dataStart], data.data(), dataSize);
    }
    else if (dataEnd > dataStart) // not wrapped
    {
      // constraint: dataEnd can equal buffer.size() here, so no operator[] on it
      size_t bytesToTail = buffer.size() - dataEnd;
      if (data_size(data) <= bytesToTail)
      {
        memcpy(buffer.data() + dataEnd, data.data(), data_size(data));
        dataEnd += data_size(data);
        dataSize += data_size(data);
      }
      else
      {
        size_t buf1Size = bytesToTail;
        size_t buf2Size = data_size(data) - buf1Size;
        memcpy(buffer.data() + dataEnd, data.data(), buf1Size);
        memcpy(&buffer[0], data.data() + buf1Size, buf2Size);
        dataEnd = buf2Size;
        dataSize += data_size(data);
      }
    }
    else
    {
      memcpy(&buffer[dataEnd], data.data(), data_size(data));
      dataEnd += data_size(data);
      dataSize += data_size(data);
    }
  }

  size_t read(dag::Span<uint8_t> data)
  {
    if (dataSize == 0)
      return 0;

    size_t bytesToCopy = eastl::min(dataSize, (size_t)data_size(data));
    size_t bytesToTail = buffer.size() - dataStart;
    if (bytesToCopy <= bytesToTail)
    {
      memcpy(data.data(), &buffer[dataStart], bytesToCopy);
      dataStart += bytesToCopy;
      if (dataStart >= buffer.size())
        dataStart -= buffer.size();
    }
    else
    {
      memcpy(data.data(), &buffer[dataStart], bytesToTail);
      memcpy(data.data() + bytesToTail, &buffer[0], bytesToCopy - bytesToTail);
      dataStart = bytesToCopy - bytesToTail;
    }
    dataSize -= bytesToCopy;
    return bytesToCopy;
  }

  void growBuf(size_t len)
  {
    size_t oldSize = buffer.size();
    buffer.resize(oldSize + len);
    if (dataSize == 0)
      return;
    // dataEnd == dataStart with dataSize > 0 is a full buffer, also wrapped
    if (dataEnd <= dataStart)
    {
      size_t bytesInHead = dataEnd;
      size_t bytesToMove = eastl::min(len, bytesInHead);
      memcpy(&buffer[oldSize], &buffer[0], bytesToMove);
      if (bytesToMove == bytesInHead)
      {
        dataEnd = oldSize + bytesToMove;
      }
      else
      {
        memmove(&buffer[0], &buffer[bytesToMove], dataEnd - bytesToMove);
        dataEnd -= bytesToMove;
      }
    }
  }

  size_t size() const { return dataSize; }

  Payload buffer;
  size_t dataStart = 0;
  size_t dataEnd = 0;
  size_t dataSize = 0;
};


class WebSocketCurlConnection
{
public:
  WebSocketCurlConnection(const eastl::string &url, const eastl::string &ip_resolve_entry, unsigned max_payload_size, bool verify_peer,
    bool verify_host, int connect_timeout_ms, const Headers &additional_headers);
  ~WebSocketCurlConnection();

  void sendWSdata(PayloadView payload, bool is_text);
  void sendWSCtrl(WSOpcodes ctrl, PayloadView payload);

  void sendClose(CloseStatusInt status);
  void sendPong();

  // Ends the upload, so curl can complete a transfer whose receive side already ended
  void endUpload();

  CURL *getCurlHandle() { return easy; }
  // constraint: CURLINFO_ACTIVESOCKET is unset during a FORBID_REUSE transfer, so the socket comes from the sockopt callback
  curl_socket_t getSocket() const { return socketFd; }

  struct WebsocketInput
  {
    bool pingReceived = false;
    bool closed = false;
    // This side queued the close frame; closeReason then holds our own text
    bool localClose = false;
    eastl::vector<ReceivedMessage> inbox;
    websocket::Error closeReason;

    void setCloseReason(CloseStatus status, eastl::string &&msg) { setCloseReason(static_cast<int>(status), eastl::move(msg)); }

    void setCloseReason(int status, eastl::string &&msg)
    {
      closeReason.code = status;
      closeReason.message = eastl::move(msg);
    }
  };

  WebsocketInput &getNetworkData() { return input; }
  bool isProtoSwitched() const { return websockProtoActive; }

  const char *lastCurlError() const { return errorBuffer; }
  // Both count from the connect until the first bytes move. The send side is
  // when curl took the bytes; the socket write itself is not observable.
  int msSinceLastRecv() const { return int(unsigned(get_time_msec()) - unsigned(lastRecvMs)); }
  int msSinceLastHandedToCurl() const { return int(unsigned(get_time_msec()) - unsigned(lastSendMs)); }
  size_t bytesQueuedBeforeCurl() const { return sndBuf.size(); }

private:
  bool processWebsocketFrame(PayloadView data);
  void sendRaw(PayloadView data);

  // curl callbacks
  size_t onHeaderData(eastl::string_view data);
  size_t pullData(dag::Span<uint8_t> out_buf);
  void consume(PayloadView data);

  friend size_t send_data(char *, size_t, size_t, void *);
  friend size_t receive_data(const char *, size_t, size_t, void *);
  friend size_t receive_header(const char *, size_t, size_t, void *);
  friend int record_socket(void *, curl_socket_t, curlsocktype);

private:
  // curl handle state and data
  CURL *easy = nullptr;
  curl_slist *headers = nullptr;
  curl_slist *resolve = nullptr;
  PrivateDnsCache dnsCache;
  RingBuffer sndBuf;
  int pauseFlags = 0;
  bool uploadEnded = false;
  curl_socket_t socketFd = CURL_SOCKET_BAD;
  char errorBuffer[CURL_ERROR_SIZE] = {};
  int lastRecvMs;
  int lastSendMs;

  // handshake state
  eastl::string secKey;
  struct
  {
    bool keyValidated = false;
    bool connUpgrade = false;
    bool upgradeWebsocket = false;
  } handshake;

  bool websockProtoActive = false;
  unsigned maxPayloadSize = 0;

  // websocket protocol state
  struct
  { //-V802
    Payload buffer;
    unsigned bufferCurPos = 0;
    Payload currentData;
    bool currentDataIsText = false;
    Payload controlPayload;
  } frame;

  // websocket input data
  WebsocketInput input;
};

static uint32_t gen_rand_mask()
{
  uint32_t mask;
  crypto::rand_bytes((uint8_t *)&mask, sizeof(mask));
  return mask;
}

static Payload xor_crypt_buf(PayloadView input, uint32_t mask)
{
  Payload result;
  result.resize(data_size(input));

  uint32_t *output32 = (uint32_t *)result.data();
  uint32_t const *input32 = (const uint32_t *)input.data();

  for (size_t i = 0; i < result.size() / 4; ++i)
    *output32++ = *input32++ ^ mask;

  uint8_t *output8 = (uint8_t *)output32;
  uint8_t *input8 = (uint8_t *)input32;

  union
  {
    uint32_t mask32;
    uint8_t mask8[4];
  } masku;
  masku.mask32 = mask;

  for (size_t i = 0; i < result.size() % 4; ++i)
    *output8++ = *input8++ ^ masku.mask8[i];

  return result;
}

void WebSocketCurlConnection::sendWSCtrl(WSOpcodes ctrl, PayloadView payload)
{
  WSHeaderPL16 header;
  header.fin = 1;
  header.reserved = 0;
  header.opcode = ctrl;
  header.mask = 1;
  header.payloadLen = data_size(payload);
  sendRaw(make_payload_view(&header, sizeof(WSHeader)));

  const uint32_t mask = gen_rand_mask();
  sendRaw(make_payload_view(&mask, sizeof(mask)));

  if (data_size(payload) > 0)
  {
    const Payload frameData = xor_crypt_buf(payload, mask);
    sendRaw(frameData);
  }
}

void WebSocketCurlConnection::sendWSdata(PayloadView payload, bool is_text)
{
  WSHeaderPL64 header;
  header.fin = 1;
  header.reserved = 0;
  header.opcode = is_text ? TEXT : BINARY;
  header.mask = 1;

  if (data_size(payload) <= 125)
  {
    header.payloadLen = data_size(payload);
    sendRaw(make_payload_view(&header, sizeof(WSHeader)));
  }
  else if (data_size(payload) <= UINT16_MAX)
  {
    WSHeaderPL16 *hdr16 = reinterpret_cast<WSHeaderPL16 *>(&header); //-V641
    header.payloadLen = 126;
    hdr16->payloadLen16 = htons(data_size(payload));
    sendRaw(make_payload_view(&header, sizeof(WSHeaderPL16)));
  }
  else
  {
    header.payloadLen = 127;
    header.payloadLen64 = htonll(data_size(payload));
    sendRaw(make_payload_view(&header, sizeof(WSHeaderPL64)));
  }

  const uint32_t mask = gen_rand_mask();
  sendRaw(make_payload_view(&mask, sizeof(mask)));

  const Payload frameData = xor_crypt_buf(payload, mask);
  sendRaw(frameData);
}

void WebSocketCurlConnection::sendRaw(PayloadView data)
{
  sndBuf.write(data);
  if (pauseFlags & CURLPAUSE_SEND)
  {
    pauseFlags &= ~CURLPAUSE_SEND;
    curl_easy_pause(easy, pauseFlags);
  }
}

size_t WebSocketCurlConnection::pullData(dag::Span<uint8_t> out_buf)
{
  if (sndBuf.size() == 0)
  {
    if (uploadEnded)
      return 0;
    pauseFlags |= CURLPAUSE_SEND;
    return CURL_READFUNC_PAUSE;
  }

  lastSendMs = get_time_msec();
  return sndBuf.read(out_buf);
}

void WebSocketCurlConnection::endUpload()
{
  uploadEnded = true;
  if (pauseFlags & CURLPAUSE_SEND)
  {
    pauseFlags &= ~CURLPAUSE_SEND;
    curl_easy_pause(easy, pauseFlags);
  }
}

size_t WebSocketCurlConnection::onHeaderData(eastl::string_view header)
{
  lastRecvMs = get_time_msec();
  static const char secWebSocketAccept[] = "Sec-WebSocket-Accept: ";
  static const char connectionUpgrade[] = "Connection: Upgrade\r\n";
  static const char upgradeWebsocket[] = "Upgrade: websocket\r\n";
  static const eastl::string_view rn("\r\n");

  if (dd_strnicmp(header.data(), secWebSocketAccept, (int)sizeof(secWebSocketAccept) - 1) == 0)
  {
    auto endPos = header.find(rn);
    size_t startPos = sizeof(secWebSocketAccept) - 1;
    eastl::string_view acceptVal = header.substr(startPos, endPos - startPos);
    eastl::string validAnswer = websocket_sec_answer(secKey);
    handshake.keyValidated = (acceptVal.compare(eastl::string_view(validAnswer.c_str())) == 0);
    if (!handshake.keyValidated)
    {
      debug("[websocket] answer key '%.*s' does not match valid key '%s'", (int)acceptVal.size(), acceptVal.data(),
        validAnswer.c_str());
    }
  }
  else if (dd_strnicmp(header.data(), connectionUpgrade, (int)header.size()) == 0)
  {
    handshake.connUpgrade = true;
  }
  else if (dd_strnicmp(header.data(), upgradeWebsocket, (int)header.size()) == 0)
  {
    handshake.upgradeWebsocket = true;
  }
  else if (header == rn)
  {
    long status = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    if (status != 101 || !handshake.keyValidated || !handshake.upgradeWebsocket || !handshake.connUpgrade)
    {
      logwarn("[websocket] handshake failed: http status %d, key validated %d, upgrade to ws %d, conn upgrade %d", status,
        handshake.keyValidated, handshake.upgradeWebsocket, handshake.connUpgrade);
      return 0;
    }
    websockProtoActive = true;
    // Verbose stays off past this point: with TLS it reports every record.
    // A failure's detail still reaches errorBuffer.
    curl_easy_setopt(easy, CURLOPT_VERBOSE, 0L);
    debug("[websocket] handshake ok. switching protocol");
  }

  return header.length();
}

void WebSocketCurlConnection::sendClose(CloseStatusInt status)
{
  status = htons(status);
  sendWSCtrl(CLOSE, make_payload_view(&status, sizeof(status)));
}

void WebSocketCurlConnection::sendPong() { sendWSCtrl(PONG, {}); }

void WebSocketCurlConnection::consume(PayloadView data)
{
  lastRecvMs = get_time_msec();
  const bool succeeded = processWebsocketFrame(data);
  if (!succeeded)
  {
    logwarn("[websocket] abandon WebSocket connection; closeStatus: %d; %s", input.closeReason.code,
      input.closeReason.message.c_str());
    sendClose((CloseStatusInt)input.closeReason.code);
    input.localClose = true;
    input.closed = true;
  }
}

bool WebSocketCurlConnection::processWebsocketFrame(PayloadView data)
{
  // https://datatracker.ietf.org/doc/html/rfc6455#section-5.4
  // Control frames (see Section 5.5) MAY be injected in the middle of
  // a fragmented message.Control frames themselves MUST NOT be fragmented.

  frame.buffer.insert(frame.buffer.end(), data.begin(), data.end());
  const PayloadView frameInput(frame.buffer);

  while (true)
  {
    const PayloadView bufferView = frameInput.subspan(frame.bufferCurPos);

    if (bufferView.size() < sizeof(WSHeader))
      break;

    const WSHeader *hdr = reinterpret_cast<const WSHeader *>(bufferView.data());
    unsigned int hdrLen = sizeof(WSHeader);
    const bool isControlFrame = hdr->opcode >= CLOSE; // https://datatracker.ietf.org/doc/html/rfc6455#section-5.5
    uint64_t payloadLen = 0;
    if (hdr->payloadLen <= 125)
    {
      payloadLen = hdr->payloadLen;
    }
    else if (hdr->payloadLen == 126)
    {
      if (bufferView.size() < sizeof(WSHeaderPL16))
        break;
      const WSHeaderPL16 *hdr16 = static_cast<const WSHeaderPL16 *>(hdr);
      payloadLen = ntohs(hdr16->payloadLen16);
      hdrLen = sizeof(WSHeaderPL16);
    }
    else // if (hdr->payloadLen == 127)
    {
      if (bufferView.size() < sizeof(WSHeaderPL64))
        break;
      const WSHeaderPL64 *hdr64 = static_cast<const WSHeaderPL64 *>(hdr);
      payloadLen = ntohll(hdr64->payloadLen64);
      hdrLen = sizeof(WSHeaderPL64);
    }

    if (isControlFrame && hdr->payloadLen > 125) // https://datatracker.ietf.org/doc/html/rfc6455#section-5.5
    {
      input.setCloseReason(CloseStatus::ProtocolError, "parsing error: control frame has extended payload");
      return false;
    }

    union
    {
      uint32_t mask32;
      uint8_t mask8[4];
    } mask;
    mask.mask32 = 0;

    const uint32_t payloadOffset = hdrLen + (hdr->mask ? sizeof(mask) : 0);
    const uint64_t frameSize = payloadOffset + payloadLen;

    if (bufferView.size() < frameSize)
      break;

    if (!isControlFrame)
    {
      if (payloadLen > maxPayloadSize || frame.currentData.size() + payloadLen > maxPayloadSize)
      {
        input.setCloseReason(CloseStatus::TooBigData,
          {eastl::string::CtorSprintf(), "parsing error: extra %llu bytes will make too big frame size: %llu bytes",
            (unsigned long long)payloadLen, (unsigned long long)(frame.currentData.size() + payloadLen)});
        return false;
      }
    }

    if (payloadLen)
    {
      if (hdr->mask)
        mask.mask32 = *(reinterpret_cast<const uint32_t *>(&bufferView[hdrLen]));

      uint8_t *outputBuffer = nullptr;
      if (!isControlFrame)
      {
        if (!frame.currentData.empty() && hdr->opcode != CONTINUATION)
        {
          input.setCloseReason(CloseStatus::ProtocolError,
            {eastl::string::CtorSprintf(), "parsing error: non-continuation opcode %d while waiting continuation", (int)hdr->opcode});
          return false;
        }

        const size_t outputOffset = frame.currentData.size();
        frame.currentData.resize(outputOffset + payloadLen);
        outputBuffer = &frame.currentData[outputOffset];
      }
      else // control frame
      {
        frame.controlPayload.resize(payloadLen);
        outputBuffer = frame.controlPayload.data();
      }

      if (mask.mask32)
      {
        uint32_t *outputWords = reinterpret_cast<uint32_t *>(outputBuffer);
        const uint32_t *inputWords = reinterpret_cast<const uint32_t *>(&bufferView[payloadOffset]);

        for (size_t i = 0; i < payloadLen / 4; ++i)
          *outputWords++ = *inputWords++ ^ mask.mask32;

        uint8_t *outputBytes = reinterpret_cast<uint8_t *>(outputWords);
        const uint8_t *inputBytes = reinterpret_cast<const uint8_t *>(inputWords);

        for (size_t i = 0; i < payloadLen % 4; ++i)
          *outputBytes++ = *inputBytes++ ^ mask.mask8[i];
      }
      else
      {
        memcpy(outputBuffer, &bufferView[payloadOffset], payloadLen);
      }
    }

    switch (hdr->opcode)
    {
      case PING: input.pingReceived = true; break;
      case PONG: break;
      case CLOSE:
        input.closed = true;
        // It may happen the close was initiated by the application, so avoid overwriting the status code and the reason
        if (input.closeReason.code == 0)
          input.closeReason = log_peer_close(frame.controlPayload);
        break;
      case CONTINUATION:
      case TEXT:
      case BINARY:
        if (hdr->opcode != CONTINUATION)
        {
          frame.currentDataIsText = hdr->opcode == TEXT;
        }
        if (hdr->fin)
        {
          ReceivedMessage message{{}, frame.currentDataIsText};
          eastl::swap(message.payload, frame.currentData);
          input.inbox.emplace_back(eastl::move(message));
        }
        break;
      default:
        input.setCloseReason(CloseStatus::ProtocolError,
          {eastl::string::CtorSprintf(), "parsing error: unknown opcode %d", (int)hdr->opcode});
        return false;
    }

    frame.bufferCurPos += static_cast<decltype(frame.bufferCurPos)>(frameSize);
  }

  frame.buffer.erase(frame.buffer.begin(), frame.buffer.begin() + frame.bufferCurPos);
  frame.bufferCurPos = 0;

  return true;
}

size_t send_data(char *buffer, size_t count, size_t nitems, void *data)
{
  WebSocketCurlConnection *pthis = (WebSocketCurlConnection *)data;
  return pthis->pullData(make_span((uint8_t *)buffer, count * nitems));
}

size_t receive_data(const char *buffer, size_t count, size_t nitems, void *data)
{
  WebSocketCurlConnection *pthis = (WebSocketCurlConnection *)data;
  pthis->consume(make_payload_view(buffer, count * nitems));
  return count * nitems;
}

size_t receive_header(const char *buffer, size_t count, size_t nitems, void *data)
{
  WebSocketCurlConnection *pthis = (WebSocketCurlConnection *)data;
  return pthis->onHeaderData(eastl::string_view(buffer, count * nitems));
}

int record_socket(void *data, curl_socket_t fd, curlsocktype purpose)
{
  if (purpose == CURLSOCKTYPE_IPCXN)
    ((WebSocketCurlConnection *)data)->socketFd = fd;
  return CURL_SOCKOPT_OK;
}

WebSocketCurlConnection::WebSocketCurlConnection(const eastl::string &uri, const eastl::string &ip_resolve_entry,
  unsigned max_payload_size, bool verify_peer, bool verify_host, int connect_timeout_ms, const Headers &additional_headers) :
  // Use INT_MAX to prevent bufferCurPos overflow and to prevent problems on 32 bit systems
  maxPayloadSize(max_payload_size > 0 ? std::min(max_payload_size, (unsigned)INT_MAX) : INT_MAX)
{
  const curl_version_info_data *cver = curl_version_info(CURLVERSION_NOW);
  if (cver->version_num < 0x073202)
    DAG_FATAL("CURL version '%s'. At least '7.50.2' is required for WebSocket to work reliably", cver->version);

  easy = curl_easy_init();
  if (!easy)
    DAG_FATAL("failed to allocate curl handle");
  lastRecvMs = lastSendMs = get_time_msec();
  curl_easy_setopt(easy, CURLOPT_HEADERDATA, this);
  curl_easy_setopt(easy, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(easy, CURLOPT_READDATA, this);
  curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, receive_header);
  curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, receive_data);
  curl_easy_setopt(easy, CURLOPT_READFUNCTION, send_data);
  curl_easy_setopt(easy, CURLOPT_SOCKOPTDATA, this);
  curl_easy_setopt(easy, CURLOPT_SOCKOPTFUNCTION, record_socket);
  curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
  // constraint: the multi is shared, so its pool must neither give nor keep this upgraded connection
  curl_easy_setopt(easy, CURLOPT_FRESH_CONNECT, 1L);
  curl_easy_setopt(easy, CURLOPT_FORBID_REUSE, 1L);
  set_common_curl_options(easy, errorBuffer, verify_peer, verify_host, connect_timeout_ms);

  eastl::string fixedUri = uri;
  if (fixedUri.find("ws://") == 0)
    fixedUri.replace(0, 5, "http://");
  else if (fixedUri.find("wss://") == 0)
    fixedUri.replace(0, 6, "https://");

  curl_easy_setopt(easy, CURLOPT_URL, fixedUri.c_str());

  curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  curl_easy_setopt(easy, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "GET");
  headers = curl_slist_append(headers, "Expect:");
  headers = curl_slist_append(headers, "Transfer-Encoding:");
  headers = curl_slist_append(headers, "Connection: Upgrade");
  headers = curl_slist_append(headers, "Upgrade: websocket");
  headers = curl_slist_append(headers, "Sec-WebSocket-Version: 13");

  secKey = generate_sec_key();
  eastl::string secKeyHeader = "Sec-WebSocket-Key: " + secKey;
  headers = curl_slist_append(headers, secKeyHeader.c_str());

  headers = append_headers(headers, additional_headers);
  curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

  if (!ip_resolve_entry.empty())
  {
    resolve = curl_slist_append(nullptr, ip_resolve_entry.c_str());
    curl_easy_setopt(easy, CURLOPT_RESOLVE, resolve);
    dnsCache.attach(easy);
  }
}

WebSocketCurlConnection::~WebSocketCurlConnection()
{
  curl_easy_cleanup(easy);
  if (headers)
    curl_slist_free_all(headers);
  if (resolve)
    curl_slist_free_all(resolve);
}

class CompatConnection final : public IoConnection
{
public:
  using IoConnection::IoConnection;
  ~CompatConnection() override;

  void start(CURLM *multi_handle, const ConnectRequest &request) override;
  void onTransferDone(CURLcode code) override;
  void send(Payload &&payload, bool is_text) override;
  void close(CloseStatus status) override;
  void perform() override;
  curl_socket_t getWaitSocket(short &events) const override;
  bool getDeadline(int &deadline_ms) const override;
  bool hasPendingWork() const override;

private:
  void reportReceived();
  void detectPeerClose();

  CURLM *multi = nullptr;
  bool httpActive = true;
  bool peerClosed = false;
  int closeSentMs = 0;
  eastl::unique_ptr<WebSocketCurlConnection> wsConn;
};

CompatConnection::~CompatConnection()
{
  if (multi)
    curl_multi_remove_handle(multi, wsConn->getCurlHandle());
}

void CompatConnection::start(CURLM *multi_handle, const ConnectRequest &request)
{
  wsConn = eastl::make_unique<WebSocketCurlConnection>(request.uri, request.ipResolveEntry, settings.maxMessageSize,
    settings.verifyPeer, settings.verifyHost, settings.connectTimeoutMs, request.headers);

  CURL *easy = wsConn->getCurlHandle();
  curl_easy_setopt(easy, CURLOPT_PRIVATE, static_cast<IoConnection *>(this));
  CURLMcode mc = curl_multi_add_handle(multi_handle, easy);
  if (mc != CURLM_OK)
  {
    logwarn("[websocket] curl_multi_add_handle() failed, code %d '%s'.", mc, curl_multi_strerror(mc));
    emitConnectFailed(
      Error{CURLE_FAILED_INIT, {eastl::string::CtorSprintf(), "curl_multi_add_handle() failed: %s", curl_multi_strerror(mc)}});
    return;
  }
  multi = multi_handle;
}

void CompatConnection::send(Payload &&payload, bool is_text)
{
  if (httpActive)
  {
    logwarn("[websocket] send failed. websocket proto is not ready");
    return;
  }

  wsConn->sendWSdata(payload, is_text);
}

void CompatConnection::close(CloseStatus status)
{
  if (httpActive)
  {
    debug("[websocket] HTTP connection is closed by the application with code %d", (int)status);
    emitConnectFailed(app_close_error(status));
    return;
  }

  auto &wsInput = wsConn->getNetworkData();
  if (!wsInput.localClose)
    closeSentMs = get_time_msec();
  wsInput.closeReason = app_close_error(status);
  wsInput.localClose = true;
  debug("[websocket] WebSocket connection is closed by the application with code %d", (int)status);

  wsConn->sendClose((CloseStatusInt)status);
}

void CompatConnection::onTransferDone(CURLcode code)
{
  if (isFinished())
    return;
  reportReceived();

  long respCode = 0;
  CURL *easy = wsConn->getCurlHandle();
  curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &respCode);
  const char *url = nullptr;
  curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &url);
  if (url == nullptr)
    url = "<unknown url>";

  websocket::Error error;
  const bool endedWithoutCloseFrame =
    !httpActive && !wsConn->getNetworkData().closed && (code == CURLE_OK || code == CURLE_GOT_NOTHING);
  if (endedWithoutCloseFrame)
  {
    debug("[websocket] '%s' closed by the peer without a close frame", url);
    error.code = (int)CloseStatus::AbnormalClosure;
    error.message.sprintf("connection closed by the peer without a close frame; last data %d ms ago", wsConn->msSinceLastRecv());
  }
  else if (code != CURLE_OK)
  {
    long osErrno = 0;
    curl_easy_getinfo(easy, CURLINFO_OS_ERRNO, &osErrno);
    const char *peerIp = nullptr;
    long localPort = 0;
    curl_easy_getinfo(easy, CURLINFO_PRIMARY_IP, &peerIp);
    curl_easy_getinfo(easy, CURLINFO_LOCAL_PORT, &localPort);
    // Only our own close text is repeated here, never a peer's reason.
    const WebSocketCurlConnection::WebsocketInput &wsInput = wsConn->getNetworkData();
    eastl::string localCloseNote;
    if (wsInput.localClose)
      localCloseNote.sprintf("; local close queued with code %d: %s", wsInput.closeReason.code, wsInput.closeReason.message.c_str());
    logwarn("[websocket] receiving from '%s' (%s, local port %ld) failed: %s (curl %d '%s', os errno %ld); "
            "last recv %d ms ago, last bytes handed to curl %d ms ago, %u bytes still queued before curl%s",
      url, peerIp ? peerIp : "", localPort, curl_easy_strerror(code), (int)code, wsConn->lastCurlError(), osErrno,
      wsConn->msSinceLastRecv(), wsConn->msSinceLastHandedToCurl(), (unsigned)wsConn->bytesQueuedBeforeCurl(), localCloseNote.c_str());
    error.code = code;
    error.message = wsConn->lastCurlError()[0] ? wsConn->lastCurlError() : curl_easy_strerror(code);
    error.message += localCloseNote;
  }
  else
  {
    debug("[websocket] receiving from '%s' succeed with code %d", url, (int)respCode);
    error.code = respCode;
    error.message.sprintf("got HTTP response instead of WebSocket upgrade. It seems the server is not configured. Response code: %d",
      (int)respCode);
  }

  const auto remoteCloseStatus = (websocket::CloseStatusInt)wsConn->getNetworkData().closeReason.code;
  if (httpActive)
    emitConnectFailed(eastl::move(error));
  else
    emitClosed(remoteCloseStatus, eastl::move(error));
}

void CompatConnection::reportReceived()
{
  if (httpActive && wsConn->isProtoSwitched())
  {
    httpActive = false;

    const char *actualConnectedIp = nullptr;
    curl_easy_getinfo(wsConn->getCurlHandle(), CURLINFO_PRIMARY_IP, &actualConnectedIp);
    actualConnectedIp = actualConnectedIp ? actualConnectedIp : "";
    long localPort = 0;
    curl_easy_getinfo(wsConn->getCurlHandle(), CURLINFO_LOCAL_PORT, &localPort);
    debug("[websocket] connected to %s from local port %ld", actualConnectedIp, localPort);

    emitConnected(actualConnectedIp);
  }

  auto &wsInput = wsConn->getNetworkData();
  for (auto &inboxMsg : wsInput.inbox)
    emitMessage(eastl::move(inboxMsg.payload), inboxMsg.isText);
  wsInput.inbox.clear();
}

curl_socket_t CompatConnection::getWaitSocket(short &events) const
{
  // constraint: with the upload paused, curl stops watching the socket after its receive side ended
  if (httpActive || peerClosed)
    return CURL_SOCKET_BAD;
  events = CURL_WAIT_POLLIN;
  return wsConn->getSocket();
}

void CompatConnection::detectPeerClose()
{
  const curl_socket_t sock = wsConn->getSocket();
  char probe;
  // zero means the peer sent FIN and curl read every byte before it
  if (sock == CURL_SOCKET_BAD || os_socket_recvfrom((os_socket_t)sock, &probe, 1, MSG_PEEK) != 0)
    return;
  peerClosed = true;
  wsConn->endUpload();
}

void CompatConnection::perform()
{
  reportReceived();

  auto &wsInput = wsConn->getNetworkData();
  if (wsInput.pingReceived)
  {
    wsConn->sendPong();
    wsInput.pingReceived = false;
  }

  if (!httpActive && !peerClosed && !wsInput.closed)
    detectPeerClose();

  if (settings.closeTimeoutMs > 0 && wsInput.localClose && !wsInput.closed && ms_since(closeSentMs) >= settings.closeTimeoutMs)
  {
    wsInput.closeReason = log_close_timeout(settings.closeTimeoutMs);
    wsInput.closed = true;
  }

  if (wsInput.closed)
  {
    websocket::Error error = eastl::move(wsInput.closeReason);
    emitClosed((CloseStatusInt)error.code, eastl::move(error));
  }
}

bool CompatConnection::getDeadline(int &deadline_ms) const
{
  if (!wsConn || httpActive || settings.closeTimeoutMs <= 0)
    return false;
  const auto &wsInput = wsConn->getNetworkData();
  if (!wsInput.localClose || wsInput.closed)
    return false;
  deadline_ms = closeSentMs + settings.closeTimeoutMs;
  return true;
}

bool CompatConnection::hasPendingWork() const
{
  if (!wsConn)
    return false;
  const auto &wsInput = wsConn->getNetworkData();
  return (httpActive && wsConn->isProtoSwitched()) || !wsInput.inbox.empty() || wsInput.pingReceived || wsInput.closed;
}

static eastl::unique_ptr<IoConnection> create_compat_connection(const IoSettings &settings, IoSlot &slot)
{
  return eastl::make_unique<CompatConnection>(settings, slot);
}

} // namespace

eastl::unique_ptr<WebSocketClient> create_compat_client(ClientConfig &&config)
{
  return create_io_client(eastl::move(config), &create_compat_connection);
}

} // namespace websocket
