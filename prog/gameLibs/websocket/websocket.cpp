// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocket/websocket.h>

#include <debug/dag_fatal.h>
#include <util/dag_strUtil.h>

#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <b64/cencode.h>
#include <curl/curl.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <string.h>

#ifdef _MSC_VER
#include <stdlib.h>
#include <Winsock2.h> // hton*, ntoh*
static int strncasecmp(const char *s1, const char *s2, size_t count) { return _strnicmp(s1, s2, count); }
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

using Payload = eastl::vector<uint8_t>;

static const char websocket_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static eastl::string ca_bundle_file;

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
      size_t bytesToTail = buffer.size() - dataEnd;
      if (data_size(data) <= bytesToTail)
      {
        memcpy(&buffer[dataEnd], data.data(), data_size(data));
        dataEnd += data_size(data);
        dataSize += data_size(data);
      }
      else
      {
        size_t buf1Size = bytesToTail;
        size_t buf2Size = data_size(data) - buf1Size;
        memcpy(&buffer[dataEnd], data.data(), buf1Size);
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
    if (dataEnd < dataStart) // data is wrapped
    {
      size_t bytesInHead = dataEnd;
      size_t bytesToMove = eastl::min(len, bytesInHead);
      memcpy(&buffer[oldSize], &buffer[0], bytesToMove);
      if (bytesToMove == bytesInHead)
      {
        dataEnd = (oldSize - 1) + bytesToMove;
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


struct ReceivedMessage
{
  Payload payload;
  bool isText = false;
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
  void sendPing();

  CURL *getCurlHandle() { return easy; }

  struct WebsocketInput
  {
    bool pingReceived = false;
    bool pongReceived = false;
    bool closed = false;
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

private:
  // curl handle state and data
  CURL *easy = nullptr;
  curl_slist *headers = nullptr;
  curl_slist *resolve = nullptr;
  RingBuffer sndBuf;
  int pauseFlags = 0;

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
  {
    Payload buffer;
    unsigned bufferCurPos = 0;
    Payload currentData;
    bool currentDataIsText = false;
    Payload controlPayload;
  } frame;

  // websocket input data
  WebsocketInput input;
};

static eastl::string generate_sec_key()
{
  uint8_t secKey[16];
  char secKeyB64[sizeof(secKey) * 2];
  memset(&secKeyB64[0], 0, sizeof(secKeyB64));
  RAND_bytes(&secKey[0], sizeof(secKey));
  base64_encodestate enc;
  base64_init_encodestate(&enc);
  int len = base64_encode_block((const char *)&secKey[0], sizeof(secKey), &secKeyB64[0], &enc);
  base64_encode_blockend(&secKeyB64[len], &enc);
  return secKeyB64;
}

static uint32_t gen_rand_mask()
{
  uint32_t mask;
  RAND_bytes((uint8_t *)&mask, sizeof(mask));
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

static eastl::string websocket_sec_answer(const eastl::string &sec_key)
{
  uint8_t webSocketRespKeySHA1[20];
  SHA_CTX shaCtx;
  SHA1_Init(&shaCtx);
  SHA1_Update(&shaCtx, sec_key.data(), sec_key.size());
  SHA1_Update(&shaCtx, &websocket_guid[0], sizeof(websocket_guid) - 1);
  SHA1_Final(&webSocketRespKeySHA1[0], &shaCtx);

  eastl::string webSocketRespKeyB64;
  webSocketRespKeyB64.resize(40);
  base64_encodestate enc;
  base64_init_encodestate(&enc);

  int len = base64_encode_block((const char *)&webSocketRespKeySHA1[0], sizeof(webSocketRespKeySHA1), &webSocketRespKeyB64[0], &enc);
  base64_encode_blockend(&webSocketRespKeyB64[len], &enc);
  return webSocketRespKeyB64;
}

static constexpr PayloadView make_payload_view(const void *data, size_t data_size)
{
  return PayloadView{static_cast<const uint8_t *>(data), data_size};
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
  else if (data_size(payload) <= INT16_MAX)
  {
    WSHeaderPL16 *hdr16 = reinterpret_cast<WSHeaderPL16 *>(&header);
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
    pauseFlags |= CURLPAUSE_SEND;
    return CURL_READFUNC_PAUSE;
  }

  return sndBuf.read(out_buf);
}

size_t WebSocketCurlConnection::onHeaderData(eastl::string_view header)
{
  static const char secWebSocketAccept[] = "Sec-WebSocket-Accept: ";
  static const char connectionUpgrade[] = "Connection: Upgrade\r\n";
  static const char upgradeWebsocket[] = "Upgrade: websocket\r\n";
  static const eastl::string_view rn("\r\n");

  if (strncasecmp(header.data(), secWebSocketAccept, sizeof(secWebSocketAccept) - 1) == 0)
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
  else if (strncasecmp(header.data(), connectionUpgrade, header.size()) == 0)
  {
    handshake.connUpgrade = true;
  }
  else if (strncasecmp(header.data(), upgradeWebsocket, header.size()) == 0)
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

void WebSocketCurlConnection::sendPing() { sendWSCtrl(PING, {}); }

void WebSocketCurlConnection::consume(PayloadView data)
{
  const bool succeeded = processWebsocketFrame(data);
  if (!succeeded)
  {
    logwarn("[websocket] abandon WebSocket connection; closeStatus: %d; %s", input.closeReason.code,
      input.closeReason.message.c_str());
    sendClose((CloseStatusInt)input.closeReason.code);
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
      case PONG: input.pongReceived = true; break;
      case CLOSE:
        input.closed = true;
        // It may happen the close was initiated by the application, so avoid overwriting the status code and the reason
        if (input.closeReason.code == 0)
        {
          CloseStatusInt remoteCloseStatus = 0;
          if (frame.controlPayload.size() >= 2)
          {
            remoteCloseStatus = ntohs(*((CloseStatusInt const *)frame.controlPayload.data()));
          }

          input.setCloseReason(remoteCloseStatus,
            {eastl::string::CtorSprintf(), "connection was closed by the remote side with code %d", remoteCloseStatus});

          debug("[websocket] connection is closed by the remote side with code %d", remoteCloseStatus);
        }
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

// Remove user SSO JWT from outgoing HTTP headers when connecting to the SEP server
static eastl::string strip_sensitive_data_from_http_headers(eastl::string_view curl_multiline_message)
{
  constexpr eastl::string_view auth_prefix = "Authorization: Bearer ";

  eastl::string result;
  result.reserve(curl_multiline_message.size());

  while (!curl_multiline_message.empty())
  {
    const size_t lineFeedPos = curl_multiline_message.find('\n');
    const size_t lineLength = lineFeedPos == eastl::string_view::npos ? curl_multiline_message.size() : lineFeedPos + 1;
    const eastl::string_view line(curl_multiline_message.data(), lineLength); // with \r\n at the end
    curl_multiline_message.remove_prefix(lineLength);

    // Check for "Authorization: Bearer" header (case-insensitive as per HTTP spec)
    if (line.size() >= auth_prefix.size() && strncasecmp(line.data(), auth_prefix.data(), auth_prefix.size()) == 0)
    {
      // JWT format: <header_as_base64url>.<payload_as_base64url>.<signature>
      const size_t lastDot = line.rfind('.');
      if (lastDot != eastl::string_view::npos)
      {
        const eastl::string_view beforeSignature = line.substr(0, lastDot + 1);
        const ptrdiff_t signatureLength = line.size() - (lastDot + 1) - strlen("\r\n");
        result.append_sprintf("%.*s.<jwt signature (length: %lld bytes)>\r\n", (int)beforeSignature.size(), beforeSignature.data(),
          (long long)signatureLength);
        continue;
      }
    }

    // Not an Authorization header or invalid JWT: copy as is
    result.append(line.data(), line.size());
  }

  return result;
}

static size_t curl_debug(CURL *, curl_infotype info_type, char *msg, size_t n, void *)
{
  const char *prefix = "";
  switch (info_type)
  {
    case CURLINFO_TEXT: prefix = "CURLINFO_TEXT"; break;
    case CURLINFO_HEADER_IN: prefix = "CURLINFO_HEADER_IN"; break;
    case CURLINFO_HEADER_OUT: prefix = "CURLINFO_HEADER_OUT"; break;
    case CURLINFO_DATA_IN: prefix = "CURLINFO_DATA_IN"; break;
    case CURLINFO_DATA_OUT: prefix = "CURLINFO_DATA_OUT"; break;
    case CURLINFO_SSL_DATA_IN: prefix = "CURLINFO_SSL_DATA_IN"; break;
    case CURLINFO_SSL_DATA_OUT: prefix = "CURLINFO_SSL_DATA_OUT"; break;
    case CURLINFO_END: prefix = "CURLINFO_END"; break;
  }

  eastl::string messageData;
  eastl::string_view messageView{msg, n};

  if (info_type == CURLINFO_SSL_DATA_IN || info_type == CURLINFO_SSL_DATA_OUT)
  {
    messageData.sprintf("%llu bytes", (unsigned long long)n);
    messageView = messageData;
  }
  else if (info_type == CURLINFO_HEADER_OUT)
  {
    messageData = strip_sensitive_data_from_http_headers(messageView);
    messageView = messageData;
  }

  // remove trailing newlines added by curl or by HTTP protocol
  while (!messageView.empty() && (messageView.back() == '\r' || messageView.back() == '\n'))
    messageView.remove_suffix(1);

  debug("[websocket/http] %s %.*s", prefix, (int)messageView.size(), messageView.data());
  return 0;
}

static eastl::string create_ip_resolve_entry(const eastl::string &connect_uri, const eastl::string &connect_ip_hint)
{
  if (connect_ip_hint.empty())
    return {};

  char *host = nullptr;
  char *port = nullptr;
  CURLU *url = curl_url();
  if (url)
  {
    CURLUcode rc = curl_url_set(url, CURLUPART_URL, connect_uri.c_str(), 0);
    if (rc == CURLUE_OK)
    {
      curl_url_get(url, CURLUPART_HOST, &host, 0);
      curl_url_get(url, CURLUPART_PORT, &port, CURLU_DEFAULT_PORT);
    }
    curl_url_cleanup(url);
  }

  eastl::string resolveEntry;
  if (host && port)
  {
    resolveEntry.sprintf("%s:%s:%s", host, port, connect_ip_hint.c_str());
  }
  else
  {
    logwarn("[websocket] ignore connect IP (%s) because failed to decode URL: %s", connect_ip_hint.c_str(), connect_uri.c_str());
  }

  curl_free(host);
  curl_free(port);

  return resolveEntry;
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
  curl_easy_setopt(easy, CURLOPT_PRIVATE, this);
  curl_easy_setopt(easy, CURLOPT_HEADERDATA, this);
  curl_easy_setopt(easy, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(easy, CURLOPT_READDATA, this);
  curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, receive_header);
  curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, receive_data);
  curl_easy_setopt(easy, CURLOPT_READFUNCTION, send_data);
  curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(easy, CURLOPT_DEBUGFUNCTION, curl_debug);
  if (connect_timeout_ms > 0)
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms);

  if (!ca_bundle_file.empty() && verify_peer)
  {
    curl_easy_setopt(easy, CURLOPT_CAINFO, ca_bundle_file.c_str());
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, verify_host ? 2L : 0L);
  }
  else
  {
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
  }

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

  for (const auto &header : additional_headers)
  {
    const eastl::string headerLine{eastl::string::CtorSprintf(), "%s: %s", header.name.c_str(), header.value.c_str()};
    headers = curl_slist_append(headers, headerLine.c_str());
  }

  curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

  if (!ip_resolve_entry.empty())
  {
    resolve = curl_slist_append(nullptr, ip_resolve_entry.c_str());
    curl_easy_setopt(easy, CURLOPT_RESOLVE, resolve);
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

class WebSocketClientImpl : public WebSocketClient
{
public:
  WebSocketClientImpl(ClientConfig &&config);
  ~WebSocketClientImpl();

  void connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
    const Headers &additional_headers) override;
  void send(PayloadView message, bool is_text) override;
  void close(CloseStatus) override;
  void poll() override;

private:
  eastl::string connectIpHint;
  ConnectCb connectCb;
  CloseCb closeCb;
  MessageCb messageCb;

  size_t maxMessageSize;
  bool verifyPeer;
  bool verifyHost;
  int connectTimeoutMs;

  CURLM *curlm = nullptr;
  bool httpActive = true;
  eastl::unique_ptr<WebSocketCurlConnection> wsConn;
};

WebSocketClientImpl::WebSocketClientImpl(ClientConfig &&config) :
  closeCb(eastl::move(config.onClose)),
  messageCb(eastl::move(config.onMessage)),
  maxMessageSize(config.maxMessageSize),
  verifyPeer(config.verifyCert),
  verifyHost(config.verifyHost),
  connectTimeoutMs(config.connectTimeoutMs)
{}

WebSocketClientImpl::~WebSocketClientImpl()
{
  if (curlm)
  {
    curl_multi_remove_handle(curlm, wsConn->getCurlHandle());
    curl_multi_cleanup(curlm);
  }
}

void WebSocketClientImpl::connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
  const Headers &additional_headers)
{
  if (wsConn)
  {
    connect_cb(this, ConnectStatus::ConnectIsActive, connectIpHint, Error::no_error());
    return;
  }

  const eastl::string ipResolveEntry = create_ip_resolve_entry(uri, connect_ip_hint);
  connectIpHint = !ipResolveEntry.empty() ? connect_ip_hint : "";

  connectCb = eastl::move(connect_cb);
  wsConn = eastl::make_unique<WebSocketCurlConnection>(uri, ipResolveEntry, maxMessageSize, verifyPeer, verifyHost, connectTimeoutMs,
    additional_headers);

  CURL *easy = wsConn->getCurlHandle();

  curlm = curl_multi_init();
  CURLMcode mc = curl_multi_add_handle(curlm, easy);
  if (mc != CURLM_OK)
  {
    logwarn("[websocket] curl_multi_add_handle() failed, code %d '%s'.", mc, curl_multi_strerror(mc));
    return;
  }
  httpActive = true;
}

void WebSocketClientImpl::send(PayloadView message, bool is_text)
{
  G_ASSERT(!httpActive && curlm);
  if (!curlm)
  {
    logwarn("[websocket] send failed. connection is not active");
    return;
  }

  if (httpActive)
  {
    logwarn("[websocket] send failed. websocket proto is not ready");
    return;
  }

  wsConn->sendWSdata(message, is_text);
}

void WebSocketClientImpl::close(CloseStatus status)
{
  if (!curlm)
    return;

  if (httpActive)
  {
    curl_multi_remove_handle(curlm, wsConn->getCurlHandle());
    curl_multi_cleanup(curlm);
    curlm = nullptr;
    wsConn.reset();

    websocket::Error error;
    error.code = (int)status;
    error.message.sprintf("connection was closed by the application with code %d", (int)status);
    debug("[websocket] HTTP connection is closed by the application with code %d", (int)status);

    connectCb(this, ConnectStatus::HttpConnectFailed, connectIpHint, eastl::move(error));
    return;
  }

  auto &wsInput = wsConn->getNetworkData();
  wsInput.setCloseReason(status, {eastl::string::CtorSprintf(), "connection was closed by the application with code %d", (int)status});
  debug("[websocket] WebSocket connection is closed by the application with code %d", (int)status);

  wsConn->sendClose((CloseStatusInt)status);
}

void WebSocketClientImpl::poll()
{
  if (!curlm)
    return;

  int nrunning = 0;
  curl_multi_perform(curlm, &nrunning);

  int msgInQueue = 0;
  while (CURLMsg *msg = curl_multi_info_read(curlm, &msgInQueue))
  {
    if (msg->msg != CURLMSG_DONE)
    {
      if (!msgInQueue)
        break;
      else
        continue;
    }

    long respCode = 0;
    CURL *easy = msg->easy_handle;
    CURLcode code = msg->data.result;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &respCode);
    const char *url = nullptr;
    curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &url);
    if (url == nullptr)
      url = "<unknown url>";

    websocket::Error error;
    if (code != CURLE_OK)
    {
      logwarn("[websocket] receiving from '%s' failed: %s", url, curl_easy_strerror(code));
      error.code = code;
      error.message = curl_easy_strerror(code);
    }
    else
    {
      debug("[websocket] receiving from '%s' succeed with code %d", url, (int)respCode);
      error.code = respCode;
      error.message.sprintf("got HTTP response instead of WebSocket upgrade. It seems the server is not configured. Response code: %d",
        (int)respCode);
    }

    curl_multi_remove_handle(curlm, easy);
    curl_multi_cleanup(curlm);
    curlm = nullptr;

    const auto remoteCloseStatus = (websocket::CloseStatusInt)wsConn->getNetworkData().closeReason.code;
    const bool isProtoSwitched = wsConn->isProtoSwitched();
    wsConn.reset();

    if (httpActive && !isProtoSwitched)
      connectCb(this, ConnectStatus::HttpConnectFailed, connectIpHint, eastl::move(error));
    else
      closeCb(this, remoteCloseStatus, eastl::move(error));
    return;
  }

  if (httpActive && wsConn->isProtoSwitched())
  {
    httpActive = false;

    const char *actualConnectedIp = nullptr;
    curl_easy_getinfo(wsConn->getCurlHandle(), CURLINFO_PRIMARY_IP, &actualConnectedIp);
    actualConnectedIp = actualConnectedIp ? actualConnectedIp : "";

    connectCb(this, ConnectStatus::Ok, actualConnectedIp, Error::no_error());
  }

  auto &wsInput = wsConn->getNetworkData();
  if (wsInput.pingReceived)
  {
    wsConn->sendPong();
    wsInput.pingReceived = false;
  }

  for (auto &inboxMsg : wsInput.inbox)
    messageCb(this, inboxMsg.payload, inboxMsg.isText);
  wsInput.inbox.clear();

  if (wsInput.closed)
  {
    curl_multi_remove_handle(curlm, wsConn->getCurlHandle());
    curl_multi_cleanup(curlm);
    curlm = nullptr;
    websocket::Error error = eastl::move(wsInput.closeReason);
    wsConn.reset();

    closeCb(this, (CloseStatusInt)error.code, eastl::move(error));
  }
}

eastl::unique_ptr<WebSocketClient> create_client(ClientConfig &&config)
{
  return eastl::make_unique<WebSocketClientImpl>(eastl::move(config));
}

void set_ca_bundle(const char *bundle_file) { ca_bundle_file = bundle_file; }

} // namespace websocket
