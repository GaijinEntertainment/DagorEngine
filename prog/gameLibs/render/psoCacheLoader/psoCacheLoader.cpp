// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/psoCacheLoader/psoCacheLoader.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_miscApi.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_commands.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <EASTL/string.h>


const char *CACHE_FILENAME = "dx12_combined_cache.blk";
const char *CACHE_PATH = "cache/dx12_combined_cache.blk";

namespace
{
class WaitableCallback final : public httprequests::IAsyncHTTPCallback
{
public:
  WaitableCallback() : previousCacheLoadedFromDisk(dblk::load(cacheFileBlk, CACHE_PATH, dblk::ReadFlag::ROBUST))
  {
    DagorStat localCacheStat;
    if (::df_stat(CACHE_PATH, &localCacheStat) != -1)
    {
      debug("Local PSO cache file '%s' loaded, size: %lld bytes", CACHE_PATH, localCacheStat.size);
    }
    else
    {
      debug("Local PSO cache file '%s' not found", CACHE_PATH);
    }
  }

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
          debug("PSO cache loaded from server, size: %u bytes", response.size());
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

namespace circuit
{
const DataBlock *get_conf();
}

void load_pso_cache(const char *game_version)
{
  if (!::dgs_get_settings()->getBlockByNameEx("dx12")->getBool("pso_cache_enabled", true))
    return;
  WaitableCallback waitableCallback;
  httprequests::AsyncRequestParams request;
  const DataBlock *circuitCfg = circuit::get_conf();
  const char *baseurl = circuitCfg ? circuitCfg->getStr("pso_cache_download_url", nullptr) : nullptr;
  const char *appkey = ::dgs_get_settings()->getBlockByNameEx("dx12")->getStr("pso_app_key", nullptr);
  if (!baseurl || !appkey)
  {
    debug("URL for pso cache has not beed setup in dx12{ pso_cache_download_url:t= }. Do not use cache.");
    return;
  }
  const eastl::string url(eastl::string::CtorSprintf(), "%s/%s/%s/%s", baseurl, appkey, game_version, CACHE_FILENAME);
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
    debug("PSO cache info. Input layouts count: %d", pipelineSet.inputLayoutSet ? pipelineSet.inputLayoutSet->blockCount() : 0);
    pipelineSet.computePipelineSet = cache->getBlockByName("compute_pipelines");
    debug("PSO cache info. Compute pipelines count: %d",
      pipelineSet.computePipelineSet ? pipelineSet.computePipelineSet->blockCount() : 0);
    pipelineSet.renderStateSet = cache->getBlockByName("render_states");
    debug("PSO cache info. Render states count: %d", pipelineSet.renderStateSet ? pipelineSet.renderStateSet->blockCount() : 0);
    pipelineSet.graphicsPipelineSet = cache->getBlockByName("graphics_pipelines");
    debug("PSO cache info. Graphics pipelines count: %d",
      pipelineSet.graphicsPipelineSet ? pipelineSet.graphicsPipelineSet->blockCount() : 0);
    pipelineSet.featureSet = cache->getBlockByName("features");
    debug("PSO cache info. Feature sets count: %d", pipelineSet.featureSet ? pipelineSet.featureSet->blockCount() : 0);
    pipelineSet.meshPipelineSet = cache->getBlockByName("mesh_pipelines");
    debug("PSO cache info. Mesh pipelines count: %d", pipelineSet.meshPipelineSet ? pipelineSet.meshPipelineSet->blockCount() : 0);
    pipelineSet.outputFormatSet = cache->getBlockByName("framebuffer_layouts");
    debug("PSO cache info. Framebuffer layouts count: %d",
      pipelineSet.outputFormatSet ? pipelineSet.outputFormatSet->blockCount() : 0);
    pipelineSet.signature = cache->getBlockByName("signature");
    debug("PSO cache info. Signatures count: %d", pipelineSet.signature ? pipelineSet.signature->blockCount() : 0);
    pipelineSet.computeSet = cache->getBlockByName("compute");
    const uint32_t computeSetCount = pipelineSet.computeSet ? pipelineSet.computeSet->blockCount() : 0;
    debug("PSO cache info. Compute set count: %d", computeSetCount);
    auto getUsagesCount = [](const DataBlock *set) -> uint32_t {
      if (!set)
        return 0;
      const uint32_t setCount = set->blockCount();
      uint32_t usagesCount = 0;
      for (uint32_t i = 0; i < setCount; ++i)
      {
        const DataBlock &variant = *set->getBlock(i);
        const uint32_t shadersCount = variant.blockCount();
        for (uint32_t j = 0; j < shadersCount; ++j)
          usagesCount += variant.getBlock(j)->paramCount();
      }
      return usagesCount;
    };
    debug("PSO cache info. Compute set usages: %d", getUsagesCount(pipelineSet.computeSet));
    pipelineSet.graphicsSet = cache->getBlockByName("graphics");
    const uint32_t graphicsSetCount = pipelineSet.graphicsSet ? pipelineSet.graphicsSet->blockCount() : 0;
    debug("PSO cache info. Graphics set variants: %d", graphicsSetCount);
    debug("PSO cache info. Graphics set usages: %d", getUsagesCount(pipelineSet.graphicsSet));
    pipelineSet.graphicsNullPixelOverrideSet = cache->getBlockByName("graphicsNullOverride");
    const uint32_t graphicsNullPixelOverrideSetCount =
      pipelineSet.graphicsNullPixelOverrideSet ? pipelineSet.graphicsNullPixelOverrideSet->blockCount() : 0;
    debug("PSO cache info. Graphics null pixel override set count: %d", graphicsNullPixelOverrideSetCount);
    debug("PSO cache info. Graphics null pixel override set usages: %d", getUsagesCount(pipelineSet.graphicsNullPixelOverrideSet));
    pipelineSet.graphicsPixelOverrideSet = cache->getBlockByName("graphicsPixelOverride");
    debug("PSO cache info. Graphics pixel override set count: %d",
      pipelineSet.graphicsPixelOverrideSet ? pipelineSet.graphicsPixelOverrideSet->blockCount() : 0);
    debug("PSO cache info. Graphics pixel override set usages: %d", getUsagesCount(pipelineSet.graphicsPixelOverrideSet));
    d3d::driver_command(Drv3dCommand::COMPILE_PIPELINE_SET, &pipelineSet);
  }
}
