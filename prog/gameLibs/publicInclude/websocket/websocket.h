//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>
#include <EASTL/span.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_moveOnlyFunction.h>

namespace websocket
{

using CloseStatusInt = int16_t;

enum class ConnectStatus
{
  Ok,
  HttpConnectFailed,
  ConnectIsActive
};

enum class CloseStatus : CloseStatusInt
{
  Normal = 1000,
  GoingAway = 1001,
  ProtocolError = 1002,
  UnsupportedData = 1003,
  FrameTooLarge = 1004,
  NoStatusRcvd = 1005,
  AbnormalClosure = 1006,
  BadMessageData = 1007,
  PolicyViolation = 1008,
  TooBigData = 1009,
  ExtensionMismatch = 1010,
  ServerError = 1011,
  UnknownError = 4000
};

using PayloadView = eastl::span<const uint8_t>;

struct Header
{
  eastl::string name;
  eastl::string value;
};
using Headers = eastl::vector<Header>;

// Error codes:
// 0 - success
// [1..99] - CURL error codes
// [100..999] - HTTP error codes
// [1000..4999] - WebSocket status codes
// [-29'000..-29'999] - Gaijin-specific server-side JSON-RPC defined errors (see SEP_AUTHENTICATION_ERROR)
// [-32'000..-32'999] - WebSocket JSON-RPC defined errors
// [-40'000..-40'999] - Gaijin-specific client-side JSON-RPC defined errors (see INVALID_SERVER_RESPONSE)
struct Error
{
  int code = 0;
  eastl::string message;

  static inline Error no_error() { return {}; }
  static inline Error missing_error() { return {-404, "Missing error info. Platform does not support WebSocket errors"}; }
};

class WebSocketClient;

template <class... Args>
using WebSocketCallback = dag::MoveOnlyFunction<void(WebSocketClient *, Args...)>;

// `actual_uri_ip` can be null if implementation does not know actual remote IP address
using ConnectCb = WebSocketCallback<ConnectStatus, const eastl::string & /* actual_uri_ip */, Error && /* error */>;
using CloseCb = WebSocketCallback<CloseStatusInt, Error && /* error */>;
using MessageCb = WebSocketCallback<PayloadView, bool>; // payload, isText

// CURL-based. All clients share one I/O thread and one worker thread, which run while any client exists.
// connect, send and close post a command from any thread and never call a callback: results come in a later poll.
// poll runs the connect, close and onMessage callbacks on the one polling thread; never call it from a callback.
// Destroy on the polling thread (a poll callback is fine), never in onMessageAsync; it waits for this client's callbacks.
class WebSocketClient
{
public:
  virtual ~WebSocketClient() = default;

  // `connect_ip_hint` is a hint for implementation to use this IP instead of real DNS resolving
  // `connect_ip_hint` should be empty string when no IP hint is provided
  virtual void connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
    const Headers &additional_headers) = 0;
  // The caller must not call send() after close() until a new connect() succeeds
  virtual void send(PayloadView message, bool is_text) = 0;
  virtual void close(CloseStatus) = 0;
  virtual void poll() = 0;
};

struct ClientConfig
{
  bool verifyCert = true;      // used only if `set_ca_bundle()` was called; value can be ignored on some platforms
  bool verifyHost = true;      // used only if `set_ca_bundle()` was called; value can be ignored on some platforms
  int connectTimeoutMs = 0;    // 0 means using default built-in connection timeout; value can be ignored on some platforms
  unsigned maxMessageSize = 0; // 0 means unlimited message size
  // Wait for the peer's close frame after close(); 0 is no limit, on expiry onClose gets 1006.
  // With a ws-capable libcurl, also the wait to flush our close frame after the peer's one or a failure; 1 s when 0.
  int closeTimeoutMs = 0;
  MessageCb onMessage;
  // Replaces onMessage when set. Runs on the worker thread in order, after the connect callback and before onClose.
  MessageCb onMessageAsync;
  CloseCb onClose;
};

eastl::unique_ptr<WebSocketClient> create_client(ClientConfig &&config);

void set_ca_bundle(const char *bundle_file);

// Call at app shutdown: closes every client still alive with GoingAway and polls the ones this thread polls for up to
// close_wait_ms, so their owners can release them in onClose; a client without a connection gets no onClose.
// Reports each client left as an error; returns their count.
unsigned shutdown(int close_wait_ms = 1000);
} // namespace websocket
