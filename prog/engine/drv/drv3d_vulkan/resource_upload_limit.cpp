// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resource_upload_limit.h"
#include "globals.h"
#include "driver_config.h"

using namespace drv3d_vulkan;

#if VULKAN_DELAY_RUB_OPERATIONS > 0

#include <ska_hash_map/flat_hash_map2.hpp>
#include <osApiWrappers/dag_critSec.h>

namespace
{
ska::flat_hash_map<uint64_t, int> debugDelayMap;
WinCritSec debugDelayMapCS;

static bool doDebugDelay(BaseTexture *bt)
{
  WinAutoLock lock(debugDelayMapCS);
  uint64_t bthash = str_hash_fnv1<64>(bt->getTexName());
  if (debugDelayMap.find(bthash) == debugDelayMap.end() || debugDelayMap[bthash] < 10)
  {
    debugDelayMap[bthash] = debugDelayMap[bthash] + 1;
    return true;
  }
  return false;
}

} // namespace

#else

namespace
{
static constexpr bool doDebugDelay(BaseTexture *) { return false; }
} // namespace

#endif

namespace
{
static thread_local bool force_no_fail = false;
} // namespace

void ResourceUploadLimit::setNoFailOnThread(bool val) { force_no_fail = val; }

void ResourceUploadLimit::reset()
{
  uint32_t oldSize = allocatedSize.exchange(0);
  if (peakSize < oldSize)
  {
    if (oldSize > Globals::cfg.texUploadLimit)
      debug("vulkan: max overalloc RUB size %u -> %u Mb", peakSize >> 20, oldSize >> 20);
    peakSize = oldSize;
  }
}

bool ResourceUploadLimit::consume(uint32_t size, BaseTexture *bt)
{
  if (force_no_fail)
  {
    allocatedSize.fetch_add(size);
    return true;
  }

  if (doDebugDelay(bt))
    return false;

  // we allow any size of buffers in case there are no other allocations
  // or if we trying to allocate RUB for texture that was used for no-other-allocations case
  // due to fact that caller code do request RUBs in loop and this loop must not fail in low memory conditions
  uint32_t oldSize = allocatedSize.fetch_add(size);
  bool ret = (oldSize == 0) || (oldSize + size) < Globals::cfg.texUploadLimit ||
             // use relaxed load, as primary use case is in-thread loop, i.e. no external sync is needed, yet TSAN may complain
             (overallocTarget.load(std::memory_order_relaxed) == bt);
  if (oldSize == 0)
    overallocTarget.store(bt, std::memory_order_release);
  return ret;
}
