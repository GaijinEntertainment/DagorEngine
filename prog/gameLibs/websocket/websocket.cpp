// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "websocket_io.h"

#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_localConv.h>

#include <EASTL/algorithm.h>
#include <EASTL/deque.h>
#include <EASTL/string_view.h>

#include <string.h>

namespace websocket
{

#if LIBCURL_VERSION_NUM >= WEBSOCKET_CURL_WS_MIN_VERSION

namespace
{

struct OutgoingFrame
{
  Payload payload;
  unsigned flags = 0;
  size_t sent = 0;
  // constraint: curl_ws_send continues an unfinished frame
  bool started = false;
};

static eastl::string to_ws_scheme(const eastl::string &uri)
{
  if (uri.find("http://") == 0)
    return "ws://" + uri.substr(7);
  if (uri.find("https://") == 0)
    return "wss://" + uri.substr(8);
  return uri;
}

static bool has_header_token(eastl::string_view value, eastl::string_view token)
{
  while (!value.empty())
  {
    const size_t comma = value.find(',');
    eastl::string_view item = value.substr(0, comma);
    while (!item.empty() && (item.front() == ' ' || item.front() == '\t'))
      item.remove_prefix(1);
    while (!item.empty() && (item.back() == ' ' || item.back() == '\t'))
      item.remove_suffix(1);
    if (item.size() == token.size() && dd_strnicmp(item.data(), token.data(), (int)token.size()) == 0)
      return true;
    if (comma == eastl::string_view::npos)
      break;
    value.remove_prefix(comma + 1);
  }
  return false;
}

static const char *find_response_header(CURL *easy, const char *name)
{
  curl_header *header = nullptr;
  return curl_easy_header(easy, name, 0, CURLH_1XX, -1, &header) == CURLHE_OK ? header->value : nullptr;
}

// constraint: libcurl checks no 101 response headers
static eastl::string check_upgrade_response(CURL *easy, const eastl::string &sec_key)
{
  const char *upgrade = find_response_header(easy, "Upgrade");
  if (!upgrade || !has_header_token(upgrade, "websocket"))
    return {eastl::string::CtorSprintf(), "invalid Upgrade header in the server response: '%s'", upgrade ? upgrade : ""};

  const char *connection = find_response_header(easy, "Connection");
  if (!connection || !has_header_token(connection, "upgrade"))
    return {eastl::string::CtorSprintf(), "invalid Connection header in the server response: '%s'", connection ? connection : ""};

  const char *accept = find_response_header(easy, "Sec-WebSocket-Accept");
  const eastl::string validAccept = websocket_sec_answer(sec_key);
  if (!accept || validAccept != accept)
    return {eastl::string::CtorSprintf(), "Sec-WebSocket-Accept '%s' in the server response does not match valid key '%s'",
      accept ? accept : "", validAccept.c_str()};

  return {};
}

class CurlWsConnection final : public IoConnection
{
public:
  using IoConnection::IoConnection;
  ~CurlWsConnection() override;

  void start(CURLM *multi_handle, const ConnectRequest &request) override;
  void onTransferDone(CURLcode code) override;
  void send(Payload &&payload, bool is_text) override;
  void close(CloseStatus status) override;
  void perform() override;
  curl_socket_t getWaitSocket(short &events) const override;
  bool getDeadline(int &deadline_ms) const override;
  bool hasPendingWork() const override { return moreToReceive; }

private:
  CURLcode receive();
  void onFrameData(PayloadView data, const curl_ws_frame &meta);
  void onPeerClose();
  void failLocally(CloseStatus status, eastl::string &&reason);
  void dropUnstartedFrames();
  CURLcode flushSendQueue();
  void queueFrame(Payload &&payload, unsigned flags);
  void queueClose(CloseStatusInt status);
  void failTransport(CURLcode code);
  int closeReplyWaitMs() const;

  // constraint: libcurl ws calls need the easy kept in multi
  CURLM *multi = nullptr;
  CURL *easy = nullptr;
  curl_slist *headers = nullptr;
  curl_slist *resolve = nullptr;
  PrivateDnsCache dnsCache;
  char errorBuffer[CURL_ERROR_SIZE] = {};
  eastl::string secKey;
  curl_socket_t wsSocket = CURL_SOCKET_BAD;

  bool open = false;
  bool localClose = false;
  bool closed = false;
  bool moreToReceive = false;
  int closeSentMs = 0;
  int closedMs = 0;
  Error closeReason;

  int lastRecvMs = 0;
  int lastSendMs = 0;

  eastl::deque<OutgoingFrame> sendQueue;
  Payload message;
  Payload control;
  Payload recvBuffer;
};

static constexpr int max_recv_calls_per_perform = 64;
static constexpr int default_close_reply_wait_ms = 1000;

CurlWsConnection::~CurlWsConnection()
{
  if (easy)
  {
    if (multi)
      curl_multi_remove_handle(multi, easy);
    curl_easy_cleanup(easy);
  }
  curl_slist_free_all(headers);
  curl_slist_free_all(resolve);
}

void CurlWsConnection::start(CURLM *multi_handle, const ConnectRequest &request)
{
  easy = curl_easy_init();
  if (!easy)
    DAG_FATAL("failed to allocate curl handle");
  lastRecvMs = lastSendMs = get_time_msec();

  set_common_curl_options(easy, errorBuffer, settings.verifyPeer, settings.verifyHost, settings.connectTimeoutMs);
  curl_easy_setopt(easy, CURLOPT_PRIVATE, static_cast<IoConnection *>(this));
  curl_easy_setopt(easy, CURLOPT_URL, to_ws_scheme(request.uri).c_str());
  curl_easy_setopt(easy, CURLOPT_CONNECT_ONLY, 2L);
  // constraint: libcurl sends auto PONG only on later calls
  curl_easy_setopt(easy, CURLOPT_WS_OPTIONS, CURLWS_NOAUTOPONG);

  // constraint: libcurl sends a given Sec-WebSocket-Key unchanged
  secKey = generate_sec_key();
  const eastl::string secKeyHeader = "Sec-WebSocket-Key: " + secKey;
  headers = curl_slist_append(nullptr, secKeyHeader.c_str());
  headers = append_headers(headers, request.headers);
  curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

  if (!request.ipResolveEntry.empty())
  {
    resolve = curl_slist_append(nullptr, request.ipResolveEntry.c_str());
    curl_easy_setopt(easy, CURLOPT_RESOLVE, resolve);
    dnsCache.attach(easy);
  }

  const CURLMcode mc = curl_multi_add_handle(multi_handle, easy);
  if (mc != CURLM_OK)
  {
    logwarn("[websocket] curl_multi_add_handle() failed, code %d '%s'.", mc, curl_multi_strerror(mc));
    emitConnectFailed(
      Error{CURLE_FAILED_INIT, {eastl::string::CtorSprintf(), "curl_multi_add_handle() failed: %s", curl_multi_strerror(mc)}});
    return;
  }
  multi = multi_handle;
}

void CurlWsConnection::send(Payload &&payload, bool is_text)
{
  if (!open)
  {
    logwarn("[websocket] send failed. websocket proto is not ready");
    return;
  }

  if (localClose)
  {
    logwarn("[websocket] send failed. connection is closing");
    return;
  }

  queueFrame(eastl::move(payload), is_text ? CURLWS_TEXT : CURLWS_BINARY);
}

void CurlWsConnection::close(CloseStatus status)
{
  if (!open)
  {
    debug("[websocket] HTTP connection is closed by the application with code %d", (int)status);
    emitConnectFailed(app_close_error(status));
    return;
  }

  if (localClose)
    return;

  localClose = true;
  closeSentMs = get_time_msec();
  closeReason = app_close_error(status);
  debug("[websocket] WebSocket connection is closed by the application with code %d", (int)status);

  queueClose((CloseStatusInt)status);
}

void CurlWsConnection::onTransferDone(CURLcode code)
{
  if (open || isFinished())
    return;

  long httpStatus = 0;
  curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &httpStatus);
  const char *url = nullptr;
  curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &url);
  url = url ? url : "<unknown url>";
  const char *peerIp = nullptr;
  curl_easy_getinfo(easy, CURLINFO_PRIMARY_IP, &peerIp);
  peerIp = peerIp ? peerIp : "";
  long localPort = 0;
  curl_easy_getinfo(easy, CURLINFO_LOCAL_PORT, &localPort);

  Error error;
  if (code == CURLE_OK && httpStatus == 101)
  {
    eastl::string handshakeError = check_upgrade_response(easy, secKey);
    if (handshakeError.empty())
    {
      open = true;
      lastRecvMs = get_time_msec();
      recvBuffer.resize(64 << 10);
      // constraint: the finished CONNECT_ONLY transfer adds no socket to the multi wait
      curl_easy_getinfo(easy, CURLINFO_ACTIVESOCKET, &wsSocket);
      // constraint: with TLS, verbose logs every record
      curl_easy_setopt(easy, CURLOPT_VERBOSE, 0L);
      debug("[websocket] connected to %s from local port %ld", peerIp, localPort);
      emitConnected(peerIp);
      return;
    }
    logwarn("[websocket] handshake with '%s' failed: %s", url, handshakeError.c_str());
    error.code = CURLE_WEIRD_SERVER_REPLY;
    error.message = eastl::move(handshakeError);
  }
  else if (code == CURLE_OK || code == CURLE_HTTP_RETURNED_ERROR)
  {
    logwarn("[websocket] '%s' (%s) refused the WebSocket upgrade with HTTP status %ld", url, peerIp, httpStatus);
    error.code = (int)httpStatus;
    error.message.sprintf("got HTTP response instead of WebSocket upgrade. It seems the server is not configured. Response code: %d",
      (int)httpStatus);
  }
  else
  {
    long osErrno = 0;
    curl_easy_getinfo(easy, CURLINFO_OS_ERRNO, &osErrno);
    logwarn("[websocket] connecting to '%s' (%s, local port %ld) failed: %s (curl %d '%s', os errno %ld)", url, peerIp, localPort,
      curl_easy_strerror(code), (int)code, errorBuffer, osErrno);
    error.code = code;
    error.message = errorBuffer[0] ? errorBuffer : curl_easy_strerror(code);
  }

  emitConnectFailed(eastl::move(error));
}

void CurlWsConnection::perform()
{
  if (!open)
    return;

  CURLcode result = flushSendQueue();
  if (result == CURLE_OK)
    result = receive();
  if (result == CURLE_OK)
    result = flushSendQueue();

  if (result != CURLE_OK && !closed)
  {
    failTransport(result);
    return;
  }

  if (settings.closeTimeoutMs > 0 && localClose && !closed && ms_since(closeSentMs) >= settings.closeTimeoutMs)
  {
    closeReason = log_close_timeout(settings.closeTimeoutMs);
    closed = true;
    sendQueue.clear();
  }

  if (closed && result == CURLE_OK && !sendQueue.empty() && ms_since(closedMs) < closeReplyWaitMs())
    return;

  if (closed)
  {
    moreToReceive = false;
    emitClosed((CloseStatusInt)closeReason.code, eastl::move(closeReason));
  }
}

curl_socket_t CurlWsConnection::getWaitSocket(short &events) const
{
  if (!open)
    return CURL_SOCKET_BAD;
  events = CURL_WAIT_POLLIN | (sendQueue.empty() ? 0 : CURL_WAIT_POLLOUT);
  return wsSocket;
}

bool CurlWsConnection::getDeadline(int &deadline_ms) const
{
  if (!open)
    return false;
  if (closed && !sendQueue.empty())
  {
    deadline_ms = closedMs + closeReplyWaitMs();
    return true;
  }
  if (localClose && !closed && settings.closeTimeoutMs > 0)
  {
    deadline_ms = closeSentMs + settings.closeTimeoutMs;
    return true;
  }
  return false;
}

int CurlWsConnection::closeReplyWaitMs() const
{
  return settings.closeTimeoutMs > 0 ? settings.closeTimeoutMs : default_close_reply_wait_ms;
}

// constraint: the cap keeps one flooding connection from starving the others in the shared I/O loop
CURLcode CurlWsConnection::receive()
{
  moreToReceive = false;
  for (int calls = 0; !closed; ++calls)
  {
    if (calls == max_recv_calls_per_perform)
    {
      moreToReceive = true;
      break;
    }
    size_t received = 0;
    const curl_ws_frame *meta = nullptr;
    const CURLcode code = curl_ws_recv(easy, recvBuffer.data(), recvBuffer.size(), &received, &meta);
    if (code == CURLE_AGAIN)
      return CURLE_OK;
    if (code != CURLE_OK)
      return code;
    lastRecvMs = get_time_msec();
    onFrameData(make_payload_view(recvBuffer.data(), received), *meta);
  }
  return CURLE_OK;
}

// constraint: curl_ws_recv can split one frame into chunks
void CurlWsConnection::onFrameData(PayloadView data, const curl_ws_frame &meta)
{
  if (meta.flags & (CURLWS_TEXT | CURLWS_BINARY))
  {
    if (meta.offset == 0)
    {
      const uint64_t frameSize = data.size() + (uint64_t)meta.bytesleft;
      if (settings.maxMessageSize > 0 && message.size() + frameSize > settings.maxMessageSize)
      {
        failLocally(CloseStatus::TooBigData,
          {eastl::string::CtorSprintf(), "incoming message is too big: %llu bytes, the limit is %u bytes",
            (unsigned long long)(message.size() + frameSize), settings.maxMessageSize});
        return;
      }
    }
    message.insert(message.end(), data.begin(), data.end());
    if (meta.bytesleft == 0 && !(meta.flags & CURLWS_CONT))
    {
      emitMessage(eastl::move(message), (meta.flags & CURLWS_TEXT) != 0);
      message = {};
    }
    return;
  }

  if (meta.offset == 0)
    control.clear();
  control.insert(control.end(), data.begin(), data.end());
  if (meta.bytesleft != 0)
    return;

  if (meta.flags & CURLWS_PING)
    queueFrame(Payload(control), CURLWS_PONG);
  else if (meta.flags & CURLWS_CLOSE)
    onPeerClose();
}

void CurlWsConnection::onPeerClose()
{
  closed = true;
  closedMs = get_time_msec();
  dropUnstartedFrames();
  if (localClose)
    return;

  closeReason = log_peer_close(control);
  const CloseStatusInt remoteCloseStatus = (CloseStatusInt)closeReason.code;

  // constraint: RFC 6455 requires a close frame reply
  if (remoteCloseStatus == (CloseStatusInt)CloseStatus::NoStatusRcvd)
    queueFrame({}, CURLWS_CLOSE);
  else
    queueClose(remoteCloseStatus);
  localClose = true;
}

void CurlWsConnection::failLocally(CloseStatus status, eastl::string &&reason)
{
  logwarn("[websocket] abandon WebSocket connection; closeStatus: %d; %s", (int)status, reason.c_str());
  closeReason = {(int)status, eastl::move(reason)};
  dropUnstartedFrames();
  if (!localClose)
    queueClose((CloseStatusInt)status);
  localClose = true;
  closed = true;
  closedMs = get_time_msec();
}

// constraint: after a close, only the current frame finishes
void CurlWsConnection::dropUnstartedFrames()
{
  auto isUnsentData = [](const OutgoingFrame &frame) { return !frame.started && frame.flags != CURLWS_CLOSE; };
  sendQueue.erase(eastl::remove_if(sendQueue.begin(), sendQueue.end(), isUnsentData), sendQueue.end());
}

void CurlWsConnection::queueFrame(Payload &&payload, unsigned flags)
{
  sendQueue.push_back();
  OutgoingFrame &frame = sendQueue.back();
  frame.payload = eastl::move(payload);
  frame.flags = flags;
}

void CurlWsConnection::queueClose(CloseStatusInt status)
{
  queueFrame(Payload{uint8_t(uint16_t(status) >> 8), uint8_t(status)}, CURLWS_CLOSE);
}

CURLcode CurlWsConnection::flushSendQueue()
{
  while (!sendQueue.empty())
  {
    OutgoingFrame &frame = sendQueue.front();
    frame.started = true;
    size_t sent = 0;
    const CURLcode code =
      curl_ws_send(easy, frame.payload.data() + frame.sent, frame.payload.size() - frame.sent, &sent, 0, frame.flags);
    frame.sent += sent;
    if (code == CURLE_AGAIN)
      return CURLE_OK;
    if (code != CURLE_OK)
      return code;
    lastSendMs = get_time_msec();
    if (frame.sent < frame.payload.size())
    {
      if (sent == 0)
        return CURLE_OK;
      continue;
    }
    sendQueue.pop_front();
  }
  return CURLE_OK;
}

void CurlWsConnection::failTransport(CURLcode code)
{
  const char *url = nullptr;
  curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &url);
  url = url ? url : "<unknown url>";
  long osErrno = 0;
  curl_easy_getinfo(easy, CURLINFO_OS_ERRNO, &osErrno);
  const char *peerIp = nullptr;
  curl_easy_getinfo(easy, CURLINFO_PRIMARY_IP, &peerIp);
  long localPort = 0;
  curl_easy_getinfo(easy, CURLINFO_LOCAL_PORT, &localPort);

  size_t bytesQueued = 0;
  for (const OutgoingFrame &frame : sendQueue)
    bytesQueued += frame.payload.size() - frame.sent;

  eastl::string localCloseNote;
  if (localClose)
    localCloseNote.sprintf("; local close queued with code %d: %s", closeReason.code, closeReason.message.c_str());

  // constraint: curl reports peer TCP close as CURLE_GOT_NOTHING
  const char *curlError = code == CURLE_GOT_NOTHING ? "connection was closed by the remote side without a close frame"
                          : errorBuffer[0]          ? errorBuffer
                                                    : curl_easy_strerror(code);
  logwarn("[websocket] receiving from '%s' (%s, local port %ld) failed: %s (curl %d '%s', os errno %ld); "
          "last recv %d ms ago, last bytes handed to curl %d ms ago, %u bytes still queued before curl%s",
    url, peerIp ? peerIp : "", localPort, curl_easy_strerror(code), (int)code, curlError, osErrno, ms_since(lastRecvMs),
    ms_since(lastSendMs), (unsigned)bytesQueued, localCloseNote.c_str());

  Error error{code, curlError};
  error.message += localCloseNote;
  moreToReceive = false;
  emitClosed((CloseStatusInt)closeReason.code, eastl::move(error));
}

static eastl::unique_ptr<IoConnection> create_curl_ws_connection(const IoSettings &settings, IoSlot &slot)
{
  return eastl::make_unique<CurlWsConnection>(settings, slot);
}

} // namespace

bool curl_has_websocket()
{
  const curl_version_info_data *ver = curl_version_info(CURLVERSION_NOW);
  if (ver->version_num < WEBSOCKET_CURL_WS_MIN_VERSION)
    return false;
  for (const char *const *protocol = ver->protocols; protocol && *protocol; ++protocol)
    if (strcmp(*protocol, "ws") == 0)
      return true;
  return false;
}

eastl::unique_ptr<WebSocketClient> create_curl_ws_client(ClientConfig &&config)
{
  return create_io_client(eastl::move(config), &create_curl_ws_connection);
}

#else

bool curl_has_websocket() { return false; }

eastl::unique_ptr<WebSocketClient> create_curl_ws_client(ClientConfig &&) { return nullptr; }

#endif

eastl::unique_ptr<WebSocketClient> create_client(ClientConfig &&config)
{
  static const bool useCurlWs = [] {
    const bool hasWebSocket = curl_has_websocket();
    debug("[websocket] libcurl %s: %s", curl_version_info(CURLVERSION_NOW)->version,
      hasWebSocket ? "using its WebSocket support" : "no WebSocket support, using the compat client");
    return hasWebSocket;
  }();

  if (useCurlWs)
    return create_curl_ws_client(eastl::move(config));
  return create_compat_client(eastl::move(config));
}

} // namespace websocket
