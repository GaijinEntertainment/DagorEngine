// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <streamIO/streamIO.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_critSec.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <EASTL/string.h>
#include <EASTL/shared_ptr.h>
#include <flat_hash_map2.hpp>
#include <time.h>

namespace streamio
{

time_t getdate(const char *p, const time_t *now);

#define HTTP_NOT_MODIFIED 304
#define HTTP_OK           200

static const char *supported_schemes[] = {"http", "https"};

static const char *format_date(char *buf, int bsize, time_t ts) // rfc1123
{
  strftime(buf, bsize, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&ts));
  return buf;
}

struct HTTPContext;

struct StreamContext
{
  eastl::shared_ptr<httprequests::RequestId> reqId;
  StreamContext() { reqId = eastl::make_shared<httprequests::RequestId>(0); }

  ~StreamContext()
  {
    if (*reqId)
      httprequests::abort_request(*reqId);
  }

  httprequests::RequestId getReqId() const { return *reqId; }

  void sendRequest(const char *url, completion_cb_t complete_cb, stream_data_cb_t stream_cb, resp_headers_cb_t resp_headers_cb,
    progress_cb_t progress_cb, void *cb_arg, int64_t modified_since, CreationParams::Timeouts const &timeouts, HTTPContext *owner);

  void syncWait()
  {
    while (*reqId)
    {
      httprequests::poll();
      sleep_msec(0);
    }
  }
};

struct HTTPContext : public Context
{
  CreationParams::Timeouts timeouts;
  ska::flat_hash_map<httprequests::RequestId, eastl::unique_ptr<StreamContext>> streams;
  WinCritSec streamsCs;

  HTTPContext(CreationParams *params)
  {
    if (params)
      timeouts = params->timeouts;
  }
  ~HTTPContext() { abort(); }

  void poll() override
  {
#ifdef __SANITIZE_THREAD__
    if ([&] {
          WinAutoLock lk(streamsCs);
          return !streams.empty();
        }())
#else
    if (!streams.empty())
#endif
      httprequests::poll();
  }

  void abort() override
  {
    httprequests::poll();
    {
      WinAutoLock lk(streamsCs);
      for (auto &it : streams)
        if (*it.second->reqId)
          httprequests::abort_request(*it.second->reqId);
    }
    httprequests::poll();

    WinAutoLock lk(streamsCs);
    streams.clear();
  }

  void abort_request(intptr_t req_id) override
  {
    httprequests::poll();
    {
      WinAutoLock lk2(streamsCs);
      if (streams.find(req_id) != streams.end())
        httprequests::abort_request(req_id);
    }
    httprequests::poll();
  }

  void removeRequest(httprequests::RequestId req_id)
  {
    WinAutoLock lk(streamsCs);
    streams.erase(req_id);
  }

  intptr_t createStream(const char *name, completion_cb_t complete_cb, stream_data_cb_t stream_cb, resp_headers_cb_t resp_headers_cb,
    progress_cb_t progress_cb, void *cb_arg, int64_t modified_since, bool do_sync) override
  {
    const char *scheme = strstr(name, "://");
    if (scheme)
    {
      for (size_t i = 0; i < sizeof(supported_schemes) / sizeof(supported_schemes[0]); ++i)
      {
        if (dd_strnicmp(name, supported_schemes[i], scheme - name) == 0)
        {
          auto ctx = eastl::make_unique<StreamContext>();
          ctx->sendRequest(name, complete_cb, stream_cb, resp_headers_cb, progress_cb, cb_arg, modified_since, timeouts, this);
          intptr_t id = ctx->getReqId();
          if (do_sync)
            ctx->syncWait();
          else
          {
            WinAutoLock lk(streamsCs);
            streams.insert(eastl::make_pair(ctx->getReqId(), eastl::move(ctx)));
          }
          return id;
        }
      }
    }
    // assume that 'name' is file name then
    FullFileLoadCB *stream = new FullFileLoadCB(name, DF_READ | DF_IGNORE_MISSING);
    int ret = stream->fileHandle ? 0 : -1;
    if (!stream->fileHandle)
      del_it(stream);
    complete_cb(name, ret, stream, cb_arg, -1, 0);
    return 0;
  }
};

Context *create(CreationParams *params) { return new HTTPContext(params); }

void StreamContext::sendRequest(const char *url, completion_cb_t complete_cb, stream_data_cb_t stream_cb,
  resp_headers_cb_t resp_headers_cb, progress_cb_t progress_cb, void *cb_arg, int64_t modified_since,
  CreationParams::Timeouts const &timeouts, HTTPContext *owner)
{
  httprequests::AsyncRequestParams reqParams;
  eastl::string urlSaved = url;
  eastl::string prUrlSaved = url;
  reqParams.url = url;
  reqParams.needResponseHeaders = modified_since >= 0 || resp_headers_cb != nullptr;
  reqParams.reqType = httprequests::HTTPReq::GET;

  char dateBuf[512];
  if (modified_since >= 0)
    reqParams.headers.push_back(eastl::make_pair("If-Modified-Since", format_date(dateBuf, sizeof(dateBuf), modified_since)));
  reqParams.reqTimeoutMs = timeouts.reqTimeoutSec * 1000;
  reqParams.connectTimeoutMs = timeouts.connectTimeoutSec * 1000;
  reqParams.lowSpeedTime = timeouts.lowSpeedTimeSec;
  reqParams.lowSpeedLimit = timeouts.lowSpeedLimitBps;
  eastl::weak_ptr<httprequests::RequestId> reqIdWptr = reqId;

  reqParams.callback = httprequests::make_http_callback(
    [complete_cb, cb_arg, urlSaved = eastl::move(urlSaved), reqIdWptr, owner](httprequests::RequestStatus status, int http_code,
      dag::ConstSpan<char> response, httprequests::StringMap const &resp_headers) {
      int result = ERR_UNKNOWN;
      MemGeneralLoadCB *loadCb = nullptr;
      int lastModified = 0;
      if (status == httprequests::RequestStatus::SUCCESS)
      {
        result = ERR_OK;
        if (http_code == HTTP_NOT_MODIFIED)
          result = ERR_NOT_MODIFIED;
        if (response.size() > 0 && http_code == HTTP_OK)
          loadCb = new MemGeneralLoadCB(response.data(), (int)response.size());
        if (auto lmIt = resp_headers.find("Last-Modified"); lmIt != resp_headers.end())
        {
          // copy to string because it is not zero-terminated
          eastl::string dateStr(lmIt->second.begin(), lmIt->second.end());
          lastModified = getdate(dateStr.c_str(), nullptr);
        }
      }
      else if (status == httprequests::RequestStatus::ABORTED || status == httprequests::RequestStatus::SHUTDOWN)
        result = ERR_ABORTED;

      auto reqIdptr = reqIdWptr.lock();
      if (reqIdptr)
      {
        complete_cb(urlSaved.c_str(), result, loadCb, cb_arg, lastModified, *reqIdptr);
        httprequests::RequestId reqId = *reqIdptr; //-V688
        *reqIdptr = 0;
        owner->removeRequest(reqId);
      }
    },
    [stream_cb, cb_arg, reqIdWptr](dag::ConstSpan<char> data) {
      if (!stream_cb)
        return false;
      if (auto reqIdptr = reqIdWptr.lock(); reqIdptr)
      {
        switch (stream_cb(data, cb_arg, *reqIdptr))
        {
          case ProcessResult::Consumed: return true;
          case ProcessResult::Discarded: return false;
          case ProcessResult::IoError: httprequests::abort_request(*reqIdptr); return false;
        }
      }
      return false;
    },
    [resp_headers_cb, cb_arg](httprequests::StringMap const &resp_headers) {
      if (resp_headers_cb)
        resp_headers_cb(resp_headers, cb_arg);
    },
    [progress_cb, prUrlSaved = eastl::move(prUrlSaved)](size_t dltotal, size_t dlnow) {
      if (progress_cb)
        progress_cb(prUrlSaved.c_str(), dltotal, dlnow);
    });
  *reqId = httprequests::async_request(reqParams);
}

} // namespace streamio
