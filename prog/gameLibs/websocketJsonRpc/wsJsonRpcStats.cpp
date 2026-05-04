// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/wsJsonRpcStats.h>

#include <websocketJsonRpc/details/urlParse.h>
#include <websocketJsonRpc/wsJsonRpcConnection.h>


namespace websocketjsonrpc
{

eastl::string_view get_known_error_name(int error_code)
{
  switch (error_code)
  {
    case -404: return "MISSING_WS_ERROR_INFO";

    case 0: return "SUCCESS";

    // CURL error codes
    case 1: return "CURLE_UNSUPPORTED_PROTOCOL";
    case 2: return "CURLE_FAILED_INIT";
    case 3: return "CURLE_URL_MALFORMAT";
    case 5: return "CURLE_COULDNT_RESOLVE_PROXY";
    case 6: return "CURLE_COULDNT_RESOLVE_HOST";
    case 7: return "CURLE_COULDNT_CONNECT";
    case 8: return "CURLE_WEIRD_SERVER_REPLY";
    case 9: return "CURLE_REMOTE_ACCESS_DENIED";
    case 16: return "CURLE_HTTP2_ERROR";
    case 18: return "CURLE_PARTIAL_FILE";
    case 22: return "CURLE_HTTP_RETURNED_ERROR";
    case 23: return "CURLE_WRITE_ERROR";
    case 25: return "CURLE_UPLOAD_FAILED";
    case 26: return "CURLE_READ_ERROR";
    case 27: return "CURLE_OUT_OF_MEMORY";
    case 28: return "CURLE_OPERATION_TIMEDOUT";
    case 33: return "CURLE_RANGE_ERROR";
    case 35: return "CURLE_SSL_CONNECT_ERROR";
    case 36: return "CURLE_BAD_DOWNLOAD_RESUME";
    case 37: return "CURLE_FILE_COULDNT_READ_FILE";
    case 42: return "CURLE_ABORTED_BY_CALLBACK";
    case 43: return "CURLE_BAD_FUNCTION_ARGUMENT";
    case 45: return "CURLE_INTERFACE_FAILED";
    case 47: return "CURLE_TOO_MANY_REDIRECTS";
    case 49: return "CURLE_SETOPT_OPTION_SYNTAX";
    case 52: return "CURLE_GOT_NOTHING";
    case 53: return "CURLE_SSL_ENGINE_NOTFOUND";
    case 54: return "CURLE_SSL_ENGINE_SETFAILED";
    case 55: return "CURLE_SEND_ERROR";
    case 56: return "CURLE_RECV_ERROR";
    case 58: return "CURLE_SSL_CERTPROBLEM";
    case 59: return "CURLE_SSL_CIPHER";
    case 60: return "CURLE_PEER_FAILED_VERIFICATION";
    case 61: return "CURLE_BAD_CONTENT_ENCODING";
    case 63: return "CURLE_FILESIZE_EXCEEDED";
    case 64: return "CURLE_USE_SSL_FAILED";
    case 65: return "CURLE_SEND_FAIL_REWIND";
    case 66: return "CURLE_SSL_ENGINE_INITFAILED";
    case 67: return "CURLE_LOGIN_DENIED";
    case 77: return "CURLE_SSL_CACERT_BADFILE";
    case 78: return "CURLE_REMOTE_FILE_NOT_FOUND";
    case 79: return "CURLE_SSH";
    case 80: return "CURLE_SSL_SHUTDOWN_FAILED";
    case 81: return "CURLE_AGAIN";
    case 82: return "CURLE_SSL_CRL_BADFILE";
    case 83: return "CURLE_SSL_ISSUER_ERROR";
    case 88: return "CURLE_CHUNK_FAILED";
    case 90: return "CURLE_SSL_PINNEDPUBKEYNOTMATCH";
    case 91: return "CURLE_SSL_INVALIDCERTSTATUS";
    case 92: return "CURLE_HTTP2_STREAM";
    case 93: return "CURLE_RECURSIVE_API_CALL";
    case 94: return "CURLE_AUTH_ERROR";
    case 95: return "CURLE_HTTP3";
    case 96: return "CURLE_QUIC_CONNECT_ERROR";
    case 97: return "CURLE_PROXY";
    case 98: return "CURLE_SSL_CLIENTCERT";
    case 99: return "CURLE_UNRECOVERABLE_POLL";
    // case 100: return "CURLE_TOO_LARGE";
    // case 101: return "CURLE_ECH_REQUIRED";

    // HTTP error codes
    // 1xx:
    case 100: return "HTTP_100_CONTINUE";
    case 101: return "HTTP_101_SWITCHING_PROTOCOLS";
    case 102: return "HTTP_102_PROCESSING";
    // 2xx:
    case 200: return "HTTP_200_OK";
    case 201: return "HTTP_201_CREATED";
    case 202: return "HTTP_202_ACCEPTED";
    case 203: return "HTTP_203_NON_AUTHORITATIVE_INFORMATION";
    case 204: return "HTTP_204_NO_CONTENT";
    case 205: return "HTTP_205_RESET_CONTENT";
    case 206: return "HTTP_206_PARTIAL_CONTENT";
    case 207: return "HTTP_207_MULTI_STATUS";
    case 208: return "HTTP_208_ALREADY_REPORTED";
    // 3xx:
    case 300: return "HTTP_300_MULTIPLE_CHOICES";
    case 301: return "HTTP_301_MOVED_PERMANENTLY";
    case 302: return "HTTP_302_FOUND";
    case 303: return "HTTP_303_SEE_OTHER";
    case 304: return "HTTP_304_NOT_MODIFIED";
    case 305: return "HTTP_305_USE_PROXY";
    case 306: return "HTTP_306_RESERVED";
    case 307: return "HTTP_307_TEMPORARY_REDIRECT";
    // 4xx:
    case 400: return "HTTP_400_BAD_REQUEST";
    case 401: return "HTTP_401_UNAUTHORIZED";
    case 402: return "HTTP_402_PAYMENT_REQUIRED";
    case 403: return "HTTP_403_FORBIDDEN";
    case 404: return "HTTP_404_NOT_FOUND";
    case 405: return "HTTP_405_METHOD_NOT_ALLOWED";
    case 406: return "HTTP_406_NOT_ACCEPTABLE";
    case 407: return "HTTP_407_PROXY_AUTHENTICATION_REQUIRED";
    case 408: return "HTTP_408_REQUEST_TIMEOUT";
    case 409: return "HTTP_409_CONFLICT";
    case 410: return "HTTP_410_GONE";
    case 411: return "HTTP_411_LENGTH_REQUIRED";
    case 412: return "HTTP_412_PRECONDITION_FAILED";
    case 413: return "HTTP_413_REQUEST_CONTENT_TOO_LARGE";
    case 414: return "HTTP_414_REQUEST_URI_TOO_LONG";
    case 415: return "HTTP_415_UNSUPPORTED_MEDIA_TYPE";
    case 416: return "HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE";
    case 417: return "HTTP_417_EXPECTATION_FAILED";
    case 421: return "HTTP_421_MISDIRECTED_REQUEST";
    case 425: return "HTTP_425_TOO_EARLY";
    case 426: return "HTTP_426_UPGRADE_REQUIRED";
    case 428: return "HTTP_428_PRECONDITION_REQUIRED";
    case 429: return "HTTP_429_TOO_MANY_REQUESTS";
    case 431: return "HTTP_431_REQUEST_HEADER_FIELDS_TOO_LARGE";
    case 451: return "HTTP_451_UNAVAILABLE_FOR_LEGAL_REASONS";
    // 5xx:
    case 500: return "HTTP_500_INTERNAL_SERVER_ERROR";
    case 501: return "HTTP_501_NOT_IMPLEMENTED";
    case 502: return "HTTP_502_BAD_GATEWAY";
    case 503: return "HTTP_503_SERVICE_UNAVAILABLE";
    case 504: return "HTTP_504_GATEWAY_TIMEOUT";
    case 505: return "HTTP_505_HTTP_VERSION_NOT_SUPPORTED";
    // Cloudflare 5xx:
    case 520: return "HTTP_520_WEB_SERVER_IS_RETURNING_AN_UNKNOWN_ERROR";
    case 521: return "HTTP_521_WEB_SERVER_IS_DOWN";
    case 522: return "HTTP_522_CONNECTION_TIMED_OUT";
    case 523: return "HTTP_523_ORIGIN_IS_UNREACHABLE";
    case 524: return "HTTP_524_A_TIMEOUT_OCCURRED";
    case 525: return "HTTP_525_SSL_HANDSHAKE_FAILED";
    case 526: return "HTTP_526_INVALID_SSL_CERTIFICATE";
    case 530: return "HTTP_530_RESOLVE_HOSTNAME_ERROR";

    // WebSocket status codes
    case 1000: return "WS_NORMAL_CLOSURE";
    case 1001: return "WS_GOING_AWAY";
    case 1002: return "WS_PROTOCOL_ERROR";
    case 1003: return "WS_UNSUPPORTED_DATA";
    case 1004: return "WS_RESERVED_1004";
    case 1005: return "WS_NO_STATUS_RCVD";
    case 1006: return "WS_ABNORMAL_CLOSURE";
    case 1007: return "WS_INVALID_FRAME_PAYLOAD_DATA";
    case 1008: return "WS_POLICY_VIOLATION";
    case 1009: return "WS_MESSAGE_TOO_BIG";
    case 1010: return "WS_EXTENSION_MISMATCH";
    case 1011: return "WS_SERVER_ERROR";
    case 1012: return "WS_SERVICE_RESTART";
    case 1013: return "WS_TRY_AGAIN_LATER";
    case 1015: return "WS_TLS_HANDSHAKE_FAILED";
    case 3000: return "WS_UNAUTHORIZED";
    case 3003: return "WS_FORBIDDEN";
    case 3008: return "WS_TIMEOUT";

    // Gaijin-specific server-side JSON-RPC defined errors
    case -29501: return "SEP_APP_RETRY_LATER";
    case -29502: return "SEP_AUTHENTICATION_ERROR";
    case -29503: return "SEP_ACCESS_DENIED";

    // JSON RPC 2.0 error:
    case -32700: return "JSON_RPC_PARSE_ERROR";
    case -32600: return "JSON_RPC_INVALID_REQUEST";
    case -32601: return "JSON_RPC_METHOD_NOT_FOUND";
    case -32602: return "JSON_RPC_INVALID_PARAMS";
    case -32603: return "JSON_RPC_INTERNAL_ERROR";
    case -32500: return "JSON_RPC_APPLICATION_ERROR";
    case -32400: return "JSON_RPC_SYSTEM_ERROR";
    case -32300: return "JSON_RPC_TRANSPORT_ERROR";
    case -32000: return "JSON_RPC_USER_ERROR_START";

    // Gaijin-specific client-side JSON-RPC defined errors
    case -40101: return "SEP_CLIENT_WEBSOCKET_CONNECTION_IS_NOT_READY";
    case -40102: return "SEP_CLIENT_LOST_WEBSOCKET_CONNECTION_AFTER_SENDING";
    case -40201: return "SEP_CLIENT_CLIENT_LIBRARY_IS_NOT_INITIALIZED";
    case -40301: return "SEP_CLIENT_INVALID_SERVER_RESPONSE";
    case -40302: return "SEP_CLIENT_RPC_CALL_TIMEOUT";
  }

  return {};
}


int32_t known_internal_errors[] = {1735550291, 1867217253, 1701670764, 1852402542, 196608};


eastl::string_view get_error_name(int error_code, eastl::string &buffer)
{
  if (error_code == 0)
  {
    return "SUCCESS";
  }

  const eastl::string_view knownErrorName = get_known_error_name(error_code);

  if (!knownErrorName.empty())
  {
    return knownErrorName;
  }

  for (const auto &error : known_internal_errors)
  {
    if (error == error_code)
    {
      return "INTERNAL_ERROR";
    }
  }

  if (error_code < 0)
    buffer.sprintf("ERROR_minus_%lld", -static_cast<long long>(error_code));
  else
    buffer.sprintf("ERROR_%d", error_code);
  return buffer;
}


namespace details
{


void report_stats_event(const WsJsonRpcStatsCallbackPtr &stats, StatsEventType event_type, WsJsonRpcConnection &connection)
{
  if (!stats)
  {
    return;
  }

  const auto &hostname = url_to_hostname(connection.getConfig().serverUrl);
  const auto &ipAddress =
    event_type == StatsEventType::CONNECT_INITIATE ? connection.getConfig().connectIpHint : connection.getActualUrlIp();

  switch (event_type)
  {
    case StatsEventType::CONNECT_INITIATE:
    {
      stats->onConnectInitiate(hostname, ipAddress);
      break;
    }
    case StatsEventType::CONNECT_SUCCESS:
    {
      stats->onConnectSuccess(hostname, ipAddress, connection.getConnectionEstablishTimeMs());
      break;
    }
    case StatsEventType::CONNECT_FAILURE:
    {
      const int errorCode = connection.getLastError().code;
      eastl::string buffer;
      const eastl::string_view errorName = get_error_name(errorCode, buffer);

      stats->onConnectFailure(hostname, ipAddress, connection.getConnectionEstablishTimeMs(), errorName);
      break;
    }
    case StatsEventType::DISCONNECT:
    {
      const int errorCode = connection.getLastError().code;
      eastl::string buffer;
      const eastl::string_view errorName = get_error_name(errorCode, buffer);

      stats->onDisconnect(hostname, ipAddress, connection.getConnectionLifetimeMs(), errorName);
      break;
    }
  }
}

} // namespace details

} // namespace websocketjsonrpc
