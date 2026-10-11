// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "websocket_common.h"

#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_localConv.h>

#include <EASTL/string_view.h>
#include <b64/cencode.h>
#include <dagCrypto/rand.h>
#include <hash/sha1.h>

#include <string.h>

namespace websocket
{

static const char websocket_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static eastl::string ca_bundle_file;

void set_ca_bundle(const char *bundle_file) { ca_bundle_file = bundle_file; }

eastl::string generate_sec_key()
{
  uint8_t secKey[16];
  char secKeyB64[sizeof(secKey) * 2];
  memset(&secKeyB64[0], 0, sizeof(secKeyB64));
  crypto::rand_bytes(&secKey[0], sizeof(secKey));
  base64_encodestate enc;
  base64_init_encodestate(&enc);
  int len = base64_encode_block((const char *)&secKey[0], sizeof(secKey), &secKeyB64[0], &enc);
  base64_encode_blockend(&secKeyB64[len], &enc);
  return secKeyB64;
}

eastl::string websocket_sec_answer(const eastl::string &sec_key)
{
  uint8_t webSocketRespKeySHA1[SHA1_DIGEST_LENGTH];
  sha1_context shaCtx;
  sha1_starts(&shaCtx);
  sha1_update(&shaCtx, (const unsigned char *)sec_key.data(), sec_key.size());
  sha1_update(&shaCtx, (const unsigned char *)&websocket_guid[0], sizeof(websocket_guid) - 1);
  sha1_finish(&shaCtx, &webSocketRespKeySHA1[0]);

  eastl::string webSocketRespKeyB64;
  webSocketRespKeyB64.resize(40);
  base64_encodestate enc;
  base64_init_encodestate(&enc);

  int len = base64_encode_block((const char *)&webSocketRespKeySHA1[0], sizeof(webSocketRespKeySHA1), &webSocketRespKeyB64[0], &enc);
  len += base64_encode_blockend(&webSocketRespKeyB64[len], &enc);
  webSocketRespKeyB64.resize(len);
  return webSocketRespKeyB64;
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
    if (line.size() >= auth_prefix.size() && dd_strnicmp(line.data(), auth_prefix.data(), (int)auth_prefix.size()) == 0)
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

static int curl_debug(CURL *, curl_infotype info_type, char *msg, size_t n, void *)
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

void PrivateDnsCache::attach(CURL *easy)
{
  share = curl_share_init();
  if (!share)
    DAG_FATAL("failed to allocate curl share handle");
  curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  curl_easy_setopt(easy, CURLOPT_SHARE, share);
}

PrivateDnsCache::~PrivateDnsCache()
{
  if (share)
    curl_share_cleanup(share);
}

eastl::string create_ip_resolve_entry(const eastl::string &connect_uri, const eastl::string &connect_ip_hint)
{
  if (connect_ip_hint.empty())
    return {};

#if LIBCURL_VERSION_NUM < 0x073E00 // URL parser API, 7.62.0
  logwarn("[websocket] ignore connect IP (%s) because libcurl %s cannot decode URL: %s", connect_ip_hint.c_str(), LIBCURL_VERSION,
    connect_uri.c_str());
  return {};
#else
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
#endif
}

Error log_close_timeout(int timeout_ms)
{
  logwarn("[websocket] no close frame from the peer in %d ms, closing the connection", timeout_ms);
  return {(int)CloseStatus::AbnormalClosure, {eastl::string::CtorSprintf(), "no close frame from the peer in %d ms", timeout_ms}};
}

Error log_peer_close(PayloadView payload)
{
  // constraint: RFC 6455 5.5.1, optional status then reason
  CloseStatusInt status = (CloseStatusInt)CloseStatus::NoStatusRcvd;
  eastl::string_view reason;
  if (payload.size() >= 2)
  {
    status = (CloseStatusInt)((payload[0] << 8) | payload[1]);
    reason = eastl::string_view((const char *)payload.data() + 2, payload.size() - 2);
  }
  if (reason.empty())
    reason = "<no reason text>";

  debug("[websocket] connection is closed by the remote side with code %d; reason: %.*s", status, (int)reason.size(), reason.data());
  return {status, {eastl::string::CtorSprintf(), "connection was closed by the remote side with code %d; reason: %.*s", status,
                    (int)reason.size(), reason.data()}};
}

Error app_close_error(CloseStatus status)
{
  return {(int)status, {eastl::string::CtorSprintf(), "connection was closed by the application with code %d", (int)status}};
}

void set_common_curl_options(CURL *easy, char *error_buffer, bool verify_peer, bool verify_host, int connect_timeout_ms)
{
  curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(easy, CURLOPT_DEBUGFUNCTION, curl_debug);
  curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error_buffer);
  if (connect_timeout_ms > 0)
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, (long)connect_timeout_ms);

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
}

curl_slist *append_headers(curl_slist *list, const Headers &headers)
{
  for (const auto &header : headers)
  {
    const eastl::string headerLine{eastl::string::CtorSprintf(), "%s: %s", header.name.c_str(), header.value.c_str()};
    list = curl_slist_append(list, headerLine.c_str());
  }
  return list;
}

} // namespace websocket
