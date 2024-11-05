// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/psoCacheLoader/psoCacheLoader.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_miscApi.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_commands.h>
#include <osApiWrappers/dag_direct.h>
#include <EASTL/string.h>


const char *CACHE_FILENAME = "dx12_combined_cache.blk";
const char *CACHE_PATH = "cache/dx12_combined_cache.blk";

namespace
{
class WaitableCallback final : public httprequests::IAsyncHTTPCallback
{
public:
  WaitableCallback() : previousCacheLoadedFromDisk(dblk::load(cacheFileBlk, CACHE_PATH, dblk::ReadFlag::ROBUST)) {}

  virtual void onRequestDone(httprequests::RequestStatus status, int http_code, dag::ConstSpan<char> response,
    httprequests::StringMap const &resp_headers) override
  {
    const bool reportError =
      ::dgs_get_settings()->getBlockByNameEx("dx12")->getBool("Report_pso_cache_loading_error", (DAGOR_DBGLEVEL == 0));
    const uint32_t failReportLevel = reportError ? LOGLEVEL_ERR : LOGLEVEL_DEBUG;
    if (status == httprequests::RequestStatus::SUCCESS)
    {
      if (http_code == 200)
      {
        cacheFileBlk.clearData();
        if (!response.empty())
        {
          DataBlock cacheBlk;
          cacheBlk.loadText(response.data(), response.size());
          cacheFileBlk.addNewBlock(&cacheBlk, "cache");
          const auto lastModified = resp_headers.find("Last-Modified");
          const eastl::string lastModifiedVal =
            lastModified != resp_headers.end() ? eastl::string(lastModified->second.data(), lastModified->second.size()) : "";
          cacheFileBlk.addStr("If-Modified-Since", lastModifiedVal.c_str());

          if (!dd_file_exists(CACHE_PATH))
            dd_mkpath(CACHE_PATH);

          cacheFileBlk.saveToTextFile(CACHE_PATH);
          debug("PSO cache updated");
        }
        else
        {
          logmessage(failReportLevel, "Empty PSO cache was loaded");
        }
      }
      else if (http_code == 304)
      {
        debug("PSO cache is up to date");
      }
      else if (http_code == 403)
      {
        debug("Cache is not present on server. It's ok if a new game version was just released.");
      }
      else
      {
        logmessage(failReportLevel, "PSO cache request failed with code %d", http_code);
      }
    }
    else
    {
      logmessage(failReportLevel, "PSO cache request failed with code: %d", http_code);
    }
    done = true;
  }
  virtual void release() override {}
  bool done = false;
  DataBlock cacheFileBlk;
  bool previousCacheLoadedFromDisk;
};
} // namespace

void load_pso_cache(const char *game_name, const char *game_version)
{
  if (!::dgs_get_settings()->getBlockByNameEx("dx12")->getBool("pso_cache_enabled", true))
    return;
  WaitableCallback waitableCallback;
  httprequests::AsyncRequestParams request;
  const eastl::string url(eastl::string::CtorSprintf(), "%s/%s/%s/%s",
    ::dgs_get_settings()->getBlockByNameEx("dx12")->getStr("pso_cache_download_url", "https://pso-cache.gaijin.net"), game_name,
    game_version, CACHE_FILENAME);
  request.url = url.c_str();
  request.reqType = httprequests::HTTPReq::GET;
  if (waitableCallback.previousCacheLoadedFromDisk)
    request.headers = {
      {"If-Modified-Since", waitableCallback.cacheFileBlk.getStr("If-Modified-Since", "")},
    };

  request.callback = &waitableCallback;

  httprequests::RequestId requestId = httprequests::async_request(request);
  httprequests::poll();
  const uint32_t AMOUNT_OF_WAITING_TIME =
    ::dgs_get_settings()->getBlockByNameEx("dx12")->getInt("pso_cache_downloads_attempts", 5000); // 5 seconds by default
  for (uint32_t i = 0; i < AMOUNT_OF_WAITING_TIME; ++i)
  {
    ::sleep_msec(1);
    if (waitableCallback.done)
      break;
    httprequests::poll();
  }
  if (!waitableCallback.done)
  {
    logwarn("PSO cache request timeout");
    httprequests::abort_request(requestId);
    debug("Waiting for request abortion callback");
    while (!waitableCallback.done)
    {
      httprequests::poll();
      ::sleep_msec(1);
    }
    debug("Done");
  }
  if (DataBlock *cache = waitableCallback.cacheFileBlk.getBlockByName("cache"))
  {
    CompilePipelineSet pipelineSet;
    pipelineSet.defaultFormat = "dx12";
    pipelineSet.inputLayoutSet = cache->getBlockByName("input_layouts");
    pipelineSet.computePipelineSet = cache->getBlockByName("compute_pipelines");
    pipelineSet.renderStateSet = cache->getBlockByName("render_states");
    pipelineSet.graphicsPipelineSet = cache->getBlockByName("graphics_pipelines");
    pipelineSet.featureSet = cache->getBlockByName("features");
    pipelineSet.meshPipelineSet = cache->getBlockByName("mesh_pipelines");
    pipelineSet.outputFormatSet = cache->getBlockByName("framebuffer_layouts");
    pipelineSet.signature = cache->getBlockByName("signature");
    pipelineSet.computeSet = cache->getBlockByName("compute");
    pipelineSet.graphicsSet = cache->getBlockByName("graphics");
    pipelineSet.graphicsNullPixelOverrideSet = cache->getBlockByName("graphicsNullOverride");
    pipelineSet.graphicsPixelOverrideSet = cache->getBlockByName("graphicsPixelOverride");
    d3d::driver_command(Drv3dCommand::COMPILE_PIPELINE_SET, &pipelineSet);
  }
}
