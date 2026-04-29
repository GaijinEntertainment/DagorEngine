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

// WebSocketClient has different implementations: CURL-based and Xbox-specific.
//
// * All class methods must be called from the same thread. They are not thread-safe.
// * User or WebSocketClient must be ready that application-provided callbacks can be called from different threads.
// * Callbacks can be also called directly from inside of methods `connect`, `send`, `close` and `poll`.
// * For example Xbox calls `onMessage` from some Microsoft internal thread.
// * User must not call `poll()` from inside of callbacks because `poll()` is not ready for this.
class WebSocketClient
{
public:
  virtual ~WebSocketClient() = default;

  // `connect_ip_hint` is a hint for implementation to use this IP instead of real DNS resolving
  // `connect_ip_hint` should be empty string when no IP hint is provided
  virtual void connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
    const Headers &additional_headers) = 0;
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
  MessageCb onMessage;
  CloseCb onClose;
};

eastl::unique_ptr<WebSocketClient> create_client(ClientConfig &&config);

void set_ca_bundle(const char *bundle_file);
} // namespace websocket
