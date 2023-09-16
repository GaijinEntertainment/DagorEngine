//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <EASTL/vector.h>
#include <EASTL/map.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <generic/dag_span.h>

namespace httprequests
{
typedef intptr_t RequestId;
typedef eastl::map<eastl::string_view, eastl::string_view> StringMap;

enum class RequestStatus
{
  SUCCESS,
  FAILED,
  ABORTED,
  SHUTDOWN
};

class IAsyncHTTPCallback
{
public:
  virtual ~IAsyncHTTPCallback() {}

  virtual void onRequestDone(RequestStatus status, int http_code, dag::ConstSpan<char> response, StringMap const &headers) = 0;
  virtual void onHttpProgress(size_t /*dltotal*/, size_t /*dlnow*/) {}
  virtual void onHttpHeaderResponse(StringMap const & /*headers*/) {}

  // return true if peace of data is processed by callback and should't be stored
  virtual bool onResponseData(dag::ConstSpan<char>) { return false; }
  virtual void release() = 0;
};

template <class F>
IAsyncHTTPCallback *make_http_callback(F on_response)
{
  class Callback : public IAsyncHTTPCallback
  {
  public:
    Callback(F on_response) : cb(eastl::move(on_response)) {}
    void onRequestDone(RequestStatus status, int http_code, dag::ConstSpan<char> response, StringMap const &headers)
    {
      cb(status, http_code, response, headers);
    }

    void release() { delete this; }

  private:
    F cb;
  };
  return new Callback(eastl::move(on_response));
}

template <class F1, class F2>
IAsyncHTTPCallback *make_http_callback(F1 on_response, F2 on_stream_data)
{
  class Callback : public IAsyncHTTPCallback
  {
  public:
    Callback(F1 on_response, F2 on_stream_data) : resp_cb(eastl::move(on_response)), stream_cb(eastl::move(on_stream_data)) {}

    void onRequestDone(RequestStatus status, int http_code, dag::ConstSpan<char> response, StringMap const &headers)
    {
      resp_cb(status, http_code, response, headers);
    }

    bool onResponseData(dag::ConstSpan<char> data) { return stream_cb(data); }

    void release() { delete this; }

  private:
    F1 resp_cb;
    F2 stream_cb;
  };
  return new Callback(eastl::move(on_response), eastl::move(on_stream_data));
}

template <class F1, class F2, class F3>
IAsyncHTTPCallback *make_http_callback(F1 on_response, F2 on_stream_data, F3 on_http_header)
{
  class Callback : public IAsyncHTTPCallback
  {
  public:
    Callback(F1 on_response, F2 on_stream_data, F3 on_http_header) :
      resp_cb(eastl::move(on_response)), stream_cb(eastl::move(on_stream_data)), header_cb(eastl::move(on_http_header))
    {}

    void onRequestDone(RequestStatus status, int http_code, dag::ConstSpan<char> response, StringMap const &headers)
    {
      resp_cb(status, http_code, response, headers);
    }

    bool onResponseData(dag::ConstSpan<char> data) { return stream_cb(data); }

    void onHttpHeaderResponse(StringMap const &headers) { header_cb(headers); }

    void release() { delete this; }

  private:
    F1 resp_cb;
    F2 stream_cb;
    F3 header_cb;
  };
  return new Callback(eastl::move(on_response), eastl::move(on_stream_data), eastl::move(on_http_header));
}

template <class F1, class F2, class F3, class F4>
IAsyncHTTPCallback *make_http_callback(F1 on_response, F2 on_stream_data, F3 on_http_header, F4 on_http_progress)
{
  class Callback : public IAsyncHTTPCallback
  {
  public:
    Callback(F1 on_response, F2 on_stream_data, F3 on_http_header, F4 on_http_progress) :
      resp_cb(eastl::move(on_response)),
      stream_cb(eastl::move(on_stream_data)),
      header_cb(eastl::move(on_http_header)),
      progress_cb(eastl::move(on_http_progress))
    {}

    void onRequestDone(RequestStatus status, int http_code, dag::ConstSpan<char> response, StringMap const &headers)
    {
      resp_cb(status, http_code, response, headers);
    }

    bool onResponseData(dag::ConstSpan<char> data) { return stream_cb(data); }

    void onHttpHeaderResponse(StringMap const &headers) { header_cb(headers); }

    void onHttpProgress(size_t dltotal, size_t dlnow) { progress_cb(dltotal, dlnow); }

    void release() { delete this; }

  private:
    F1 resp_cb;
    F2 stream_cb;
    F3 header_cb;
    F4 progress_cb;
  };
  return new Callback(eastl::move(on_response), eastl::move(on_stream_data), eastl::move(on_http_header),
    eastl::move(on_http_progress));
}

enum class HTTPReq
{
  GET,
  POST,
  HEAD,
  DEL
};

using Header = eastl::pair<const char *, const char *>;

struct AsyncRequestParams
{
  // required params
  const char *url = nullptr;
  IAsyncHTTPCallback *callback = nullptr;

  // optional params
  HTTPReq reqType = HTTPReq::POST;
  bool verifyCert = true;
  bool verifyHost = true;
  bool allowHttpContentEncoding = true;
  bool needHeaders = false;

  const char *clientCertificateFile = nullptr;
  const char *clientPrivateKeyFile = nullptr;

  const char *userAgent = nullptr;
  eastl::vector<Header> headers;
  dag::ConstSpan<char> postData;

  size_t maxDownloadSize = 0;

  // total request timeout for curl
  // ignored in xbox1 implementation
  int reqTimeoutMs = 0;

  // tcp connect timeout
  int connectTimeoutMs = 0;

  // CURLOPT_LOW_SPEED_TIME/CURLOPT_LOW_SPEED_LIMIT
  // in curl implementation this settings set lower limit
  // for avarage download speed. this settings disable timeoutMs
  int lowSpeedTime = 0;
  int lowSpeedLimit = 0;

  // CURLOPT_RANGE for chunked download
  // https://curl.se/libcurl/c/CURLOPT_RANGE.html
  const char *chunkRange = nullptr;

  bool sendResponseInMainThreadOnly = false;
};

RequestId async_request(AsyncRequestParams const &params);

void abort_request(RequestId req_id);

enum class AbortMode
{
  Abort,
  Shutdown
};
void abort_all_requests(AbortMode mode = AbortMode::Abort);

void poll();

struct InitAsyncParams
{
  const char *defaultUserAgent = nullptr;
  const char *caBundleFile = nullptr;
  bool verboseDebug = false;
  bool pollInThread = false;
};

void init_async(InitAsyncParams const &params);
void shutdown_async();
} // namespace httprequests
