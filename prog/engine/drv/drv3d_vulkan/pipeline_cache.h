// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_critSec.h>

#include <atomic>

#include "vk_wrapped_handles.h"
#include "pipeline_cache_file.h"

namespace drv3d_vulkan
{

class PipelineCache
{
  VulkanPipelineCacheHandle handle = VulkanPipelineCacheHandle();
  WinCritSec mutex;

  PipelineCache(const PipelineCache &);
  PipelineCache &operator=(const PipelineCache &);
  std::atomic<bool> asyncInProgress{false};

  void waitAsyncStoreComplete();
  bool isCacheTooBig(size_t byte_count);

public:
  PipelineCache() = default;

  VulkanPipelineCacheHandle getHandle() const { return handle; }
  WinCritSec *getMutex();

  void onDeviceReset();
  void shutdown();
  void load(PipelineCacheFile &src);
  void store(PipelineCacheFile &dst);
  void storeAsync();
};

} // namespace drv3d_vulkan
