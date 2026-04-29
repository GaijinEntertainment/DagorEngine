// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if DAGOR_DBGLEVEL > 0

#include "drv_log_defs.h"

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_commands.h>
#include <3d/dag_resourceDump.h>
#include <generic/dag_tab.h>
#include <drv/3d/dag_resourceChecker.h>
#if RESOURCE_CHECKER_DETAILED_FRAMEMEM_LOG
#include <EASTL/vector.h>
#include <EASTL/sort.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <osApiWrappers/dag_spinlock.h>
#endif

uint32_t ResourceChecker::uploaded_framemem = 0;
uint32_t ResourceChecker::uploaded_total = 0;
uint32_t ResourceChecker::uploaded_framemem_limit = 0;
uint32_t ResourceChecker::uploaded_total_limit = 0;

#if RESOURCE_CHECKER_DETAILED_FRAMEMEM_LOG
static OSSpinlock framemem_log_lock;
static eastl::vector<ResourceChecker::FramememUpdateEntry> framemem_log_entries;

void ResourceChecker::recordFramememUpdate(const char *name, uint32_t size, int lock_flags)
{
  OSSpinlockScopedLock guard(framemem_log_lock);
  framemem_log_entries.push_back({name ? name : "<unknown>", size, lock_flags});
}
#endif

void ResourceChecker::init()
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  // 0 means no limit
  uploaded_framemem_limit = blk_video.getInt("frameLimitBufferFramememKB", 2 << 10) << 10;
  uploaded_total_limit = blk_video.getInt("frameLimitBufferTotalKB", 0) << 10;

  interlocked_exchange(uploaded_total, 0u);
  interlocked_exchange(uploaded_framemem, 0u);

#if RESOURCE_CHECKER_DETAILED_FRAMEMEM_LOG
  {
    OSSpinlockScopedLock guard(framemem_log_lock);
    framemem_log_entries.clear();
  }
#endif
}

void ResourceChecker::report()
{
  uint32_t total_size = interlocked_exchange(uploaded_total, 0u);
  uint32_t framemem_size = interlocked_exchange(uploaded_framemem, 0u);

  if (uploaded_total_limit && total_size > uploaded_total_limit)
    D3D_ERROR("[Buffer] total uploaded size per frame was %d while limit is %d", total_size, uploaded_total_limit);
  if (uploaded_framemem_limit && framemem_size > uploaded_framemem_limit)
  {
    Tab<ResourceDumpInfo> dumpContainer;

    logwarn("[FRAMEMEM] Dumping framemem buffers!\n==============================================");
    if (d3d::driver_command(Drv3dCommand::GET_RESOURCE_STATISTICS, &dumpContainer))
    {
      int tSize = 0;
      for (auto const &resource : dumpContainer)
        if (resource.index() == 1)
        {
          if (const ResourceDumpBuffer &bufferDump = eastl::get<ResourceDumpBuffer>(resource); (bufferDump.flags & SBCF_FRAMEMEM) != 0)
          {
            logwarn("[FRAMEMEM] Buffer '%s' with allocated size %d", bufferDump.name, bufferDump.memSize);
            tSize += bufferDump.memSize;
          }
        }
      logwarn("===============================");
      logwarn("[FRAMEMEM] Total allocated size framemem buffers %d", tSize);
    }

#if RESOURCE_CHECKER_DETAILED_FRAMEMEM_LOG
    {
      eastl::vector<FramememUpdateEntry> entries;
      {
        OSSpinlockScopedLock guard(framemem_log_lock);
        entries = eastl::move(framemem_log_entries);
        framemem_log_entries.clear();
      }

      struct BufferAggregated
      {
        const char *name;
        uint64_t totalBytes = 0;
        uint64_t discardBytes = 0;
        uint64_t nooverwriteBytes = 0;
        uint32_t discardCount = 0;
        uint32_t nooverwriteCount = 0;
        uint32_t totalCount = 0;
      };

      ska::flat_hash_map<const char *, BufferAggregated> aggregated;
      for (const auto &e : entries)
      {
        auto &agg = aggregated[e.name];
        agg.name = e.name;
        agg.totalBytes += e.size;
        agg.totalCount++;
        if (e.lockFlags & VBLOCK_DISCARD)
        {
          agg.discardCount++;
          agg.discardBytes += e.size;
        }
        if (e.lockFlags & VBLOCK_NOOVERWRITE)
        {
          agg.nooverwriteCount++;
          agg.nooverwriteBytes += e.size;
        }
      }

      eastl::vector<BufferAggregated> sorted;
      sorted.reserve(aggregated.size());
      for (const auto &pair : aggregated)
        sorted.push_back(pair.second);

      eastl::sort(sorted.begin(), sorted.end(),
        [](const BufferAggregated &a, const BufferAggregated &b) { return a.totalBytes > b.totalBytes; });

      logwarn("[FRAMEMEM] Per-buffer upload details (sorted by total bytes):");
      uint64_t grandTotalBytes = 0;
      uint32_t grandTotalCalls = 0;
      for (const auto &buf : sorted)
      {
        logwarn("[FRAMEMEM]   '%s': %llu bytes (discard=%llu, nooverwrite=%llu), calls=%u (discard=%u, nooverwrite=%u)", buf.name,
          (unsigned long long)buf.totalBytes, (unsigned long long)buf.discardBytes, (unsigned long long)buf.nooverwriteBytes,
          buf.totalCount, buf.discardCount, buf.nooverwriteCount);
        grandTotalBytes += buf.totalBytes;
        grandTotalCalls += buf.totalCount;
      }
      logwarn("[FRAMEMEM] Grand total: %llu bytes across %u calls from %u unique buffers", (unsigned long long)grandTotalBytes,
        grandTotalCalls, (uint32_t)sorted.size());
    }
#endif

    D3D_ERROR("[Buffer] framemem uploaded size per frame was %d while limit is %d", framemem_size, uploaded_framemem_limit);
  }
#if RESOURCE_CHECKER_DETAILED_FRAMEMEM_LOG
  else
  {
    OSSpinlockScopedLock guard(framemem_log_lock);
    framemem_log_entries.clear();
  }
#endif
}

#endif
