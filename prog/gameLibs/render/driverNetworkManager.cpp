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
#include <ioSys/dag_zstdIo.h>
#include <statsd/statsd.h>

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

namespace circuit
{
const DataBlock *get_conf();
}

void DriverNetworkManager::sendPsoCacheBlkSync(const DataBlock &cache_blk)
{
  if (!::dgs_get_settings()->getBlockByNameEx("dx12")->getBool("pso_cache_sending_enabled", true))
    return;
  const DataBlock *circuitCfg = circuit::get_conf();
  const char *baseurl = circuitCfg ? circuitCfg->getStr("pso_cache_download_url", nullptr) : nullptr;
  if (!baseurl)
  {
    debug("URL for pso cache has not beed setup in <curcuit_name>{ pso_cache_download_url:t= }. Do not send cache.");
    return;
  }
  const char *appkey = ::dgs_get_settings()->getBlockByNameEx("dx12")->getStr("pso_app_key", nullptr);
  if (!appkey)
  {
    debug("App key for pso cache has not beed setup in dx12{ pso_app_key:t= }. Do not send cache.");
    return;
  }
  httprequests::AsyncRequestParams request;
  String url(0, "%s/data", baseurl);
  request.url = url;
  request.reqType = httprequests::HTTPReq::POST; // -V1048
  request.needResponseHeaders = false;
  request.headers = {
    {"Content-Type", "text/plain"},
    {"game", appkey},
    {"version", gameVersion.c_str()},
    {"Content-Encoding", "zstd"},
  };
  WaitableCallback waitableCallback;
  request.callback = &waitableCallback;
  DynamicMemGeneralSaveCB blkTxt(framemem_ptr());
  ZstdSaveCB zstdCompressor(blkTxt, 10); // Compression level 10 was chosen as a good trade-off between speed and ratio based on
                                         // experiments
  cache_blk.printToTextStreamLimited(zstdCompressor);
  zstdCompressor.finish();
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

void DriverNetworkManager::reportSlowPsoCompilation(const char *pipeline_name, long duration_ms, const char *pipeline_type)
{
  statsd::profile("dx12.pso_creation_time_ms", duration_ms, {{"pipeline", pipeline_name}, {"type", pipeline_type}});
}
