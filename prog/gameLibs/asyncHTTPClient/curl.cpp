// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <asyncHTTPClient/asyncHTTPClient.h>

#include <debug/dag_log.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_events.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_delayedAction.h>
#include <math/random/dag_random.h>

#include <EASTL/list.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

#include <ioSys/dag_brotliIo.h>

#if defined(USE_XCURL)
#include <Xcurl.h>
#else
#include <curl/curl.h>
#endif

#if _TARGET_XBOX
#include <osApiWrappers/gdk/network.h>
#include <gdk/main.h>
#endif

#if _TARGET_PC_WIN
#include <winsock2.h>
#elif _TARGET_C3




#endif

#if _TARGET_C3

#endif

#define debug(...) logmessage(_MAKE4C('HTTP'), __VA_ARGS__)
#define DEBUG_VERBOSE(...) \
  do                       \
  {                        \
    if (verbose_debug)     \
    {                      \
      debug(__VA_ARGS__);  \
    }                      \
  } while (0)

namespace httprequests
{

static eastl::string to_string(eastl::string_view const &sv) { return eastl::string(sv.begin(), sv.end()); }

using HeadersList = eastl::vector<eastl::pair<eastl::string, eastl::string>>;
static HeadersList copy_string_map_view(StringMap const &sm)
{
  HeadersList result;
  result.reserve(sm.size());
  for (auto const &kv : sm)
    result.push_back(eastl::make_pair(to_string(kv.first), to_string(kv.second)));
  return result;
}

static CURLM *curlm;
static CURLSH *curlsh;
static WinCritSec *mutex;
static WinCritSec *queue_mutex;
static long timeout_ms;

static const unsigned int TCP_CONNECT_TIMEOUT_SEC = 12;

static eastl::string default_user_agent = "dagor2";
static eastl::string ca_bundle_file;
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
static size_t header_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

static int progress_callback(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t);
static bool verbose_debug = false;

static size_t curl_debug(CURL *, curl_infotype it, char *msg, size_t n, void *)
{
  if (it == CURLINFO_DATA_IN || it == CURLINFO_SSL_DATA_IN)
    if (grnd() % 20)
      return 0;

  const char *prefix = "";
  switch (it)
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

  eastl::string_view message = "";
  if (it != CURLINFO_SSL_DATA_IN && it != CURLINFO_SSL_DATA_OUT && it != CURLINFO_DATA_IN && it != CURLINFO_DATA_OUT && n > 1)
    message = eastl::string_view(msg, n - 1);

  debug("[http] %s %.*s", prefix, (int)message.size(), message.data());
  return 0;
}

static void logerr_http_in_retail(const eastl::string &url)
{
#if _TARGET_XBOX
  static constexpr uint32_t httpsValues[] = {
    0x70747468, // http
    0x2f2f3a73, // s://
  };
  if (url.length() >= 8 && gdk::is_retail_environment())
  {
    eastl::string lUrl = url.substr(0, 8);
    lUrl.make_lower();
    uint32_t *as_uints = (uint32_t *)lUrl.c_str();
    // Notify if http protocol is explicitly specified. In other cases HTTPS will be used/
    if (as_uints[0] == httpsValues[0] && as_uints[1] != httpsValues[1])
      logerr("HTTP protocol usage in production environment violates XR-12. URL: <%s>", url.c_str());
  }
#endif
  G_UNUSED(url);
}

static CURL *make_curl_handle(const char *url, const char *user_agent, bool verify_peer, bool verify_host, void *user_data,
  const char *chunk_range, bool need_resp_headers)
{
  G_UNUSED(need_resp_headers);
  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  // To consider: always pass header callback for pre-reserve `response` using `Content-Length` header
#ifndef USE_XCURL // For custom decompressor (e.g. brotli) setup
  if (need_resp_headers || verbose_debug)
#endif
  {
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, user_data);
  }
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
  curl_easy_setopt(curl, CURLOPT_SHARE, curlsh);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TCP_CONNECT_TIMEOUT_SEC);
  curl_easy_setopt(curl, CURLOPT_DNS_USE_GLOBAL_CACHE, 0);
  curl_easy_setopt(curl, CURLOPT_HEADER, 0);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, user_data);
  curl_easy_setopt(curl, CURLOPT_XFERINFODATA, user_data);
  curl_easy_setopt(curl, CURLOPT_PRIVATE, user_data);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose_debug ? 1L : 0L);
  curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug);

  if (chunk_range)
    curl_easy_setopt(curl, CURLOPT_RANGE, const_cast<char *>(chunk_range));

#ifndef _TARGET_C3
  if (!ca_bundle_file.empty() && verify_peer)
  {
    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_bundle_file.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  }
  else
  {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  }
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verify_host ? 2L : 0L);
#else












#endif //_TARGET_C3

  return curl;
}

class RequestState
{
public:
  RequestState(AsyncRequestParams const &params)
  {
    urlStr = params.url;
    logerr_http_in_retail(urlStr);
    curlHandle = make_curl_handle(urlStr.c_str(), params.userAgent ? params.userAgent : default_user_agent.c_str(), params.verifyCert,
      params.verifyHost, this, params.chunkRange, params.needResponseHeaders);
    callback = params.callback;
    needResponseHeaders = params.needResponseHeaders;
    maxDownloadSize = params.maxDownloadSize;
    sendResponseInMainThreadOnly = params.sendResponseInMainThreadOnly;
    headersList = nullptr;

    DEBUG_VERBOSE("RequestState constructor url:%s, Http request:%d, userAgent:%s", params.url, (int)params.reqType, params.userAgent);
    DEBUG_VERBOSE("RequestState constructor verifyCert:%d, verifyHost:%d reqTimeoutMs:%d", params.verifyCert, params.verifyHost,
      params.reqTimeoutMs);

    if (params.lowSpeedTime > 0 && params.lowSpeedLimit > 0)
    {
      curl_easy_setopt(curlHandle, CURLOPT_LOW_SPEED_TIME, params.lowSpeedTime);
      curl_easy_setopt(curlHandle, CURLOPT_LOW_SPEED_LIMIT, params.lowSpeedLimit);
    }

    if (params.reqTimeoutMs > 0)
      curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT_MS, params.reqTimeoutMs);

    if (params.connectTimeoutMs > 0)
      curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT_MS, params.connectTimeoutMs);

    if (params.allowHttpContentEncoding)
      curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "");

    if (params.clientCertificateFile && params.clientPrivateKeyFile)
    {
      curl_easy_setopt(curlHandle, CURLOPT_SSLKEYTYPE, "PEM");
      curl_easy_setopt(curlHandle, CURLOPT_SSLCERT, params.clientCertificateFile);
      curl_easy_setopt(curlHandle, CURLOPT_SSLKEY, params.clientPrivateKeyFile);
    }

    for (Header const &h : params.headers)
    {
#if defined(USE_XCURL)
      if (nullptr != strchr(h.second, '\n'))
        logerr("xCurl doesn't support multiline headers. URL: <%s>, HEADER: <%s>", urlStr.c_str(), h.first);
#endif
      eastl::string hstr = h.first;
      hstr += ": ";
      hstr += h.second;
      headersList = curl_slist_append(headersList, hstr.c_str());
      DEBUG_VERBOSE("append header:%s", hstr.c_str());
    }

    if (params.postData.size())
      reqData.insert(reqData.end(), params.postData.data(), params.postData.data() + params.postData.size());

    if (params.reqType == HTTPReq::POST)
    {
      curl_easy_setopt(curlHandle, CURLOPT_POST, 1);

      if (!reqData.empty())
      {
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, reqData.data());
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, reqData.size());

        eastl::string hstr = "Content-Length: " + eastl::to_string(reqData.size());
        headersList = curl_slist_append(headersList, hstr.c_str());
      }
      else
      {
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, 0);
      }

      DEBUG_VERBOSE("HTTP Request POST. Request data:%.*s", reqData.size(), reqData.data());
    }
    else if (params.reqType == HTTPReq::HEAD)
    {
      curl_easy_setopt(curlHandle, CURLOPT_NOBODY, 1);
      DEBUG_VERBOSE("HTTP Request HEAD");
    }
    else if (params.reqType == HTTPReq::DEL)
    {
      curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
      DEBUG_VERBOSE("HTTP Request DELETE");
    }

    if (headersList)
      curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headersList);

#if DAGOR_DBGLEVEL > 0
    debug("%s %p", __FUNCTION__, this);
#endif
  }

  ~RequestState()
  {
    if (headersList)
      curl_slist_free_all(headersList);

    if (curlHandle)
    {
      curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1);
      curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, nullptr);
      curl_easy_setopt(curlHandle, CURLOPT_XFERINFOFUNCTION, nullptr);
      curl_easy_cleanup(curlHandle);
    }

#if DAGOR_DBGLEVEL > 0
    debug("%s %p", __FUNCTION__, this);
#endif
  }

  CURL *getCurlHandle() { return curlHandle; }

  void onRequestFinished(int http_code, CURLcode curl_code)
  {
    httpCode = http_code;
    curlResult = curl_code;
    DEBUG_VERBOSE("onRequestFinish http_code: %d, error: %s", http_code, curl_easy_strerror(curl_code));
  }

  void sendResult()
  {
    G_ASSERT(callback);

    RequestStatus result;
    if (abortFlag)
      result = RequestStatus::ABORTED;
    else if (shutdownFlag)
      result = RequestStatus::SHUTDOWN;
    else
      result = curlResult == CURLE_OK ? RequestStatus::SUCCESS : RequestStatus::FAILED;

    DEBUG_VERBOSE("sendResult result: %d", (int)result);

    if (!sendResponseInMainThreadOnly || is_main_thread())
    {
      callback->onRequestDone(result, httpCode, response, respHeadersMap);
      callback->release();
    }
    else
    {
      delayed_call([callback = this->callback, result, httpCode = this->httpCode, response = eastl::move(response),
                     resp_headers = needResponseHeaders ? copy_string_map_view(respHeadersMap) : HeadersList{}] {
        StringMap respHeaders(resp_headers.begin(), resp_headers.end());
        callback->onRequestDone(result, httpCode, response, respHeaders);
        callback->release();
      });
    }
    callback = nullptr;
  }

  void abort()
  {
    debug("request to '%s' aborted", urlStr.c_str());
    abortFlag = true;
  }

  void shutdown() { shutdownFlag = true; }

  bool isAborted() const { return abortFlag; }

  size_t updateResponse(dag::Span<char> data)
  {
    size_t downloadedSize = data.size();
#if defined(USE_XCURL)
    Tab<char> decoded;
    if (decompressor)
    {
      auto res = decompressor->decompress(dag::Span<uint8_t>((uint8_t *)data.begin(), data.size()), decoded);
      if (res != StreamDecompressResult::FINISH && res != StreamDecompressResult::NEED_MORE_INPUT)
      {
        debug("Failed to decompress data from '%s'. Request aborted", urlStr.c_str());
        return 0;
      }
      data = make_span(decoded.data(), decoded.size());
    }
#endif

    if (callback->onResponseData(data))
      return downloadedSize;

    size_t bytesAfterUpdate = data.size() + response.size();

    if (maxDownloadSize != 0 && bytesAfterUpdate > maxDownloadSize)
    {
      debug("warning: response from '%s' exceeded maximum limit of %d bytes. Request aborted", urlStr.c_str(), maxDownloadSize);
      return 0;
    }

    response.insert(response.end(), data.begin(), data.end());
    return downloadedSize;
  }

#if defined(USE_XCURL)
  void trySetDecompressorByHeader(dag::Span<char> header)
  {
    eastl::string_view hv(header.data(), header.size());
    if (eastl::string_view::size_type pos = hv.find("Content-Encoding:"); pos != hv.npos)
    {
      decompressor.reset();
      pos = hv.find("br", pos);
      if (pos != hv.npos)
        decompressor.reset(new BrotliStreamDecompress());
    }
  }
#endif

  void addResponseHeader(dag::Span<char> header)
  {
    DEBUG_VERBOSE("add response header:%.*s", header.size(), header.data());
#if defined(USE_XCURL)
    trySetDecompressorByHeader(header);
#endif
    if (needResponseHeaders)
    {
      responseHeaders.emplace_back(header.begin(), header.end());
      responseHeaders.back().reserve(sizeof(void *) * 3); // Disable SSO for string_view in `respHeadersMap`
    }
  }

  void finishResponseHeader()
  {
    long respCode = 0;
    curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &respCode);
    // mean redirect response, RFC2616 Section 10.3
    if (respCode >= 300 && respCode < 400)
    {
      responseHeaders.clear();
      return;
    }

    for (const auto &header : responseHeaders)
    {
      DEBUG_VERBOSE("finishResponseHeader header: %s", header.c_str());
      eastl::string_view hv(header.data(), header.size());
      if (auto delimPos = hv.find(":"); delimPos != hv.npos)
      {
        auto key = hv.substr(0, delimPos);
        auto value = hv.substr(delimPos + 1);
        if (auto crlfPos = value.find("\r\n"); crlfPos != value.npos)
          value.remove_suffix(value.size() - crlfPos);
        if (auto nonSpacePos = value.find_first_not_of(' '); nonSpacePos != value.npos)
        {
          value.remove_prefix(nonSpacePos);
          respHeadersMap[key] = value;
        }
      }
    }

    if (needResponseHeaders)
      callback->onHttpHeadersResponse(respHeadersMap);
  }

  void onDownloadProgress(size_t dltotal, size_t dlnow)
  {
    // this callback is called within mutex lock
    // so no public methods can be called from here
    callback->onHttpProgress(dltotal, dlnow);
  }

  int responseSize() const { return (int)response.size(); }

  const char *getUrl() const
  {
    if (!urlStr.empty())
      return urlStr.c_str();
    return nullptr;
  }

public:
  CURL *curlHandle;
  IAsyncHTTPCallback *callback;
  size_t maxDownloadSize;
  curl_slist *headersList;
  eastl::string urlStr;
  dag::Vector<char> reqData;

  bool needResponseHeaders;
  bool sendResponseInMainThreadOnly;
  bool abortFlag = false;
  bool shutdownFlag = false;

  int httpCode = 0;
  CURLcode curlResult = CURLE_OK;
  dag::Vector<char> response;
  dag::Vector<eastl::string> responseHeaders; // Note: string_view in `respHeadersMap` are pointing within
  httprequests::StringMap respHeadersMap;
#ifdef USE_XCURL
  eastl::unique_ptr<IStreamDecompress> decompressor;
#endif
};

using RequestStatePtr = eastl::unique_ptr<RequestState>;

static eastl::list<RequestStatePtr> active_requests;
static eastl::list<RequestStatePtr> requests_queue;

static int progress_callback(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
{
  RequestState *context = (RequestState *)userdata;
  if (dlnow > 0 || dltotal > 0)
    context->onDownloadProgress(dltotal, dlnow);
  return 0;
}

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
  RequestState *context = (RequestState *)userdata;
  size_t nbytes = size * nmemb;
  return context->updateResponse(make_span((char *)ptr, nbytes));
}

static size_t header_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  RequestState *context = (RequestState *)userdata;
  size_t nbytes = size * nmemb;
  if (DAGOR_LIKELY(strcmp(ptr, "\r\n") != 0))
    context->addResponseHeader(make_span(ptr, nbytes));
  else
    context->finishResponseHeader();
  return nbytes;
}

static int timer_func(CURLM *, long timeout, void *)
{
  if (timeout >= 0)
    timeout_ms = get_time_msec() + timeout;
  else
    timeout_ms = 0;
  return 0;
}

static void move_queue_to_active_requests_nolock()
{
#if USE_XCURL
  static constexpr uint8_t MAX_ACTIVE_REQUESTS = 4;
#else
  static constexpr uint8_t MAX_ACTIVE_REQUESTS = 32;
#endif

#if _TARGET_XBOX
  if (!gdk::has_network_access())
    return;
#endif

  if (queue_mutex)
  {
    WinAutoLock lock(*queue_mutex);
    while (active_requests.size() <= MAX_ACTIVE_REQUESTS && !requests_queue.empty())
    {
      RequestStatePtr &context = requests_queue.front();
      curl_multi_add_handle(curlm, context->getCurlHandle());
      active_requests.push_back(eastl::move(context));
      requests_queue.pop_front();
    }
  }
}

RequestId async_request(AsyncRequestParams const &params)
{
  if (!curlm)
    init_async(InitAsyncParams());

  RequestStatePtr context = eastl::make_unique<RequestState>(params);
  RequestState *ctxPtr = context.get();
  {
    if (mutex->tryLock())
    {
      active_requests.push_back(eastl::move(context));
      curl_multi_add_handle(curlm, ctxPtr->getCurlHandle());
      mutex->unlock();
    }
    else
    {
      debug("Curl: cannot lock active_requests list. Put request to the queue.");
      WinAutoLock lock(*queue_mutex);
      requests_queue.push_back(eastl::move(context));
    }
  }
  debug("send request to '%s'", params.url);
  return intptr_t(ctxPtr);
}

void abort_request(RequestId req_id)
{
  {
    WinAutoLock lock(*mutex);
    move_queue_to_active_requests_nolock();
    RequestState *reqState = (RequestState *)req_id;
    auto it = eastl::find_if(active_requests.begin(), active_requests.end(),
      [reqState](RequestStatePtr const &ptr) { return ptr.get() == reqState; });
    if (it != active_requests.end())
    {
      reqState->abort();
      return;
    }
  }
  debug("HTTP: trying to abort non-existent http request %p", (void *)req_id);
}

void abort_all_requests(AbortMode mode)
{
  // this function is called during xbox suspend.
  // it could happen even before init or after shutdown.
  if (!mutex)
    return;

  eastl::list<RequestStatePtr> aborted;
  {
    WinAutoLock lock(*mutex);
    move_queue_to_active_requests_nolock();
    aborted = eastl::move(active_requests);
    for (RequestStatePtr &rstate : aborted)
      curl_multi_remove_handle(curlm, rstate->getCurlHandle());
  }

  for (RequestStatePtr &rstate : aborted)
  {
    if (mode == AbortMode::Abort)
      rstate->abort();
    else
      rstate->shutdown();
    rstate->sendResult();
  }
}

static void update_multi_nolock(eastl::list<RequestStatePtr> &finished)
{
  int numfds = 0;
  int nrunning = 0;
  curl_multi_wait(curlm, nullptr, 0, 0, &numfds);
  const bool needPerform = numfds > 0 || !active_requests.empty();

  if (needPerform)
  {
    curl_multi_perform(curlm, &nrunning);
    int msgInQueue = 0;

    while (CURLMsg *msg = curl_multi_info_read(curlm, &msgInQueue))
    {
      if (msg->msg != CURLMSG_DONE)
      {
        DEBUG_VERBOSE("curl msg not done. msgInQueue: %d", msgInQueue);
        if (!msgInQueue)
          break;
        else
          continue;
      }

      long respCode = 0;
      CURL *easy = msg->easy_handle;
      CURLcode code = msg->data.result;
      DEBUG_VERBOSE("Curl error code: %s", curl_easy_strerror(code));

      curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &respCode);
      DEBUG_VERBOSE("Curl response code: %ld", respCode);

      void *ptr = nullptr;
      curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ptr);
      RequestState *reqState = static_cast<RequestState *>(ptr);

      const char *url = nullptr;
      curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &url);
      if (url == nullptr)
        url = reqState->getUrl();
      if (url == nullptr)
        url = "<unknown url>";
      DEBUG_VERBOSE("Curl url: %s", url);

      char *addr = NULL;
      curl_easy_getinfo(easy, CURLINFO_PRIMARY_IP, &addr);
      DEBUG_VERBOSE("Curl addr: %s", addr);

      double connectTime = 0;
      double totalTime = 0;
      double nameLookupTime = 0;

      curl_easy_getinfo(easy, CURLINFO_TOTAL_TIME, &totalTime);
      curl_easy_getinfo(easy, CURLINFO_CONNECT_TIME, &connectTime);
      curl_easy_getinfo(easy, CURLINFO_NAMELOOKUP_TIME, &nameLookupTime);
      DEBUG_VERBOSE("Curl totalTime: %f, connectTime: %f, nameLookupTime: %f", totalTime, connectTime, nameLookupTime);

      curl_multi_remove_handle(curlm, easy);

      if (code != CURLE_OK)
      {
        debug("request to '%s' ip: %s failed: %s. times: total %.2fms, conn %.2fms, "
              "nl %.2fms. size: %d",
          url, addr, curl_easy_strerror(code), totalTime * 1000, connectTime * 1000, nameLookupTime * 1000, reqState->responseSize());
      }
      else
      {
        debug("request to '%s' ip: %s succeed with code %d. times: total %.2fms, conn %.2fms, "
              "nl %.2fms. size: %d",
          url, addr, (int)respCode, totalTime * 1000, connectTime * 1000, nameLookupTime * 1000, reqState->responseSize());
      }

      reqState->onRequestFinished(respCode, code);

      auto it = eastl::find_if(active_requests.begin(), active_requests.end(),
        [reqState](RequestStatePtr const &ptr) { return ptr.get() == reqState; });

      if (it != active_requests.end())
      {
        finished.push_back(eastl::move(*it));
        active_requests.erase(it);
      }
    }
  }
}

static void collect_aborted_requests_nolock(eastl::list<RequestStatePtr> &finished)
{
  for (auto it = active_requests.begin(); it != active_requests.end();)
  {
    if ((*it)->isAborted())
    {
      curl_multi_remove_handle(curlm, (*it)->getCurlHandle());
      finished.push_back(eastl::move(*it));
      it = active_requests.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

static void poll_on_idle_cycle(void *) { httprequests::poll(); }

struct CurlPollThread final : public DaThread
{
  int64_t thread_id = 0;
  os_event_t event;

  CurlPollThread() : DaThread("CurlPollThread", 128 << 10, 0, WORKER_THREADS_AFFINITY_MASK) { os_event_create(&event); }
  ~CurlPollThread() { os_event_destroy(&event); }

  void execute() override
  {
    thread_id = get_current_thread_id();
    while (!isThreadTerminating())
    {
      // Thread will be blocked until event is set or 10ms timeout elapsed
      // We need to make thread alertable because Sleep is not guaranteed
      // to wake up at any specific interval
      os_event_wait(&event, 10);
      httprequests::poll();
    }
  }

  void wakeup() { os_event_set(&event); }
};

static eastl::unique_ptr<CurlPollThread> g_poll_thread;


void poll()
{
  if (g_poll_thread && g_poll_thread->thread_id != get_current_thread_id())
  {
    g_poll_thread->wakeup();
    return;
  }

  TIME_PROFILE(http_requests_poll);

  eastl::list<RequestStatePtr> finished;

  {
    // Note: consider poll() call after shutdown or before init is not an error (TODO: investigate this)
    if (!mutex)
      return;

    if (!mutex->tryLock())
      return;

    move_queue_to_active_requests_nolock();
    update_multi_nolock(finished);
    collect_aborted_requests_nolock(finished);

    mutex->unlock();
  }
  for (RequestStatePtr &request : finished)
    request->sendResult();
}


void init_async(InitAsyncParams const &params)
{
  if (!curlm)
  {
    curlm = curl_multi_init();
    curl_multi_setopt(curlm, CURLMOPT_TIMERFUNCTION, &timer_func);
    curl_multi_setopt(curlm, CURLMOPT_TIMERDATA, NULL);
    curl_multi_setopt(curlm, CURLMOPT_PIPELINING, 1);
    curl_multi_setopt(curlm, CURLMOPT_MAX_TOTAL_CONNECTIONS, 10);

    curlsh = curl_share_init();
    curl_share_setopt(curlsh, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(curlsh, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(curlsh, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);

    G_FAST_ASSERT(!mutex);
    mutex = new WinCritSec;
    queue_mutex = new WinCritSec;

    bool pollInThread = params.pollInThread;

#if defined(USE_XCURL)
    // xCurl will block suspend until all in-progress requests are
    // completed, and failing to call curl_multi_perform may cause
    // title to timeout during suspend.
    pollInThread = true;
#endif

    if (pollInThread)
    {
      g_poll_thread.reset(new CurlPollThread);
      G_VERIFY(g_poll_thread->start());
    }
    else
      register_regular_action_to_idle_cycle(&poll_on_idle_cycle, NULL);
  }

  if (params.defaultUserAgent)
    default_user_agent = params.defaultUserAgent;

  if (params.caBundleFile)
    ca_bundle_file = params.caBundleFile;

  verbose_debug = params.verboseDebug;

  debug("AsyncHTTPClient ca bundle: '%s'", ca_bundle_file.c_str());
}

static void safe_delete_mutex(WinCritSec *&mutex_ptr)
{
  WinCritSec *tmp = mutex_ptr;
  tmp->lock();
  mutex_ptr = nullptr;
  tmp->unlock();
  delete tmp;
}

void shutdown_async()
{
  if (!curlm)
    return;

  // During shutdown stage we don't want to call script callbacks
  // The script VM might be destroyed already
  // Also, script cals might produce side effects to the library (make more http requests, for example)
  abort_all_requests(AbortMode::Shutdown);

  if (g_poll_thread)
  {
    g_poll_thread->terminate(true /* wait */);
    g_poll_thread.reset();
  }

  unregister_regular_action_to_idle_cycle(&poll_on_idle_cycle, NULL);
  curl_multi_cleanup(curlm);
  curl_share_cleanup(curlsh);

  curlm = NULL;
  curlsh = NULL;

  safe_delete_mutex(mutex);
  safe_delete_mutex(queue_mutex);
}

struct StaticCleanup
{
  ~StaticCleanup() { shutdown_async(); }

} static_cleanup;

} // namespace httprequests
