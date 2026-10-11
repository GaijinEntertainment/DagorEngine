// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <websocket/websocket.h>

#include <dag/dag_vector.h>

#include <curl/curl.h>

// shortcut: 8.22.0 only - lower once older libcurl tested
#define WEBSOCKET_CURL_WS_MIN_VERSION 0x081600

namespace websocket
{

// constraint: resize() does not zero-fill, every resized byte is written before it is read
using Payload = dag::Vector<uint8_t, EASTLAllocatorType, false>;

struct ReceivedMessage
{
  Payload payload;
  bool isText = false;
};

inline constexpr PayloadView make_payload_view(const void *data, size_t data_size)
{
  return PayloadView{static_cast<const uint8_t *>(data), data_size};
}

eastl::string generate_sec_key();
eastl::string websocket_sec_answer(const eastl::string &sec_key);

eastl::string create_ip_resolve_entry(const eastl::string &connect_uri, const eastl::string &connect_ip_hint);

// constraint: a CURLOPT_RESOLVE entry is permanent in the DNS cache, so a hinted easy handle must not use the shared multi's
// cache. Destroy this after curl_easy_cleanup() of the handle it is attached to.
class PrivateDnsCache
{
public:
  PrivateDnsCache() = default;
  PrivateDnsCache(const PrivateDnsCache &) = delete;
  PrivateDnsCache &operator=(const PrivateDnsCache &) = delete;
  ~PrivateDnsCache();

  void attach(CURL *easy);

private:
  CURLSH *share = nullptr;
};

Error log_close_timeout(int timeout_ms);
Error log_peer_close(PayloadView payload);
Error app_close_error(CloseStatus status);

void set_common_curl_options(CURL *easy, char *error_buffer, bool verify_peer, bool verify_host, int connect_timeout_ms);
curl_slist *append_headers(curl_slist *list, const Headers &headers);

bool curl_has_websocket();

eastl::unique_ptr<WebSocketClient> create_curl_ws_client(ClientConfig &&config);
eastl::unique_ptr<WebSocketClient> create_compat_client(ClientConfig &&config);

} // namespace websocket
