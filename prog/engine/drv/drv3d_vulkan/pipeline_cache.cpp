// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline_cache.h"
#include "vulkan_device.h"
#include "globals.h"
#include "physical_device_set.h"
#include "device_context.h"
#include "vulkan_allocation_callbacks.h"

using namespace drv3d_vulkan;

void PipelineCache::shutdown()
{
  {
    debug("vulkan: extracting & saving pipeline cache...");
    PipelineCacheOutFile pipelineCacheFile;
    store(pipelineCacheFile);
  }

  VULKAN_LOG_CALL(Globals::VK::dev.vkDestroyPipelineCache(Globals::VK::dev.get(), handle, VKALLOC(pipeline_cache)));
  handle = VulkanNullHandle();
}

void PipelineCache::onDeviceReset()
{
  // destroy without saving, because on reset it may be corrupted
  VULKAN_LOG_CALL(Globals::VK::dev.vkDestroyPipelineCache(Globals::VK::dev.get(), handle, VKALLOC(pipeline_cache)));
  handle = VulkanNullHandle();
}

void PipelineCache::load(PipelineCacheFile &src)
{
  VkPipelineCacheCreateInfo pcci;
  pcci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  pcci.pNext = NULL;
  pcci.flags = 0;
  Tab<uint8_t> data;
  if (src.getCacheFor(Globals::VK::phy.properties.pipelineCacheUUID, data))
  {
    pcci.initialDataSize = data.size();
    pcci.pInitialData = data.data();
  }
  else
  {
    pcci.initialDataSize = 0;
    pcci.pInitialData = NULL;
  }

  VulkanPipelineCacheHandle loadedCache;

  if (
    VULKAN_CHECK_OK(Globals::VK::dev.vkCreatePipelineCache(Globals::VK::dev.get(), &pcci, VKALLOC(pipeline_cache), ptr(loadedCache))))
  {
    debug("vulkan: loaded pipeline cache with size %uKb", pcci.initialDataSize >> 10);

    if (is_null(handle))
      handle = loadedCache;
    else
      Globals::ctx.addPipelineCache(loadedCache);
  }
  else if (is_null(handle))
  {
    pcci.initialDataSize = 0;
    pcci.pInitialData = NULL;

    VULKAN_CHECK_OK(Globals::VK::dev.vkCreatePipelineCache(Globals::VK::dev.get(), &pcci, VKALLOC(pipeline_cache), ptr(handle)));
    debug("vulkan: failed to load pipeline cache, empty created");
  }
}

void PipelineCache::waitAsyncStoreComplete()
{
  if (asyncInProgress.load())
    spin_wait([this]() { return asyncInProgress.load(); });
}

void PipelineCache::storeAsync()
{
  waitAsyncStoreComplete();

  size_t count;
  if (VULKAN_CHECK_FAIL(Globals::VK::dev.vkGetPipelineCacheData(Globals::VK::dev.get(), handle, &count, NULL)))
    count = 0;

  debug("vulkan: async extracting pipeline cache data of size %u Kb from driver", count >> 10);

  if (count)
  {
    // alloc and API getter can't be moved to thread, yet can consume tens of ms
    // but disk IO may take much more time, so move IO to thread
    Tab<uint8_t> data;
    data.resize(count);
    if (VULKAN_CHECK_OK(Globals::VK::dev.vkGetPipelineCacheData(Globals::VK::dev.get(), handle, &count, data.data())))
    {
      asyncInProgress.store(true);
      execute_in_new_thread(
        [data = eastl::move(data), this](auto) {
          PipelineCacheOutFile dst;
          dst.storeCacheFor(Globals::VK::phy.properties.pipelineCacheUUID, data);
          asyncInProgress.store(false);
        },
        "vulkan_async_pipeline_cache_store");
    }
  }
}

void PipelineCache::store(PipelineCacheFile &dst)
{
  waitAsyncStoreComplete();
  size_t count;
  if (VULKAN_CHECK_FAIL(Globals::VK::dev.vkGetPipelineCacheData(Globals::VK::dev.get(), handle, &count, NULL)))
    count = 0;

  debug("vulkan: extracting pipeline cache data of size %u Kb from driver", count >> 10);

  if (count)
  {
    Tab<uint8_t> data;
    data.resize(count);
    if (VULKAN_CHECK_OK(Globals::VK::dev.vkGetPipelineCacheData(Globals::VK::dev.get(), handle, &count, data.data())))
      dst.storeCacheFor(Globals::VK::phy.properties.pipelineCacheUUID, data);
  }
}

WinCritSec *PipelineCache::getMutex()
{
  if (Globals::cfg.bits.usePipelineCacheMutex)
    return &mutex;
  return nullptr;
}
