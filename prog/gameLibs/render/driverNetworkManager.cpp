// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/driverNetworkManager.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <memory/dag_framemem.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <startup/dag_globalSettings.h>
#include <eventLog/eventLog.h>
#include <breakpad/binder.h>

namespace
{
class WaitableCallback final : public httprequests::IAsyncHTTPCallback
{
public:
  WaitableCallback() {}

  virtual void onRequestDone(httprequests::RequestStatus, int, dag::ConstSpan<char>, httprequests::StringMap const &) override {}
  virtual void release() override { done = true; }
  bool done = false;
};
} // namespace

void DriverNetworkManager::sendPsoCacheBlkSync(const DataBlock &cache_blk)
{
  if (!::dgs_get_settings()->getBlockByNameEx("dx12")->getBool("pso_cache_sending_enabled", true))
    return;
  httprequests::AsyncRequestParams request;
  const char *defaultUrl =
#if DAGOR_DBGLEVEL > 0
    "https://beta.pso-cache.gaijin.net/data"
#else
    "https://pso-cache.gaijin.net/data"
#endif
    ;
  request.url = ::dgs_get_settings()->getBlockByNameEx("dx12")->getStr("pso_cache_server_url", defaultUrl);
  request.reqType = httprequests::HTTPReq::POST; // -V1048
  request.needResponseHeaders = false;
  request.headers = {
    {"Content-Type", "text/plain"},
    {"game", gameName.c_str()},
    {"version", gameVersion.c_str()},
  };
  WaitableCallback waitableCallback;
  request.callback = &waitableCallback;
  DynamicMemGeneralSaveCB blkTxt(framemem_ptr());
  cache_blk.printToTextStreamLimited(blkTxt);
  request.postData = dag::Span(reinterpret_cast<char *>(blkTxt.data()), blkTxt.size());

  httprequests::RequestId requestId = httprequests::async_request(request);
  httprequests::poll();
  const uint32_t AMOUNT_OF_WAITING_TIME =
    ::dgs_get_settings()->getBlockByNameEx("dx12")->getInt("pso_cache_request_attempts", 5000); // 5 seconds by default
  for (uint32_t i = 0; i < AMOUNT_OF_WAITING_TIME; ++i)
  {
    ::sleep_msec(1);
    if (waitableCallback.done)
      return;
    httprequests::poll();
  }
  logwarn("PSO cache request timeout");
  httprequests::abort_request(requestId);
  httprequests::poll();
  G_ASSERT(waitableCallback.done); // Either sent or aborted
}

void DriverNetworkManager::sendHttpEventLog(const char *type, const void *data, uint32_t size, Json::Value *meta)
{
  event_log::send_http_instant(type, data, size, meta);
}

void DriverNetworkManager::addFileToCrashReport(const char *path) { breakpad::add_file_to_report(path); }
